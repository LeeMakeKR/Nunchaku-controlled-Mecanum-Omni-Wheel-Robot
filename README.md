# Nunchaku-controlled Mecanum/Omni Wheel Robot

Wii Nunchaku 컨트롤러를 사용하여 메카넘 휠 및 옴니휠 로봇 베이스를 제어하는 프로젝트입니다.



## 개요

<img src="pic/20251116_180231_1.jpg" width="500">



<details>
<summary>더 많은 이미지 보기</summary>

<img src="pic/20251129_235622.jpg" width="500">
<img src="pic/20251130_003352.jpg" width="500">

</details>


## Kinematics


이 프로젝트는 Wii Nunchaku의 조이스틱과 가속도 센서를 활용하여 메카넘 휠 또는 옴니휠이 장착된 로봇의 전방향 이동을 직관적으로 제어합니다.

<img src="pic/Gemini_Generated_Image_1.png" width="500">


XYZ 3방향 이동과 Z축 회전(yaw)을 조합하여 로봇의 움직임을 제어합니다. 


- **3각형 바디 베이스**: 3개의 스텝모터와 옴니휠
- **4각형 바디 베이스**: 4개의 스텝모터와 메카넘 휠
- Arduino를 이용해 제어
- OLED 디스플레이 포함
- 스텝모터 드라이버: TMC2209



## Wii Nunchaku 개요

닌텐도의 Wii Nunchaku는 2006년에 발매된 Nintendo Wii의 조이스틱과 가속도 센서를 포함한 컨트롤러입니다. I2C 방식으로 신호를 주고받으며 아두이노 라이브러리를 통해 제어할 수 있습니다. 


<img src="pic/20251116_180353.jpg" width="500">
<img src="pic/screenshot_114613.png" width="500">
<img src="pic/screenshot_114649.png" width="500">
<img src="pic/screenshot_114636.png" width="500">

