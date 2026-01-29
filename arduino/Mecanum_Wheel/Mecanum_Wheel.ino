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
#define MOTOR_FL_DIR    21
#define MOTOR_FL_STEP   22

// Rear Left Motor
#define MOTOR_RL_DIR    24
#define MOTOR_RL_STEP   25

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
const int MICROSTEPS = 32;              // 마이크로스텝 설정 (TMC2209)
const int TOTAL_STEPS_PER_REV = STEPS_PER_REV * MICROSTEPS;  // 6400
const float MIN_STEP_DELAY_US = 200;    // 최소 스텝 딜레이 (마이크로초)
const float MAX_STEP_DELAY_US = 1000;   // 최대 스텝 딜레이 (마이크로초)

// 조이스틱 데드존 및 스케일
const int JOY_CENTER = 128;
const int JOY_DEADZONE = 20;
const float MAX_VELOCITY = 100.0;       // mm/s

// 모터 속도 (rad/s)
float omega_FL = 0, omega_RL = 0, omega_RR = 0, omega_FR = 0;

void setup() {
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
  
  // 모터 활성화
  digitalWrite(ENABLE_PIN, LOW);
  
  delay(100);
}

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

// 조이스틱 입력을 차체 속도로 변환 (벡터 기반)
void joystickToVelocity(int joyX, int joyY, float &v_x, float &v_y, float &omega) {
  // 중심 기준으로 변환
  int dx = joyX - JOY_CENTER;  // 양수=오른쪽, 음수=왼쪽
  int dy = joyY - JOY_CENTER;  // 양수=앞, 음수=뒤
  
  // 데드존 적용
  if(abs(dx) < JOY_DEADZONE) dx = 0;
  if(abs(dy) < JOY_DEADZONE) dy = 0;
  
  // 벡터 크기(기울기) 계산
  float magnitude = sqrt(dx * dx + dy * dy);
  
  if(magnitude > 0) {
    // 최대 크기로 정규화
    float maxMagnitude = sqrt(JOY_CENTER * JOY_CENTER * 2);
    float normalizedMag = magnitude / maxMagnitude;
    normalizedMag = constrain(normalizedMag, 0.0, 1.0);
    
    // 속도 크기 계산
    float velocity = normalizedMag * MAX_VELOCITY;
    
    // 정규화된 방향 벡터
    float dir_x = dx / magnitude;
    float dir_y = dy / magnitude;
    
    // 조이스틱 방향을 차체 좌표로 매핑
    // dy(전후) → v_x(전후), -dx(좌우) → v_y(좌우, 오른쪽을 음수로)
    v_x = velocity * dir_y;   // 앞으로 기울이면 전진(양수)
    v_y = velocity * (-dir_x); // 오른쪽으로 기울이면 우측(음수)
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
  
  // 최대 각속도로 스텝 딜레이 계산 (각속도가 클수록 딜레이 작음)
  float maxOmega = max(max(abs(w_FL), abs(w_RL)), max(abs(w_RR), abs(w_FR)));
  float stepDelay = MAX_STEP_DELAY_US;
  if(maxOmega > 0.1) {  // 최소 각속도 임계값
    // 각속도에 반비례하는 딜레이 (각속도 클수록 딜레이 짧음)
    stepDelay = map(maxOmega * 100, 0, MAX_VELOCITY / WHEEL_RADIUS * 100, MAX_STEP_DELAY_US, MIN_STEP_DELAY_US);
    stepDelay = constrain(stepDelay, MIN_STEP_DELAY_US, MAX_STEP_DELAY_US);
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
    // 조이스틱 -> 차체 속도
    float v_x, v_y, omega;
    joystickToVelocity(nunchaku.joyX, nunchaku.joyY, v_x, v_y, omega);
    
    // 차체 속도 -> 바퀴 각속도
    inverseKinematics(v_x, v_y, omega, omega_FL, omega_RL, omega_RR, omega_FR);
    
    // 로봇 이동 (dt = 0.05초)
    moveRobot(omega_FL, omega_RL, omega_RR, omega_FR, 0.05);
    
    // LED 업데이트
    updateButtonLED();
  }
  
 //  delay(50);  // 50ms = 20Hz 제어 주기
}
