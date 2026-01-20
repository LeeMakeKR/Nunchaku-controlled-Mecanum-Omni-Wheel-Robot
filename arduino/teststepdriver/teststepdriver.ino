// TMC2209 스텝 드라이버 테스트 코드
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

// 배터리 전압 측정 (VBAT)
#define BATTERY_PIN   34  // D34 (아날로그 입력)

// WS2812 LED 설정
#define LED_PIN       23  // D23 (WS2812 출력)
#define NUM_LEDS      1   // LED 개수
#define LED_BRIGHTNESS 50 // 밝기 (0-255)

// 스텝 모터 타이밍 설정
const int STEP_DELAY = 600;  // 스텝 간 딜레이 (마이크로초)

void setup() {
  // Front Left 모터 핀 모드 설정
  pinMode(MOTOR_FL_DIR, OUTPUT);
  pinMode(MOTOR_FL_STEP, OUTPUT);
  // Rear Left 모터 핀 모드 설정
  pinMode(MOTOR_RL_DIR, OUTPUT);
  pinMode(MOTOR_RL_STEP, OUTPUT);
  // Rear Right 모터 핀 모드 설정
  pinMode(MOTOR_RR_DIR, OUTPUT);
  pinMode(MOTOR_RR_STEP, OUTPUT);
  // Front Right 모터 핀 모드 설정
  pinMode(MOTOR_FR_DIR, OUTPUT);
  pinMode(MOTOR_FR_STEP, OUTPUT);
  

  

  
  // 공통 ENABLE 핀 설정
  pinMode(ENABLE_PIN, OUTPUT);
  
  // 드라이버 활성화 (LOW = 활성화)
  digitalWrite(ENABLE_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("TMC2209 테스트 시작");
}

// 차체 전진 함수
void moveForward(int steps_FL, int steps_RL, int steps_FR, int steps_RR, int stepDelay) {
  digitalWrite(MOTOR_FL_DIR, HIGH);  // Front Left 정회전
  digitalWrite(MOTOR_RL_DIR, HIGH);  // Rear Left 정회전
  digitalWrite(MOTOR_FR_DIR, LOW);   // Front Right 정회전
  digitalWrite(MOTOR_RR_DIR, LOW);   // Rear Right 정회전
  
  int maxSteps = max(max(steps_FL, steps_RL), max(steps_FR, steps_RR));
  
  for(int i = 0; i < maxSteps; i++) {
    if(i < steps_FL) digitalWrite(MOTOR_FL_STEP, HIGH);
    if(i < steps_RL) digitalWrite(MOTOR_RL_STEP, HIGH);
    if(i < steps_FR) digitalWrite(MOTOR_FR_STEP, HIGH);
    if(i < steps_RR) digitalWrite(MOTOR_RR_STEP, HIGH);
    delayMicroseconds(stepDelay);
    
    digitalWrite(MOTOR_FL_STEP, LOW);
    digitalWrite(MOTOR_RL_STEP, LOW);
    digitalWrite(MOTOR_FR_STEP, LOW);
    digitalWrite(MOTOR_RR_STEP, LOW);
    delayMicroseconds(stepDelay);
  }
}

// 차체 후진 함수
void moveBackward(int steps_FL, int steps_RL, int steps_FR, int steps_RR, int stepDelay) {
  digitalWrite(MOTOR_FL_DIR, LOW);   // Front Left 역회전
  digitalWrite(MOTOR_RL_DIR, LOW);   // Rear Left 역회전
  digitalWrite(MOTOR_FR_DIR, HIGH);  // Front Right 역회전
  digitalWrite(MOTOR_RR_DIR, HIGH);  // Rear Right 역회전
  
  int maxSteps = max(max(steps_FL, steps_RL), max(steps_FR, steps_RR));
  
  for(int i = 0; i < maxSteps; i++) {
    if(i < steps_FL) digitalWrite(MOTOR_FL_STEP, HIGH);
    if(i < steps_RL) digitalWrite(MOTOR_RL_STEP, HIGH);
    if(i < steps_FR) digitalWrite(MOTOR_FR_STEP, HIGH);
    if(i < steps_RR) digitalWrite(MOTOR_RR_STEP, HIGH);
    delayMicroseconds(stepDelay);
    
    digitalWrite(MOTOR_FL_STEP, LOW);
    digitalWrite(MOTOR_RL_STEP, LOW);
    digitalWrite(MOTOR_FR_STEP, LOW);
    digitalWrite(MOTOR_RR_STEP, LOW);
    delayMicroseconds(stepDelay);
  }
}

// 차체 좌진 함수 (왼쪽 이동)
void moveLeft(int steps_FL, int steps_RL, int steps_FR, int steps_RR, int stepDelay) {
  digitalWrite(MOTOR_FL_DIR, LOW);   // Front Left 역회전
  digitalWrite(MOTOR_RL_DIR, HIGH);  // Rear Left 정회전
  digitalWrite(MOTOR_FR_DIR, LOW);   // Front Right 정회전
  digitalWrite(MOTOR_RR_DIR, HIGH);  // Rear Right 역회전
  
  int maxSteps = max(max(steps_FL, steps_RL), max(steps_FR, steps_RR));
  
  for(int i = 0; i < maxSteps; i++) {
    if(i < steps_FL) digitalWrite(MOTOR_FL_STEP, HIGH);
    if(i < steps_RL) digitalWrite(MOTOR_RL_STEP, HIGH);
    if(i < steps_FR) digitalWrite(MOTOR_FR_STEP, HIGH);
    if(i < steps_RR) digitalWrite(MOTOR_RR_STEP, HIGH);
    delayMicroseconds(stepDelay);
    
    digitalWrite(MOTOR_FL_STEP, LOW);
    digitalWrite(MOTOR_RL_STEP, LOW);
    digitalWrite(MOTOR_FR_STEP, LOW);
    digitalWrite(MOTOR_RR_STEP, LOW);
    delayMicroseconds(stepDelay);
  }
}

// 차체 우진 함수 (오른쪽 이동)
void moveRight(int steps_FL, int steps_RL, int steps_FR, int steps_RR, int stepDelay) {
  digitalWrite(MOTOR_FL_DIR, HIGH);  // Front Left 정회전
  digitalWrite(MOTOR_RL_DIR, LOW);   // Rear Left 역회전
  digitalWrite(MOTOR_FR_DIR, HIGH);  // Front Right 역회전
  digitalWrite(MOTOR_RR_DIR, LOW);   // Rear Right 정회전
  
  int maxSteps = max(max(steps_FL, steps_RL), max(steps_FR, steps_RR));
  
  for(int i = 0; i < maxSteps; i++) {
    if(i < steps_FL) digitalWrite(MOTOR_FL_STEP, HIGH);
    if(i < steps_RL) digitalWrite(MOTOR_RL_STEP, HIGH);
    if(i < steps_FR) digitalWrite(MOTOR_FR_STEP, HIGH);
    if(i < steps_RR) digitalWrite(MOTOR_RR_STEP, HIGH);
    delayMicroseconds(stepDelay);
    
    digitalWrite(MOTOR_FL_STEP, LOW);
    digitalWrite(MOTOR_RL_STEP, LOW);
    digitalWrite(MOTOR_FR_STEP, LOW);
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
