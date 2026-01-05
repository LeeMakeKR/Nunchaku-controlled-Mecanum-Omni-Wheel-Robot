// TMC2209 스텝 드라이버 테스트 코드
// Left Front Motor
#define MOTOR_RR_DIR    16  // RX2
#define MOTOR_RR_STEP   17  // TX2

// Right Front Motor
#define MOTOR_RF_DIR    18  // D18
#define MOTOR_RF_STEP   19  // D19

// Left Rear Motor
#define MOTOR_LR_DIR    13  // D13
#define MOTOR_LR_STEP   14  // D14

// Right Rear Motor
#define MOTOR_LF_DIR    25  // D25
#define MOTOR_LF_STEP   26  // D26

// 공통 ENABLE 핀
#define ENABLE_PIN    27  // D27

// 배터리 전압 측정 (VBAT)
#define BATTERY_PIN   34  // D34 (아날로그 입력)

// WS2812 LED 설정
#define LED_PIN       23  // D23 (WS2812 출력)
#define NUM_LEDS      1   // LED 개수
#define LED_BRIGHTNESS 50 // 밝기 (0-255)

// 스텝 모터 타이밍 설정
const int STEP_DELAY = 600;  // 스텝 간 딜레이 (마이크로초)

void setup() {
  // Left Front 모터 핀 모드 설정
  pinMode(MOTOR_LF_DIR, OUTPUT);
  pinMode(MOTOR_LF_STEP, OUTPUT);
  // Left Rear 모터 핀 모드 설정
  pinMode(MOTOR_LR_DIR, OUTPUT);
  pinMode(MOTOR_LR_STEP, OUTPUT);
  // Right Rear 모터 핀 모드 설정
  pinMode(MOTOR_RR_DIR, OUTPUT);
  pinMode(MOTOR_RR_STEP, OUTPUT);
  // Right Front 모터 핀 모드 설정
  pinMode(MOTOR_RF_DIR, OUTPUT);
  pinMode(MOTOR_RF_STEP, OUTPUT);
  

  

  
  // 공통 ENABLE 핀 설정
  pinMode(ENABLE_PIN, OUTPUT);
  
  // 드라이버 활성화 (LOW = 활성화)
  digitalWrite(ENABLE_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("TMC2209 테스트 시작");
}

// 차체 전진 함수
void moveForward(int steps_LF, int steps_LR, int steps_RF, int steps_RR, int stepDelay) {
  digitalWrite(MOTOR_LF_DIR, HIGH);  // Left Front 정회전
  digitalWrite(MOTOR_LR_DIR, HIGH);  // Left Rear 정회전
  digitalWrite(MOTOR_RF_DIR, LOW);   // Right Front 정회전
  digitalWrite(MOTOR_RR_DIR, LOW);   // Right Rear 정회전
  
  int maxSteps = max(max(steps_LF, steps_LR), max(steps_RF, steps_RR));
  
  for(int i = 0; i < maxSteps; i++) {
    if(i < steps_LF) digitalWrite(MOTOR_LF_STEP, HIGH);
    if(i < steps_LR) digitalWrite(MOTOR_LR_STEP, HIGH);
    if(i < steps_RF) digitalWrite(MOTOR_RF_STEP, HIGH);
    if(i < steps_RR) digitalWrite(MOTOR_RR_STEP, HIGH);
    delayMicroseconds(stepDelay);
    
    digitalWrite(MOTOR_LF_STEP, LOW);
    digitalWrite(MOTOR_LR_STEP, LOW);
    digitalWrite(MOTOR_RF_STEP, LOW);
    digitalWrite(MOTOR_RR_STEP, LOW);
    delayMicroseconds(stepDelay);
  }
}

// 차체 후진 함수
void moveBackward(int steps_LF, int steps_LR, int steps_RF, int steps_RR, int stepDelay) {
  digitalWrite(MOTOR_LF_DIR, LOW);   // Left Front 역회전
  digitalWrite(MOTOR_LR_DIR, LOW);   // Left Rear 역회전
  digitalWrite(MOTOR_RF_DIR, HIGH);  // Right Front 역회전
  digitalWrite(MOTOR_RR_DIR, HIGH);  // Right Rear 역회전
  
  int maxSteps = max(max(steps_LF, steps_LR), max(steps_RF, steps_RR));
  
  for(int i = 0; i < maxSteps; i++) {
    if(i < steps_LF) digitalWrite(MOTOR_LF_STEP, HIGH);
    if(i < steps_LR) digitalWrite(MOTOR_LR_STEP, HIGH);
    if(i < steps_RF) digitalWrite(MOTOR_RF_STEP, HIGH);
    if(i < steps_RR) digitalWrite(MOTOR_RR_STEP, HIGH);
    delayMicroseconds(stepDelay);
    
    digitalWrite(MOTOR_LF_STEP, LOW);
    digitalWrite(MOTOR_LR_STEP, LOW);
    digitalWrite(MOTOR_RF_STEP, LOW);
    digitalWrite(MOTOR_RR_STEP, LOW);
    delayMicroseconds(stepDelay);
  }
}

// 차체 좌진 함수 (왼쪽 이동)
void moveLeft(int steps_LF, int steps_LR, int steps_RF, int steps_RR, int stepDelay) {
  digitalWrite(MOTOR_LF_DIR, LOW);   // Left Front 역회전
  digitalWrite(MOTOR_LR_DIR, HIGH);  // Left Rear 정회전
  digitalWrite(MOTOR_RF_DIR, LOW);   // Right Front 정회전
  digitalWrite(MOTOR_RR_DIR, HIGH);  // Right Rear 역회전
  
  int maxSteps = max(max(steps_LF, steps_LR), max(steps_RF, steps_RR));
  
  for(int i = 0; i < maxSteps; i++) {
    if(i < steps_LF) digitalWrite(MOTOR_LF_STEP, HIGH);
    if(i < steps_LR) digitalWrite(MOTOR_LR_STEP, HIGH);
    if(i < steps_RF) digitalWrite(MOTOR_RF_STEP, HIGH);
    if(i < steps_RR) digitalWrite(MOTOR_RR_STEP, HIGH);
    delayMicroseconds(stepDelay);
    
    digitalWrite(MOTOR_LF_STEP, LOW);
    digitalWrite(MOTOR_LR_STEP, LOW);
    digitalWrite(MOTOR_RF_STEP, LOW);
    digitalWrite(MOTOR_RR_STEP, LOW);
    delayMicroseconds(stepDelay);
  }
}

// 차체 우진 함수 (오른쪽 이동)
void moveRight(int steps_LF, int steps_LR, int steps_RF, int steps_RR, int stepDelay) {
  digitalWrite(MOTOR_LF_DIR, HIGH);  // Left Front 정회전
  digitalWrite(MOTOR_LR_DIR, LOW);   // Left Rear 역회전
  digitalWrite(MOTOR_RF_DIR, HIGH);  // Right Front 역회전
  digitalWrite(MOTOR_RR_DIR, LOW);   // Right Rear 정회전
  
  int maxSteps = max(max(steps_LF, steps_LR), max(steps_RF, steps_RR));
  
  for(int i = 0; i < maxSteps; i++) {
    if(i < steps_LF) digitalWrite(MOTOR_LF_STEP, HIGH);
    if(i < steps_LR) digitalWrite(MOTOR_LR_STEP, HIGH);
    if(i < steps_RF) digitalWrite(MOTOR_RF_STEP, HIGH);
    if(i < steps_RR) digitalWrite(MOTOR_RR_STEP, HIGH);
    delayMicroseconds(stepDelay);
    
    digitalWrite(MOTOR_LF_STEP, LOW);
    digitalWrite(MOTOR_LR_STEP, LOW);
    digitalWrite(MOTOR_RF_STEP, LOW);
    digitalWrite(MOTOR_RR_STEP, LOW);
    delayMicroseconds(stepDelay);
  }
}

void loop() {
  // 테스트: 전진
  Serial.println("차체 전진");
  moveForward(6400, 6400, 6400, 6400, STEP_DELAY);
  delay(1000);
  
  // 테스트: 후진
  Serial.println("차체 후진");
  moveBackward(6400, 6400, 6400, 6400, STEP_DELAY);
  delay(1000); 
  
  // 테스트: 좌진
  Serial.println("차체 좌진");
  moveLeft(6400, 6400, 6400, 6400, STEP_DELAY);
  delay(1000);
  
  // 테스트: 우진
  Serial.println("차체 우진");
  moveRight(6400, 6400, 6400, 6400, STEP_DELAY);
  delay(1000);
}
