# Capítulo 11 · Programar el brazo (Python y C++)

> **Objetivo:** controlar el brazo desde tus propios programas, de lo más
> sencillo (leer ángulos) a lo más completo (MoveIt con obstáculos).

Todos los ejemplos funcionan igual con **mock, Gazebo o el robot real**,
porque hablan con ros2_control/MoveIt y no con el hardware directamente.

| # | Ejemplo | Lenguaje | Qué enseña |
|---|---|---|---|
| 1 | [`leer_estado.py`](../../src/brazo_examples/scripts/leer_estado.py) | Python | Nodo, suscripción a un topic |
| 2 | [`mover_articulaciones.py`](../../src/brazo_examples/scripts/mover_articulaciones.py) | Python | Cliente de **acción** `FollowJointTrajectory`, sin MoveIt |
| 3 | [`moveit_ejemplo.cpp`](../../src/brazo_examples/src/moveit_ejemplo.cpp) | C++ | `MoveGroupInterface`: postura con nombre, pose, cartesiano, obstáculos |

## Ejemplo 1 · Leer el estado

```bash
# Terminal 1 (cualquier modo)
ros2 launch brazo_bringup brazo.launch.py
# Terminal 2
ros2 run brazo_examples leer_estado.py
```

## Ejemplo 2 · Trayectoria articular

```bash
ros2 run brazo_examples mover_articulaciones.py
```

Edita la lista `SECUENCIA` del script para crear tus propias posturas (en grados).

## Ejemplo 3 · MoveIt desde C++

```bash
# Terminal 1
ros2 launch brazo_moveit_config demo.launch.py
# Terminal 2
ros2 launch brazo_examples moveit_ejemplo.launch.py
```

Verás en RViz cómo aparece una caja (obstáculo), el brazo va a `listo`, se
desplaza 10 cm, dibuja un cuadrado en línea recta y vuelve a `home`.

### Topics, servicios y acciones: ¿cuándo usar cada uno?

| Mecanismo | Patrón | Ejemplo en este proyecto |
|---|---|---|
| **Topic** | Flujo continuo, sin respuesta | `/joint_states` |
| **Servicio** | Pregunta → respuesta inmediata | `/controller_manager/list_controllers` |
| **Acción** | Objetivo largo con progreso y resultado | `/brazo_controller/follow_joint_trajectory` |

## Ideas para seguir

- Añadir una **pinza** (nuevo eslabón + controlador `parallel_gripper_action_controller`).
- **Teleoperación** con mando de PlayStation/Xbox usando MoveIt Servo.
- **Visión**: cámara RealSense u OAK-D y detección de objetos para *pick and place*.
- **Interfaz web** con `rosbridge` para manejar el brazo desde el móvil.

⬅️ [Capítulo 10](../10_hardware_paso_a_paso/README.md) · 🏠 [Inicio](../../README.md)
