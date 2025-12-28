// TMC2209 스텝모터 4개 제어 코드
// ESP32 DEVKIT 사용
// 참고: D21=SDA, D22=SCL (I2C 핀)

// 핀 정의
// 모터 1
#define MOTOR1_DIR    16  // RX2
#define MOTOR1_STEP   17  // TX2

// 모터 2
#define MOTOR2_DIR    18  // D18
#define MOTOR2_STEP   19  // D19

// 모터 3
#define MOTOR3_DIR    13  // D13
#define MOTOR3_STEP   14  // D14

// 모터 4
#define MOTOR4_DIR    25  // D25
#define MOTOR4_STEP   26  // D26

// 공통 ENABLE 핀
#define ENABLE_PIN    27  // D27

// 배터리 전압 측정
#define BATTERY_PIN   34  // D34 (아날로그 입력)

// WS2812 LED
#define LED_PIN       23  // D23 (WS2812 출력)

// 스텝 모터 파라미터
const int stepsPerRevolution = 200;  // 모터 1회전당 스텝 수 (1.8도 스텝 각도)
const int stepDelay = 1000;          // 스텝 간 딜레이 (마이크로초)

void setup() {
  // 핀 모드 설정
  // 모터 1
  pinMode(MOTOR1_DIR, OUTPUT);
  pinMode(MOTOR1_STEP, OUTPUT);
  
  // 모터 2
  pinMode(MOTOR2_DIR, OUTPUT);
  pinMode(MOTOR2_STEP, OUTPUT);
  
  // 모터 3
  pinMode(MOTOR3_DIR, OUTPUT);
  pinMode(MOTOR3_STEP, OUTPUT);
  
  // 모터 4
  pinMode(MOTOR4_DIR, OUTPUT);
  pinMode(MOTOR4_STEP, OUTPUT);
  
  // Enable 핀 (LOW = 활성화, HIGH = 비활성화)
  pinMode(ENABLE_PIN, OUTPUT);
  
  // 배터리 전압 측정 핀
  pinMode(BATTERY_PIN, INPUT);
  
  // WS2812 LED 핀
  pinMode(LED_PIN, OUTPUT);
  
  // 모터 활성화
  digitalWrite(ENABLE_PIN, LOW);
  
  // 초기 방향 설정 (HIGH = 시계방향, LOW = 반시계방향)
  digitalWrite(MOTOR1_DIR, HIGH);
  digitalWrite(MOTOR2_DIR, HIGH);
  digitalWrite(MOTOR3_DIR, HIGH);
  digitalWrite(MOTOR4_DIR, HIGH);
  
  delay(100);
}

void loop() {
  // 방향 설정: 전진
  digitalWrite(MOTOR1_DIR, HIGH);
  digitalWrite(MOTOR2_DIR, HIGH);
  digitalWrite(MOTOR3_DIR, HIGH);
  digitalWrite(MOTOR4_DIR, HIGH);
  
  // 200스텝 회전
  for(int i = 0; i < 1600; i++) {
    digitalWrite(MOTOR1_STEP, HIGH);
    digitalWrite(MOTOR2_STEP, HIGH);
    digitalWrite(MOTOR3_STEP, HIGH);
    digitalWrite(MOTOR4_STEP, HIGH);
    delayMicroseconds(200);
    
    digitalWrite(MOTOR1_STEP, LOW);
    digitalWrite(MOTOR2_STEP, LOW);
    digitalWrite(MOTOR3_STEP, LOW);
    digitalWrite(MOTOR4_STEP, LOW);
    delayMicroseconds(200);
  }
  /*
  // 방향 설정: 후진
  digitalWrite(MOTOR1_DIR, LOW);
  digitalWrite(MOTOR2_DIR, LOW);
  digitalWrite(MOTOR3_DIR, LOW);
  digitalWrite(MOTOR4_DIR, LOW);
  
  // 200스텝 회전
  for(int i = 0; i < 1600; i++) {
    digitalWrite(MOTOR1_STEP, HIGH);
    digitalWrite(MOTOR2_STEP, HIGH);
    digitalWrite(MOTOR3_STEP, HIGH);
    digitalWrite(MOTOR4_STEP, HIGH);
    delayMicroseconds(100);
    
    digitalWrite(MOTOR1_STEP, LOW);
    digitalWrite(MOTOR2_STEP, LOW);
    digitalWrite(MOTOR3_STEP, LOW);
    digitalWrite(MOTOR4_STEP, LOW);
    delayMicroseconds(100);
  }
  */
}
