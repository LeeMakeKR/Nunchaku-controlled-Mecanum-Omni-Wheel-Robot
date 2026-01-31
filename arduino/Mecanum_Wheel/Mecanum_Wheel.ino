// TMC2209 메카넘 휠 제어 코드
// ESP32 DEVKIT 사용
// Wii Nunchaku로 제어

#include <U8g2lib.h>
#include <Wire.h>
#include <FastLED.h>

// U8G2 OLED 디스플레이 설정
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// 핀 정의
// Front Left Motor 
#define MOTOR_FL_DIR    16
#define MOTOR_FL_STEP   17

// Rear Left Motor
#define MOTOR_RL_DIR    18
#define MOTOR_RL_STEP   19

// Rear Right Motor
#define MOTOR_RR_DIR    13
#define MOTOR_RR_STEP   14

// Front Right Motor
#define MOTOR_FR_DIR    25
#define MOTOR_FR_STEP   26

// 공통 ENABLE 핀
#define ENABLE_PIN    27  // D27

// WS2812 LED 설정
#define LED_PIN       23  // D23 (WS2812 출력)
#define NUM_LEDS      1
#define LED_BRIGHTNESS 50

CRGB leds[NUM_LEDS];

// Wii Nunchaku I2C 주소
#define NUNCHAKU_ADDRESS 0x52

// Nunchaku 데이터 구조체
struct NunchakuData {
  int joyX;       // 조이스틱 X (0-255)
  int joyY;       // 조이스틱 Y (0-255)
  int accelX;     // 가속도계 X
  int accelY;     // 가속도계 Y
  int accelZ;     // 가속도계 Z
  bool buttonC;   // C 버튼
  bool buttonZ;   // Z 버튼
};

NunchakuData nunchaku;

// 메카넘 휠 파라미터 (mm)
const float WHEEL_DIAMETER = 65.0;      // 바퀴 지름
const float WHEEL_RADIUS = WHEEL_DIAMETER / 2.0;
const float WHEELBASE_X = 138.0;        // 전후 바퀴 축간 거리
const float WHEELBASE_Y = 168.0;        // 좌우 바퀴 중심 거리
const float L_X = WHEELBASE_X / 2.0;    // 69mm
const float L_Y = WHEELBASE_Y / 2.0;    // 84mm
const float L_SUM = L_X + L_Y;          // 153mm

// 모터 파라미터
const int STEPS_PER_REV = 200;          // 1회전당 스텝 (1.8도)
const int MICROSTEPS = 8;              // 마이크로스텝 설정 (TMC2209)
const int TOTAL_STEPS_PER_REV = STEPS_PER_REV * MICROSTEPS;  // 1,600
const float MIN_STEP_DELAY_US = 50;    // 최소 스텝 딜레이 (μs) - 최대 속도 기준

// 조이스틱 데드존 및 스케일
const int JOY_CENTER = 128;
const int JOY_DEADZONE = 20;
const int JOY_THRESHOLD = 20;  // 셋업 모드용
const float MAX_VELOCITY = 12.0;       // mm/s (최대 속도)
const float MAX_ACCELERATION = 1.5;   // mm/s^2 (최대 가속도)

// 가속도계 데드존 (변수로 설정하여 수정 가능)
int ACCEL_DEADZONE = 10;  // 중심값 512 ± 10 범위는 무시

// 현재 속도 (가속도 제어용)
float current_v_x = 0, current_v_y = 0, current_omega = 0;

// 모터 속도 (rad/s)
float omega_FL = 0, omega_RL = 0, omega_RR = 0, omega_FR = 0;

// ========== Nunchaku 함수 (setup.h 포함 전에 정의) ==========

// Nunchaku 초기화
void nunchakuInit() {
  Wire.beginTransmission(NUNCHAKU_ADDRESS);
  Wire.write(0xF0);
  Wire.write(0x55);
  Wire.endTransmission();
  delay(1);
  
  Wire.beginTransmission(NUNCHAKU_ADDRESS);
  Wire.write(0xFB);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(1);
}

// Nunchaku 데이터 요청
void nunchakuRequest() {
  Wire.beginTransmission(NUNCHAKU_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();
}

// Nunchaku 데이터 읽기
bool nunchakuRead() {
  uint8_t data[6];
  int cnt = 0;
  
  Wire.requestFrom(NUNCHAKU_ADDRESS, 6);
  
  while(Wire.available()) {
    data[cnt] = Wire.read();
    cnt++;
  }
  
  nunchakuRequest();
  
  if(cnt >= 5) {
    nunchaku.joyX = data[0];
    nunchaku.joyY = data[1];
    nunchaku.accelX = (data[2] << 2) | ((data[5] >> 2) & 0x03);
    nunchaku.accelY = (data[3] << 2) | ((data[5] >> 4) & 0x03);
    nunchaku.accelZ = (data[4] << 2) | ((data[5] >> 6) & 0x03);
    nunchaku.buttonC = !((data[5] >> 1) & 0x01);
    nunchaku.buttonZ = !(data[5] & 0x01);
    return true;
  }
  return false;
}

// setup.h 포함 (이전 변수들이 정의된 후에 포함)
#include "setup.h"  // 셋업 모드 포함

void setup() {
  // Serial 초기화
  Serial.begin(115200);
  
  // I2C 초기화
  Wire.begin();
  
  // 핀 모드 설정
  pinMode(MOTOR_FL_DIR, OUTPUT);
  pinMode(MOTOR_FL_STEP, OUTPUT);
  pinMode(MOTOR_RL_DIR, OUTPUT);
  pinMode(MOTOR_RL_STEP, OUTPUT);
  pinMode(MOTOR_RR_DIR, OUTPUT);
  pinMode(MOTOR_RR_STEP, OUTPUT);
  pinMode(MOTOR_FR_DIR, OUTPUT);
  pinMode(MOTOR_FR_STEP, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  
  // WS2812 LED 초기화
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  
  // Nunchaku 초기화
  nunchakuInit();
  
  delay(100);
  // 셋업 모드 진입 체크 (부팅 시 C+Z 버튼)
  if(nunchakuRead() && nunchaku.buttonC && nunchaku.buttonZ) {
    Serial.println("Entering setup mode");
  //  runSetupMode();
  }
  
  // 모터 활성화
  digitalWrite(ENABLE_PIN, LOW);
  
  delay(100);
}

// 조이스틱 입력을 차체 속도로 변환 (벡터 기반)
void joystickToVelocity(int joyX, int joyY, float &v_x, float &v_y, float &omega) {
  // Nunchaku 조이스틱 범위: 0-255 (중립값: 128)
  // X축(joyX): 좌우 (0=좌, 128=중립, 255=우)
  // Y축(joyY): 전후 (0=후, 128=중립, 255=전)
  
  int joyX_offset = joyX - JOY_CENTER;  // -128 ~ 127
  int joyY_offset = joyY - JOY_CENTER;  // -128 ~ 127
  
  // 데드존 적용 (JOY_DEADZONE: 중립±20 범위는 무시)
  if(abs(joyX_offset) < JOY_DEADZONE) joyX_offset = 0;
  if(abs(joyY_offset) < JOY_DEADZONE) joyY_offset = 0;
  
  // 조이스틱 값을 정규화 (-1.0 ~ 1.0)
  float joyX_norm = constrain(joyX_offset / 128.0, -1.0, 1.0);
  float joyY_norm = constrain(joyY_offset / 128.0, -1.0, 1.0);
  
  // 벡터 크기 계산
  float magnitude = sqrt(joyX_norm * joyX_norm + joyY_norm * joyY_norm);
  
  if(magnitude > 0) {
    // 정규화
    magnitude = constrain(magnitude, 0.0, 1.0);
    
    // 속도 크기 계산: 조이스틱이 움직일수록 빠르게
    float velocity = magnitude * magnitude * MAX_VELOCITY;
    
    // 정규화된 방향 벡터
    float dir_x = joyY_norm / magnitude;
    float dir_y = -joyX_norm / magnitude;  // X축 반대로 매칭 (왼쪽 입력 = 오른쪽 이동)
    
    // 조이스틱 방향을 차체 좌표로 매핑
    // joyY(전후) → v_x(전후), joyX(좌우) → v_y(좌우)
    v_x = velocity * dir_x;   // 앞쪽 입력 = 전진
    v_y = velocity * dir_y;   // 왼쪽 입력 = 오른쪽 이동
  } else {
    v_x = 0;
    v_y = 0;
  }
  
  omega = 0;  // 회전은 나중에 추가 (버튼으로 제어 가능)
}

// 가속도계 기울임을 차체 속도로 변환
void accelerometerToVelocity(int accelX, int accelY, int accelZ, float &v_x, float &v_y, float &omega) {
  // Nunchaku 가속도계 범위: 0-1023 (중립값: 512)
  // 기울임 각도 계산 (라디안)
  // X축 기울임: accelY 사용 (좌우 기울임)
  // Y축 기울임: accelX 사용 (전후 기울임)
  
  int accelX_offset = accelX - 512;  // -512 ~ 511
  int accelY_offset = accelY - 512;  // -512 ~ 511
  
  // 데드존 적용 (ACCEL_DEADZONE: 512±값 범위는 무시)
  if(abs(accelX_offset) < ACCEL_DEADZONE) accelX_offset = 0;
  if(abs(accelY_offset) < ACCEL_DEADZONE) accelY_offset = 0;
  
  // 가속도 값을 정규화 (-1.0 ~ 1.0)
  float accelX_norm = constrain(accelX_offset / 512.0, -1.0, 1.0);
  float accelY_norm = constrain(accelY_offset / 512.0, -1.0, 1.0);
  
  // 벡터 크기 계산
  float magnitude = sqrt(accelX_norm * accelX_norm + accelY_norm * accelY_norm);
  
  if(magnitude > 0) {
    // 정규화
    magnitude = constrain(magnitude, 0.0, 1.0);
    
    // 속도 크기 계산: 기울기에 따라 비선형으로 속도 증가 (제곱 함수로 더 가파른 곡선)
    // magnitude가 작을 때는 천천히, 클수록 빠르게 증가
    float velocity = magnitude * magnitude * MAX_VELOCITY;
    
    // 정규화된 방향 벡터
    float dir_x = accelY_norm / magnitude;
    float dir_y = -accelX_norm / magnitude;  // X축 반대로 매칭 (왼쪽 기울임 = 오른쪽 이동)
    
    // 가속도 방향을 차체 좌표로 매핑
    // accelY(좌우 기울임) → v_y(좌우), accelX(전후 기울임) → v_x(전후)
    v_x = velocity * dir_x;   // 전방으로 기울이면 전진
    v_y = velocity * dir_y;   // 왼쪽으로 기울이면 오른쪽 이동
  } else {
    v_x = 0;
    v_y = 0;
  }
  
  omega = 0;  // 회전은 나중에 추가 (버튼으로 제어 가능)
}

// 메카넘 역기구학: 차체 속도 -> 바퀴 각속도 (rad/s)
void inverseKinematics(float v_x, float v_y, float omega, 
                       float &w_FL, float &w_RL, float &w_RR, float &w_FR) {
  // Mecanum_calc.md의 공식 사용
  w_FL = (v_x - v_y - L_SUM * omega) / WHEEL_RADIUS;
  w_RL = (v_x + v_y - L_SUM * omega) / WHEEL_RADIUS;
  w_RR = (v_x - v_y + L_SUM * omega) / WHEEL_RADIUS;
  w_FR = (v_x + v_y + L_SUM * omega) / WHEEL_RADIUS;
}

// 가속도 제한을 적용하여 현재 속도를 목표 속도로 업데이트
void applyAcceleration(float target_v_x, float target_v_y, float target_omega, 
                       float &current_v_x, float &current_v_y, float &current_omega, 
                       float dt) {
  // 최대 속도 변화량 (가속도 * 시간)
  float max_delta_v = MAX_ACCELERATION * dt;
  
  // v_x 가속도 제한
  float delta_v_x = target_v_x - current_v_x;
  if(abs(delta_v_x) > max_delta_v) {
    delta_v_x = (delta_v_x > 0) ? max_delta_v : -max_delta_v;
  }
  current_v_x += delta_v_x;
  
  // v_y 가속도 제한
  float delta_v_y = target_v_y - current_v_y;
  if(abs(delta_v_y) > max_delta_v) {
    delta_v_y = (delta_v_y > 0) ? max_delta_v : -max_delta_v;
  }
  current_v_y += delta_v_y;
  
  // omega 가속도 제한 (각가속도는 선가속도와 같은 비율 적용)
  float max_delta_omega = (MAX_ACCELERATION / WHEEL_RADIUS) * dt;
  float delta_omega = target_omega - current_omega;
  if(abs(delta_omega) > max_delta_omega) {
    delta_omega = (delta_omega > 0) ? max_delta_omega : -max_delta_omega;
  }
  current_omega += delta_omega;
}

// 각속도를 스텝 수와 방향으로 변환
void angularVelocityToSteps(float omega, int &steps, bool &dir, float dt) {
  // dt 시간 동안 회전할 각도 (라디안)
  float theta = omega * dt;
  
  // 스텝 수 계산
  float revolutions = theta / (2.0 * PI);
  steps = abs(revolutions * TOTAL_STEPS_PER_REV);
  
  // 방향 결정
  dir = (omega >= 0);
}

// 4개 모터 동시 제어 (최대 스텝 수 기준, 가변 속도)
void moveRobot(float w_FL, float w_RL, float w_RR, float w_FR, float dt) {
  int steps_FL, steps_RL, steps_RR, steps_FR;
  bool dir_FL, dir_RL, dir_RR, dir_FR;
  
  // 각 바퀴의 스텝 수와 방향 계산
  angularVelocityToSteps(w_FL, steps_FL, dir_FL, dt);
  angularVelocityToSteps(w_RL, steps_RL, dir_RL, dt);
  angularVelocityToSteps(w_RR, steps_RR, dir_RR, dt);
  angularVelocityToSteps(w_FR, steps_FR, dir_FR, dt);
  
  // 방향 설정 (L모터: HIGH=정회전, R모터: LOW=정회전)
  digitalWrite(MOTOR_FL_DIR, dir_FL ? HIGH : LOW);
  digitalWrite(MOTOR_RL_DIR, dir_RL ? HIGH : LOW);
  digitalWrite(MOTOR_RR_DIR, dir_RR ? LOW : HIGH);
  digitalWrite(MOTOR_FR_DIR, dir_FR ? LOW : HIGH);
  
  // 최대 스텝 수 계산
  int maxSteps = max(max(steps_FL, steps_RL), max(steps_RR, steps_FR));
  
  // 최대 속도에 의해 역으로 계산되는 스텝 딜레이 (속도가 클수록 딜레이 작음)
  float maxOmega = max(max(abs(w_FL), abs(w_RL)), max(abs(w_RR), abs(w_FR)));
  float stepDelay = MIN_STEP_DELAY_US;
  
  if(maxOmega > 0.01) {  // 최소 각속도 임계값
    // 최대 각속도 계산: MAX_VELOCITY(200mm/s) / WHEEL_RADIUS(32.5mm) ≈ 6.15 rad/s
    float maxOmegaExpected = MAX_VELOCITY / WHEEL_RADIUS;
    // 역함수: 속도가 최대일 때 MIN_STEP_DELAY_US, 속도가 작을 때 더 큼
    float normalizedOmega = constrain(maxOmega / maxOmegaExpected, 0.01, 1.0);
    stepDelay = MIN_STEP_DELAY_US / normalizedOmega;
  }
  
  // 동시 스텝 실행
  for(int i = 0; i < maxSteps; i++) {
    if(i < steps_FL) digitalWrite(MOTOR_FL_STEP, HIGH);
    if(i < steps_RL) digitalWrite(MOTOR_RL_STEP, HIGH);
    if(i < steps_RR) digitalWrite(MOTOR_RR_STEP, HIGH);
    if(i < steps_FR) digitalWrite(MOTOR_FR_STEP, HIGH);
    delayMicroseconds(stepDelay);
    
    digitalWrite(MOTOR_FL_STEP, LOW);
    digitalWrite(MOTOR_RL_STEP, LOW);
    digitalWrite(MOTOR_RR_STEP, LOW);
    digitalWrite(MOTOR_FR_STEP, LOW);
    delayMicroseconds(stepDelay);
  }
}

// 버튼 상태에 따른 LED 색상
void updateButtonLED() {
  if(nunchaku.buttonC && nunchaku.buttonZ) {
    leds[0] = CRGB(255, 0, 255);  // 마젠타
  }
  else if(nunchaku.buttonC) {
    leds[0] = CRGB(255, 0, 0);  // 빨강
  }
  else if(nunchaku.buttonZ) {
    leds[0] = CRGB(0, 0, 255);  // 파랑
  }
  else {
    leds[0] = CRGB(0, 255, 0);  // 초록 (정상 작동)
  }
  FastLED.show();
}

void loop() {
  // Nunchaku 데이터 읽기
  if(nunchakuRead()) {
    // Z 버튼이 눌려있을 때만 이동
    if(nunchaku.buttonZ) {
      // 조이스틱 -> 목표 차체 속도
      float target_v_x, target_v_y, target_omega;
      joystickToVelocity(nunchaku.joyX, nunchaku.joyY, target_v_x, target_v_y, target_omega);
      
      // 가속도 제한 적용 (dt = 0.1초)
      applyAcceleration(target_v_x, target_v_y, target_omega, 
                        current_v_x, current_v_y, current_omega, 0.1);
      
      // 현재 속도 -> 바퀴 각속도
      inverseKinematics(current_v_x, current_v_y, current_omega, omega_FL, omega_RL, omega_RR, omega_FR);
      
      // 로봇 이동 (dt = 0.1초)
      moveRobot(omega_FL, omega_RL, omega_RR, omega_FR, 0.1);
    } else {
      // Z 버튼 미입력 시 모터 정지 및 현재 속도 리셋
      current_v_x = 0;
      current_v_y = 0;
      current_omega = 0;
      omega_FL = 0;
      omega_RL = 0;
      omega_RR = 0;
      omega_FR = 0;
    }
    
    // LED 업데이트
    updateButtonLED();
  }
}
