# Capítulo 02 · El workspace de ROS 2

> **Objetivo:** entender qué es un *workspace*, cómo se organiza este repositorio
> y compilarlo por primera vez.

## ¿Qué es un workspace?

Es una carpeta donde viven tus **paquetes** de ROS 2. Al compilar con `colcon`
aparecen tres carpetas nuevas:

```
rollbot_ws/
├── src/        ← TU código (lo único que se sube a GitHub)
├── build/      ← archivos intermedios de compilación
├── install/    ← resultado final: lo que ROS ejecuta
└── log/        ← registros de cada compilación
```

`build/`, `install/` y `log/` están en el [`.gitignore`](../../.gitignore):
se regeneran en cada máquina y no deben subirse.

## Este repositorio es un workspace

```bash
git clone https://github.com/DonDoney/ROS2_ROLLBOT.git ~/rollbot_ws
```

Dentro de `src/` hay **seis paquetes**, cada uno con una responsabilidad:

| Paquete | Tipo | Para qué sirve |
|---|---|---|
| `brazo_description` | ament_cmake | Modelo URDF/Xacro, RViz, mallas |
| `brazo_bringup` | ament_cmake | Launch y configuración de ros2_control |
| `brazo_gazebo` | ament_cmake | Mundo y launch de simulación |
| `brazo_moveit_config` | ament_cmake | Configuración de MoveIt 2 |
| `brazo_hardware` | ament_cmake (C++) | Plugin que habla con el ESP32 |
| `brazo_examples` | ament_cmake (C++ y Python) | Programas de ejemplo |

> Separar en paquetes es la convención de ROS (lo hacen Universal Robots,
> Franka, Kinova…). Así puedes, por ejemplo, reutilizar `brazo_description`
> en otro proyecto sin arrastrar MoveIt.

## Anatomía de un paquete

```
brazo_description/
├── package.xml      ← nombre, versión, dependencias (lo lee rosdep)
├── CMakeLists.txt   ← qué se compila y qué se instala
├── urdf/            ← recursos propios del paquete
├── launch/
└── rviz/
```

Para crear un paquete nuevo desde cero:

```bash
cd ~/rollbot_ws/src
ros2 pkg create mi_paquete --build-type ament_cmake --dependencies rclcpp   # C++
ros2 pkg create mi_paquete_py --build-type ament_python --dependencies rclpy # Python
```

## Compilar

Automático: [`crear_workspace.sh`](crear_workspace.sh). Manual:

```bash
cd ~/rollbot_ws
source /opt/ros/jazzy/setup.bash
rosdep install --from-paths src --ignore-src -r -y   # dependencias
colcon build --symlink-install                        # compilar
source install/setup.bash                             # "activar" el workspace
```

### Comandos útiles de colcon

| Comando | Qué hace |
|---|---|
| `colcon build --packages-select brazo_hardware` | Compila solo un paquete |
| `colcon build --packages-up-to brazo_bringup` | Un paquete y sus dependencias |
| `colcon build --symlink-install` | Enlaza archivos para no recompilar al editar URDF/launch/Python |
| `rm -rf build install log` | Limpieza total (si algo se queda "raro") |

## Comprobación

```bash
ros2 pkg list | grep brazo
```

Deben aparecer los seis paquetes.

## ¿Por qué hay que hacer `source` dos veces?

- `/opt/ros/jazzy/setup.bash` → carga ROS (la "base", llamada *underlay*).
- `~/rollbot_ws/install/setup.bash` → carga tus paquetes **encima** (*overlay*).

El orden importa: primero la base, después tu workspace.

⬅️ [Capítulo 01](../01_instalacion_ros2/README.md) · ➡️ [Capítulo 03 · Herramientas](../03_herramientas/README.md)
