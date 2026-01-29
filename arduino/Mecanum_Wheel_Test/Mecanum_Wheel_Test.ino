// TMC2209 메카넘 휠 종합 테스트 코드
// ESP32 DEVKIT 사용
// Wii Nunchaku로 제어
// 단계별 하드웨어 테스트 프로그램

#include <U8g2lib.h>
#include <Wire.h>
#include <FastLED.h>
#include <EEPROM.h>

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
NunchakuData prevNunchaku;  // 이전 상태 저장

// 조이스틱 캘리브레이션 데이터
struct CalibrationData {
  int centerX;
  int centerY;
  int minX;
  int maxX;
  int minY;
  int maxY;
} calibration = {128, 128, 0, 255, 0, 255};

// EEPROM 설정
#define EEPROM_SIZE 64
#define EEPROM_MAGIC 0xCAFE  // 캘리브레이션 데이터 유효성 확인용
#define EEPROM_ADDR_MAGIC 0
#define EEPROM_ADDR_CALIB 2

bool calibrationLoaded = false;  // EEPROM에서 캘리브레이션 로드 여부

// 조이스틱 파라미터
const int JOY_CENTER = 128;
const int JOY_DEADZONE = 20;
const int JOY_THRESHOLD = 50;  // 메뉴 선택 임계값

// 모터 파라미터
const int STEPS_PER_REV = 200;
const int MICROSTEPS = 32;
const int TOTAL_STEPS_PER_REV = STEPS_PER_REV * MICROSTEPS;

// 테스트 상태
enum TestState {
  TEST_OLED,
  TEST_JOYSTICK_BASIC,
  MAIN_MENU,
  TEST_LED,
  TEST_JOYSTICK_FULL,
  TEST_MOTOR,
  TEST_COMPLETE
};

TestState currentState = TEST_OLED;

// 메뉴 항목
enum MenuItem {
  MENU_LED_TEST,
  MENU_JOYSTICK_TEST,
  MENU_MOTOR_TEST,
  MENU_EXIT,
  MENU_COUNT
};

MenuItem currentMenu = MENU_LED_TEST;

// LED 테스트 변수
int ledColorIndex = 0;  // 0=Red, 1=Green, 2=Blue
int ledBrightnessLevel = 2;  // 0~4 (5단계)
const int LED_BRIGHTNESS_LEVELS[] = {25, 50, 100, 180, 255};

// 조이스틱 테스트 서브메뉴
enum JoystickSubMenu {
  JOY_SUB_XY_CALIB,
  JOY_SUB_ACCEL_CALIB,
  JOY_SUB_VIEW,
  JOY_SUB_EXIT,
  JOY_SUB_COUNT
};
JoystickSubMenu currentJoyMenu = JOY_SUB_XY_CALIB;

// 조이스틱 XY 캘리브레이션 단계
enum CalibStep {
  CALIB_INIT,
  CALIB_LEFT,
  CALIB_RIGHT,
  CALIB_UP,
  CALIB_DOWN,
  CALIB_CENTER,
  CALIB_DONE
};
CalibStep currentCalibStep = CALIB_INIT;

// 가속도계 캘리브레이션 단계
enum AccelCalibStep {
  ACCEL_INIT,
  ACCEL_X_MAX,
  ACCEL_X_MIN,
  ACCEL_Y_MAX,
  ACCEL_Y_MIN,
  ACCEL_Z_MAX,
  ACCEL_Z_MIN,
  ACCEL_CENTER,
  ACCEL_DONE
};
AccelCalibStep currentAccelStep = ACCEL_INIT;

// 모터 테스트 변수
enum MotorUnderTest {
  MOTOR_TEST_FL,
  MOTOR_TEST_RL,
  MOTOR_TEST_RR,
  MOTOR_TEST_FR
};

MotorUnderTest currentMotor = MOTOR_TEST_FL;
int motorStepCount = 0;  // 현재 스텝 카운트
int motorRevolutionCount = 0;  // 1회전 카운트

// ========== 초기화 함수 ==========

void setup() {
  Serial.begin(115200);
  Serial.println("Mecanum Wheel Test Starting...");
  
  // EEPROM 초기화
  EEPROM.begin(EEPROM_SIZE);
  
  // I2C 초기화
  Wire.begin();
  delay(100);
  
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
  
  // 모터 비활성화 (안전)
  digitalWrite(ENABLE_PIN, HIGH);
  
  // WS2812 LED 초기화
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS_LEVELS[ledBrightnessLevel]);
  leds[0] = CRGB::Black;
  FastLED.show();
  
  // OLED 초기화 시도
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 15, "OLED Test");
  u8g2.drawStr(0, 30, "Initializing...");
  u8g2.sendBuffer();
  delay(1000);
  
  // Nunchaku 초기화
  nunchakuInit();
  delay(100);
  
  // EEPROM에서 캘리브레이션 로드 시도
  calibrationLoaded = loadCalibration();
  
  if(calibrationLoaded) {
    Serial.println("Calibration loaded from EEPROM");
    displayInfo("Calibration", "Loaded from", "EEPROM", "");
    delay(1500);
  } else {
    Serial.println("No calibration found, will calibrate");
    displayInfo("Calibration", "Not found", "Will calibrate", "");
    delay(1500);
  }
  
  // 초기 상태로 설정
  currentState = TEST_OLED;
  currentCalibStep = CALIB_INIT;
  currentAccelStep = ACCEL_INIT;
}

// ========== Nunchaku 함수 ==========

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

// ========== EEPROM 함수 ==========

// EEPROM에 캘리브레이션 저장
void saveCalibration() {
  // Magic number 저장
  EEPROM.write(EEPROM_ADDR_MAGIC, (EEPROM_MAGIC >> 8) & 0xFF);
  EEPROM.write(EEPROM_ADDR_MAGIC + 1, EEPROM_MAGIC & 0xFF);
  
  // 캘리브레이션 데이터 저장
  int addr = EEPROM_ADDR_CALIB;
  EEPROM.put(addr, calibration.centerX); addr += sizeof(int);
  EEPROM.put(addr, calibration.centerY); addr += sizeof(int);
  EEPROM.put(addr, calibration.minX); addr += sizeof(int);
  EEPROM.put(addr, calibration.maxX); addr += sizeof(int);
  EEPROM.put(addr, calibration.minY); addr += sizeof(int);
  EEPROM.put(addr, calibration.maxY); addr += sizeof(int);
  
  // 체크섬 계산 및 저장
  uint16_t checksum = calibration.centerX + calibration.centerY + 
                      calibration.minX + calibration.maxX + 
                      calibration.minY + calibration.maxY;
  EEPROM.put(addr, checksum);
  
  EEPROM.commit();
  
  Serial.println("Calibration saved to EEPROM");
  Serial.printf("Center: X=%d Y=%d\n", calibration.centerX, calibration.centerY);
  Serial.printf("X Range: %d ~ %d\n", calibration.minX, calibration.maxX);
  Serial.printf("Y Range: %d ~ %d\n", calibration.minY, calibration.maxY);
}

// EEPROM에서 캘리브레이션 로드
bool loadCalibration() {
  // Magic number 확인
  uint16_t magic = (EEPROM.read(EEPROM_ADDR_MAGIC) << 8) | 
                   EEPROM.read(EEPROM_ADDR_MAGIC + 1);
  
  if(magic != EEPROM_MAGIC) {
    Serial.println("No valid calibration in EEPROM");
    return false;
  }
  
  // 캘리브레이션 데이터 로드
  int addr = EEPROM_ADDR_CALIB;
  EEPROM.get(addr, calibration.centerX); addr += sizeof(int);
  EEPROM.get(addr, calibration.centerY); addr += sizeof(int);
  EEPROM.get(addr, calibration.minX); addr += sizeof(int);
  EEPROM.get(addr, calibration.maxX); addr += sizeof(int);
  EEPROM.get(addr, calibration.minY); addr += sizeof(int);
  EEPROM.get(addr, calibration.maxY); addr += sizeof(int);
  
  // 체크섬 확인
  uint16_t storedChecksum, calculatedChecksum;
  EEPROM.get(addr, storedChecksum);
  
  calculatedChecksum = calibration.centerX + calibration.centerY + 
                       calibration.minX + calibration.maxX + 
                       calibration.minY + calibration.maxY;
  
  if(storedChecksum != calculatedChecksum) {
    Serial.println("Calibration checksum mismatch");
    return false;
  }
  
  Serial.println("Calibration loaded successfully");
  Serial.printf("Center: X=%d Y=%d\n", calibration.centerX, calibration.centerY);
  Serial.printf("X Range: %d ~ %d\n", calibration.minX, calibration.maxX);
  Serial.printf("Y Range: %d ~ %d\n", calibration.minY, calibration.maxY);
  
  return true;
}

// EEPROM 캘리브레이션 데이터 삭제
void clearCalibration() {
  EEPROM.write(EEPROM_ADDR_MAGIC, 0);
  EEPROM.write(EEPROM_ADDR_MAGIC + 1, 0);
  EEPROM.commit();
  Serial.println("Calibration cleared from EEPROM");
}

void nunchakuRequest() {
  Wire.beginTransmission(NUNCHAKU_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();
}

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
    prevNunchaku = nunchaku;  // 이전 상태 저장
    
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

// 버튼 누름 감지 (상승 엣지)
bool buttonCPressed() {
  return nunchaku.buttonC && !prevNunchaku.buttonC;
}

bool buttonZPressed() {
  return nunchaku.buttonZ && !prevNunchaku.buttonZ;
}

// ========== 디스플레이 함수 ==========

void displayYesNo(const char* question, bool yesSelected) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  
  // 질문 표시 (여러 줄 지원)
  int y = 12;
  u8g2.drawStr(0, y, question);
  
  // Yes/No 버튼
  y = 50;
  if(yesSelected) {
    u8g2.drawBox(10, y-10, 40, 14);
    u8g2.setDrawColor(0);
    u8g2.drawStr(20, y, "YES");
    u8g2.setDrawColor(1);
    u8g2.drawStr(80, y, "NO");
  } else {
    u8g2.drawStr(20, y, "YES");
    u8g2.drawBox(70, y-10, 40, 14);
    u8g2.setDrawColor(0);
    u8g2.drawStr(80, y, "NO");
    u8g2.setDrawColor(1);
  }
  
  u8g2.sendBuffer();
}

void displayMenu() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, "=== Test Menu ===");
  
  int y = 25;
  const char* menuItems[] = {
    "LED Test",
    "Joystick Test",
    "Motor Test",
    "Exit"
  };
  
  for(int i = 0; i < MENU_COUNT; i++) {
    if(i == currentMenu) {
      u8g2.drawStr(0, y, ">");
    }
    u8g2.drawStr(10, y, menuItems[i]);
    y += 12;
  }
  
  u8g2.drawStr(0, 64, "C:Select Z:Back");
  u8g2.sendBuffer();
}

void displayInfo(const char* title, const char* line1, const char* line2 = "", const char* line3 = "") {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, title);
  u8g2.drawLine(0, 12, 128, 12);
  
  if(line1[0] != '\0') u8g2.drawStr(0, 25, line1);
  if(line2[0] != '\0') u8g2.drawStr(0, 38, line2);
  if(line3[0] != '\0') u8g2.drawStr(0, 51, line3);
  
  u8g2.sendBuffer();
}

// ========== 테스트 함수 ==========

// 1. OLED 테스트
void testOLED() {
  static bool yesSelected = true;
  static unsigned long lastUpdate = 0;
  
  if(millis() - lastUpdate > 100) {
    displayYesNo("OLED OK?", yesSelected);
    lastUpdate = millis();
  }
  
  if(nunchakuRead()) {
    // 조이스틱 좌우로 선택
    if(nunchaku.joyX < JOY_CENTER - JOY_THRESHOLD) {
      yesSelected = true;
    } else if(nunchaku.joyX > JOY_CENTER + JOY_THRESHOLD) {
      yesSelected = false;
    }
    
    // C 버튼으로 확인
    if(buttonCPressed()) {
      if(yesSelected) {
        Serial.println("OLED Test: PASS");
        currentState = TEST_JOYSTICK_BASIC;
      } else {
        Serial.println("OLED Test: FAIL - Check connection");
        displayInfo("OLED FAIL", "Check I2C", "connection");
        delay(2000);
      }
    }
  }
}

// 2. 조이스틱 기본 테스트
void testJoystickBasic() {
  static bool testComplete = false;
  static unsigned long startTime = 0;
  static bool joyMoved = false;
  
  // 캘리브레이션이 없으면 강제로 캘리브레이션 진행
  if(!calibrationLoaded) {
    displayInfo("No Calibration", "Starting", "calibration...", "");
    delay(1500);
    testJoystickXYCalib();
    calibrationLoaded = true;
    currentState = MAIN_MENU;
    return;
  }
  
  if(startTime == 0) {
    startTime = millis();
    displayInfo("Joystick Test", "Move joystick", "and press C");
  }
  
  if(nunchakuRead()) {
    // 조이스틱 움직임 감지
    if(abs(nunchaku.joyX - JOY_CENTER) > JOY_THRESHOLD || 
       abs(nunchaku.joyY - JOY_CENTER) > JOY_THRESHOLD) {
      joyMoved = true;
    }
    
    // 화면 업데이트
    char buf1[32], buf2[32];
    sprintf(buf1, "X:%3d Y:%3d", nunchaku.joyX, nunchaku.joyY);
    sprintf(buf2, "C:%d Z:%d", nunchaku.buttonC, nunchaku.buttonZ);
    displayInfo("Joystick Test", buf1, buf2, joyMoved ? "Moved! Press C" : "");
    
    // C 버튼으로 확인
    if(buttonCPressed() && joyMoved) {
      Serial.println("Joystick Basic Test: PASS");
      currentState = MAIN_MENU;
      startTime = 0;
    }
  }
}

// 3. 메인 메뉴
void mainMenu() {
  static unsigned long lastUpdate = 0;
  
  if(millis() - lastUpdate > 100) {
    displayMenu();
    lastUpdate = millis();
  }
  
  if(nunchakuRead()) {
    // 조이스틱 상하로 메뉴 선택
    if(nunchaku.joyY < JOY_CENTER - JOY_THRESHOLD) {
      // 위로 (이전 메뉴)
      if(currentMenu > 0) {
        currentMenu = (MenuItem)((int)currentMenu - 1);
        delay(200);
      }
    } else if(nunchaku.joyY > JOY_CENTER + JOY_THRESHOLD) {
      // 아래로 (다음 메뉴)
      if(currentMenu < MENU_COUNT - 1) {
        currentMenu = (MenuItem)((int)currentMenu + 1);
        delay(200);
      }
    }
    
    // C 버튼으로 선택
    if(buttonCPressed()) {
      switch(currentMenu) {
        case MENU_LED_TEST:
          currentState = TEST_LED;
          ledColorIndex = 0;
          ledBrightness = 50;
          break;
        case MENU_JOYSTICK_TEST:
          currentState = TEST_JOYSTICK_FULL;
          break;
        case MENU_MOTOR_TEST:
          currentState = TEST_MOTOR;
          currentMotor = MOTOR_TEST_FL;
          digitalWrite(ENABLE_PIN, LOW);  // 모터 활성화
          break;
        case MENU_EXIT:
          currentState = TEST_COMPLETE;
          break;
      }
    }
  }
}

// 4. LED 테스트
void testLED() {
  static unsigned long lastUpdate = 0;
  static int prevJoyX = JOY_CENTER;
  static int prevJoyY = JOY_CENTER;
  
  if(nunchakuRead()) {
    // 좌우로 색상 변경
    if(nunchaku.joyX < JOY_CENTER - JOY_THRESHOLD && prevJoyX >= JOY_CENTER - JOY_THRESHOLD) {
      ledColorIndex = (ledColorIndex + 2) % 3;  // 왼쪽: 역방향
    } else if(nunchaku.joyX > JOY_CENTER + JOY_THRESHOLD && prevJoyX <= JOY_CENTER + JOY_THRESHOLD) {
      ledColorIndex = (ledColorIndex + 1) % 3;  // 오른쪽: 정방향
    }
    
    // 상하로 밝기 변경 (5단계)
    if(nunchaku.joyY < JOY_CENTER - JOY_THRESHOLD && prevJoyY >= JOY_CENTER - JOY_THRESHOLD) {
      ledBrightnessLevel = min(4, ledBrightnessLevel + 1);  // 위: 밝게
    } else if(nunchaku.joyY > JOY_CENTER + JOY_THRESHOLD && prevJoyY <= JOY_CENTER + JOY_THRESHOLD) {
      ledBrightnessLevel = max(0, ledBrightnessLevel - 1);   // 아래: 어둡게
    }
    
    prevJoyX = nunchaku.joyX;
    prevJoyY = nunchaku.joyY;
    
    // LED 업데이트
    FastLED.setBrightness(LED_BRIGHTNESS_LEVELS[ledBrightnessLevel]);
    switch(ledColorIndex) {
      case 0: leds[0] = CRGB::Red; break;
      case 1: leds[0] = CRGB::Green; break;
      case 2: leds[0] = CRGB::Blue; break;
    }
    FastLED.show();
    
    // 화면 업데이트
    if(millis() - lastUpdate > 100) {
      const char* colors[] = {"Red", "Green", "Blue"};
      char buf1[32], buf2[32];
      sprintf(buf1, "Color: %s", colors[ledColorIndex]);
      sprintf(buf2, "Level: %d/5", ledBrightnessLevel + 1);
      displayInfo("LED Test", buf1, buf2, "Z:Back");
      lastUpdate = millis();
    }
    
    // Z 버튼으로 메뉴 복귀
    if(buttonZPressed()) {
      leds[0] = CRGB::Black;
      FastLED.show();
      currentState = MAIN_MENU;
    }
  }
}

// 5. 조이스틱 전체 테스트 (서브메뉴)
void testJoystickFull() {
  static unsigned long lastUpdate = 0;
  
  // 서브메뉴 표시
  if(millis() - lastUpdate > 100) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 10, "Joy Test Menu");
    
    int y = 25;
    const char* menuItems[] = {
      "XY Calibration",
      "Accel Calib",
      "View Values",
      "Clear EEPROM",
      "Exit"
    };
    
    int itemCount = 5;
    for(int i = 0; i < itemCount; i++) {
      if(i == currentJoyMenu) {
        u8g2.drawStr(0, y, ">");
      }
      u8g2.drawStr(10, y, menuItems[i]);
      y += 10;
    }
    
    u8g2.drawStr(0, 64, "C:Select Z:Back");
    u8g2.sendBuffer();
    lastUpdate = millis();
  }
  
  if(nunchakuRead()) {
    // 조이스틱 상하로 메뉴 선택
    if(nunchaku.joyY < JOY_CENTER - JOY_THRESHOLD) {
      if(currentJoyMenu > 0) {
        currentJoyMenu = (JoystickSubMenu)((int)currentJoyMenu - 1);
        delay(200);
      }
    } else if(nunchaku.joyY > JOY_CENTER + JOY_THRESHOLD) {
      if(currentJoyMenu < JOY_SUB_COUNT) {  // EXIT 포함 5개 항목
        currentJoyMenu = (JoystickSubMenu)((int)currentJoyMenu + 1);
        delay(200);
      }
    }
    
    // C 버튼으로 선택
    if(buttonCPressed()) {
      switch(currentJoyMenu) {
        case JOY_SUB_XY_CALIB:
          testJoystickXYCalib();
          calibrationLoaded = true;
          break;
        case JOY_SUB_ACCEL_CALIB:
          testAccelCalib();
          break;
        case JOY_SUB_VIEW:
          testJoystickView();
          break;
        case JOY_SUB_EXIT:
          currentState = MAIN_MENU;
          break;
        default:
          // Clear EEPROM (4번째 메뉴)
          if((int)currentJoyMenu == 3) {
            displayInfo("Clear EEPROM", "Are you sure?", "C:Yes Z:No", "");
            delay(500);
            while(true) {
              if(nunchakuRead()) {
                if(buttonCPressed()) {
                  clearCalibration();
                  calibrationLoaded = false;
                  displayInfo("EEPROM", "Cleared!", "", "");
                  delay(1500);
                  break;
                } else if(buttonZPressed()) {
                  break;
                }
              }
              delay(20);
            }
          }
          break;
      }
    }
    
    // Z 버튼으로 메뉴 복귀
    if(buttonZPressed()) {
      currentState = MAIN_MENU;
    }
  }
}

// 5-1. 조이스틱 XY 캘리브레이션
void testJoystickXYCalib() {
  currentCalibStep = CALIB_INIT;
  
  while(currentCalibStep != CALIB_DONE) {
    if(nunchakuRead()) {
      switch(currentCalibStep) {
        case CALIB_INIT:
          displayInfo("XY Calibration", "Press C to", "start", "");
          if(buttonCPressed()) {
            currentCalibStep = CALIB_LEFT;
            calibration.minX = 255;
            calibration.maxX = 0;
            calibration.minY = 255;
            calibration.maxY = 0;
          }
          break;
          
        case CALIB_LEFT:
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.drawStr(0, 10, "Move LEFT <---");
          u8g2.drawStr(0, 25, "Hold position");
          char buf[32];
          sprintf(buf, "X: %d", nunchaku.joyX);
          u8g2.drawStr(0, 40, buf);
          u8g2.drawStr(0, 55, "C: Next");
          u8g2.sendBuffer();
          
          calibration.minX = min(calibration.minX, nunchaku.joyX);
          
          if(buttonCPressed()) {
            Serial.printf("Left Max: X=%d\n", calibration.minX);
            currentCalibStep = CALIB_RIGHT;
            delay(300);
          }
          break;
          
        case CALIB_RIGHT:
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.drawStr(0, 10, "Move RIGHT --->");
          u8g2.drawStr(0, 25, "Hold position");
          sprintf(buf, "X: %d", nunchaku.joyX);
          u8g2.drawStr(0, 40, buf);
          u8g2.drawStr(0, 55, "C: Next");
          u8g2.sendBuffer();
          
          calibration.maxX = max(calibration.maxX, nunchaku.joyX);
          
          if(buttonCPressed()) {
            Serial.printf("Right Max: X=%d\n", calibration.maxX);
            currentCalibStep = CALIB_UP;
            delay(300);
          }
          break;
          
        case CALIB_UP:
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.drawStr(0, 10, "Move UP");
          u8g2.drawStr(0, 20, "    ^");
          u8g2.drawStr(0, 30, "Hold position");
          sprintf(buf, "Y: %d", nunchaku.joyY);
          u8g2.drawStr(0, 45, buf);
          u8g2.drawStr(0, 58, "C: Next");
          u8g2.sendBuffer();
          
          calibration.maxY = max(calibration.maxY, nunchaku.joyY);
          
          if(buttonCPressed()) {
            Serial.printf("Up Max: Y=%d\n", calibration.maxY);
            currentCalibStep = CALIB_DOWN;
            delay(300);
          }
          break;
          
        case CALIB_DOWN:
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.drawStr(0, 10, "Move DOWN");
          u8g2.drawStr(0, 20, "    v");
          u8g2.drawStr(0, 30, "Hold position");
          sprintf(buf, "Y: %d", nunchaku.joyY);
          u8g2.drawStr(0, 45, buf);
          u8g2.drawStr(0, 58, "C: Next");
          u8g2.sendBuffer();
          
          calibration.minY = min(calibration.minY, nunchaku.joyY);
          
          if(buttonCPressed()) {
            Serial.printf("Down Max: Y=%d\n", calibration.minY);
            currentCalibStep = CALIB_CENTER;
            delay(300);
          }
          break;
          
        case CALIB_CENTER:
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.drawStr(0, 10, "Release stick");
          u8g2.drawStr(0, 25, "to CENTER");
          sprintf(buf, "X:%d Y:%d", nunchaku.joyX, nunchaku.joyY);
          u8g2.drawStr(0, 40, buf);
          u8g2.drawStr(0, 55, "C: Done");
          u8g2.sendBuffer();
          
          if(buttonCPressed()) {
            calibration.centerX = nunchaku.joyX;
            calibration.centerY = nunchaku.joyY;
            Serial.printf("Center: X=%d Y=%d\n", calibration.centerX, calibration.centerY);
            
            // EEPROM에 저장
            saveCalibration();
            
            displayInfo("XY Calibration", "Complete!", "Saved to EEPROM", "");
            delay(1500);
            currentCalibStep = CALIB_DONE;
          }
          break;
          
        default:
          break;
      }
    }
    delay(20);
  }
}

// 5-2. 가속도계 캘리브레이션
void testAccelCalib() {
  currentAccelStep = ACCEL_INIT;
  int accelMaxX = 0, accelMinX = 1023;
  int accelMaxY = 0, accelMinY = 1023;
  int accelMaxZ = 0, accelMinZ = 1023;
  int accelCenterX = 512, accelCenterY = 512, accelCenterZ = 512;
  
  while(currentAccelStep != ACCEL_DONE) {
    if(nunchakuRead()) {
      char buf[32];
      
      switch(currentAccelStep) {
        case ACCEL_INIT:
          displayInfo("Accel Calib", "Press C to", "start", "");
          if(buttonCPressed()) {
            currentAccelStep = ACCEL_X_MAX;
            delay(300);
          }
          break;
          
        case ACCEL_X_MAX:
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.drawStr(0, 10, "Tilt RIGHT");
          u8g2.drawStr(0, 25, "Maximum X+");
          sprintf(buf, "X: %d", nunchaku.accelX);
          u8g2.drawStr(0, 40, buf);
          u8g2.drawStr(0, 55, "C: Next");
          u8g2.sendBuffer();
          
          accelMaxX = max(accelMaxX, nunchaku.accelX);
          
          if(buttonCPressed()) {
            Serial.printf("Accel X Max: %d\n", accelMaxX);
            currentAccelStep = ACCEL_X_MIN;
            delay(300);
          }
          break;
          
        case ACCEL_X_MIN:
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.drawStr(0, 10, "Tilt LEFT");
          u8g2.drawStr(0, 25, "Maximum X-");
          sprintf(buf, "X: %d", nunchaku.accelX);
          u8g2.drawStr(0, 40, buf);
          u8g2.drawStr(0, 55, "C: Next");
          u8g2.sendBuffer();
          
          accelMinX = min(accelMinX, nunchaku.accelX);
          
          if(buttonCPressed()) {
            Serial.printf("Accel X Min: %d\n", accelMinX);
            currentAccelStep = ACCEL_Y_MAX;
            delay(300);
          }
          break;
          
        case ACCEL_Y_MAX:
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.drawStr(0, 10, "Tilt FORWARD");
          u8g2.drawStr(0, 25, "Maximum Y+");
          sprintf(buf, "Y: %d", nunchaku.accelY);
          u8g2.drawStr(0, 40, buf);
          u8g2.drawStr(0, 55, "C: Next");
          u8g2.sendBuffer();
          
          accelMaxY = max(accelMaxY, nunchaku.accelY);
          
          if(buttonCPressed()) {
            Serial.printf("Accel Y Max: %d\n", accelMaxY);
            currentAccelStep = ACCEL_Y_MIN;
            delay(300);
          }
          break;
          
        case ACCEL_Y_MIN:
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.drawStr(0, 10, "Tilt BACKWARD");
          u8g2.drawStr(0, 25, "Maximum Y-");
          sprintf(buf, "Y: %d", nunchaku.accelY);
          u8g2.drawStr(0, 40, buf);
          u8g2.drawStr(0, 55, "C: Next");
          u8g2.sendBuffer();
          
          accelMinY = min(accelMinY, nunchaku.accelY);
          
          if(buttonCPressed()) {
            Serial.printf("Accel Y Min: %d\n", accelMinY);
            currentAccelStep = ACCEL_Z_MAX;
            delay(300);
          }
          break;
          
        case ACCEL_Z_MAX:
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.drawStr(0, 10, "Point UP");
          u8g2.drawStr(0, 25, "Maximum Z+");
          sprintf(buf, "Z: %d", nunchaku.accelZ);
          u8g2.drawStr(0, 40, buf);
          u8g2.drawStr(0, 55, "C: Next");
          u8g2.sendBuffer();
          
          accelMaxZ = max(accelMaxZ, nunchaku.accelZ);
          
          if(buttonCPressed()) {
            Serial.printf("Accel Z Max: %d\n", accelMaxZ);
            currentAccelStep = ACCEL_Z_MIN;
            delay(300);
          }
          break;
          
        case ACCEL_Z_MIN:
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.drawStr(0, 10, "Point DOWN");
          u8g2.drawStr(0, 25, "Maximum Z-");
          sprintf(buf, "Z: %d", nunchaku.accelZ);
          u8g2.drawStr(0, 40, buf);
          u8g2.drawStr(0, 55, "C: Next");
          u8g2.sendBuffer();
          
          accelMinZ = min(accelMinZ, nunchaku.accelZ);
          
          if(buttonCPressed()) {
            Serial.printf("Accel Z Min: %d\n", accelMinZ);
            currentAccelStep = ACCEL_CENTER;
            delay(300);
          }
          break;
          
        case ACCEL_CENTER:
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.drawStr(0, 10, "Neutral pos");
          u8g2.drawStr(0, 25, "(your choice)");
          sprintf(buf, "X:%d Y:%d", nunchaku.accelX, nunchaku.accelY);
          u8g2.drawStr(0, 40, buf);
          sprintf(buf, "Z:%d", nunchaku.accelZ);
          u8g2.drawStr(0, 50, buf);
          u8g2.drawStr(0, 60, "C: Done");
          u8g2.sendBuffer();
          
          if(buttonCPressed()) {
            accelCenterX = nunchaku.accelX;
            accelCenterY = nunchaku.accelY;
            accelCenterZ = nunchaku.accelZ;
            Serial.printf("Accel Center: X=%d Y=%d Z=%d\n", accelCenterX, accelCenterY, accelCenterZ);
            displayInfo("Accel Calib", "Complete!", "", "");
            delay(1000);
            currentAccelStep = ACCEL_DONE;
          }
          break;
          
        default:
          break;
      }
    }
    delay(20);
  }
}

// 5-3. 조이스틱 값 보기
void testJoystickView() {
  unsigned long startTime = millis();
  
  while(millis() - startTime < 10000) {  // 10초 동안 표시
    if(nunchakuRead()) {
      char buf1[32], buf2[32], buf3[32];
      sprintf(buf1, "Joy X:%d Y:%d", nunchaku.joyX, nunchaku.joyY);
      sprintf(buf2, "Acc X:%d Y:%d", nunchaku.accelX/4, nunchaku.accelY/4);
      sprintf(buf3, "Z:%d C:%d Z:%d", nunchaku.accelZ/4, nunchaku.buttonC, nunchaku.buttonZ);
      displayInfo("Joystick View", buf1, buf2, buf3);
      
      if(buttonZPressed()) break;
    }
    delay(50);
  }
}

// 6. 모터 테스트
void testMotor() {
  static unsigned long lastUpdate = 0;
  static int prevJoyX = JOY_CENTER;
  static int prevJoyY = JOY_CENTER;
  
  const char* motorNames[] = {"FL", "RL", "RR", "FR"};
  
  if(nunchakuRead()) {
    // 좌우로 모터 선택
    if(nunchaku.joyX < JOY_CENTER - JOY_THRESHOLD && prevJoyX >= JOY_CENTER - JOY_THRESHOLD) {
      currentMotor = (MotorUnderTest)(((int)currentMotor + 3) % 4);  // 왼쪽: 이전 모터
      motorStepCount = 0;
      motorRevolutionCount = 0;
    } else if(nunchaku.joyX > JOY_CENTER + JOY_THRESHOLD && prevJoyX <= JOY_CENTER + JOY_THRESHOLD) {
      currentMotor = (MotorUnderTest)(((int)currentMotor + 1) % 4);  // 오른쪽: 다음 모터
      motorStepCount = 0;
      motorRevolutionCount = 0;
    }
    
    // 위로 움직일 때 200스텝 CW 회전
    if(nunchaku.joyY < JOY_CENTER - JOY_THRESHOLD && prevJoyY >= JOY_CENTER - JOY_THRESHOLD) {
      displayInfo("Motor Test", "Running...", motorNames[currentMotor], "");
      runMotorSteps(currentMotor, true, 200);
      motorStepCount += 200;
    }
    
    prevJoyX = nunchaku.joyX;
    prevJoyY = nunchaku.joyY;
    
    // C 버튼으로 1회전 완료 체크
    if(buttonCPressed()) {
      motorRevolutionCount++;
      
      // 마이크로스텝 계산
      int actualMicrosteps = motorStepCount / (STEPS_PER_REV * motorRevolutionCount);
      
      char buf[64];
      sprintf(buf, "%s: %d steps", motorNames[currentMotor], motorStepCount);
      Serial.println(buf);
      sprintf(buf, "Revolutions: %d", motorRevolutionCount);
      Serial.println(buf);
      sprintf(buf, "Calculated microstep: %d", actualMicrosteps);
      Serial.println(buf);
      
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(0, 10, "1 Rev Complete!");
      sprintf(buf, "Motor: %s", motorNames[currentMotor]);
      u8g2.drawStr(0, 25, buf);
      sprintf(buf, "Steps: %d", motorStepCount);
      u8g2.drawStr(0, 38, buf);
      sprintf(buf, "Rev: %d", motorRevolutionCount);
      u8g2.drawStr(0, 51, buf);
      
      if(actualMicrosteps == MICROSTEPS) {
        u8g2.drawStr(0, 64, "Microstep: OK");
      } else {
        sprintf(buf, "Microstep: %d?", actualMicrosteps);
        u8g2.drawStr(0, 64, buf);
      }
      
      u8g2.sendBuffer();
      delay(2000);
      
      // 리셋
      motorStepCount = 0;
      motorRevolutionCount = 0;
    }
    
    // 화면 업데이트
    if(millis() - lastUpdate > 100) {
      char buf1[32], buf2[32], buf3[32];
      sprintf(buf1, "Motor: %s", motorNames[currentMotor]);
      sprintf(buf2, "Steps: %d", motorStepCount);
      sprintf(buf3, "Up:+200 C:1Rev");
      
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(0, 10, "Motor Test");
      u8g2.drawLine(0, 12, 128, 12);
      u8g2.drawStr(0, 25, buf1);
      u8g2.drawStr(0, 38, buf2);
      
      // 모터 인디케이터
      int motorX[] = {30, 30, 80, 80};  // FL, RL, RR, FR 위치
      int motorY[] = {50, 62, 62, 50};
      for(int i = 0; i < 4; i++) {
        if(i == currentMotor) {
          u8g2.drawBox(motorX[i]-3, motorY[i]-3, 6, 6);
        } else {
          u8g2.drawFrame(motorX[i]-3, motorY[i]-3, 6, 6);
        }
      }
      
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.drawStr(0, 64, buf3);
      u8g2.sendBuffer();
      
      lastUpdate = millis();
    }
    
    // Z 버튼으로 메뉴 복귀
    if(buttonZPressed()) {
      digitalWrite(ENABLE_PIN, HIGH);  // 모터 비활성화
      currentState = MAIN_MENU;
    }
  }
}

void runMotorSteps(MotorUnderTest motor, bool clockwise, int steps) {
  int dirPin, stepPin;
  
  switch(motor) {
    case MOTOR_TEST_FL:
      dirPin = MOTOR_FL_DIR;
      stepPin = MOTOR_FL_STEP;
      break;
    case MOTOR_TEST_RL:
      dirPin = MOTOR_RL_DIR;
      stepPin = MOTOR_RL_STEP;
      break;
    case MOTOR_TEST_RR:
      dirPin = MOTOR_RR_DIR;
      stepPin = MOTOR_RR_STEP;
      break;
    case MOTOR_TEST_FR:
      dirPin = MOTOR_FR_DIR;
      stepPin = MOTOR_FR_STEP;
      break;
    default:
      return;
  }
  
  // 방향 설정
  digitalWrite(dirPin, clockwise ? HIGH : LOW);
  
  // 스텝 실행
  for(int i = 0; i < steps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(500);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(500);
  }
  
  Serial.printf("Motor %d: +%d steps (Total: %d)\n", motor, steps, motorStepCount + steps);
}

// 7. 테스트 완료
void testComplete() {
  displayInfo("Test Complete!", "All tests", "finished!", "Reset to retry");
  delay(5000);
  
  // 무한 대기
  while(true) {
    delay(1000);
  }
}

// ========== 메인 루프 ==========

void loop() {
  switch(currentState) {
    case TEST_OLED:
      testOLED();
      break;
      
    case TEST_JOYSTICK_BASIC:
      testJoystickBasic();
      break;
      
    case MAIN_MENU:
      mainMenu();
      break;
      
    case TEST_LED:
      testLED();
      break;
      
    case TEST_JOYSTICK_FULL:
      testJoystickFull();
      break;
      
    case TEST_MOTOR:
      testMotor();
      break;
      
    case TEST_COMPLETE:
      testComplete();
      break;
  }
  
  delay(20);  // 50Hz 업데이트
}
