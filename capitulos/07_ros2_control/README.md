# Capítulo 07 · ros2_control: controladores y hardware

> **Objetivo:** mover el brazo con controladores de verdad, primero con un
> hardware simulado (*mock*), para que luego el paso a Gazebo o al robot real
> sea solo cambiar un argumento.

## La idea de ros2_control

```
            ┌──────────────── controller_manager (100 Hz) ─────────────────┐
            │                                                               │
 MoveIt ──▶ │  brazo_controller          joint_state_broadcaster            │ ──▶ /joint_states
 (acción)   │  (JointTrajectoryController)                                  │
            │        │ command: position          ▲ state: position, velocity│
            │        ▼                            │                          │
            │  ┌──────────── Resource Manager ─────────────┐                 │
            │  │  plugin de hardware: mock | gazebo | real │                 │
            │  └───────────────────────────────────────────┘                 │
            └───────────────────────────────────────────────────────────────┘
```

- Los **controladores** no saben qué hardware hay debajo: escriben y leen
  *interfaces* (`joint_1/position`…).
- El **plugin de hardware** traduce esas interfaces a lo que sea: nada (mock),
  el simulador o el puerto serie del ESP32.

Por eso en el URDF solo cambia la línea `<plugin>` según `modo_hardware`
(ver [`brazo.ros2_control.xacro`](../../src/brazo_description/urdf/brazo.ros2_control.xacro)).

## Archivos implicados

| Archivo | Contenido |
|---|---|
| [`brazo.ros2_control.xacro`](../../src/brazo_description/urdf/brazo.ros2_control.xacro) | Articulaciones, interfaces y plugin |
| [`brazo_controladores.yaml`](../../src/brazo_bringup/config/brazo_controladores.yaml) | Frecuencia y controladores |
| [`brazo.launch.py`](../../src/brazo_bringup/launch/brazo.launch.py) | Arranca todo |

## Prueba con hardware simulado (mock)

```bash
ros2 launch brazo_bringup brazo.launch.py
```

En otra terminal:

```bash
# ¿Qué controladores hay y en qué estado?
ros2 control list_controllers
#   joint_state_broadcaster  ... active
#   brazo_controller         ... active

# ¿Qué interfaces ofrece el hardware?
ros2 control list_hardware_interfaces

# Mandar una trayectoria a mano (un solo punto, llegar en 3 s)
ros2 action send_goal /brazo_controller/follow_joint_trajectory \
  control_msgs/action/FollowJointTrajectory "{
  trajectory: {
    joint_names: [joint_1, joint_2, joint_3, joint_4, joint_5, joint_6],
    points: [{positions: [0.5, 0.3, 0.8, 0.0, 0.6, 0.0], time_from_start: {sec: 3}}]
  }}"
```

El brazo se moverá en RViz. También puedes usar el ejemplo Python:

```bash
ros2 run brazo_examples mover_articulaciones.py
```

## Controladores usados

| Controlador | Paquete | Qué hace |
|---|---|---|
| `joint_state_broadcaster` | joint_state_broadcaster | Lee las *state interfaces* y publica `/joint_states` |
| `brazo_controller` | joint_trajectory_controller | Recibe trayectorias (lista de puntos con tiempo) y las interpola |

### Parámetros importantes del YAML

- `update_rate`: frecuencia del bucle. Con el ESP32 a 115200 baudios, 100 Hz es seguro.
- `command_interfaces: [position]`: los drivers de pasos se mandan en posición.
- `constraints`: si el error de seguimiento supera `trajectory`, la trayectoria
  se aborta. **Importante con el robot real**: detecta que un eje se ha atascado.

## Comandos útiles

```bash
ros2 control list_controller_types          # tipos disponibles
ros2 control set_controller_state brazo_controller inactive   # pausar
ros2 control set_controller_state brazo_controller active     # reanudar
ros2 control set_hardware_component_state BrazoSystem inactive   # deshabilitar drivers
```

⬅️ [Capítulo 06](../06_descripcion_brazo/README.md) · ➡️ [Capítulo 08 · Gazebo](../08_simulacion_gazebo/README.md)
