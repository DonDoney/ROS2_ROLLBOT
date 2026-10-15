# Capítulo 08 · Simulación con física en Gazebo Harmonic

| Nivel | Tiempo aproximado | Necesitas |
|---|---|---|
| 🟡 Medio | 1 hora | Capítulo [07](../07_ros2_control/README.md) y un PC con tarjeta gráfica decente |

---

## 1. Introducción

En el modo *mock* del capítulo anterior, el brazo obedecía al instante y sin
esfuerzo: no pesaba, no tenía inercia y podía atravesar cualquier cosa.

**Gazebo** es un simulador que añade **física**: gravedad, masa, rozamiento
y choques. Es como un videojuego muy realista donde puedes probar el brazo:

- sin arriesgarte a romper piezas ni motores,
- sin necesidad de tener el robot construido,
- y tantas veces como quieras.

Lo mejor es que usaremos **exactamente los mismos controladores** que en el
modo mock y que en el robot real. Solo cambia el "plugin" de hardware.

---

## 2. Objetivos

Al terminar este capítulo serás capaz de:

- [ ] Lanzar el brazo en Gazebo Harmonic junto con RViz.
- [ ] Explicar qué hace cada pieza del lanzador de simulación.
- [ ] Mover el brazo simulado con los mismos programas del capítulo 07.
- [ ] Entender el "tiempo simulado" y por qué hace falta `use_sim_time`.
- [ ] Modificar el mundo de la simulación (añadir objetos).

---

## 3. Conceptos clave

| Concepto | Explicación sencilla |
|---|---|
| **Gazebo Harmonic** | La versión de Gazebo emparejada con ROS 2 Jazzy. Se abre con `gz sim` |
| **Mundo (`.sdf`)** | Archivo que describe el escenario: suelo, luces, objetos, gravedad |
| **SDF** | Formato de Gazebo para describir mundos y modelos (parecido al URDF) |
| **Spawn / create** | "Hacer aparecer" el robot dentro del mundo simulado |
| **gz_ros2_control** | Plugin que pone un controller manager **dentro** de Gazebo |
| **Puente (bridge)** | Programa que traduce mensajes entre Gazebo y ROS 2 (hablan "idiomas" distintos) |
| **Tiempo simulado** | Gazebo lleva su propio reloj; puede ir más rápido o más lento que el real |
| **`use_sim_time`** | Parámetro que hace que un nodo use el reloj de Gazebo en vez del del PC |
| **Inercia** | Resistencia de una pieza a empezar o dejar de girar; depende de su masa y su forma |

---

## 4. Estructura

### Archivos implicados

```
src/brazo_gazebo/
├── launch/gazebo.launch.py     ← arranca la simulación completa (comentado línea a línea)
└── worlds/mundo_brazo.sdf      ← el escenario: suelo, luz y una mesa

src/brazo_description/urdf/
└── brazo.gazebo.xacro          ← plugin gz_ros2_control y colores para Gazebo

src/brazo_bringup/config/
└── brazo_controladores.yaml    ← los MISMOS controladores del capítulo 07
```

### Qué arranca el lanzador

```
gazebo.launch.py
 │
 ├─ 1. gz sim mundo_brazo.sdf             Abre Gazebo con el escenario
 ├─ 2. robot_state_publisher              Publica el modelo (generado con modo_hardware:=gazebo)
 ├─ 3. ros_gz_sim create                  Hace aparecer el brazo en Gazebo
 │        └─ el plugin gz_ros2_control arranca un controller manager dentro de Gazebo
 ├─ 4. ros_gz_bridge  /clock              Pasa el reloj de Gazebo a ROS
 ├─ 5. spawner joint_state_broadcaster    (espera a que el brazo exista)
 ├─ 6. spawner brazo_controller           (espera al anterior)
 └─ 7. RViz                               Opcional
```

### Mock frente a Gazebo

| | Modo mock (cap. 07) | Gazebo (este capítulo) |
|---|---|---|
| ¿Quién crea el controller manager? | `ros2_control_node` | El plugin `gz_ros2_control` |
| Plugin de hardware | `mock_components/GenericSystem` | `gz_ros2_control/GazeboSimSystem` |
| Física (gravedad, masa, choques) | ❌ | ✅ |
| Reloj | El del PC | El de la simulación (`use_sim_time: true`) |
| Controladores | `brazo_controladores.yaml` | El **mismo** archivo |

---

## 5. Proceso detallado

### Paso 1 · Lanzar la simulación

```bash
ros2 launch brazo_gazebo gazebo.launch.py
```

Tras unos segundos verás:

- **Gazebo**: el brazo de pie, una mesa marrón delante y el suelo gris.
- **RViz**: el mismo brazo, en la misma postura.

> ⏳ La primera vez Gazebo puede tardar en abrir mientras prepara los gráficos.

### Paso 2 · Comprobar que todo está listo

```bash
ros2 control list_controllers
# joint_state_broadcaster ... active
# brazo_controller        ... active
```

### Paso 3 · Mover el brazo simulado

Usa el mismo programa del capítulo 07:

```bash
ros2 run brazo_examples mover_articulaciones.py
```

El brazo se moverá en Gazebo y en RViz a la vez.

### Paso 4 · Entender cómo se activa Gazebo en el modelo

En [`brazo.urdf.xacro`](../../src/brazo_description/urdf/brazo.urdf.xacro) hay un
bloque condicional:

```xml
<!-- Solo si generamos el modelo con modo_hardware:=gazebo -->
<xacro:if value="${'$(arg modo_hardware)' == 'gazebo'}">
  <xacro:brazo_gazebo prefijo="$(arg prefijo)"
                      archivo_controladores="$(arg archivo_controladores)"/>
</xacro:if>
```

Y en [`brazo.gazebo.xacro`](../../src/brazo_description/urdf/brazo.gazebo.xacro) se
carga el plugin, indicándole el archivo de controladores:

```xml
<gazebo>
  <plugin filename="gz_ros2_control-system"
          name="gz_ros2_control::GazeboSimROS2ControlPlugin">
    <parameters>${archivo_controladores}</parameters>   <!-- el YAML del capítulo 07 -->
  </plugin>
</gazebo>
```

### Paso 5 · Sin RViz (más ligero)

Si tu PC va justo, lanza solo Gazebo:

```bash
ros2 launch brazo_gazebo gazebo.launch.py rviz:=false
```

---

## 6. Ejemplos

### Ejemplo 1 · Ver el reloj simulado

```bash
ros2 topic echo /clock --once
# clock:
#   sec: 42           ← segundos desde que empezó la simulación
#   nanosec: 120000000
```

Pausa la simulación con el botón ⏸️ de Gazebo y vuelve a ejecutarlo: el
tiempo no avanza. Por eso los nodos necesitan `use_sim_time: true`.

### Ejemplo 2 · Añadir un objeto al mundo

Abre [`mundo_brazo.sdf`](../../src/brazo_gazebo/worlds/mundo_brazo.sdf) y, justo
antes de `</world>`, añade una caja roja encima de la mesa:

```xml
<model name="caja_roja">
  <pose>0.45 0 0.25 0 0 0</pose>               <!-- x y z (m) y giros (rad) -->
  <link name="link">
    <inertial>                                  <!-- 100 g: Gazebo la dejará caer -->
      <mass>0.1</mass>
      <inertia>                                 <!-- inercia de un cubo de 5 cm -->
        <ixx>0.00004</ixx><iyy>0.00004</iyy><izz>0.00004</izz>
      </inertia>
    </inertial>
    <collision name="colision">
      <geometry><box><size>0.05 0.05 0.05</size></box></geometry>
    </collision>
    <visual name="visual">
      <geometry><box><size>0.05 0.05 0.05</size></box></geometry>
      <material><diffuse>1 0 0 1</diffuse></material>   <!-- rojo -->
    </visual>
  </link>
</model>
```

Vuelve a lanzar la simulación: la caja caerá sobre la mesa por la gravedad.

### Ejemplo 3 · Comprobar que la masa importa

En `brazo_propiedades.xacro`, multiplica por 10 la masa de `l3_masa` (de
`1.5` a `15.0`) y lanza la simulación. Ejecuta el programa de movimiento y
observa si el brazo sigue la trayectoria igual de bien. Después **deja el
valor original**.

---

## 7. Cierre

### Resumen

- Gazebo añade **física** a la simulación: masa, gravedad, inercia y choques.
- El plugin **gz_ros2_control** pone el controller manager dentro de Gazebo.
- Se usan **los mismos controladores** que en mock y en el robot real.
- Los nodos deben usar el **tiempo simulado** (`use_sim_time: true`).
- El escenario se define en un archivo `.sdf` que puedes editar.

### Comprueba que lo tienes

- [ ] El brazo aparece en Gazebo y en RViz.
- [ ] `mover_articulaciones.py` mueve el brazo simulado.
- [ ] Has añadido la caja roja al mundo.

### Si algo falla

| Síntoma | Causa probable | Solución |
|---|---|---|
| Gazebo se abre pero el brazo no aparece | Error al crear el robot | Busca el primer error en la terminal; comprueba `ros2 topic echo /robot_description --once` |
| `Could not contact service /controller_manager/...` | No cargó el plugin gz_ros2_control | `sudo apt install ros-jazzy-gz-ros2-control` y recompila |
| El brazo tiembla o "explota" | Masas o inercias poco realistas | Revisa `brazo_propiedades.xacro` |
| Gazebo muy lento o pantalla negra | Máquina virtual sin aceleración 3D | `export LIBGL_ALWAYS_SOFTWARE=1` o usa Ubuntu nativo |
| No se ven las piezas STL (cuando las añadas) | Gazebo no sabe dónde buscarlas | `export GZ_SIM_RESOURCE_PATH=$GZ_SIM_RESOURCE_PATH:$(ros2 pkg prefix brazo_description)/share` |

➡️ **Siguiente capítulo:** [09 · MoveIt 2](../09_moveit2/README.md), donde el
brazo aprenderá a llegar solo a un punto sin chocar.
