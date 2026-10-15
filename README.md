# 🤖 ROS2_ROLLBOT · Aprende a construir un brazo robótico con ROS 2

[![ROS 2 Jazzy](https://img.shields.io/badge/ROS%202-Jazzy-22314E?logo=ros)](https://docs.ros.org/en/jazzy/)
[![Ubuntu 24.04](https://img.shields.io/badge/Ubuntu-24.04-E95420?logo=ubuntu&logoColor=white)](https://releases.ubuntu.com/24.04/)
[![Gazebo Harmonic](https://img.shields.io/badge/Gazebo-Harmonic-F58113)](https://gazebosim.org/docs/harmonic)
[![MoveIt 2](https://img.shields.io/badge/MoveIt-2-1F6FB2)](https://moveit.picknik.ai/)
[![Licencia MIT](https://img.shields.io/badge/licencia-MIT-green)](LICENSE)

Guía paso a paso, **en español**, para **aprender ROS 2 construyendo un brazo
robótico de 6 articulaciones**. Empieza desde cero (instalar ROS) y termina
moviendo el brazo con **MoveIt 2**, primero en simulación y después con
**motores reales**.

Está pensada para **principiantes y nivel medio**: cada capítulo explica los
conceptos con ejemplos cotidianos y todo el código está comentado línea a línea.

<!-- Cuando tengas capturas, guárdalas en docs/img/ y descomenta:
![Brazo en RViz](docs/img/rviz.png)
-->

---

## 🧭 ¿Qué vas a construir?

```
  Tú (RViz o un programa)  ──▶  MoveIt 2  ──▶  ros2_control  ──▶  ┌ mock (de mentira, para aprender)
  "lleva la pinza ahí"          planifica       mueve cada         ├ Gazebo (simulación con física)
                                sin chocar      articulación       └ Robot real (ESP32 + motores NEMA)
```

Lo más interesante: **el mismo software sirve para los tres casos**. Lo que
pruebas en el ordenador funciona igual en el robot real.

---

## 📚 Capítulos

| # | Capítulo | Nivel | Al terminar… |
|---|---|---|---|
| 01 | [Instalación de ROS 2 Jazzy](capitulos/01_instalacion_ros2/README.md) | 🟢 | Tienes ROS 2 funcionando |
| 02 | [El workspace](capitulos/02_workspace/README.md) | 🟢 | Tienes el proyecto descargado y compilado |
| 03 | [Las herramientas](capitulos/03_herramientas/README.md) | 🟢 | Conoces RViz, ros2_control, Gazebo y MoveIt |
| 04 | [Describir un robot: URDF y Xacro](capitulos/04_urdf_xacro/README.md) | 🟢→🟡 | Sabes "dibujar" un robot con texto |
| 05 | [Ver el robot en RViz](capitulos/05_visualizacion_rviz/README.md) | 🟢 | Mueves el brazo con deslizadores y entiendes las TF |
| 06 | [El modelo del brazo de 6 GDL](capitulos/06_descripcion_brazo/README.md) | 🟡 | Sabes adaptar el modelo a tu diseño |
| 07 | [ros2_control](capitulos/07_ros2_control/README.md) | 🟡 | El brazo sigue trayectorias con controladores |
| 08 | [Simulación en Gazebo](capitulos/08_simulacion_gazebo/README.md) | 🟡 | El brazo tiene peso, gravedad y choques |
| 09 | [Planificación con MoveIt 2](capitulos/09_moveit2/README.md) | 🟡 | El brazo llega solo a un punto sin chocar |
| 10 | [El brazo real](capitulos/10_hardware_paso_a_paso/README.md) | 🟡→🔴 | Los motores se mueven de verdad |
| 11 | [Programar el brazo](capitulos/11_programacion_ejemplos/README.md) | 🟡 | Escribes tus propios programas |

🟢 Principiante · 🟡 Medio · 🔴 Avanzado

### Cómo está organizado cada capítulo

Todos los capítulos siguen el mismo orden, para que sepas siempre dónde buscar:

1. **Introducción** — de qué va y por qué es importante.
2. **Objetivos** — lo que sabrás hacer al terminar.
3. **Conceptos clave** — vocabulario explicado con ejemplos cotidianos.
4. **Estructura** — qué archivos intervienen y cómo se relacionan.
5. **Proceso detallado** — los pasos, con cada comando comentado.
6. **Ejemplos** — prácticas y ejercicios para afianzar lo aprendido.
7. **Cierre** — resumen, lista de comprobación y solución de problemas.

> 💡 **Consejo:** sigue los capítulos **en orden**. Cada uno usa lo que
> aprendiste en el anterior.

---

## 🚀 Inicio rápido

Si ya tienes Ubuntu 24.04 y ROS 2 Jazzy (si no, empieza por el [capítulo 01](capitulos/01_instalacion_ros2/README.md)):

```bash
git clone https://github.com/DonDoney/ROS2_ROLLBOT.git ~/rollbot_ws   # descarga el proyecto
cd ~/rollbot_ws
rosdep install --from-paths src --ignore-src -r -y                    # instala lo que necesita
colcon build --symlink-install                                        # compila
source install/setup.bash                                             # lo activa
```

| Quiero… | Comando |
|---|---|
| Ver el brazo y moverlo con deslizadores | `ros2 launch brazo_description visualizar.launch.py` |
| Moverlo con controladores (sin hardware) | `ros2 launch brazo_bringup brazo.launch.py` |
| Simularlo con física | `ros2 launch brazo_gazebo gazebo.launch.py` |
| Planificar movimientos con MoveIt | `ros2 launch brazo_moveit_config demo.launch.py` |
| Mover el robot real | `ros2 launch brazo_bringup brazo.launch.py modo_hardware:=real` |

---

## 🗂️ Qué hay en el repositorio

```
ROS2_ROLLBOT/
├── README.md                      ← estás aquí
├── capitulos/                     ← LA GUÍA: un capítulo por carpeta
│   ├── 01_instalacion_ros2/          guía + script de instalación de ROS 2
│   ├── 02_workspace/                 guía + script para preparar el proyecto
│   ├── 03_herramientas/              guía + script de herramientas
│   ├── 04_urdf_xacro/ejemplos/       modelos de práctica (1 y 2 articulaciones)
│   ├── 05_visualizacion_rviz/
│   ├── 06_descripcion_brazo/
│   ├── 07_ros2_control/
│   ├── 08_simulacion_gazebo/
│   ├── 09_moveit2/
│   ├── 10_hardware_paso_a_paso/
│   └── 11_programacion_ejemplos/
├── src/                           ← EL CÓDIGO: paquetes de ROS 2
│   ├── brazo_description/            cómo ES el robot (forma y medidas)
│   ├── brazo_bringup/                cómo se ARRANCA (controladores)
│   ├── brazo_gazebo/                 cómo se SIMULA con física
│   ├── brazo_moveit_config/          cómo se PLANIFICAN sus movimientos
│   ├── brazo_hardware/               cómo se HABLA con los motores reales
│   └── brazo_examples/               programas de EJEMPLO (Python y C++)
├── firmware/                      ← programa para la placa ESP32
│   ├── PROTOCOLO.md                  cómo se comunican el PC y la placa
│   └── esp32_brazo/                  proyecto PlatformIO
├── docs/img/                      ← capturas y diagramas
└── .github/workflows/ci.yml       ← GitHub comprueba que todo compila en cada cambio
```

---

## 📖 Mini glosario

| Palabra | Significado |
|---|---|
| **ROS 2** | Conjunto de herramientas para programar robots |
| **Nodo** | Un programa que forma parte del sistema ROS |
| **Topic** | Canal por el que los nodos se envían mensajes |
| **Paquete** | Carpeta de código de ROS con una tarea concreta |
| **URDF / Xacro** | Archivos que describen la forma del robot |
| **TF** | Posición de cada pieza respecto a las demás |
| **GDL** | Grados de libertad: cuántas articulaciones independientes tiene el robot |
| **Controlador** | Algoritmo que decide qué orden enviar a cada motor |
| **Cinemática inversa** | Calcular los ángulos necesarios para llevar la herramienta a un punto |
| **Firmware** | Programa que se ejecuta dentro del microcontrolador |

---

## 🛠️ Hardware de referencia

| Elemento | Modelo de referencia |
|---|---|
| Microcontrolador | ESP32 DevKit V1 (ESP32-WROOM-32) |
| Drivers ejes 1-3 | De bucle cerrado tipo HBS860H (NEMA 34) |
| Drivers ejes 4-6 | De bucle cerrado tipo HBS57 (NEMA 23) |
| Adaptador de tensión | 74HCT245 a 5 V |
| Finales de carrera | Interruptores normalmente cerrados (NC), uno por eje |

Todos los detalles de cableado y puesta en marcha están en el [capítulo 10](capitulos/10_hardware_paso_a_paso/README.md).

---

## 🗺️ Hoja de ruta

- [x] Modelo de 6 GDL parametrizado con Xacro
- [x] ros2_control en modo mock, Gazebo y real
- [x] Configuración de MoveIt 2
- [x] Firmware ESP32 con homing y watchdog
- [ ] Piezas STL del diseño mecánico definitivo
- [ ] Pinza y su controlador
- [ ] Teleoperación con MoveIt Servo
- [ ] Visión para coger objetos (*pick and place*)

---

## 📄 Licencia

[MIT](LICENSE): puedes usarlo, modificarlo y compartirlo libremente.
