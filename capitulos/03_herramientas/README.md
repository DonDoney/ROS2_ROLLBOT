# Capítulo 03 · Las herramientas: RViz, ros2_control, Gazebo y MoveIt

| Nivel | Tiempo aproximado | Necesitas |
|---|---|---|
| 🟢 Principiante | 30-45 minutos | Capítulos [01](../01_instalacion_ros2/README.md) y [02](../02_workspace/README.md) |

---

## 1. Introducción

ROS 2 por sí solo es la "base". Para un brazo robótico necesitamos además
varias **herramientas especializadas**, cada una con un trabajo distinto:

- Una para **ver** el robot en 3D.
- Otra para **controlar** sus motores.
- Otra para **simularlo** con física, sin tener el robot de verdad.
- Y otra para **planificar** movimientos inteligentes que eviten chocar.

En este capítulo las instalamos, entendemos para qué sirve cada una y las
probamos por separado. En los capítulos siguientes las iremos usando con
nuestro brazo.

---

## 2. Objetivos

Al terminar este capítulo serás capaz de:

- [ ] Instalar todas las herramientas del proyecto con un solo script.
- [ ] Explicar qué hace cada herramienta y en qué capítulo la usaremos.
- [ ] Distinguir entre Gazebo "clásico" y Gazebo Harmonic.
- [ ] Usar los comandos básicos de diagnóstico de ROS 2.

---

## 3. Conceptos clave

| Herramienta | ¿Qué hace? | Analogía | Capítulo |
|---|---|---|---|
| **xacro** | Permite escribir el modelo del robot con variables y "plantillas" | Una plantilla de Word con campos que se rellenan | 04, 06 |
| **RViz2** | Visor 3D: muestra el robot, sus ejes y lo que planea hacer | Una cámara que ve "dentro de la cabeza" del robot | 05 |
| **ros2_control** | Capa que conecta los controladores con los motores (reales o simulados) | Un enchufe universal: el aparato funciona con cualquier toma | 07 |
| **Gazebo Harmonic** | Simulador con física: gravedad, peso, choques | Un videojuego realista donde probar el robot sin romper nada | 08 |
| **MoveIt 2** | Calcula cómo mover cada articulación para llegar a un punto sin chocar | El GPS del brazo | 09 |
| **pyserial** | Librería de Python para hablar por el puerto USB | El cable de teléfono entre el PC y la placa | 10 |

### Gazebo: ¡ojo con las versiones!

Existen dos "Gazebo" y en Internet verás tutoriales de ambos:

| | Gazebo Classic (antiguo) | **Gazebo Harmonic** (el nuestro) |
|---|---|---|
| Comando | `gazebo` | `gz sim` |
| Estado | Sin soporte, **no funciona con Jazzy** | Actual, emparejado con Jazzy |
| Paquetes ROS | `gazebo_ros`, `gazebo_ros2_control` | `ros_gz_sim`, `gz_ros2_control` |
| Crear robot | `spawn_entity.py` | `create` |

> 💡 Si un tutorial usa `gazebo_ros` o `spawn_entity.py`, es de la versión
> antigua: no te servirá tal cual.

---

## 4. Estructura

### Archivos del capítulo

```
capitulos/03_herramientas/
├── README.md                     ← esta guía
└── instalar_herramientas.sh      ← instala todas las herramientas
```

### Cómo encajan las herramientas entre sí

```
                ┌────────────────────┐
                │      MoveIt 2      │   "¿Cómo llego a ese punto sin chocar?"
                └─────────┬──────────┘
                          │  envía una trayectoria (lista de posiciones en el tiempo)
                ┌─────────▼──────────┐
                │    ros2_control    │   "Muevo cada articulación según la trayectoria"
                └─────────┬──────────┘
          ┌───────────────┼────────────────┐
   ┌──────▼─────┐  ┌──────▼──────┐  ┌──────▼───────┐
   │    mock    │  │   Gazebo    │  │ Robot real   │   ← elegimos UNO
   │ (de mentira)│  │ (simulado) │  │ ESP32 + NEMA │
   └────────────┘  └─────────────┘  └──────────────┘

   RViz2 observa todo lo anterior y lo dibuja en 3D
```

La gran ventaja: **MoveIt y ros2_control no cambian** aunque cambiemos de
"mock" a Gazebo o al robot real. Lo que pruebas en simulación funciona igual
en el robot.

---

## 5. Proceso detallado

### Paso 1 · Ejecutar el script de instalación

```bash
cd ~/rollbot_ws/capitulos/03_herramientas
chmod +x instalar_herramientas.sh
./instalar_herramientas.sh
```

El script instala estos grupos de paquetes (puedes abrirlo para ver cada línea comentada):

| Grupo | Paquetes principales |
|---|---|
| Modelado y visualización | `ros-jazzy-xacro`, `ros-jazzy-joint-state-publisher-gui`, `ros-jazzy-rviz2`, `liburdfdom-tools` |
| Control | `ros-jazzy-ros2-control`, `ros-jazzy-ros2-controllers` |
| Simulación | `ros-jazzy-ros-gz` (instala Gazebo Harmonic), `ros-jazzy-gz-ros2-control` |
| Planificación | `ros-jazzy-moveit` |
| Hardware | `python3-serial` + permiso para usar el puerto USB |

### Paso 2 · Cerrar sesión y volver a entrar

El script te añade al grupo `dialout`, que da permiso para usar el puerto USB
(`/dev/ttyUSB0`). Ese cambio **solo se aplica al volver a iniciar sesión**.

### Paso 3 · Recompilar el workspace

Ahora que están todas las herramientas, compila de nuevo para asegurarte de
que todo encaja:

```bash
cd ~/rollbot_ws
colcon build --symlink-install
source install/setup.bash
```

---

## 6. Ejemplos

Prueba cada herramienta por separado. Cierra cada una con `Ctrl + C` en la
terminal (o cerrando su ventana) antes de pasar a la siguiente.

### Ejemplo 1 · RViz (visor 3D)

```bash
rviz2
```

Se abrirá una ventana con una rejilla negra. Aún está vacía: en el capítulo 05
le pondremos el brazo.

### Ejemplo 2 · Gazebo Harmonic (simulador)

```bash
gz sim shapes.sdf
#       └─ un mundo de ejemplo que viene con Gazebo (un cubo, una esfera, un cilindro)
```

Pulsa el botón ▶️ abajo a la izquierda para que empiece la física.

### Ejemplo 3 · ¿Están los controladores de ros2_control?

```bash
ros2 pkg list | grep controller
# Debes ver, entre otros: joint_state_broadcaster, joint_trajectory_controller
```

### Ejemplo 4 · ¿Está MoveIt?

```bash
ros2 pkg list | grep moveit_ros_move_group
# Si aparece, MoveIt está instalado
```

### Chuleta de comandos de diagnóstico

Guárdala: la usarás en todos los capítulos.

| Comando | ¿Qué muestra? |
|---|---|
| `ros2 node list` | Los nodos (programas) que están funcionando |
| `ros2 topic list` | Los topics (canales) activos |
| `ros2 topic echo /joint_states` | Los mensajes que pasan por un topic |
| `rqt_graph` | Un dibujo de qué nodos hablan con cuáles |
| `ros2 run tf2_tools view_frames` | Un PDF con el "árbol" de piezas del robot |
| `ros2 control list_controllers` | Los controladores cargados y si están activos |

---

## 7. Cierre

### Resumen

- **xacro** escribe el modelo, **RViz** lo muestra, **ros2_control** mueve las
  articulaciones, **Gazebo** simula la física y **MoveIt** planifica.
- Usamos **Gazebo Harmonic** (`gz sim`), no el Gazebo clásico.
- El mismo sistema de control sirve para simulación y para el robot real.

### Comprueba que lo tienes

- [ ] `rviz2` abre una ventana.
- [ ] `gz sim shapes.sdf` abre el simulador.
- [ ] `groups` muestra `dialout` entre tus grupos (tras volver a iniciar sesión).

### Si algo falla

| Síntoma | Causa probable | Solución |
|---|---|---|
| Gazebo va muy lento o se ve negro | Estás en máquina virtual sin aceleración gráfica | Prueba `export LIBGL_ALWAYS_SOFTWARE=1` o usa Ubuntu nativo |
| `Unable to locate package ros-jazzy-...` | Lista de paquetes desactualizada | `sudo apt update` y repite el script |
| `groups` no muestra `dialout` | No has vuelto a iniciar sesión | Cierra sesión o reinicia el PC |

➡️ **Siguiente capítulo:** [04 · URDF y Xacro](../04_urdf_xacro/README.md),
donde aprenderás a "dibujar" un robot con texto.
