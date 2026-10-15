# Capítulo 08 · Simulación física en Gazebo Harmonic

> **Objetivo:** simular el brazo con gravedad, inercias y colisiones, usando
> exactamente los mismos controladores que el robot real.

## Lanzar

```bash
ros2 launch brazo_gazebo gazebo.launch.py
```

Se abre Gazebo con el brazo y una mesa delante, y RViz mostrando lo mismo.
Prueba a moverlo:

```bash
ros2 run brazo_examples mover_articulaciones.py
```

## Cómo funciona

```
 gazebo.launch.py
   ├─ gz sim mundo_brazo.sdf              (simulador)
   ├─ robot_state_publisher               (URDF con modo_hardware:=gazebo)
   ├─ ros_gz_sim create -topic robot_description   (mete el robot en Gazebo)
   │      └─ plugin gz_ros2_control  ──▶ controller_manager DENTRO de Gazebo
   ├─ ros_gz_bridge  /clock               (tiempo simulado → ROS)
   ├─ spawner joint_state_broadcaster     (cuando el robot ya existe)
   └─ spawner brazo_controller
```

Diferencias respecto al modo mock:

| | mock | Gazebo |
|---|---|---|
| ¿Quién crea el controller_manager? | `ros2_control_node` | El plugin `gz_ros2_control` |
| Plugin de hardware | `mock_components/GenericSystem` | `gz_ros2_control/GazeboSimSystem` |
| Física | No | Sí (gravedad, inercias, colisiones) |
| Reloj | Real | Simulado (`use_sim_time: true`) |

## Archivos

| Archivo | Para qué |
|---|---|
| [`gazebo.launch.py`](../../src/brazo_gazebo/launch/gazebo.launch.py) | Orquesta todo lo anterior |
| [`mundo_brazo.sdf`](../../src/brazo_gazebo/worlds/mundo_brazo.sdf) | Suelo, luz y mesa |
| [`brazo.gazebo.xacro`](../../src/brazo_description/urdf/brazo.gazebo.xacro) | Plugin gz_ros2_control y colores |

## Gazebo + MoveIt

```bash
# Terminal 1
ros2 launch brazo_gazebo gazebo.launch.py rviz:=false
# Terminal 2
ros2 launch brazo_moveit_config move_group.launch.py use_sim_time:=true
```

## Problemas frecuentes

- **El brazo se cae o tiembla** → revisa masas e inercias en
  `brazo_propiedades.xacro`. Inercias muy pequeñas o masas desproporcionadas
  hacen inestable la física.
- **`[spawner] Could not contact service /controller_manager/...`** → el
  plugin no se cargó. Comprueba que está instalado `ros-jazzy-gz-ros2-control`
  y que el URDF se generó con `modo_hardware:=gazebo`.
- **No aparecen las mallas STL** (cuando las añadas) → Gazebo necesita saber
  dónde buscarlas; ejecuta `export GZ_SIM_RESOURCE_PATH=$GZ_SIM_RESOURCE_PATH:$(ros2 pkg prefix brazo_description)/share`
  antes de lanzar.
- **Va muy lento** → en máquinas virtuales Gazebo usa render por software;
  prueba `export LIBGL_ALWAYS_SOFTWARE=1` o mejor, Ubuntu nativo.

⬅️ [Capítulo 07](../07_ros2_control/README.md) · ➡️ [Capítulo 09 · MoveIt 2](../09_moveit2/README.md)
