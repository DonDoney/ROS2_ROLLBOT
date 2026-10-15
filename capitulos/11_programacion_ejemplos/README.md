# Capítulo 11 · Programar el brazo en Python y C++

| Nivel | Tiempo aproximado | Necesitas |
|---|---|---|
| 🟡 Medio | 1-2 horas | Capítulos [07](../07_ros2_control/README.md) y [09](../09_moveit2/README.md). Nociones básicas de Python |

---

## 1. Introducción

Hasta ahora has movido el brazo con deslizadores, con comandos en la terminal
y con el ratón en RViz. Para que el robot haga **tareas** (coger una pieza,
dibujar, repetir un movimiento cien veces) hay que **programarlo**.

En este capítulo veremos tres programas de ejemplo, de más sencillo a más
completo. Todos funcionan **igual en modo mock, en Gazebo y con el robot
real**, porque hablan con ros2_control y MoveIt, no directamente con los motores.

| # | Programa | Lenguaje | Dificultad | Qué aprendes |
|---|---|---|---|---|
| 1 | `leer_estado.py` | Python | 🟢 | Crear un nodo y **escuchar** un topic |
| 2 | `mover_articulaciones.py` | Python | 🟡 | Enviar trayectorias con una **acción** |
| 3 | `moveit_ejemplo.cpp` | C++ | 🟡 | Usar **MoveIt** desde código: posturas, poses, líneas rectas y obstáculos |

---

## 2. Objetivos

Al terminar este capítulo serás capaz de:

- [ ] Explicar la estructura básica de un nodo de ROS 2 en Python.
- [ ] Distinguir cuándo usar un topic, un servicio o una acción.
- [ ] Leer el estado del brazo desde un programa.
- [ ] Enviar secuencias de posturas propias.
- [ ] Usar MoveIt desde C++ para mover la herramienta a un punto y en línea recta.

---

## 3. Conceptos clave

| Concepto | Explicación sencilla |
|---|---|
| **`rclpy` / `rclcpp`** | Las librerías de ROS 2 para Python y para C++ |
| **Nodo** | Tu programa dentro de ROS. Se crea heredando de `Node` |
| **Callback** | Función que ROS llama automáticamente cuando pasa algo (por ejemplo, llega un mensaje) |
| **`spin`** | "Quédate esperando y atiende los callbacks". Sin `spin`, los mensajes no se procesan |
| **Suscriptor** | Parte del nodo que escucha un topic |
| **Cliente de acción** | Parte del nodo que envía un objetivo largo y espera el resultado |
| **`MoveGroupInterface`** | Clase de MoveIt (C++) para planificar y mover un grupo de articulaciones |
| **Trayectoria cartesiana** | Movimiento en el que la **herramienta** va en línea recta (no las articulaciones) |

### Topic, servicio o acción: ¿cuál uso?

| Mecanismo | Cómo funciona | Analogía | Ejemplo en el proyecto |
|---|---|---|---|
| **Topic** | Flujo continuo; nadie responde | La radio: emite aunque nadie escuche | `/joint_states` |
| **Servicio** | Pregunta → respuesta rápida | Preguntar la hora | `/controller_manager/list_controllers` |
| **Acción** | Objetivo largo, con progreso y resultado final; se puede cancelar | Pedir comida a domicilio y seguir el pedido | `/brazo_controller/follow_joint_trajectory` |

---

## 4. Estructura

### Archivos del paquete `brazo_examples`

```
src/brazo_examples/
├── scripts/
│   ├── leer_estado.py             ← ejemplo 1 (Python)
│   └── mover_articulaciones.py    ← ejemplo 2 (Python)
├── src/
│   └── moveit_ejemplo.cpp         ← ejemplo 3 (C++)
├── launch/
│   └── moveit_ejemplo.launch.py   ← lanza el ejemplo 3 con la configuración de MoveIt
├── CMakeLists.txt                 ← compila el C++ e instala los scripts
└── package.xml
```

### Esqueleto de cualquier nodo en Python

Todos los programas de Python de ROS 2 siguen este patrón:

```python
import rclpy                      # librería de ROS 2 para Python
from rclpy.node import Node


class MiNodo(Node):               # 1) Tu nodo es una clase que hereda de Node
    def __init__(self):
        super().__init__('mi_nodo')          # 2) Nombre con el que aparece en "ros2 node list"
        # 3) Aquí creas suscriptores, publicadores, clientes, temporizadores…


def main():
    rclpy.init()                  # 4) Arranca ROS 2
    nodo = MiNodo()
    rclpy.spin(nodo)              # 5) Atiende los callbacks hasta que pulses Ctrl + C
    nodo.destroy_node()
    rclpy.shutdown()              # 6) Apaga ROS 2 limpiamente


if __name__ == '__main__':
    main()
```

---

## 5. Proceso detallado

Antes de cada ejemplo, el robot tiene que estar funcionando. Elige un modo:

```bash
# Opción sencilla (sin física) para los ejemplos 1 y 2:
ros2 launch brazo_bringup brazo.launch.py

# Para el ejemplo 3 (necesita MoveIt):
ros2 launch brazo_moveit_config demo.launch.py
```

### Ejemplo 1 · Leer el estado — [`leer_estado.py`](../../src/brazo_examples/scripts/leer_estado.py)

**Qué hace:** escucha `/joint_states` y muestra los ángulos **en grados**, dos
veces por segundo.

```bash
ros2 run brazo_examples leer_estado.py
# [INFO] [lector_estado]: joint_1=   0.00°  joint_2=  23.10°  joint_3=  45.80° ...
```

**Las líneas clave:**

```python
# Suscribirse: tipo de mensaje, nombre del topic, función a llamar, tamaño de la cola
self.create_subscription(JointState, '/joint_states', self.al_recibir, 10)

def al_recibir(self, msg: JointState):       # ROS llama a esta función con cada mensaje
    texto = '  '.join(
        f'{nombre}={math.degrees(pos):7.2f}°'  # convierte radianes a grados
        for nombre, pos in zip(msg.name, msg.position))
    self.get_logger().info(texto)            # lo muestra en la terminal
```

> 💡 `/joint_states` llega unas 100 veces por segundo. El programa usa un
> contador para mostrar solo 1 de cada 50 mensajes y no llenar la pantalla.

### Ejemplo 2 · Secuencia de posturas — [`mover_articulaciones.py`](../../src/brazo_examples/scripts/mover_articulaciones.py)

**Qué hace:** envía al `brazo_controller` una trayectoria de 4 posturas y
espera a que termine.

```bash
ros2 run brazo_examples mover_articulaciones.py
```

**Las líneas clave:**

```python
# 1) La lista de posturas: (ángulos en GRADOS, instante en SEGUNDOS)
SECUENCIA = [
    ([0, 0, 0, 0, 0, 0], 3.0),           # a los 3 s: vertical
    ([45, 20, 60, 0, 60, 0], 7.0),       # a los 7 s: inclinado a la izquierda
    ([-45, 20, 60, 0, 60, 90], 12.0),    # a los 12 s: a la derecha, girando la brida
    ([0, 0, 0, 0, 0, 0], 16.0),          # a los 16 s: vuelta a vertical
]

# 2) Crear el cliente de la acción del controlador
self.cliente = ActionClient(self, FollowJointTrajectory,
                            '/brazo_controller/follow_joint_trajectory')

# 3) Convertir cada postura en un punto de la trayectoria
punto.positions = [math.radians(g) for g in grados]   # ROS trabaja en radianes
punto.time_from_start = segundos_a_duration(t)

# 4) Enviar y esperar el resultado
futuro = self.cliente.send_goal_async(objetivo)
```

> ⚠️ Este método **no comprueba choques**: el brazo va en línea recta *en
> ángulos* de una postura a otra. Con el robot real, revisa bien las posturas
> o usa MoveIt (ejemplo 3).

### Ejemplo 3 · MoveIt desde C++ — [`moveit_ejemplo.cpp`](../../src/brazo_examples/src/moveit_ejemplo.cpp)

**Qué hace**, en orden:

1. Añade una **caja** (obstáculo) delante del robot.
2. Va a la postura con nombre **`listo`**.
3. Mueve la herramienta **10 cm a la izquierda** (objetivo en pose).
4. Dibuja un **cuadrado de 8 cm en línea recta** (trayectoria cartesiana).
5. Vuelve a **`home`** y quita la caja.

```bash
# Terminal 1
ros2 launch brazo_moveit_config demo.launch.py
# Terminal 2
ros2 launch brazo_examples moveit_ejemplo.launch.py
```

**Las líneas clave:**

```cpp
// Conectar con MoveIt para el grupo "brazo" (definido en el SRDF)
MoveGroupInterface brazo(nodo, "brazo");
brazo.setMaxVelocityScalingFactor(0.3);           // 30 % de la velocidad máxima

// a) Ir a una postura con nombre
brazo.setNamedTarget("listo");
planificarYEjecutar(brazo, log);

// b) Ir a una pose: la actual desplazada 10 cm en Y
geometry_msgs::msg::Pose objetivo = brazo.getCurrentPose().pose;
objetivo.position.y += 0.10;
brazo.setPoseTarget(objetivo);
planificarYEjecutar(brazo, log);

// c) Línea recta por una lista de puntos
double fraccion = brazo.computeCartesianPath(puntos, 0.005, 0.0, trayectoria);
//                                             │       │      └─ (salto máx., 0 = sin comprobar)
//                                             │       └─ un punto intermedio cada 5 mm
//                                             └─ lista de poses por las que pasar
if (fraccion > 0.95) brazo.execute(trayectoria);  // solo si se pudo planificar casi entera
```

> 🧩 **¿Por qué hay un archivo launch para este ejemplo?** El programa
> necesita conocer el robot (URDF, SRDF y cinemática), igual que MoveIt. El
> launch se los pasa como parámetros.

---

## 6. Ejemplos para practicar

1. **Tu propia coreografía.** Copia `mover_articulaciones.py` como
   `mi_coreografia.py`, cambia `SECUENCIA` y añádelo a `CMakeLists.txt`
   (en `install(PROGRAMS …)`). Recompila con
   `colcon build --packages-select brazo_examples`.
2. **Avisar de posturas peligrosas.** Modifica `leer_estado.py` para que muestre
   un aviso (`self.get_logger().warn(...)`) si `joint_2` supera los 60°.
3. **Otra figura.** En `moveit_ejemplo.cpp`, cambia el cuadrado por un
   triángulo (3 puntos) y recompila.
4. **Un nuevo obstáculo.** Añade una segunda caja en el ejemplo 3, en otra
   posición, y observa cómo cambia la ruta.

### Ideas para seguir aprendiendo

- Añadir una **pinza** al modelo y un controlador para abrirla y cerrarla.
- **Teleoperar** el brazo con un mando de consola usando *MoveIt Servo*.
- Añadir una **cámara** y detectar objetos para cogerlos (*pick and place*).
- Crear una **interfaz web** con `rosbridge` para manejar el brazo desde el móvil.

---

## 7. Cierre

### Resumen

- Un nodo de ROS 2 es una clase que hereda de `Node` y se mantiene vivo con `spin`.
- **Topics** para datos continuos, **servicios** para preguntas rápidas,
  **acciones** para tareas largas como mover el brazo.
- Para moverlo sin MoveIt se envía una trayectoria a `brazo_controller`.
- Con **MoveIt** puedes pedir posturas con nombre, poses y líneas rectas evitando obstáculos.
- Los mismos programas funcionan en mock, Gazebo y con el robot real.

### Comprueba que lo tienes

- [ ] Has ejecutado los tres ejemplos.
- [ ] Has creado al menos una secuencia de posturas propia.
- [ ] Sabes explicar la diferencia entre topic, servicio y acción.

### Si algo falla

| Síntoma | Causa probable | Solución |
|---|---|---|
| `No executable found` | El paquete no está compilado o no está en `CMakeLists.txt` | Revisa `install(PROGRAMS …)` y recompila |
| `No hay servidor. ¿Está lanzado brazo_controller?` | El robot no está en marcha | Lanza primero `brazo.launch.py`, Gazebo o la demo de MoveIt |
| El ejemplo 3 se queda esperando | `move_group` no está en marcha | Lanza `demo.launch.py` antes |
| `Trayectoria incompleta: no se ejecuta` | La línea recta pasa por una zona inalcanzable | Haz la figura más pequeña o cambia la postura inicial |
| Un script de Python no se ejecuta | Le falta el permiso de ejecución | `chmod +x src/brazo_examples/scripts/*.py` |

---

🎉 **¡Has terminado la guía!** Ya tienes un brazo de 6 GDL que se visualiza,
se simula, planifica sus movimientos y está listo para moverse en el mundo real.

⬅️ [Capítulo 10](../10_hardware_paso_a_paso/README.md) · 🏠 [Volver al inicio](../../README.md)
