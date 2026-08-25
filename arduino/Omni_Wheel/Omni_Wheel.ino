// TMC2209 3륜 옴니휠(Kiwi Drive) 제어 코드
// ESP32 DEVKIT 사용
// Wii Nunchaku로 제어
//
// 역기구학 근거: OmniWheel_calc.md 3~5장 (접선 구동 Kiwi, 정삼각형 120도 배치)
// 조작: 조이스틱 = XY 이동 / 넌차쿠 좌우 기울임 = 제자리 회전 / Z 버튼 = 주행 허용
//
// [Mecanum_Wheel.ino와 공유하는 미해결 항목]
//  - dt = 0.1 이 실제 루프 주기와 무관한 상수다. 실측 후 두 스케치를 함께 수정할 것.
//  - 스텝 계산의 소수점 버림으로 저속 입력이 사라진다.
//  - 배터리 ADC가 만충(12.6V) 부근에서 포화된다. 측정 상한은 약 12.5V.

#include <Wire.h>
#include <FastLED.h>

// ========== 핀 정의 ==========
// PCB의 4개 드라이버 중 U2 / U3 / U4 (MOTOR-FL/A, RL/B, RR/C)를 사용한다.
// U5(MOTOR-FR, GPIO25/26)는 3륜 구성에서 사용하지 않는다.

// Wheel A (θ = 0°)  - U2 / MOTOR-FL/A
#define MOTOR_A_DIR     16
#define MOTOR_A_STEP    17

// Wheel B (θ = 120°) - U3 / MOTOR-RL/B
#define MOTOR_B_DIR     18
#define MOTOR_B_STEP    19

// Wheel C (θ = 240°) - U4 / MOTOR-RR/C
#define MOTOR_C_DIR     13
#define MOTOR_C_STEP    14

// 공통 ENABLE 핀
#define ENABLE_PIN    27  // D27

// TMC2209 ENABLE 극성 (액티브 LOW)
#define ENABLE_ACTIVE_LEVEL     LOW   // 드라이버 활성화
#define ENABLE_INACTIVE_LEVEL   HIGH  // 드라이버 비활성화

// 배터리 모니터링 핀
#define BATTERY_PIN   34  // D34 (ADC1_CH6)

// WS2812 LED 설정
#define LED_PIN       23  // D23 (WS2812 출력)
#define NUM_LEDS      1
#define LED_BRIGHTNESS 50

CRGB leds[NUM_LEDS];

// Wii Nunchaku I2C 주소
#define NUNCHAKU_ADDRESS 0x52

// ========== 휠 배치 ==========
//
//                +x (전방)
//                 ▲
//                 A          θ = 0°
//                / \
//               /   \
//              B     C
//        θ=120°       θ=240°
//      (후방 좌측)      (후방 우측)
//
//   +y = 좌측 (ROS 표준, Mecanum_calc.md와 동일)
//   ω  = 반시계(CCW)가 양(+)
//
//   각 휠의 구동 방향은 원의 접선 방향이며, 세 휠 모두 CCW 접선을 양(+)으로 본다.
//   → 순수 CCW 회전에서 세 휠이 같은 방향, 같은 크기로 회전한다.

// Nunchaku 데이터 구조체
struct NunchakuData {
  int joyX;       // 조이스틱 X (0-255)
  int joyY;       // 조이스틱 Y (0-255)
  int accelX;     // 가속도계 X (좌우 기울임)
  int accelY;     // 가속도계 Y (전후 기울임)
  int accelZ;     // 가속도계 Z
  bool buttonC;   // C 버튼
  bool buttonZ;   // Z 버튼
};

NunchakuData nunchaku;

// ========== 차체 파라미터 ==========
// 아래 2개는 실측 후 반드시 수정할 것.
const float WHEEL_DIAMETER = 65.0;      // 옴니휠 지름 (mm) - 실측 필요
const float WHEEL_RADIUS = WHEEL_DIAMETER / 2.0;
const float ROBOT_RADIUS = 100.0;       // 차체 중심 ~ 휠 중심 거리 R (mm) - 실측 필요

// 모터 극성
// 해당 휠에 양(+) 접선 속도를 줬을 때 DIR = HIGH가 정회전이면 true.
// 실기에서 특정 바퀴만 반대로 돌면 그 바퀴만 false로 바꾼다.
// 2026-08-25 실측: 세 바퀴 모두 반대 방향이어서 전역 반전(false)으로 확정.
const bool MOTOR_A_POSITIVE_IS_HIGH = false;
const bool MOTOR_B_POSITIVE_IS_HIGH = false;
const bool MOTOR_C_POSITIVE_IS_HIGH = false;

// 모터 파라미터

const int STEPS_PER_REV = 200;          // 1회전당 스텝 (1.8도)
const int MICROSTEPS = 8;               // 마이크로스텝 설정 (TMC2209 DIP 전부 OFF = 1/8)
const int TOTAL_STEPS_PER_REV = STEPS_PER_REV * MICROSTEPS;  // 1,600
const float MIN_STEP_DELAY_US = 50;     // 최소 스텝 딜레이 (μs) - 최대 속도 기준

// 조이스틱 데드존 및 스케일
const int JOY_CENTER = 128;
const int JOY_DEADZONE = 20;
const float MAX_VELOCITY = 12.0;        // mm/s (최대 이동 속도)
const float MAX_ACCELERATION = 1.5;     // mm/s^2 (최대 가속도)

// 회전(요) 파라미터
// 순수 회전 시 바퀴 각속도(R * omega / r)가 순수 이동 시(MAX_VELOCITY / r)와
// 같아지도록 정의한다. MAX_VELOCITY를 조정하면 회전 속도도 같은 비율로 따라간다.
const float MAX_OMEGA = MAX_VELOCITY / ROBOT_RADIUS;  // rad/s

// 가속도계(기울임 -> 회전) 파라미터
// 아래 값들은 실측 후 조정해야 한다.
const int ACCEL_DEADZONE = 20;              // 중심 부근 무시 범위 (카운트)
const int ACCEL_X_CENTER = 512;             // 좌우 수평일 때의 accelX
const float ACCEL_COUNTS_PER_G = 200.0;     // 1g당 카운트
const float MAX_TILT_G = 0.5;               // 최대 입력으로 볼 기울임 (약 30도)
const float TILT_DIRECTION = 1.0;           // 실기에서 회전 방향이 반대면 -1.0으로

// 통신 실패 재초기화 임계값
const int COMM_FAIL_LIMIT = 10;
int commFailCount = 0;

// 모터 드라이버 활성화 상태 (부팅 시 반드시 비활성)
bool motorsEnabled = false;

// 현재 속도 (가속도 제어용)
float current_v_x = 0, current_v_y = 0, current_omega = 0;

// 모터 속도 (rad/s)
float omega_A = 0, omega_B = 0, omega_C = 0;

// 배터리 저전압 상태
bool isLowVolt = false;
unsigned long lastBlinkTime = 0;
bool ledBlinkState = false;

// 배터리 전압 캐시
float cachedBattVolt = 0;
unsigned long lastBatteryReadTime = 0;
const unsigned long BATTERY_READ_INTERVAL = 1000;  // 1초마다 배터리 읽기

// 배터리 측정 파라미터
const int BATTERY_SAMPLE_COUNT = 10;            // 평균에 사용할 샘플 수
const int BATTERY_SAMPLE_INTERVAL_US = 200;     // 샘플 간 간격
const int BATTERY_ADC_SATURATED = 4000;         // 이 값 이상은 만충(측정 상한 부근)으로 보고 제외
const float LOW_VOLTAGE_THRESHOLD = 10.0;       // 이 값 이하이면 저전압 진입
const float LOW_VOLTAGE_RECOVER = 10.3;         // 이 값 이상이면 저전압 해제 (히스테리시스)
const unsigned long LOW_VOLT_BLINK_INTERVAL = 250;  // 저전압 + 주행 시 LED 점멸 주기 (ms)

// 배터리 전압 계산 상수 (R12 = 10k / R13 = 3.3k 분압)
const float ADC_MAX = 4095.0;
const float VOLTAGE_CONVERSION_FACTOR = 3.3 / 0.2481;  // 13.30
float BATTERY_CONVERSION = 0;  // setup()에서 계산

// ========== 모터 드라이버 ENABLE 제어 ==========

// 3개 TMC2209의 공통 ENABLE 핀 제어
// 유효한 Nunchaku 조작이 있을 때만 활성화한다.
void setMotorsEnabled(bool enabled) {
  if(motorsEnabled == enabled) return;  // 상태가 같으면 불필요한 출력 갱신 생략

  motorsEnabled = enabled;
  digitalWrite(ENABLE_PIN, enabled ? ENABLE_ACTIVE_LEVEL : ENABLE_INACTIVE_LEVEL);

  Serial.println(enabled ? "Motor driver: ENABLED" : "Motor driver: DISABLED");
}

// 드라이버 비활성화 + 속도 상태 초기화
// 통신이 끊겼다가 복구되어도 직전 속도로 갑자기 재가동하지 않도록 한다.
void enterFailSafe() {
  setMotorsEnabled(false);

  current_v_x = 0;
  current_v_y = 0;
  current_omega = 0;
  omega_A = 0;
  omega_B = 0;
  omega_C = 0;
}

// ========== Nunchaku 함수 ==========

// Nunchaku 초기화 (암호화 없이 사용하는 방식)
void nunchakuInit() {
  Wire.beginTransmission(NUNCHAKU_ADDRESS);
  Wire.write(0xF0);
  Wire.write(0x55);
  if(Wire.endTransmission() != 0) {
    Serial.println("Nunchaku init failed (0xF0 0x55)");
  }
  delay(1);

  Wire.beginTransmission(NUNCHAKU_ADDRESS);
  Wire.write(0xFB);
  Wire.write(0x00);
  if(Wire.endTransmission() != 0) {
    Serial.println("Nunchaku init failed (0xFB 0x00)");
  }
  delay(1);
}

// 다음 읽기를 위한 레지스터 포인터 쓰기
void nunchakuRequest() {
  Wire.beginTransmission(NUNCHAKU_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();
}

// Nunchaku 데이터 읽기
// 정확히 6바이트를 받은 경우에만 값을 갱신한다.
bool nunchakuRead() {
  uint8_t data[6];

  uint8_t received = Wire.requestFrom((uint8_t)NUNCHAKU_ADDRESS, (uint8_t)6);
  if(received != 6) {
    while(Wire.available()) Wire.read();  // 잔여 바이트 폐기
    return false;
  }

  for(uint8_t i = 0; i < 6; i++) {
    data[i] = Wire.read();
  }

  // 전 바이트가 0x00 또는 0xFF이면 버스 이상으로 간주한다.
  // 특히 0x00은 그대로 해석하면 'Z 누름 + 조이스틱 최대'가 되어 폭주로 이어진다.
  bool allZero = true;
  bool allOnes = true;
  for(uint8_t i = 0; i < 6; i++) {
    if(data[i] != 0x00) allZero = false;
    if(data[i] != 0xFF) allOnes = false;
  }
  if(allZero || allOnes) {
    nunchakuRequest();
    return false;
  }

  nunchaku.joyX = data[0];
  nunchaku.joyY = data[1];
  nunchaku.accelX = (data[2] << 2) | ((data[5] >> 2) & 0x03);
  nunchaku.accelY = (data[3] << 2) | ((data[5] >> 4) & 0x03);
  nunchaku.accelZ = (data[4] << 2) | ((data[5] >> 6) & 0x03);
  nunchaku.buttonC = !((data[5] >> 1) & 0x01);
  nunchaku.buttonZ = !(data[5] & 0x01);

  nunchakuRequest();
  return true;
}

// ========== 입력 변환 ==========

// 조이스틱 한 축의 데드존 처리 및 정규화
// 데드존 바깥 구간을 0.0~1.0으로 재매핑하여 경계에서 속도가 튀지 않게 한다.
float normalizeJoyAxis(int offset) {
  int magnitude = abs(offset);
  if(magnitude < JOY_DEADZONE) return 0;

  float span = (float)(JOY_CENTER - JOY_DEADZONE);
  float value = (magnitude - JOY_DEADZONE) / span;
  value = constrain(value, 0.0, 1.0);

  return (offset < 0) ? -value : value;
}

// 조이스틱 입력을 차체 이동 속도로 변환 (벡터 기반)
void joystickToVelocity(int joyX, int joyY, float &v_x, float &v_y) {
  // Nunchaku 조이스틱 범위: 0-255 (중립값 128)
  // X축: 좌우 (0=좌, 255=우) / Y축: 전후 (0=후, 255=전)
  float joyX_norm = normalizeJoyAxis(joyX - JOY_CENTER);
  float joyY_norm = normalizeJoyAxis(joyY - JOY_CENTER);

  float magnitude = sqrt(joyX_norm * joyX_norm + joyY_norm * joyY_norm);

  if(magnitude <= 0) {
    v_x = 0;
    v_y = 0;
    return;
  }

  // 방향 벡터는 클램프 전 크기로 정규화한다.
  // (클램프된 값으로 나누면 대각선 입력에서 합성 속도가 최대치를 넘는다)
  float dir_x = joyY_norm / magnitude;   // 앞쪽 입력 = 전진(+x)
  float dir_y = -joyX_norm / magnitude;  // 왼쪽 입력 = 좌측 이동(+y)

  // 속도 크기: 많이 밀수록 빠르게 (제곱 곡선)
  float speed = constrain(magnitude, 0.0, 1.0);
  float velocity = speed * speed * MAX_VELOCITY;

  v_x = velocity * dir_x;
  v_y = velocity * dir_y;
}

// 넌차쿠 좌우 기울임(roll)을 요 각속도로 변환
// 조이스틱 = XY 이동 전담, 좌우 기울임 = 제자리 회전 전담
float accelerometerToOmega(int accelX) {
  int offset = accelX - ACCEL_X_CENTER;

  // 데드존: 손떨림 수준의 미세한 기울임은 무시
  if(abs(offset) < ACCEL_DEADZONE) return 0;

  // 데드존 바깥 구간을 0.0~1.0으로 재매핑
  float span = ACCEL_COUNTS_PER_G * MAX_TILT_G - ACCEL_DEADZONE;
  float tilt = (abs(offset) - ACCEL_DEADZONE) / span;
  tilt = constrain(tilt, 0.0, 1.0);

  // 많이 기울일수록 빠르게 (조이스틱과 동일한 제곱 곡선)
  float omega = tilt * tilt * MAX_OMEGA;

  // 좌표계 규칙: 반시계(CCW)가 양(+)
  // 왼쪽으로 기울이면 왼쪽으로 회전(CCW), 오른쪽으로 기울이면 오른쪽으로 회전(CW)
  if(offset > 0) omega = -omega;

  return omega * TILT_DIRECTION;
}

// ========== 역기구학 ==========

// Kiwi Drive 역기구학: 차체 속도 -> 바퀴 각속도 (rad/s)
// OmniWheel_calc.md 4장의 식을 그대로 사용한다.
//   s_A = v_y + R*ω
//   s_B = -(√3/2)v_x - (1/2)v_y + R*ω
//   s_C = +(√3/2)v_x - (1/2)v_y + R*ω
void inverseKinematics(float v_x, float v_y, float omega,
                       float &w_A, float &w_B, float &w_C) {
  const float SQRT3_2 = 0.8660254;  // √3 / 2

  // 각 바퀴의 접선 선속도 (mm/s)
  float s_A = v_y + ROBOT_RADIUS * omega;
  float s_B = -SQRT3_2 * v_x - 0.5 * v_y + ROBOT_RADIUS * omega;
  float s_C = SQRT3_2 * v_x - 0.5 * v_y + ROBOT_RADIUS * omega;

  // 선속도 -> 바퀴 각속도 (OmniWheel_calc.md 5.1)
  w_A = s_A / WHEEL_RADIUS;
  w_B = s_B / WHEEL_RADIUS;
  w_C = s_C / WHEEL_RADIUS;
}

// 가속도 제한을 적용하여 현재 속도를 목표 속도로 업데이트
void applyAcceleration(float target_v_x, float target_v_y, float target_omega,
                       float &cur_v_x, float &cur_v_y, float &cur_omega,
                       float dt) {
  // 최대 속도 변화량 (가속도 * 시간)
  float max_delta_v = MAX_ACCELERATION * dt;

  // v_x 가속도 제한
  float delta_v_x = target_v_x - cur_v_x;
  if(abs(delta_v_x) > max_delta_v) {
    delta_v_x = (delta_v_x > 0) ? max_delta_v : -max_delta_v;
  }
  cur_v_x += delta_v_x;

  // v_y 가속도 제한
  float delta_v_y = target_v_y - cur_v_y;
  if(abs(delta_v_y) > max_delta_v) {
    delta_v_y = (delta_v_y > 0) ? max_delta_v : -max_delta_v;
  }
  cur_v_y += delta_v_y;

  // omega 가속도 제한
  // R(차체 중심 ~ 휠 중심 거리)로 나누어 선가속도와 같은 비율을 적용한다.
  float max_delta_omega = (MAX_ACCELERATION / ROBOT_RADIUS) * dt;
  float delta_omega = target_omega - cur_omega;
  if(abs(delta_omega) > max_delta_omega) {
    delta_omega = (delta_omega > 0) ? max_delta_omega : -max_delta_omega;
  }
  cur_omega += delta_omega;
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

// ========== 모터 구동 ==========

// 3개 모터 동시 제어 (최대 스텝 수 기준, 가변 속도)
void moveRobot(float w_A, float w_B, float w_C, float dt) {
  int steps_A, steps_B, steps_C;
  bool dir_A, dir_B, dir_C;

  // 각 바퀴의 스텝 수와 방향 계산
  angularVelocityToSteps(w_A, steps_A, dir_A, dt);
  angularVelocityToSteps(w_B, steps_B, dir_B, dt);
  angularVelocityToSteps(w_C, steps_C, dir_C, dt);

  // 방향 설정 (모터별 극성 상수로 보정)
  digitalWrite(MOTOR_A_DIR, (dir_A == MOTOR_A_POSITIVE_IS_HIGH) ? HIGH : LOW);
  digitalWrite(MOTOR_B_DIR, (dir_B == MOTOR_B_POSITIVE_IS_HIGH) ? HIGH : LOW);
  digitalWrite(MOTOR_C_DIR, (dir_C == MOTOR_C_POSITIVE_IS_HIGH) ? HIGH : LOW);

  // 최대 스텝 수 계산
  int maxSteps = max(max(steps_A, steps_B), steps_C);
  if(maxSteps <= 0) return;

  // 최대 각속도에 의해 역으로 계산되는 스텝 딜레이 (속도가 클수록 딜레이 작음)
  float maxOmega = max(max(abs(w_A), abs(w_B)), abs(w_C));
  float stepDelay = MIN_STEP_DELAY_US;

  if(maxOmega > 0.01) {  // 최소 각속도 임계값
    float maxOmegaExpected = MAX_VELOCITY / WHEEL_RADIUS;
    float normalizedOmega = constrain(maxOmega / maxOmegaExpected, 0.01, 1.0);
    stepDelay = MIN_STEP_DELAY_US / normalizedOmega;
  }

  // 동시 스텝 실행
  for(int i = 0; i < maxSteps; i++) {
    if(i < steps_A) digitalWrite(MOTOR_A_STEP, HIGH);
    if(i < steps_B) digitalWrite(MOTOR_B_STEP, HIGH);
    if(i < steps_C) digitalWrite(MOTOR_C_STEP, HIGH);
    delayMicroseconds(stepDelay);

    digitalWrite(MOTOR_A_STEP, LOW);
    digitalWrite(MOTOR_B_STEP, LOW);
    digitalWrite(MOTOR_C_STEP, LOW);
    delayMicroseconds(stepDelay);
  }
}

// ========== 배터리 전압 ==========

// 배터리 전압을 측정하고 저전압 상태를 갱신한다.
// 반드시 모터가 비활성인 상태에서만 호출할 것.
// 주행 중에는 순간 4~5A의 부하로 전압이 크게 강하해 저전압을 오검출한다.
void updateBatteryVoltage() {
  long sum = 0;
  int validCount = 0;

  for(int i = 0; i < BATTERY_SAMPLE_COUNT; i++) {
    int raw = analogRead(BATTERY_PIN);

    // 만충 부근에서는 ADC가 상한에 붙어 값이 의미가 없으므로 평균에서 제외한다.
    // (분압비 0.2481에서 측정 가능 상한은 약 12.5V)
    if(raw < BATTERY_ADC_SATURATED) {
      sum += raw;
      validCount++;
    }

    delayMicroseconds(BATTERY_SAMPLE_INTERVAL_US);
  }

  // 유효 샘플이 하나도 없으면 만충 상태다. 저전압일 수 없다.
  if(validCount == 0) {
    cachedBattVolt = ADC_MAX * BATTERY_CONVERSION;  // 측정 상한값으로 표시
    isLowVolt = false;
    return;
  }

  cachedBattVolt = (sum / (float)validCount) * BATTERY_CONVERSION;

  // 임계값 근처에서 상태가 떨리지 않도록 진입/해제 값을 다르게 둔다.
  if(cachedBattVolt <= LOW_VOLTAGE_THRESHOLD) {
    isLowVolt = true;
  } else if(cachedBattVolt >= LOW_VOLTAGE_RECOVER) {
    isLowVolt = false;
  }
}

// ========== 상태 표시 ==========

// 버튼 및 배터리 상태에 따른 LED 색상
void updateStatusLED() {
  // 저전압 상태 (버튼 색상보다 우선)
  //   정지 중  : 빨강 상시 점등
  //   Z 입력 중 : 빨강 점멸 (저전압인데 주행하려는 상태)
  if(isLowVolt) {
    if(nunchaku.buttonZ) {
      unsigned long currentTime = millis();
      if(currentTime - lastBlinkTime >= LOW_VOLT_BLINK_INTERVAL) {
        lastBlinkTime = currentTime;
        ledBlinkState = !ledBlinkState;
      }
      leds[0] = ledBlinkState ? CRGB(255, 0, 0) : CRGB(0, 0, 0);
    } else {
      // 다음에 Z를 눌렀을 때 점등부터 시작하도록 초기화
      lastBlinkTime = millis();
      ledBlinkState = true;
      leds[0] = CRGB(255, 0, 0);
    }
  }
  // 정상 상태: 버튼에 따른 색상
  else if(nunchaku.buttonC && nunchaku.buttonZ) {
    leds[0] = CRGB(255, 0, 255);  // 마젠타
  }
  else if(nunchaku.buttonC) {
    leds[0] = CRGB(255, 0, 0);    // 빨강
  }
  else if(nunchaku.buttonZ) {
    leds[0] = CRGB(0, 0, 255);    // 파랑
  }
  else {
    leds[0] = CRGB(0, 255, 0);    // 초록 (정상 작동)
  }
  FastLED.show();
}

// 통신 두절 표시: 노랑
void showCommLostLED() {
  leds[0] = CRGB(255, 120, 0);
  FastLED.show();
}

// ========== 초기화 ==========

void setup() {
  Serial.begin(115200);

  // 모터 드라이버 비활성화를 최우선으로 처리한다.
  // 출력 래치를 먼저 비활성 레벨로 올린 뒤 OUTPUT으로 전환해야
  // pinMode() 직후 한순간도 드라이버가 여자되지 않는다.
  digitalWrite(ENABLE_PIN, ENABLE_INACTIVE_LEVEL);
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, ENABLE_INACTIVE_LEVEL);
  motorsEnabled = false;

  // I2C 초기화
  Wire.begin();

  // 핀 모드 설정
  pinMode(MOTOR_A_DIR, OUTPUT);
  pinMode(MOTOR_A_STEP, OUTPUT);
  pinMode(MOTOR_B_DIR, OUTPUT);
  pinMode(MOTOR_B_STEP, OUTPUT);
  pinMode(MOTOR_C_DIR, OUTPUT);
  pinMode(MOTOR_C_STEP, OUTPUT);
  pinMode(BATTERY_PIN, INPUT);

  digitalWrite(MOTOR_A_STEP, LOW);
  digitalWrite(MOTOR_B_STEP, LOW);
  digitalWrite(MOTOR_C_STEP, LOW);

  // WS2812 LED 초기화
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);

  // Nunchaku 초기화
  nunchakuInit();
  nunchakuRequest();  // 첫 읽기 전에 레지스터 포인터를 0x00으로 맞춘다
  delay(100);

  // 배터리 전압 계산 계수 미리 계산
  BATTERY_CONVERSION = VOLTAGE_CONVERSION_FACTOR / ADC_MAX;

  // 부팅 직후 1회 측정 (이 시점에는 모터가 확실히 비활성이다)
  updateBatteryVoltage();
  Serial.print("Battery: ");
  Serial.print(cachedBattVolt, 2);
  Serial.println(isLowVolt ? " V  (LOW)" : " V");

  // 모터는 여기서 활성화하지 않는다.
  // 유효한 Nunchaku 응답과 Z 버튼 입력이 확인된 뒤 loop()에서 활성화한다.
  Serial.println("Omni wheel (Kiwi drive) ready.");
  Serial.println("Motor driver is DISABLED until Nunchaku input.");

  delay(100);
}

// ========== 메인 루프 ==========

void loop() {
  // Loop 시작 시 1번만 currentTime 측정
  unsigned long currentTime = millis();

  // Nunchaku 데이터 읽기
  if(nunchakuRead()) {
    commFailCount = 0;

    // Z 버튼이 눌려있을 때만 이동
    if(nunchaku.buttonZ) {
      // 유효한 조작 입력이 확인된 경우에만 드라이버 활성화
      setMotorsEnabled(true);

      // 조이스틱 -> 목표 차체 이동 속도 (XY)
      float target_v_x, target_v_y;
      joystickToVelocity(nunchaku.joyX, nunchaku.joyY, target_v_x, target_v_y);

      // 넌차쿠 좌우 기울임 -> 목표 요 각속도 (제자리 회전)
      float target_omega = accelerometerToOmega(nunchaku.accelX);

      // 가속도 제한 적용 (dt = 0.1초)
      applyAcceleration(target_v_x, target_v_y, target_omega,
                        current_v_x, current_v_y, current_omega, 0.1);

      // 현재 속도 -> 바퀴 각속도
      inverseKinematics(current_v_x, current_v_y, current_omega, omega_A, omega_B, omega_C);

      // 로봇 이동 (dt = 0.1초)
      moveRobot(omega_A, omega_B, omega_C, 0.1);
    } else {
      // Z 버튼 미입력 시 드라이버 비활성화 및 현재 속도 리셋
      enterFailSafe();
    }

    // LED 업데이트
    updateStatusLED();

    // 배터리 전압 업데이트 (1초마다 한 번)
    // 모터가 비활성일 때만 측정한다. Z를 누르고 주행 중이면 직전 값을 유지한다.
    if(!motorsEnabled && currentTime - lastBatteryReadTime >= BATTERY_READ_INTERVAL) {
      lastBatteryReadTime = currentTime;
      updateBatteryVoltage();
    }
  } else {
    // 통신 실패: 즉시 정지시키고 속도 상태를 버린다.
    enterFailSafe();
    showCommLostLED();

    // 실패가 이어지면 넌차쿠가 재연결된 것으로 보고 다시 초기화한다.
    commFailCount++;
    if(commFailCount >= COMM_FAIL_LIMIT) {
      commFailCount = 0;
      Serial.println("Nunchaku communication lost. Re-initializing...");
      nunchakuInit();
      nunchakuRequest();
    }

    delay(10);
  }
}
