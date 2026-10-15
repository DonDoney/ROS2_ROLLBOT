# Capítulo 07 · ros2_control: mover el brazo con controladores

| Nivel | Tiempo aproximado | Necesitas |
|---|---|---|
| 🟡 Medio | 1-1,5 horas | Capítulos [05](../05_visualizacion_rviz/README.md) y [06](../06_descripcion_brazo/README.md) |

---

## 1. Introducción

Hasta ahora el brazo se movía con deslizadores: tú decidías el ángulo y el
brazo "aparecía" en esa posición al instante. Un robot real no funciona así.
Necesita algo que:

- Reciba una **trayectoria**: "ve de aquí a allí en 3 segundos".
- Calcule, **100 veces por segundo**, dónde debe estar cada articulación.
- Envíe esas órdenes a los motores y **compruebe** que las están cumpliendo.

Eso es **ros2_control**. Su gran idea es separar el **"qué hacer"** (los
controladores) del **"con qué hacerlo"** (el hardware). Así, el mismo
controlador puede mover:

- un robot de mentira (*mock*), para probar sin nada conectado,
- un robot simulado en Gazebo,
- o nuestro brazo real con motores paso a paso.

En este capítulo lo usaremos en modo **mock**, que es la forma más segura y
rápida de aprender.

---

## 2. Objetivos

Al terminar este capítulo serás capaz de:

- [ ] Explicar las tres capas de ros2_control: controller manager, controladores y hardware.
- [ ] Entender qué son las *command interfaces* y las *state interfaces*.
- [ ] Lanzar el brazo con controladores en modo mock.
- [ ] Consultar el estado de los controladores desde la terminal.
- [ ] Enviar una trayectoria al brazo, a mano y con un programa.

---

## 3. Conceptos clave

| Concepto | Explicación sencilla | Analogía (una orquesta) |
|---|---|---|
| **controller_manager** | Programa central que ejecuta el bucle de control y gestiona los controladores | El director de orquesta |
| **Controlador** | Algoritmo que decide qué orden mandar a cada articulación | Un músico |
| **Hardware interface (plugin)** | Traduce las órdenes al "idioma" del hardware real o simulado | El instrumento |
| **Command interface** | Valor que el controlador **escribe** (aquí: la posición deseada) | La nota que toca el músico |
| **State interface** | Valor que el controlador **lee** (aquí: posición y velocidad reales) | Lo que el músico escucha |
| **Bucle read → update → write** | Cada ciclo: leer estado, calcular, enviar orden | Escuchar, pensar, tocar |
| **`update_rate`** | Cuántas veces por segundo se repite el bucle (100 Hz = cada 10 ms) | El tempo |
| **Trayectoria** | Lista de posturas, cada una con el instante en que hay que alcanzarla | Una partitura |
| **Acción** | Forma de comunicación para tareas largas: envías un objetivo, recibes progreso y un resultado final | Pedir una pizza y seguir el pedido en la app |
| **Spawner** | Pequeño programa que carga y activa un controlador | El regidor que da la entrada a cada músico |
| **mock** | Hardware "de mentira": copia la orden al estado al instante | Un ensayo sin instrumentos |

---

## 4. Estructura

### Las capas de ros2_control

```
            ┌──────────────── controller_manager (100 veces/segundo) ───────────────┐
            │                                                                       │
 Trayectoria│   brazo_controller               joint_state_broadcaster               │──▶ /joint_states
 (acción) ─▶│   "Sigo la trayectoria"          "Publico los ángulos actuales"         │
            │        │ escribe: posición             ▲ lee: posición, velocidad      │
            │        ▼                               │                               │
            │   ┌──────────────── Hardware (UNO de estos) ────────────────┐          │
            │   │  mock  │  Gazebo  │  brazo_hardware (ESP32 + motores)    │          │
            │   └─────────────────────────────────────────────────────────┘          │
            └───────────────────────────────────────────────────────────────────────┘
```

Fíjate: ahora es el **joint_state_broadcaster** quien publica `/joint_states`.
Ocupa el lugar que tenían los deslizadores en el capítulo 05.

### Archivos implicados

```
src/brazo_description/urdf/
└── brazo.ros2_control.xacro     ← qué articulaciones hay, qué interfaces tienen y qué plugin usar

src/brazo_bringup/
├── config/brazo_controladores.yaml   ← frecuencia del bucle y controladores
└── launch/brazo.launch.py            ← arranca todo
```

### 4.1 · El bloque `<ros2_control>` del modelo

En [`brazo.ros2_control.xacro`](../../src/brazo_description/urdf/brazo.ros2_control.xacro)
se elige el plugin según `modo_hardware`:

```xml
<ros2_control name="BrazoSystem" type="system">
  <hardware>
    <!-- modo mock: hardware de mentira -->
    <plugin>mock_components/GenericSystem</plugin>
    <!-- modo gazebo: gz_ros2_control/GazeboSimSystem -->
    <!-- modo real:   brazo_hardware/BrazoSystemHardware -->
  </hardware>

  <joint name="joint_1">
    <command_interface name="position"/>   <!-- el controlador ESCRIBE la posición deseada -->
    <state_interface name="position"/>     <!-- y LEE la posición real... -->
    <state_interface name="velocity"/>     <!-- ...y la velocidad -->
  </joint>
  <!-- … igual para joint_2 … joint_6 -->
</ros2_control>
```

### 4.2 · El archivo de controladores

[`brazo_controladores.yaml`](../../src/brazo_bringup/config/brazo_controladores.yaml), resumido:

```yaml
controller_manager:
  ros__parameters:
    update_rate: 100                     # el bucle se repite 100 veces por segundo

    joint_state_broadcaster:             # publica /joint_states
      type: joint_state_broadcaster/JointStateBroadcaster

    brazo_controller:                    # sigue trayectorias
      type: joint_trajectory_controller/JointTrajectoryController

brazo_controller:
  ros__parameters:
    joints: [joint_1, joint_2, joint_3, joint_4, joint_5, joint_6]
    command_interfaces: [position]       # los motores paso a paso se mandan en posición
    state_interfaces: [position, velocity]
    constraints:                         # tolerancias: si el error es mayor, se aborta
      joint_1: { trajectory: 0.2, goal: 0.02 }   # 0,2 rad durante el movimiento, 0,02 al final
```

> 🛡️ Las **tolerancias** (`constraints`) son una medida de seguridad muy útil
> con el robot real: si un eje se atasca y se queda atrás, el controlador lo
> detecta y detiene la trayectoria.

---

## 5. Proceso detallado

### Paso 1 · Lanzar el brazo en modo mock

```bash
ros2 launch brazo_bringup brazo.launch.py
#                                         (por defecto modo_hardware:=mock)
```

El launch arranca, en este orden:

1. `robot_state_publisher` → publica el modelo y las TF.
2. `ros2_control_node` → el controller manager con el hardware mock.
3. Spawner de `joint_state_broadcaster` → empieza a publicar `/joint_states`.
4. Spawner de `brazo_controller` → listo para recibir trayectorias.
5. RViz.

Esta vez **no hay deslizadores**: el brazo solo se moverá cuando le mandemos
una trayectoria.

### Paso 2 · Comprobar que los controladores están activos

En otra terminal:

```bash
ros2 control list_controllers
# joint_state_broadcaster  joint_state_broadcaster/JointStateBroadcaster       active
# brazo_controller         joint_trajectory_controller/JointTrajectoryController active
#                                                                              └── debe decir "active"
```

```bash
ros2 control list_hardware_interfaces
# command interfaces
#     joint_1/position [available] [claimed]    ← "claimed": la usa brazo_controller
# state interfaces
#     joint_1/position
#     joint_1/velocity
#     ...
```

### Paso 3 · Enviar una trayectoria a mano

Enviamos un único punto: "llega a esta postura en 3 segundos".

```bash
ros2 action send_goal /brazo_controller/follow_joint_trajectory \
  control_msgs/action/FollowJointTrajectory "{
  trajectory: {
    joint_names: [joint_1, joint_2, joint_3, joint_4, joint_5, joint_6],
    points: [
      { positions: [0.5, 0.3, 0.8, 0.0, 0.6, 0.0],
        time_from_start: { sec: 3 } }
    ]
  }}"
```

Qué significa cada parte:

| Parte | Significado |
|---|---|
| `/brazo_controller/follow_joint_trajectory` | Nombre de la acción que ofrece el controlador |
| `control_msgs/action/FollowJointTrajectory` | Tipo de mensaje |
| `joint_names` | Orden en que damos los ángulos |
| `positions` | Ángulos objetivo en **radianes** (0,5 rad ≈ 29°) |
| `time_from_start` | En cuánto tiempo hay que llegar |

Verás el brazo moverse suavemente en RViz y, al terminar, el mensaje
`Goal finished with status: SUCCEEDED`.

### Paso 4 · Pausar y reanudar un controlador

```bash
ros2 control set_controller_state brazo_controller inactive   # pausar
ros2 control set_controller_state brazo_controller active     # reanudar
```

Con el controlador inactivo, prueba a repetir el paso 3: la acción será rechazada.

---

## 6. Ejemplos

### Ejemplo 1 · Secuencia de posturas con Python

El programa [`mover_articulaciones.py`](../../src/brazo_examples/scripts/mover_articulaciones.py)
envía 4 posturas seguidas (escritas en **grados** para que sea más fácil):

```bash
ros2 run brazo_examples mover_articulaciones.py
```

Ábrelo y modifica la lista `SECUENCIA` para crear tu propio movimiento:

```python
SECUENCIA = [
    ([0, 0, 0, 0, 0, 0], 3.0),         # (ángulos en grados, instante en segundos)
    ([45, 20, 60, 0, 60, 0], 7.0),
    ...
]
```

### Ejemplo 2 · Ver los ángulos en grados

```bash
ros2 run brazo_examples leer_estado.py
```

Déjalo funcionando mientras ejecutas el ejemplo 1.

### Ejemplo 3 · Provocar un error a propósito

Repite el envío del paso 3, pero escribe mal un nombre: `joint_9` en lugar de
`joint_6`. El controlador **rechazará** la trayectoria (`Goal was rejected`)
porque no conoce esa articulación. Aprender a reconocer este mensaje te
ahorrará tiempo más adelante.

> ⚠️ El `joint_trajectory_controller` **no comprueba** los límites de ángulo
> del modelo: si le pides 3 rad en `joint_2`, en modo mock lo hará. Por eso,
> con el robot real, las trayectorias deben venir de **MoveIt** (capítulo 09),
> que sí respeta los límites, o de programas que tú hayas revisado.

---

## 7. Cierre

### Resumen

- **ros2_control** separa los controladores (qué hacer) del hardware (con qué).
- El **controller manager** ejecuta 100 veces por segundo: leer → calcular → escribir.
- Usamos dos controladores: `joint_state_broadcaster` (publica ángulos) y
  `brazo_controller` (sigue trayectorias).
- Cambiar de mock a Gazebo o al robot real es **solo cambiar el plugin**.

### Comprueba que lo tienes

- [ ] `ros2 control list_controllers` muestra ambos controladores `active`.
- [ ] Has enviado una trayectoria a mano y el brazo se ha movido en RViz.
- [ ] Has modificado `SECUENCIA` en el ejemplo de Python.

### Si algo falla

| Síntoma | Causa probable | Solución |
|---|---|---|
| `Could not contact service /controller_manager/...` | El controller manager no arrancó | Mira la primera terminal: busca el primer error en rojo |
| Controlador en estado `inactive` o `unconfigured` | El spawner falló | `ros2 control load_controller --set-state active brazo_controller` |
| `Goal was rejected` | Nombres de joints mal escritos o controlador inactivo | Revisa `joint_names` y `ros2 control list_controllers` |
| El brazo no aparece en RViz | No hay `/joint_states` | Comprueba que `joint_state_broadcaster` está `active` |

➡️ **Siguiente capítulo:** [08 · Simulación en Gazebo](../08_simulacion_gazebo/README.md),
donde el brazo tendrá peso, gravedad y podrá chocar.
