# 🤖 ROS2_ROLLBOT · Brazo robótico de 6 GDL con ROS 2 Jazzy

[![ROS 2 Jazzy](https://img.shields.io/badge/ROS%202-Jazzy-22314E?logo=ros)](https://docs.ros.org/en/jazzy/)
[![Ubuntu 24.04](https://img.shields.io/badge/Ubuntu-24.04-E95420?logo=ubuntu&logoColor=white)](https://releases.ubuntu.com/24.04/)
[![Gazebo Harmonic](https://img.shields.io/badge/Gazebo-Harmonic-F58113)](https://gazebosim.org/docs/harmonic)
[![MoveIt 2](https://img.shields.io/badge/MoveIt-2-1F6FB2)](https://moveit.picknik.ai/)
[![Licencia MIT](https://img.shields.io/badge/licencia-MIT-green)](LICENSE)

Guía paso a paso, **en español y con todo el código comentado**, para
desarrollar un brazo robótico de 6 grados de libertad con **ROS 2 Jazzy**:
desde instalar ROS hasta moverlo con **MoveIt 2** en simulación (**Gazebo
Harmonic**) y en un robot real con **motores paso a paso NEMA 23/34 de bucle
cerrado** controlados por un **ESP32**.

<!-- Cuando tengas capturas, guárdalas en docs/img/ y descomenta:
![Brazo en RViz](docs/img/rviz.png)
-->

## 📚 Capítulos

| # | Capítulo | Resultado |
|---|---|---|
| 01 | [Instalación de ROS 2 Jazzy](capitulos/01_instalacion_ros2/README.md) | ROS 2 funcionando en Ubuntu 24.04 |
| 02 | [El workspace](capitulos/02_workspace/README.md) | Repositorio clonado y compilado |
| 03 | [Herramientas: RViz, ros2_control, Gazebo, MoveIt](capitulos/03_herramientas/README.md) | Todo instalado y comprobado |
| 04 | [URDF y Xacro con ejemplos](capitulos/04_urdf_xacro/README.md) | Modelos de 1 y 2 GDL para practicar |
| 05 | [Visualización en RViz y TF](capitulos/05_visualizacion_rviz/README.md) | El brazo moviéndose con deslizadores |
| 06 | [Descripción del brazo de 6 GDL](capitulos/06_descripcion_brazo/README.md) | Modelo completo y parametrizado |
| 07 | [ros2_control](capitulos/07_ros2_control/README.md) | Controladores de trayectoria funcionando |
| 08 | [Simulación en Gazebo Harmonic](capitulos/08_simulacion_gazebo/README.md) | Brazo con física y gravedad |
| 09 | [Planificación con MoveIt 2](capitulos/09_moveit2/README.md) | Planificar y ejecutar desde RViz |
| 10 | [Hardware: ESP32 + drivers de bucle cerrado](capitulos/10_hardware_paso_a_paso/README.md) | El brazo real moviéndose |
| 11 | [Programación en Python y C++](capitulos/11_programacion_ejemplos/README.md) | Tus propios programas |

## 🚀 Inicio rápido

Con Ubuntu 24.04 y ROS 2 Jazzy ya instalados (si no, empieza por el capítulo 01):

```bash
git clone https://github.com/DonDoney/ROS2_ROLLBOT.git ~/rollbot_ws
cd ~/rollbot_ws
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
source install/setup.bash
```

| Quiero… | Comando |
|---|---|
| Ver el modelo y moverlo con deslizadores | `ros2 launch brazo_description visualizar.launch.py` |
| Probar ros2_control sin hardware | `ros2 launch brazo_bringup brazo.launch.py` |
| Simular en Gazebo | `ros2 launch brazo_gazebo gazebo.launch.py` |
| Planificar con MoveIt | `ros2 launch brazo_moveit_config demo.launch.py` |
| Mover el robot real | `ros2 launch brazo_bringup brazo.launch.py modo_hardware:=real` |

## 🗂️ Estructura del repositorio

```
ROS2_ROLLBOT/
├── README.md                      ← estás aquí
├── capitulos/                     ← la guía, un capítulo por carpeta
│   ├── 01_instalacion_ros2/          README + script de instalación
│   ├── 02_workspace/                 README + script para crear el workspace
│   ├── 03_herramientas/              README + script de herramientas
│   ├── 04_urdf_xacro/ejemplos/       URDF/Xacro de práctica (1 y 2 GDL)
│   ├── 05_visualizacion_rviz/
│   ├── 06_descripcion_brazo/
│   ├── 07_ros2_control/
│   ├── 08_simulacion_gazebo/
│   ├── 09_moveit2/
│   ├── 10_hardware_paso_a_paso/
│   └── 11_programacion_ejemplos/
├── src/                           ← paquetes ROS 2 (lo que compila colcon)
│   ├── brazo_description/            URDF/Xacro, RViz, mallas
│   ├── brazo_bringup/                launch + config de ros2_control
│   ├── brazo_gazebo/                 mundo y launch de Gazebo Harmonic
│   ├── brazo_moveit_config/          SRDF, cinemática, límites, launch MoveIt
│   ├── brazo_hardware/               plugin C++ ros2_control ↔ ESP32 por serie
│   └── brazo_examples/               ejemplos Python y C++
├── firmware/                      ← código del microcontrolador (fuera de colcon)
│   ├── PROTOCOLO.md                  protocolo serie PC ↔ ESP32
│   └── esp32_brazo/                  proyecto PlatformIO
├── docs/img/                      ← capturas y diagramas
└── .github/workflows/ci.yml       ← compila el workspace en cada push
```

## 🧩 Arquitectura

```
 MoveIt 2 (move_group) ──FollowJointTrajectory──▶ brazo_controller
                                                        │
                                              ros2_control (100 Hz)
                                                        │
              ┌─────────────────────────────────────────┼──────────────────────────┐
     mock_components/GenericSystem        gz_ros2_control/GazeboSimSystem   brazo_hardware/BrazoSystemHardware
        (sin hardware)                        (Gazebo Harmonic)              (USB → ESP32 → drivers → NEMA)
```

El **mismo** modelo, los **mismos** controladores y la **misma** configuración
de MoveIt sirven para los tres casos: solo cambia el argumento `modo_hardware`.

## 🛠️ Hardware de referencia

| Elemento | Modelo de referencia |
|---|---|
| Microcontrolador | ESP32 DevKit V1 (ESP32-WROOM-32) |
| Drivers ejes 1-3 | Bucle cerrado tipo HBS860H (NEMA 34) |
| Drivers ejes 4-6 | Bucle cerrado tipo HBS57 (NEMA 23) |
| Adaptación de niveles | 74HCT245 a 5 V |
| Finales de carrera | Interruptores NC, uno por eje |

Detalles de cableado y puesta en marcha en el [capítulo 10](capitulos/10_hardware_paso_a_paso/README.md).

## 🗺️ Hoja de ruta

- [x] Modelo 6 GDL parametrizado con Xacro
- [x] ros2_control en modo mock, Gazebo y real
- [x] Configuración de MoveIt 2
- [x] Firmware ESP32 con homing y watchdog
- [ ] Mallas STL del diseño mecánico definitivo
- [ ] Pinza y controlador de pinza
- [ ] Teleoperación con MoveIt Servo
- [ ] Visión para *pick and place*

## 📄 Licencia

[MIT](LICENSE) — úsalo, modifícalo y compártelo libremente.
