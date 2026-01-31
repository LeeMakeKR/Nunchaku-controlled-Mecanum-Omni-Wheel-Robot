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
enum SetupState {
  SETUP_OLED,
  SETUP_JOYSTICK_BASIC,
  MAIN_MENU,
  SETUP_LED,
  SETUP_JOYSTICK_FULL,
  SETUP_MOTOR,
  SETUP_COMPLETE
};

SetupState currentState = SETUP_OLED;

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

// ========== 함수 전방 선언 ==========
void displayInfo(const char* title, const char* line1, const char* line2 = "", const char* line3 = "");
void displayYesNo(const char* question, bool yesSelected);
void displayMenu();
bool buttonCPressed();
bool buttonZPressed();
void saveCalibration();
bool loadCalibration();
void clearCalibration();
void testOLED();
void testJoystickBasic();
void mainMenu();
void testLED();
void testJoystickFull();
void testJoystickXYCalib();
void testAccelCalib();
void testJoystickView();
void testMotor();
void runMotorSteps(MotorUnderTest motor, bool clockwise, int steps);
void setupComplete();
void runSetupMode();

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

// ========== 테스트 함수 ==========

void testOLED() {
  static bool yesSelected = true;
  static unsigned long lastUpdate = 0;
  
  if(millis() - lastUpdate > 100) {
    displayYesNo("OLED OK?", yesSelected);
    lastUpdate = millis();
  }
  
  if(nunchakuRead()) {
    if(nunchaku.joyX < JOY_CENTER - JOY_THRESHOLD) yesSelected = true;
    else if(nunchaku.joyX > JOY_CENTER + JOY_THRESHOLD) yesSelected = false;
    
    if(buttonCPressed()) {
      currentState = yesSelected ? SETUP_JOYSTICK_BASIC : SETUP_OLED;
      if(!yesSelected) { displayInfo("OLED FAIL", "Check I2C", "connection"); delay(2000); }
    }
  }
}

void testJoystickBasic() {
  static unsigned long startTime = 0;
  static bool joyMoved = false;
  
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
    displayInfo("Joystick Setup", "Move joystick", "and press C");
  }
  
  if(nunchakuRead()) {
    if(abs(nunchaku.joyX - JOY_CENTER) > JOY_THRESHOLD || abs(nunchaku.joyY - JOY_CENTER) > JOY_THRESHOLD) joyMoved = true;
    
    char buf1[32], buf2[32];
    sprintf(buf1, "X:%3d Y:%3d", nunchaku.joyX, nunchaku.joyY);
    sprintf(buf2, "C:%d Z:%d", nunchaku.buttonC, nunchaku.buttonZ);
    displayInfo("Joystick Setup", buf1, buf2, joyMoved ? "Moved! Press C" : "");
    
    if(buttonCPressed() && joyMoved) {
      currentState = MAIN_MENU;
      startTime = 0;
    }
  }
}

void mainMenu() {
  static unsigned long lastUpdate = 0;
  static int prevJoyY = JOY_CENTER;
  
  if(millis() - lastUpdate > 100) {
    displayMenu();
    lastUpdate = millis();
  }
  
  if(nunchakuRead()) {
    if(nunchaku.joyY < JOY_CENTER - JOY_THRESHOLD && prevJoyY >= JOY_CENTER - JOY_THRESHOLD) {
      if(currentMenu < MENU_COUNT - 1) currentMenu = (MenuItem)((int)currentMenu + 1);
    } else if(nunchaku.joyY > JOY_CENTER + JOY_THRESHOLD && prevJoyY <= JOY_CENTER + JOY_THRESHOLD) {
      if(currentMenu > 0) currentMenu = (MenuItem)((int)currentMenu - 1);
    }
    prevJoyY = nunchaku.joyY;
    
    if(buttonCPressed()) {
      switch(currentMenu) {
        case MENU_LED_TEST:
          currentState = SETUP_LED;
          ledColorIndex = 0;
          ledBrightnessLevel = 1;
          FastLED.setBrightness(LED_BRIGHTNESS_LEVELS[ledBrightnessLevel]);
          break;
        case MENU_JOYSTICK_TEST: currentState = SETUP_JOYSTICK_FULL; break;
        case MENU_MOTOR_TEST:
          currentState = SETUP_MOTOR;
          currentMotor = MOTOR_TEST_FL;
          digitalWrite(ENABLE_PIN, LOW);
          break;
        case MENU_EXIT: currentState = SETUP_COMPLETE; break;
        case MENU_COUNT: break;
      }
    }
  }
}

void testLED() {
  static unsigned long lastUpdate = 0;
  static int prevJoyX = JOY_CENTER, prevJoyY = JOY_CENTER;
  
  if(nunchakuRead()) {
    if(nunchaku.joyX < JOY_CENTER - JOY_THRESHOLD && prevJoyX >= JOY_CENTER - JOY_THRESHOLD) {
      ledColorIndex = (ledColorIndex + 2) % 3;
    } else if(nunchaku.joyX > JOY_CENTER + JOY_THRESHOLD && prevJoyX <= JOY_CENTER + JOY_THRESHOLD) {
      ledColorIndex = (ledColorIndex + 1) % 3;
    }
    
    if(nunchaku.joyY > JOY_CENTER + JOY_THRESHOLD && prevJoyY <= JOY_CENTER + JOY_THRESHOLD) {
      ledBrightnessLevel = min(4, ledBrightnessLevel + 1);
    } else if(nunchaku.joyY < JOY_CENTER - JOY_THRESHOLD && prevJoyY >= JOY_CENTER - JOY_THRESHOLD) {
      ledBrightnessLevel = max(0, ledBrightnessLevel - 1);
    }
    
    prevJoyX = nunchaku.joyX;
    prevJoyY = nunchaku.joyY;
    
    FastLED.setBrightness(LED_BRIGHTNESS_LEVELS[ledBrightnessLevel]);
    switch(ledColorIndex) {
      case 0: leds[0] = CRGB::Red; break;
      case 1: leds[0] = CRGB::Green; break;
      case 2: leds[0] = CRGB::Blue; break;
    }
    FastLED.show();
    
    if(millis() - lastUpdate > 100) {
      const char* colors[] = {"Red", "Green", "Blue"};
      char buf1[32], buf2[32];
      sprintf(buf1, "Color: %s", colors[ledColorIndex]);
      sprintf(buf2, "Level: %d/5", ledBrightnessLevel + 1);
      displayInfo("LED Setup", buf1, buf2, "Z:Back");
      lastUpdate = millis();
    }
    
    if(buttonZPressed()) {
      leds[0] = CRGB::Black;
      FastLED.show();
      currentState = MAIN_MENU;
    }
  }
}

void testJoystickFull() {
  static unsigned long lastUpdate = 0;
  static int prevJoyY = JOY_CENTER;
  
  if(millis() - lastUpdate > 100) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 10, "Joystick Setup Menu");
    
    const char* menuItems[] = {"XY Calibration", "Accel Calib", "View Values", "Clear EEPROM", "Exit"};
    int y = 25;
    for(int i = 0; i < 5; i++) {
      if(i == currentJoyMenu) u8g2.drawStr(0, y, ">");
      u8g2.drawStr(10, y, menuItems[i]);
      y += 10;
    }
    u8g2.drawStr(0, 64, "C:Select Z:Back");
    u8g2.sendBuffer();
    lastUpdate = millis();
  }
  
  if(nunchakuRead()) {
    if(nunchaku.joyY > JOY_CENTER + JOY_THRESHOLD && prevJoyY <= JOY_CENTER + JOY_THRESHOLD) {
      if(currentJoyMenu > 0) currentJoyMenu = (JoystickSubMenu)((int)currentJoyMenu - 1);
    } else if(nunchaku.joyY < JOY_CENTER - JOY_THRESHOLD && prevJoyY >= JOY_CENTER - JOY_THRESHOLD) {
      if(currentJoyMenu < JOY_SUB_COUNT) currentJoyMenu = (JoystickSubMenu)((int)currentJoyMenu + 1);
    }
    prevJoyY = nunchaku.joyY;
    
    if(buttonCPressed()) {
      switch(currentJoyMenu) {
        case JOY_SUB_XY_CALIB: testJoystickXYCalib(); calibrationLoaded = true; break;
        case JOY_SUB_ACCEL_CALIB: testAccelCalib(); break;
        case JOY_SUB_VIEW: testJoystickView(); break;
        case JOY_SUB_EXIT: currentState = MAIN_MENU; break;
        default:
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
                } else if(buttonZPressed()) break;
              }
              delay(20);
            }
          }
          break;
      }
    }
    
    if(buttonZPressed()) currentState = MAIN_MENU;
  }
}

void testJoystickXYCalib() {
  currentCalibStep = CALIB_INIT;
  
  while(currentCalibStep != CALIB_DONE) {
    if(nunchakuRead()) {
      char buf[32];
      
      switch(currentCalibStep) {
        case CALIB_INIT:
          displayInfo("XY Calibration", "Press C to", "start", "");
          if(buttonCPressed()) {
            currentCalibStep = CALIB_LEFT;
            calibration.minX = 255; calibration.maxX = 0;
            calibration.minY = 255; calibration.maxY = 0;
          }
          break;
        
        case CALIB_LEFT:
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.drawStr(0, 10, "Move LEFT <---");
          u8g2.drawStr(0, 25, "Hold position");
          sprintf(buf, "X: %d", nunchaku.joyX);
          u8g2.drawStr(0, 40, buf);
          u8g2.drawStr(0, 55, "C: Next");
          u8g2.sendBuffer();
          calibration.minX = min(calibration.minX, nunchaku.joyX);
          if(buttonCPressed()) { currentCalibStep = CALIB_RIGHT; delay(300); }
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
          if(buttonCPressed()) { currentCalibStep = CALIB_UP; delay(300); }
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
          if(buttonCPressed()) { currentCalibStep = CALIB_DOWN; delay(300); }
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
          if(buttonCPressed()) { currentCalibStep = CALIB_CENTER; delay(300); }
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
            saveCalibration();
            displayInfo("XY Calibration", "Complete!", "Saved to EEPROM", "");
            delay(1500);
            currentCalibStep = CALIB_DONE;
          }
          break;
        
        default: break;
      }
    }
    delay(20);
  }
}

void testAccelCalib() {
  displayInfo("Accel Calib", "Not implemented", "Press Z", "");
  while(true) {
    if(nunchakuRead() && buttonZPressed()) break;
    delay(20);
  }
}

void testJoystickView() {
  unsigned long startTime = millis();
  while(millis() - startTime < 10000) {
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

void testMotor() {
  static unsigned long lastUpdate = 0;
  static int prevJoyX = JOY_CENTER, prevJoyY = JOY_CENTER;
  static bool motorEnabled = false;
  
  if(!motorEnabled) {
    digitalWrite(ENABLE_PIN, LOW);
    motorEnabled = true;
  }
  
  const char* motorNames[] = {"FL", "RL", "RR", "FR"};
  
  if(nunchakuRead()) {
    if(nunchaku.joyX < JOY_CENTER - JOY_THRESHOLD && prevJoyX >= JOY_CENTER - JOY_THRESHOLD) {
      currentMotor = (MotorUnderTest)(((int)currentMotor + 3) % 4);
      motorStepCount = 0; motorRevolutionCount = 0;
    } else if(nunchaku.joyX > JOY_CENTER + JOY_THRESHOLD && prevJoyX <= JOY_CENTER + JOY_THRESHOLD) {
      currentMotor = (MotorUnderTest)(((int)currentMotor + 1) % 4);
      motorStepCount = 0; motorRevolutionCount = 0;
    }
    
    if(nunchaku.joyY > JOY_CENTER + JOY_THRESHOLD && prevJoyY <= JOY_CENTER + JOY_THRESHOLD) {
      displayInfo("Motor Setup", "CW Running...", motorNames[currentMotor], "");
      runMotorSteps(currentMotor, true, 1600);
      motorStepCount += 1600;
    }
    
    if(nunchaku.joyY < JOY_CENTER - JOY_THRESHOLD && prevJoyY >= JOY_CENTER - JOY_THRESHOLD) {
      displayInfo("Motor Setup", "CCW Running...", motorNames[currentMotor], "");
      runMotorSteps(currentMotor, false, 1600);
      motorStepCount += 1600;
    }
    
    prevJoyX = nunchaku.joyX;
    prevJoyY = nunchaku.joyY;
    
    if(buttonCPressed()) {
      motorRevolutionCount++;
      int actualMicrosteps = motorStepCount / (STEPS_PER_REV * motorRevolutionCount);
      
      char buf[64];
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
      
      motorStepCount = 0; motorRevolutionCount = 0;
    }
    
    if(millis() - lastUpdate > 100) {
      char buf1[32], buf2[32], buf3[32];
      sprintf(buf1, "Motor: %s", motorNames[currentMotor]);
      sprintf(buf2, "Steps: %d", motorStepCount);
      sprintf(buf3, "Up:CW Dn:CCW");
      
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(0, 10, "Motor Setup");
      u8g2.drawLine(0, 12, 128, 12);
      u8g2.drawStr(0, 25, buf1);
      u8g2.drawStr(0, 38, buf2);
      
      int motorX[] = {30, 30, 80, 80};
      int motorY[] = {50, 62, 62, 50};
      for(int i = 0; i < 4; i++) {
        if(i == currentMotor) u8g2.drawBox(motorX[i]-3, motorY[i]-3, 6, 6);
        else u8g2.drawFrame(motorX[i]-3, motorY[i]-3, 6, 6);
      }
      
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.drawStr(0, 64, buf3);
      u8g2.sendBuffer();
      
      lastUpdate = millis();
    }
    
    if(buttonZPressed()) {
      digitalWrite(ENABLE_PIN, HIGH);
      motorEnabled = false;
      currentState = MAIN_MENU;
    }
  }
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
  displayInfo("Setup Complete!", "All setup", "finished!", "Reset to retry");
  delay(5000);
  while(true) delay(1000);
}

// ========== 셋업 모드 실행 ==========

void runSetupMode() {
  // EEPROM 초기화
  EEPROM.begin(EEPROM_SIZE);
  
  // OLED 초기화
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 20, "SETUP MODE");
  u8g2.drawStr(0, 35, "Starting...");
  u8g2.sendBuffer();
  delay(1000);
  
  // EEPROM에서 캘리브레이션 로드
  calibrationLoaded = loadCalibration();
  if(calibrationLoaded) {
    displayInfo("Calibration", "Loaded from", "EEPROM", "");
  } else {
    displayInfo("Calibration", "Not found", "Will calibrate", "");
  }
  delay(1500);
  
  // 초기 상태
  currentState = SETUP_OLED;
  currentCalibStep = CALIB_INIT;
  currentAccelStep = ACCEL_INIT;
  
  // 셋업 모드 메인 루프
  while(true) {
    switch(currentState) {
      case SETUP_OLED: testOLED(); break;
      case SETUP_JOYSTICK_BASIC: testJoystickBasic(); break;
      case MAIN_MENU: mainMenu(); break;
      case SETUP_LED: testLED(); break;
      case SETUP_JOYSTICK_FULL: testJoystickFull(); break;
      case SETUP_MOTOR: testMotor(); break;
      case SETUP_COMPLETE: setupComplete(); break;
    }
    delay(20);
  }
}

#endif // SETUP_H
