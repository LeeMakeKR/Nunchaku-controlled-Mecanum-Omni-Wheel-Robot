# 3-옴니휠(정삼각형 배치, Kiwi Drive) 역기구학 계산식 정리

정삼각형의 꼭지점에 3개의 옴니휠이 배치된 차체가 **평면 이동(translation)** 과 **평면 회전(yaw)** 을 동시에 할 때,  
차체의 **이동 속도 벡터**와 **회전 각속도**를 결합하여 각 바퀴의 **선속도 / 각속도 / RPM**을 계산한다.

---

## 1. 변수 정의

- 바퀴 지름: `D` [m]
- 바퀴 반지름: `r_w = D/2` [m]
- 차체 중심에서 각 휠 중심까지 거리: `R` [m]
- 차체 좌표계(Body frame)
  - `+x`: 전방(Forward)
  - `+y`: 좌측(Left)
  - `+z`: 위쪽(Up)
- 차체 이동 속도 벡터:
  $$
  \mathbf{v}=
  \begin{bmatrix}
  v_x\\
  v_y
  \end{bmatrix}
  \quad [m/s]
  $$
- 차체 회전(요, yaw) 각속도:
  $$
  \omega_z \quad [rad/s]
  $$
  - `ωz > 0`는 반시계(CCW) 회전으로 정의.

---

## 2. 이동 벡터 + 회전 벡터 결합(일반형)

휠 `i`의 차체 중심 기준 위치벡터:
$$
\mathbf{r}_i=
\begin{bmatrix}
x_i\\y_i
\end{bmatrix}
$$

차체 회전으로 인해 휠 위치에서 생기는 속도(평면):
$$
\mathbf{v}_{rot,i}=
\begin{bmatrix}
-\omega_z y_i\\
\;\omega_z x_i
\end{bmatrix}
$$

휠 위치에서의 합성 속도:
$$
\mathbf{v}_i = \mathbf{v} + \mathbf{v}_{rot,i}
$$

옴니휠은 **구동(굴림) 방향 성분만** 바퀴 회전으로 만들므로,  
휠 `i`의 구동 단위벡터를 `t_i`라 하면(2D):
$$
\mathbf{t}_i=
\begin{bmatrix}
t_{ix}\\t_{iy}
\end{bmatrix}
\quad (||\mathbf{t}_i||=1)
$$

휠 `i`의 구동 선속도(접선 방향 선속도):
$$
s_i = \mathbf{t}_i^\top \mathbf{v}_i
= \mathbf{t}_i^\top(\mathbf{v} + \mathbf{v}_{rot,i})
\quad [m/s]
$$

> 위 식이 “차체 이동 벡터”와 “차체 회전 벡터(각속도)”를 결합하는 핵심 계산식이다.

---

## 3. 정삼각형(120°) + 접선 구동(Kiwi) 닫힌형

### 3.1 휠 배치 각도

정삼각형 꼭지점 위치(120° 간격)로 휠을 배치한다고 가정한다.

- 휠 각도:
  $$
  \theta_1=0^\circ,\quad \theta_2=120^\circ,\quad \theta_3=240^\circ
  $$

- 휠 위치벡터:
  $$
  \mathbf{r}_i = R
  \begin{bmatrix}
  \cos\theta_i\\
  \sin\theta_i
  \end{bmatrix}
  $$

### 3.2 Kiwi(접선) 구동 방향

각 휠의 구동 방향이 원의 **접선 방향**(tangent)이라고 가정한다.

- 접선 단위벡터:
  $$
  \mathbf{t}_i=
  \begin{bmatrix}
  -\sin\theta_i\\
  \cos\theta_i
  \end{bmatrix}
  $$

이 경우 회전 기여분이 간단해진다:
$$
\mathbf{t}_i^\top \mathbf{v}_{rot,i} = R\omega_z
$$
즉, **회전 성분은 3개 바퀴에 동일하게 `R*ωz`로 더해진다.**

---

## 4. 최종 선속도 식 (s1, s2, s3)

$$
s_i = \mathbf{t}_i^\top\mathbf{v} + R\omega_z
$$

이를 전개하면:

- 휠 1 (θ=0°):
  $$
  s_1 = v_y + R\omega_z
  $$

- 휠 2 (θ=120°):
  $$
  s_2 = -\frac{\sqrt{3}}{2}v_x -\frac{1}{2}v_y + R\omega_z
  $$

- 휠 3 (θ=240°):
  $$
  s_3 = +\frac{\sqrt{3}}{2}v_x -\frac{1}{2}v_y + R\omega_z
  $$

행렬 형태:

$$
\begin{bmatrix}
s_1\\s_2\\s_3
\end{bmatrix}
=
\begin{bmatrix}
0 & 1\\
-\frac{\sqrt{3}}{2} & -\frac{1}{2}\\
\frac{\sqrt{3}}{2} & -\frac{1}{2}
\end{bmatrix}
\begin{bmatrix}
v_x\\v_y
\end{bmatrix}
+
\begin{bmatrix}
R\\R\\R
\end{bmatrix}
\omega_z
$$

---

## 5. 바퀴 각속도(rad/s) 및 RPM 변환

### 5.1 각속도(rad/s)

바퀴 선속도 `s_i` → 바퀴 각속도 `ω_i`:

$$
\omega_i = \frac{s_i}{r_w} = \frac{2s_i}{D}
\quad [rad/s]
$$

### 5.2 RPM

$$
RPM_i = \omega_i\cdot \frac{60}{2\pi}
= s_i\cdot \frac{60}{\pi D}
\quad [rev/min]
$$

---

## 6. 간단 단위 검증(권장)

- 순수 회전만 있는 경우:
  - `v_x = v_y = 0`, `ωz ≠ 0`
  - 결과:
    $$
    s_1=s_2=s_3=R\omega_z
    $$
  - 세 바퀴가 같은 방향/크기로 회전해야 정상.

- 순수 +y 이동만 있는 경우:
  - `v_x = 0`, `v_y > 0`, `ωz=0`
  - 결과:
    $$
    s_1=v_y,\quad s_2=-0.5v_y,\quad s_3=-0.5v_y
    $$
  - 3륜 옴니(kiwi)에서 전형적인 분배 패턴.

---

## 7. 주의사항(실차 적용 시)

- 바퀴 “정회전(+)” 방향과 `s_i` 부호 정의가 다르면, 모터 드라이버/엔코더 기준에 맞춰 부호를 일괄적으로 뒤집어야 한다.
- 본 문서는 “접선 구동(Kiwi)” 가정을 기준으로 한다.
  - 휠이 다른 각도로 장착된 경우: 각 휠의 구동 단위벡터 `t_i`를 실제 장착 각도에 맞게 재정의하여 2장의 일반형을 사용하면 된다.
