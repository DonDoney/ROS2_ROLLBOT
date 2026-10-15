# Capítulo 03 · Herramientas: RViz, ros2_control, Gazebo y MoveIt

> **Objetivo:** instalar y conocer las herramientas que usaremos en el resto
> del proyecto, y comprobar que cada una funciona.

## Instalación

```bash
cd capitulos/03_herramientas
chmod +x instalar_herramientas.sh
./instalar_herramientas.sh
```

Ver el script [`instalar_herramientas.sh`](instalar_herramientas.sh) para el
detalle de cada paquete.

## Mapa de herramientas

```
                ┌────────────────────┐
                │      MoveIt 2      │  planifica trayectorias sin colisiones
                └─────────┬──────────┘
                          │ FollowJointTrajectory (acción)
                ┌─────────▼──────────┐
                │    ros2_control    │  controladores + bucle read/update/write
                └─────────┬──────────┘
          ┌───────────────┼────────────────┐
   ┌──────▼─────┐  ┌──────▼──────┐  ┌──────▼──────┐
   │    mock    │  │   Gazebo    │  │ brazo_hw    │
   │ (sin nada) │  │  Harmonic   │  │ ESP32+NEMA  │
   └────────────┘  └─────────────┘  └─────────────┘

   RViz2: visualiza TODO lo anterior (modelo, TF, planes, escena)
```

| Herramienta | Para qué la usamos | Capítulo |
|---|---|---|
| **xacro** | Escribir el URDF con variables, macros y fórmulas | 04, 06 |
| **RViz2** | Ver el robot, sus ejes (TF) y los planes de MoveIt | 05 |
| **ros2_control** | Capa común entre controladores y hardware (real o simulado) | 07 |
| **Gazebo Harmonic** | Simulación física (gravedad, colisiones, inercias) | 08 |
| **MoveIt 2** | Cinemática inversa, planificación y evitación de obstáculos | 09 |

### Sobre Gazebo

Hay dos "Gazebos":

- **Gazebo Classic** (gazebo 11): versión antigua, **ya sin soporte** y no
  disponible para Jazzy.
- **Gazebo (antes Ignition)**: la versión actual. Con Jazzy se usa
  **Gazebo Harmonic**. El comando es `gz sim`, no `gazebo`.

Si en tutoriales antiguos ves `gazebo_ros`, `libgazebo_ros2_control.so` o
`spawn_entity.py`, son de Gazebo Classic: aquí usamos `ros_gz_sim`,
`gz_ros2_control` y `create`.

## Comprobaciones rápidas

```bash
# RViz
rviz2

# Gazebo Harmonic con un mundo de ejemplo
gz sim shapes.sdf

# Un robot de ejemplo de MoveIt (si no está, no pasa nada: usaremos el nuestro)
ros2 pkg list | grep moveit_resources

# ros2_control: lista de tipos de controladores disponibles
ros2 pkg list | grep controller
```

## Herramientas de diagnóstico que usarás mucho

| Comando | Qué muestra |
|---|---|
| `ros2 node list` | Nodos en marcha |
| `ros2 topic list` / `ros2 topic echo /joint_states` | Topics y su contenido |
| `rqt_graph` | Grafo de nodos y topics |
| `ros2 run tf2_tools view_frames` | PDF con el árbol de transformadas |
| `ros2 control list_controllers` | Controladores cargados y su estado |
| `ros2 control list_hardware_interfaces` | Interfaces de hardware |

⬅️ [Capítulo 02](../02_workspace/README.md) · ➡️ [Capítulo 04 · URDF y Xacro](../04_urdf_xacro/README.md)
