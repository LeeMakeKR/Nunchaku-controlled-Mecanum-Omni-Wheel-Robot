// TMC2209 스텝 드라이버 테스트 코드
// 아두이노 나노 + TMC2209

#define DIR_PIN 3
#define STEP_PIN 4
#define EN_PIN 5

void setup() {
  // 핀 모드 설정
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  
  // 드라이버 활성화 (LOW = 활성화)
  digitalWrite(EN_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("TMC2209 테스트 시작");
}

void loop() {
  // 정방향 200스텝
  digitalWrite(DIR_PIN, HIGH);
  Serial.println("정방향 회전");
  for(int i = 0; i < 1600; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(500);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(500);
  }
  delay(500);
  
  // 역방향 200스텝
  digitalWrite(DIR_PIN, LOW);
  Serial.println("역방향 회전");
  for(int i = 0; i < 1600; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(500);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(500);
  }
  delay(500);
}
