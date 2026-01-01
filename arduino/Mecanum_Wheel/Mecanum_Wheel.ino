// TMC2209 스텝모터 4개 제어 코드
// ESP32 DEVKIT 사용
// 참고: D21=SDA, D22=SCL (I2C 핀)

#include <U8g2lib.h>
#include <Wire.h>
#include <FastLED.h>

// U8G2 OLED 디스플레이 설정
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

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

// 배터리 전압 측정 (VBAT)
#define BATTERY_PIN   34  // D34 (아날로그 입력)

// WS2812 LED 설정
#define LED_PIN       23  // D23 (WS2812 출력)
#define NUM_LEDS      1   // LED 개수
#define LED_BRIGHTNESS 50 // 밝기 (0-255)

CRGB leds[NUM_LEDS];
uint8_t gHue = 0;  // 레인보우 효과용 색상 변수

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

// 스텝 모터 파라미터
const int stepsPerRevolution = 200;  // 모터 1회전당 스텝 수 (1.8도 스텝 각도)
const int stepDelay = 1000;          // 스텝 간 딜레이 (마이크로초)

// 모터 상태 변수
int currentSteps = 0;
String motorDirection = "STOP";
int motorSpeed = 0;

void setup() {
  // 시리얼 통신 초기화
  Serial.begin(115200);
  Serial.println("Mecanum Motor Control with Nunchaku");
  
  // I2C 초기화 (OLED 디스플레이 및 Nunchaku용)
  Wire.begin();
  
  // OLED 디스플레이 초기화
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 15, "Motor Control");
  u8g2.drawStr(0, 30, "Initializing...");
  u8g2.sendBuffer();
  
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
  
  // 배터리 전압 측정 핀 (VBAT)
  pinMode(BATTERY_PIN, INPUT);
  
  // WS2812 LED 초기화
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  
  // Nunchaku 초기화
  nunchakuInit();
  Serial.println("Nunchaku initialized");
  
  // 모터 활성화
  digitalWrite(ENABLE_PIN, LOW);
  
  // 초기 방향 설정 (HIGH = 시계방향, LOW = 반시계방향)
  digitalWrite(MOTOR1_DIR, HIGH);
  digitalWrite(MOTOR2_DIR, HIGH);
  digitalWrite(MOTOR3_DIR, HIGH);
  digitalWrite(MOTOR4_DIR, HIGH);
  
  delay(100);
  
  // 디스플레이 준비 완료 메시지
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 15, "Ready!");
  u8g2.sendBuffer();
  delay(1000);
}

// OLED에 가속도 그래프 표시
void displayAccelGraph() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x7_tr);
  
  // 제목
  u8g2.drawStr(0, 7, "Accel Graph");
  
  // 가속도 범위: 약 200~800 (중심 500)
  // OLED 그래프 범위: 0~60 픽셀
  int baseY = 63;  // 그래프 바닥
  int graphHeight = 50;  // 그래프 높이
  
  // X축 기준선
  int refLine = baseY - 25;
  u8g2.drawLine(20, refLine, 127, refLine);
  
  // X 가속도 (빨간색 - 왼쪽)
  int xBar = map(nunchaku.accelX, 200, 800, 0, graphHeight);
  xBar = constrain(xBar, 0, graphHeight);
  u8g2.drawBox(25, baseY - xBar, 8, xBar);
  u8g2.drawStr(25, 63, "X");
  
  // Y 가속도 (중간)
  int yBar = map(nunchaku.accelY, 200, 800, 0, graphHeight);
  yBar = constrain(yBar, 0, graphHeight);
  u8g2.drawBox(55, baseY - yBar, 8, yBar);
  u8g2.drawStr(55, 63, "Y");
  
  // Z 가속도 (오른쪽)
  int zBar = map(nunchaku.accelZ, 200, 800, 0, graphHeight);
  zBar = constrain(zBar, 0, graphHeight);
  u8g2.drawBox(85, baseY - zBar, 8, zBar);
  u8g2.drawStr(85, 63, "Z");
  
  // 수치 표시
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.setCursor(95, 20);
  u8g2.print("X:");
  u8g2.print(nunchaku.accelX);
  u8g2.setCursor(95, 30);
  u8g2.print("Y:");
  u8g2.print(nunchaku.accelY);
  u8g2.setCursor(95, 40);
  u8g2.print("Z:");
  u8g2.print(nunchaku.accelZ);
  
  // 버튼 상태
  u8g2.setCursor(95, 55);
  if(nunchaku.buttonC) u8g2.print("C");
  if(nunchaku.buttonZ) u8g2.print("Z");
  
  u8g2.sendBuffer();
}

void updateDisplay() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  
  // 제목
  u8g2.drawStr(0, 10, "Mecanum Motor");
  
  // 방향 표시
  u8g2.drawStr(0, 25, "Dir:");
  u8g2.setCursor(35, 25);
  u8g2.print(motorDirection);
  
  // 스텝 수 표시
  u8g2.drawStr(0, 40, "Steps:");
  u8g2.setCursor(45, 40);
  u8g2.print(currentSteps);
  
  // 속도 표시
  u8g2.drawStr(0, 55, "Speed:");
  u8g2.setCursor(45, 55);
  u8g2.print(motorSpeed);
  u8g2.drawStr(80, 55, "us");
  
  u8g2.sendBuffer();
}

void updateRainbow() {
  // 레인보우 효과
  fill_rainbow(leds, NUM_LEDS, gHue, 14);
  FastLED.show();
  gHue++;
}

// 버튼 상태에 따른 LED 색상 출력
void updateButtonLED() {
  if(nunchaku.buttonC && nunchaku.buttonZ) {
    // 둘 다 누르면: 빨간색 + 파란색 = 마젤타 (R+B)
    leds[0] = CRGB(255, 0, 255);  // 마젠타
  }
  else if(nunchaku.buttonC) {
    // C 버튼만 누르면: 빨간색
    leds[0] = CRGB(255, 0, 0);  // Red
  }
  else if(nunchaku.buttonZ) {
    // Z 버튼만 누르면: 파란색
    leds[0] = CRGB(0, 0, 255);  // Blue
  }
  else {
    // 아무것도 안 누르면: 꺼짐
    leds[0] = CRGB(0, 0, 0);  // OFF
  }
  FastLED.show();
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

// Nunchaku 데이터 시리얼 출력 (시리얼 플로터용)
void printNunchakuData() {
  // 시리얼 플로터 형식: Label:value 형식으로 공백 구분
  Serial.print("JoyX:");
  Serial.print(nunchaku.joyX);
  Serial.print(" JoyY:");
  Serial.print(nunchaku.joyY);
  Serial.print(" AccelX:");
  Serial.print(nunchaku.accelX);
  Serial.print(" AccelY:");
  Serial.print(nunchaku.accelY);
  Serial.print(" AccelZ:");
  Serial.print(nunchaku.accelZ);
  Serial.print(" ButtonC:");
  Serial.print(nunchaku.buttonC ? 1 : 0);
  Serial.print(" ButtonZ:");
  Serial.println(nunchaku.buttonZ ? 1 : 0);
}

void loop() {
  // Nunchaku 데이터 읽기 및 OLED 그래프 표시
  if(nunchakuRead()) {
    displayAccelGraph();
    // 버튼 상태에 따른 LED 업데이트
    updateButtonLED();
  }
  
  delay(50);  // 50ms 대기
  
  /*
  // 방향 설정: 전진
  motorDirection = "FORWARD";
  motorSpeed = 200;
  currentSteps = 0;
  updateDisplay();
  
  digitalWrite(MOTOR1_DIR, HIGH);B
  digitalWrite(MOTOR2_DIR, HIGH);
  digitalWrite(MOTOR3_DIR, HIGH);
  digitalWrite(MOTOR4_DIR, HIGH);
  
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
  
  // 완료 후 잠시 대기
  motorDirection = "STOP";
  updateDisplay();
  delay(2000);
 
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
