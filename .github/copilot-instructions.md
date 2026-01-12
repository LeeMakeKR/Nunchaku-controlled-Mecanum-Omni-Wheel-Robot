# Copilot Instructions: Nunchaku-controlled Mecanum/Omni Wheel Robot

## Project Overview
이 프로젝트는 **Wii Nunchaku**로 제어되는 메카넘/옴니휠 로봇입니다. ESP32 DevKit과 TMC2209 스텝모터 드라이버로 4륜(메카넘) 또는 3륜(옴니) 구성을 제어합니다.

### 핵심 아키텍처
- **하드웨어**: ESP32 DevKit + TMC2209 드라이버(4개) + NEMA17 스텝모터 + WS2812 LED + OLED(I2C)
- **제어 입력**: Wii Nunchaku (I2C 0x52) - 조이스틱 + 가속도계 + 2버튼
- **수학 엔진**: 역기구학 계산으로 조이스틱 입력 → 차체 속도 → 개별 휠 각속도 변환
- **펌웨어**: Arduino 프레임워크 (C++) on ESP32

## 역기구학 시스템 (Critical)

### 메카넘 휠 (4륜, X 패턴)
[Mecanum_calc.md](../Mecanum_calc.md)의 공식 사용:
```cpp
// 차체 좌표계 속도 (mm/s, rad/s) → 휠 각속도 (rad/s)
w_LF = (v_x - v_y - L_SUM * omega) / WHEEL_RADIUS;
w_RF = (v_x + v_y + L_SUM * omega) / WHEEL_RADIUS;
w_LR = (v_x + v_y - L_SUM * omega) / WHEEL_RADIUS;
w_RR = (v_x - v_y + L_SUM * omega) / WHEEL_RADIUS;
// L_SUM = L_X + L_Y (차체 중심에서 휠까지 거리 합)
```

### 옴니휠 (3륜, 정삼각형 Kiwi Drive)
[OmniWheel_calc.md](../OmniWheel_calc.md) 참조:
- 120° 간격 배치, 접선 방향 구동
- 회전 속도 벡터와 병진 속도 벡터를 결합하여 각 휠의 선속도 계산

### 좌표계 변환
전역 좌표계 → 차체 좌표계 회전 변환 시 `θ`(yaw 각도) 사용:
```cpp
v_x = V_X * cos(θ) + V_Y * sin(θ);
v_y = -V_X * sin(θ) + V_Y * cos(θ);
```

## Arduino 코드 패턴

### 핀 배치 규칙 (ESP32 DevKit)
[Mecanum_Wheel.ino](../arduino/Mecanum_Wheel/Mecanum_Wheel.ino) 참조:
- **모터**: LF(25,26), RF(18,19), LR(13,14), RR(16,17) - DIR/STEP 쌍
- **공통 ENABLE**: GPIO27 (LOW=활성화)
- **I2C**: 기본 SDA/SCL (21/22) - Nunchaku + OLED 공유
- **LED**: GPIO23 (WS2812)
- **배터리 모니터링**: GPIO34 (ADC)

### Nunchaku 통신 프로토콜
I2C 0x52 주소, 비표준 초기화 시퀀스 필요:
```cpp
// 초기화: 0xF0 0x55, 0xFB 0x00 전송
// 읽기: 0x00 전송 후 6바이트 수신
// 데이터 구조: [joyX, joyY, accelX(8bit), accelY(8bit), accelZ(8bit), buttons(6bit)]
// 버튼: bit0=Z(!), bit1=C(!)
```

### 스텝모터 타이밍 전략
```cpp
// 각속도 → 스텝 변환
steps = abs(omega * dt / (2*PI) * TOTAL_STEPS_PER_REV);
// TOTAL_STEPS_PER_REV = 200 * 32 = 6400 (1.8° 모터 + 32 마이크로스텝)

// 가변 속도 제어
stepDelay = map(maxOmega, 0, MAX_VELOCITY/WHEEL_RADIUS, MAX_DELAY, MIN_DELAY);
// 200-1000 μs 범위, 각속도 클수록 짧아짐
```

### 모터 방향 극성
```cpp
// Left 모터: HIGH=정회전, Right 모터: LOW=정회전
digitalWrite(MOTOR_LF_DIR, dir_LF ? HIGH : LOW);
digitalWrite(MOTOR_RF_DIR, dir_RF ? LOW : HIGH);
```

## 하드웨어 제작 가이드

### PCB/회로 ([Circuit_Design.md](../Circuit_Design.md))
- 거버 파일: `gerber/` 폴더 - zip 그대로 PCB 업체 전송 가능
- 온라인 회로도: https://oshwlab.com/pashiran/mecanum-omni_wheel_platform
- DC-DC 컨버터: 12V → 5V (ESP32 공급)

### 3D 부품 ([Hardware_Assembly.md](../Hardware_Assembly.md))
- OnShape 설계 공개: https://cad.onshape.com/documents/ff57bdec1c6d6779bf98a37f
- **파라메트릭 휠 설계**: 지름, 두께, 보조바퀴 수 조정 가능
- **재질**: 메인 프레임(PLA), 바퀴 접촉부(TPU 권장)
- **휠 구매 옵션**: https://ko.aliexpress.com/item/1005007408718710.html

## 개발 워크플로우

### 업로드/테스트
```bash
# Arduino IDE에서:
# 1. 보드: "ESP32 Dev Module" 선택
# 2. Port: 자동 감지된 COM 포트
# 3. Upload Speed: 921600
# 4. 라이브러리: FastLED, U8g2, Wire (내장)

# 모터 단독 테스트: arduino/teststepdriver/teststepdriver.ino 사용
# 전체 제어: arduino/Mecanum_Wheel/Mecanum_Wheel.ino
```

### TMC2209 마이크로스테핑 설정
- MS1, MS2 핀 설정으로 32 마이크로스텝 구성
- 대체 드라이버(A4988/DRV8825) 사용 시 MS3 핀 수동 납땜 필요

### 디버깅 전략
- **LED 상태**: 버튼 없음(초록), C버튼(빨강), Z버튼(파랑), 둘다(마젠타)
- **OLED 출력**: 실시간 조이스틱 좌표, 속도, 배터리 전압
- **시리얼 모니터**: 115200 baud (초기화 메시지 확인)

## 프로젝트 특이사항

### 조이스틱 → 속도 변환 로직
```cpp
// 벡터 크기 기반 정규화 (대각선 이동 시 속도 일관성)
magnitude = sqrt(dx*dx + dy*dy);
normalizedMag = magnitude / sqrt(JOY_CENTER^2 * 2);
velocity = normalizedMag * MAX_VELOCITY;

// 방향 벡터 적용
v_x = velocity * (dy/magnitude);    // 조이스틱 Y → 전후
v_y = velocity * (-dx/magnitude);   // 조이스틱 X → 좌우(부호 반전)
```

### 데드존 처리
```cpp
const int JOY_CENTER = 128;  // 8비트 중심값
const int JOY_DEADZONE = 20; // ±20 범위는 0으로 처리
```

### 동시 스텝 실행
4개 모터 동시 제어 시 최대 스텝 수를 기준으로 루프, 각 모터는 필요 스텝만큼만 펄스 생성.

## 문서 참조 우선순위
1. **역기구학 공식**: [Mecanum_calc.md](../Mecanum_calc.md) / [OmniWheel_calc.md](../OmniWheel_calc.md)
2. **회로 연결**: [Circuit_Design.md](../Circuit_Design.md)
3. **기계 조립**: [Hardware_Assembly.md](../Hardware_Assembly.md)
4. **코드 예제**: [arduino/Mecanum_Wheel/Mecanum_Wheel.ino](../arduino/Mecanum_Wheel/Mecanum_Wheel.ino)
