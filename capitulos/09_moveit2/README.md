# Capítulo 09 · Planificación de movimientos con MoveIt 2

> **Objetivo:** pedir al brazo "pon la herramienta aquí" y que MoveIt calcule
> los ángulos (cinemática inversa) y una trayectoria sin colisiones.

## Lanzar la demo

```bash
ros2 launch brazo_moveit_config demo.launch.py
```

En RViz, panel **MotionPlanning**:

1. Pestaña *Planning* → *Planning Group*: `brazo`.
2. Arrastra la **bola/flechas** de la punta del brazo a otra posición
   (o elige un *Goal State* con nombre: `home`, `listo`, `plegado`).
3. Pulsa **Plan** → verás el movimiento previsto en violeta.
4. Pulsa **Execute** → el brazo lo ejecuta a través de `brazo_controller`.

## Piezas de MoveIt

```
               ┌───────────────────────── move_group ─────────────────────────┐
  RViz ──────▶ │  Cinemática inversa (KDL)   Planificador (OMPL)              │
  tu programa  │  Escena de planificación (obstáculos)   Comprobación de choques│
               └───────────────────────────────┬──────────────────────────────┘
                                               │ FollowJointTrajectory
                                               ▼
                                     brazo_controller (ros2_control)
```

| Archivo | Para qué |
|---|---|
| [`brazo.srdf`](../../src/brazo_moveit_config/config/brazo.srdf) | Grupo `brazo`, posturas con nombre y pares de colisión ignorados |
| [`kinematics.yaml`](../../src/brazo_moveit_config/config/kinematics.yaml) | Solucionador de IK (KDL) |
| [`joint_limits.yaml`](../../src/brazo_moveit_config/config/joint_limits.yaml) | Velocidades y **aceleraciones** máximas; escala por defecto al 10 % |
| [`moveit_controllers.yaml`](../../src/brazo_moveit_config/config/moveit_controllers.yaml) | A qué controlador se mandan las trayectorias |
| [`moveit.rviz`](../../src/brazo_moveit_config/config/moveit.rviz) | RViz con el plugin MotionPlanning |
| [`demo.launch.py`](../../src/brazo_moveit_config/launch/demo.launch.py) | Todo en uno con hardware mock |
| [`move_group.launch.py`](../../src/brazo_moveit_config/launch/move_group.launch.py) | Solo MoveIt, para Gazebo o el robot real |

## Tres formas de usar MoveIt

| Modo | Comando |
|---|---|
| Solo simulación cinemática | `ros2 launch brazo_moveit_config demo.launch.py` |
| Con Gazebo | `gazebo.launch.py rviz:=false` + `move_group.launch.py use_sim_time:=true` |
| Con el robot real | `brazo.launch.py modo_hardware:=real rviz:=false` + `move_group.launch.py` |

## Programar con MoveIt

Ver el ejemplo C++ comentado [`moveit_ejemplo.cpp`](../../src/brazo_examples/src/moveit_ejemplo.cpp)
(capítulo 11): postura con nombre, objetivo en pose, trayectoria cartesiana y
obstáculos en la escena.

## Regenerar la configuración con el Setup Assistant

Esta configuración está escrita a mano para que cada archivo sea legible.
Si cambias mucho el robot (otra cinemática, una pinza…), puedes usar el
asistente gráfico:

```bash
ros2 launch moveit_setup_assistant setup_assistant.launch.py
```

1. *Edit Existing MoveIt Configuration Package* → elige `src/brazo_moveit_config`.
2. **Self-Collisions** → *Generate Collision Matrix* (recalcula qué pares ignorar).
3. Revisa grupos y posturas, y en *Configuration Files* desmarca los archivos
   que no quieras sobrescribir (por ejemplo los launch personalizados).

## Mejoras posibles

- **pick_ik** o **TRAC-IK** en lugar de KDL: IK más rápida y robusta
  (`sudo apt install ros-jazzy-pick-ik`, y cambiar `kinematics.yaml`).
- **Pilz Industrial Motion Planner**: movimientos tipo robot industrial (PTP, LIN, CIRC).
- **MoveIt Servo**: control en tiempo real con joystick o teclado.

⬅️ [Capítulo 08](../08_simulacion_gazebo/README.md) · ➡️ [Capítulo 10 · Hardware](../10_hardware_paso_a_paso/README.md)
