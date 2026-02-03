// setup.h - 셋업 모드 통합 헤더
// Mecanum_Wheel_Test.ino의 모든 기능을 포함

#ifndef SETUP_H
#define SETUP_H

#include <EEPROM.h>

// ========== 전역 변수 (셋업 모드용) ==========

// EEPROM 설정
#define EEPROM_SIZE 64
#define EEPROM_MAGIC 0xCAFE
#define EEPROM_ADDR_MAGIC 0
#define EEPROM_ADDR_CALIB 2

// 조이스틱 캘리브레이션
struct CalibrationData {
  int centerX, centerY;
  int minX, maxX;
  int minY, maxY;
};

CalibrationData calibration = {128, 128, 0, 255, 0, 255};
bool calibrationLoaded = false;

// 셋업 상태
//enum SetupState {
//  SETUP_OLED,
//  SETUP_JOYSTICK_BASIC,
//  MAIN_MENU,
//  SETUP_LED,
//  SETUP_JOYSTICK_FULL,
//  SETUP_MOTOR,
//  SETUP_COMPLETE
//};
//
//SetupState currentState = SETUP_OLED;

// 메뉴
enum MenuItem { MENU_LED_TEST, MENU_JOYSTICK_TEST, MENU_MOTOR_TEST, MENU_EXIT, MENU_COUNT };
MenuItem currentMenu = MENU_LED_TEST;

// LED 테스트
int ledColorIndex = 0;
int ledBrightnessLevel = 2;
const int LED_BRIGHTNESS_LEVELS[] = {25, 50, 100, 180, 255};

// 조이스틱 서브메뉴
enum JoystickSubMenu { JOY_SUB_XY_CALIB, JOY_SUB_ACCEL_CALIB, JOY_SUB_VIEW, JOY_SUB_EXIT, JOY_SUB_COUNT };
JoystickSubMenu currentJoyMenu = JOY_SUB_XY_CALIB;

// 조이스틱 캘리브레이션 단계
enum CalibStep { CALIB_INIT, CALIB_LEFT, CALIB_RIGHT, CALIB_UP, CALIB_DOWN, CALIB_CENTER, CALIB_DONE };
CalibStep currentCalibStep = CALIB_INIT;

// 가속도계 캘리브레이션 단계
enum AccelCalibStep { ACCEL_INIT, ACCEL_X_MAX, ACCEL_X_MIN, ACCEL_Y_MAX, ACCEL_Y_MIN, ACCEL_Z_MAX, ACCEL_Z_MIN, ACCEL_CENTER, ACCEL_DONE };
AccelCalibStep currentAccelStep = ACCEL_INIT;

// 모터 테스트
enum MotorUnderTest { MOTOR_TEST_FL, MOTOR_TEST_RL, MOTOR_TEST_RR, MOTOR_TEST_FR };
MotorUnderTest currentMotor = MOTOR_TEST_FL;
int motorStepCount = 0;
int motorRevolutionCount = 0;

// 이전 Nunchaku 상태
NunchakuData prevNunchaku;

// 함수 전방 선언
//void displayInfo(const char* title, const char* line1, const char* line2 = "", const char* line3 = "");
//void displayYesNo(const char* question, bool yesSelected);
//void displayMenu();
bool buttonCPressed();
bool buttonZPressed();
void saveCalibration();
bool loadCalibration();
void clearCalibration();
//void testOLED();
//void testJoystickBasic();
//void mainMenu();
//void testLED();
//void testJoystickFull();
//void testJoystickXYCalib();
//void testAccelCalib();
//void testJoystickView();
//void testMotor();
void runMotorSteps(MotorUnderTest motor, bool clockwise, int steps);
//void setupComplete();
//void runSetupMode();

// ========== EEPROM 함수 ==========

void saveCalibration() {
  EEPROM.write(EEPROM_ADDR_MAGIC, (EEPROM_MAGIC >> 8) & 0xFF);
  EEPROM.write(EEPROM_ADDR_MAGIC + 1, EEPROM_MAGIC & 0xFF);
  
  int addr = EEPROM_ADDR_CALIB;
  EEPROM.put(addr, calibration.centerX); addr += sizeof(int);
  EEPROM.put(addr, calibration.centerY); addr += sizeof(int);
  EEPROM.put(addr, calibration.minX); addr += sizeof(int);
  EEPROM.put(addr, calibration.maxX); addr += sizeof(int);
  EEPROM.put(addr, calibration.minY); addr += sizeof(int);
  EEPROM.put(addr, calibration.maxY); addr += sizeof(int);
  
  uint16_t checksum = calibration.centerX + calibration.centerY + calibration.minX + calibration.maxX + calibration.minY + calibration.maxY;
  EEPROM.put(addr, checksum);
  EEPROM.commit();
}

bool loadCalibration() {
  uint16_t magic = (EEPROM.read(EEPROM_ADDR_MAGIC) << 8) | EEPROM.read(EEPROM_ADDR_MAGIC + 1);
  if(magic != EEPROM_MAGIC) return false;
  
  int addr = EEPROM_ADDR_CALIB;
  EEPROM.get(addr, calibration.centerX); addr += sizeof(int);
  EEPROM.get(addr, calibration.centerY); addr += sizeof(int);
  EEPROM.get(addr, calibration.minX); addr += sizeof(int);
  EEPROM.get(addr, calibration.maxX); addr += sizeof(int);
  EEPROM.get(addr, calibration.minY); addr += sizeof(int);
  EEPROM.get(addr, calibration.maxY); addr += sizeof(int);
  
  uint16_t storedChecksum, calculatedChecksum;
  EEPROM.get(addr, storedChecksum);
  calculatedChecksum = calibration.centerX + calibration.centerY + calibration.minX + calibration.maxX + calibration.minY + calibration.maxY;
  
  return (storedChecksum == calculatedChecksum);
}

void clearCalibration() {
  EEPROM.write(EEPROM_ADDR_MAGIC, 0);
  EEPROM.write(EEPROM_ADDR_MAGIC + 1, 0);
  EEPROM.commit();
}

// ========== 버튼 함수 ==========

bool buttonCPressed() {
  return nunchaku.buttonC && !prevNunchaku.buttonC;
}

bool buttonZPressed() {
  return nunchaku.buttonZ && !prevNunchaku.buttonZ;
}

// ========== 디스플레이 함수 ==========
/*
void displayYesNo(const char* question, bool yesSelected) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 12, question);
  
  int y = 50;
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
  u8g2.drawStr(0, 10, "=== Setup Menu ===");
  
  const char* menuItems[] = {"LED Setup", "Joystick Setup", "Motor Setup", "Exit"};
  const int MAX_VISIBLE_ITEMS = 3;
  int scrollOffset = (currentMenu >= MAX_VISIBLE_ITEMS) ? (currentMenu - MAX_VISIBLE_ITEMS + 1) : 0;
  
  int y = 25;
  for(int i = scrollOffset; i < min(scrollOffset + MAX_VISIBLE_ITEMS, (int)MENU_COUNT); i++) {
    if(i == currentMenu) u8g2.drawStr(0, y, ">");
    u8g2.drawStr(10, y, menuItems[i]);
    y += 12;
  }
  
  u8g2.drawStr(0, 64, "C:Select Z:Back");
  u8g2.sendBuffer();
}

void displayInfo(const char* title, const char* line1, const char* line2, const char* line3) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, title);
  u8g2.drawLine(0, 12, 128, 12);
  if(line1[0] != '\0') u8g2.drawStr(0, 25, line1);
  if(line2[0] != '\0') u8g2.drawStr(0, 38, line2);
  if(line3[0] != '\0') u8g2.drawStr(0, 51, line3);
  u8g2.sendBuffer();
}
*/

// ========== 테스트 함수 ==========
// 다음 함수들은 OLED 관련으로 비활성화됨

void testJoystickXYCalib() {
  // OLED 관련 코드 비활성화됨
}

void testAccelCalib() {
  // OLED 관련 코드 비활성화됨
}

void testJoystickView() {
  // OLED 관련 코드 비활성화됨
}

void testMotor() {
  // OLED 관련 코드 비활성화됨
}

void runMotorSteps(MotorUnderTest motor, bool clockwise, int steps) {
  int dirPin, stepPin;
  
  switch(motor) {
    case MOTOR_TEST_FL: dirPin = MOTOR_FL_DIR; stepPin = MOTOR_FL_STEP; break;
    case MOTOR_TEST_RL: dirPin = MOTOR_RL_DIR; stepPin = MOTOR_RL_STEP; break;
    case MOTOR_TEST_RR: dirPin = MOTOR_RR_DIR; stepPin = MOTOR_RR_STEP; break;
    case MOTOR_TEST_FR: dirPin = MOTOR_FR_DIR; stepPin = MOTOR_FR_STEP; break;
    default: return;
  }
  
  bool dirSignal;
  if(motor == MOTOR_TEST_FL || motor == MOTOR_TEST_RL) dirSignal = clockwise;
  else dirSignal = !clockwise;
  
  digitalWrite(dirPin, dirSignal ? HIGH : LOW);
  delayMicroseconds(10);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(10);
  
  for(int i = 0; i < steps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(500);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(500);
  }
}

void setupComplete() {
  // OLED 관련 코드 비활성화됨
}

// ========== 셋업 모드 실행 ==========

void runSetupMode() {
  // OLED 관련 코드 비활성화됨
}

#endif // SETUP_H
