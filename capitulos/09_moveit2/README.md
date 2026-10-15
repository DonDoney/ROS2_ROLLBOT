# Capítulo 09 · Planificar movimientos con MoveIt 2

| Nivel | Tiempo aproximado | Necesitas |
|---|---|---|
| 🟡 Medio | 1-1,5 horas | Capítulos [07](../07_ros2_control/README.md) y [08](../08_simulacion_gazebo/README.md) |

---

## 1. Introducción

Hasta ahora le decíamos al brazo **qué ángulo** debía tener cada
articulación. Pero en la vida real lo que queremos es otra cosa:

> *"Pon la punta de la herramienta en este punto, mirando hacia abajo, y no
> choques con la mesa."*

Resolver eso a mano es muy difícil: hay que calcular 6 ángulos a la vez,
elegir entre varias soluciones posibles y comprobar que ninguna pieza choca
durante todo el recorrido.

**MoveIt 2** hace todo eso por nosotros. Es como el **GPS del brazo**: tú le
das el destino y él calcula la ruta, evitando obstáculos y respetando los
límites de cada articulación. Después envía esa ruta al `brazo_controller` que
ya conoces.

---

## 2. Objetivos

Al terminar este capítulo serás capaz de:

- [ ] Explicar qué es la cinemática inversa y qué hace un planificador.
- [ ] Usar MoveIt desde RViz: arrastrar el objetivo, planificar y ejecutar.
- [ ] Conocer los archivos de configuración de MoveIt y para qué sirve cada uno.
- [ ] Usar MoveIt con el modo mock, con Gazebo y (más adelante) con el robot real.
- [ ] Ajustar la velocidad de los movimientos.

---

## 3. Conceptos clave

| Concepto | Explicación sencilla | Analogía |
|---|---|---|
| **Cinemática directa** | De ángulos → a posición de la mano (lo hacía `robot_state_publisher`) | "Si doblo así el codo, ¿dónde queda mi mano?" |
| **Cinemática inversa (IK)** | De posición de la mano → a ángulos | "Para tocar la mesa, ¿cuánto doblo el codo?" |
| **Pose** | Posición (x, y, z) **más** orientación de la herramienta | Dónde está la mano y hacia dónde apunta |
| **Planificador (OMPL)** | Busca una ruta sin choques entre la postura actual y el objetivo | El GPS buscando una ruta |
| **Grupo de planificación** | Conjunto de articulaciones que se mueven juntas (aquí: `brazo`, las 6) | "El brazo derecho" |
| **SRDF** | Archivo que complementa al URDF con información para MoveIt: grupos, posturas con nombre, choques a ignorar | Las instrucciones de uso del robot |
| **Escena de planificación** | El "mundo" que MoveIt tiene en cuenta: el robot y los obstáculos | El mapa del GPS |
| **move_group** | El nodo principal de MoveIt: recibe peticiones, planifica y ejecuta | El GPS en sí |
| **Escalado de velocidad** | Porcentaje de la velocidad máxima que se usa (por defecto, 10 %) | Conducir más despacio en el primer viaje |

---

## 4. Estructura

### Cómo encaja MoveIt

```
   Tú (RViz o un programa)
        │  "lleva la herramienta a este punto"
        ▼
 ┌───────────────────────── move_group ─────────────────────────┐
 │  1. Cinemática inversa (KDL): ¿qué ángulos hacen falta?       │
 │  2. Planificador (OMPL): ¿qué ruta evita los obstáculos?      │
 │  3. Perfil de velocidad: ¿cómo acelero y freno suavemente?    │
 └───────────────────────────────┬───────────────────────────────┘
                                 │ trayectoria (acción FollowJointTrajectory)
                                 ▼
                      brazo_controller (ros2_control, cap. 07)
                                 │
                       mock · Gazebo · robot real
```

### Archivos del paquete `brazo_moveit_config`

```
src/brazo_moveit_config/
├── config/
│   ├── brazo.srdf               ← grupo "brazo", posturas con nombre, choques a ignorar
│   ├── kinematics.yaml          ← qué solucionador de IK usar (KDL)
│   ├── joint_limits.yaml        ← velocidades, ACELERACIONES y escala por defecto (10 %)
│   ├── moveit_controllers.yaml  ← a qué controlador enviar las trayectorias
│   └── moveit.rviz              ← RViz con el panel de MoveIt
└── launch/
    ├── demo.launch.py           ← TODO en uno, con hardware mock (para aprender)
    └── move_group.launch.py     ← SOLO MoveIt (para Gazebo o el robot real)
```

### Las posturas con nombre (del SRDF)

| Nombre | Descripción |
|---|---|
| `home` | Brazo completamente vertical (todo a 0) |
| `listo` | Brazo inclinado hacia delante, herramienta mirando hacia abajo |
| `plegado` | Brazo recogido, para transportarlo |

Así se define una en [`brazo.srdf`](../../src/brazo_moveit_config/config/brazo.srdf):

```xml
<group_state name="listo" group="brazo">
  <joint name="joint_1" value="0"/>
  <joint name="joint_2" value="0.4"/>    <!-- ángulos en radianes -->
  <joint name="joint_3" value="1.3"/>
  <joint name="joint_4" value="0"/>
  <joint name="joint_5" value="1.2"/>
  <joint name="joint_6" value="0"/>
</group_state>
```

---

## 5. Proceso detallado

### Paso 1 · Lanzar la demo

```bash
ros2 launch brazo_moveit_config demo.launch.py
```

Se abre RViz con un panel nuevo, **MotionPlanning**, abajo a la izquierda.

### Paso 2 · Planificar arrastrando el objetivo

1. En la pestaña **Planning**, comprueba que *Planning Group* es `brazo`.
2. En la punta del brazo verás una **bola azul con flechas** (marcador
   interactivo). Arrástrala: aparecerá un brazo **naranja** que muestra la
   postura objetivo.
   - Las **flechas** mueven la herramienta en línea recta.
   - Los **anillos** la giran.
3. Pulsa **Plan**. MoveIt calcula la ruta y la reproduce como una animación.
4. Si te gusta, pulsa **Execute**: el brazo la ejecuta a través de `brazo_controller`.

> 💡 Si el brazo naranja se pone **rojo**, esa postura choca o no se puede
> alcanzar. Mueve el objetivo a otro sitio.

### Paso 3 · Usar una postura con nombre

En la pestaña **Planning**, en *Goal State*, elige `listo`, pulsa **Plan** y
luego **Execute**. Prueba también `plegado` y `home`.

### Paso 4 · Cambiar la velocidad

En la misma pestaña verás *Velocity Scaling* y *Acceleration Scaling*. Por
defecto están al **0,1 (10 %)**, configurado en
[`joint_limits.yaml`](../../src/brazo_moveit_config/config/joint_limits.yaml):

```yaml
default_velocity_scaling_factor: 0.1       # 10 % de la velocidad máxima
default_acceleration_scaling_factor: 0.1
```

Es bajo **a propósito**: cuando conectes el robot real, los primeros
movimientos deben ser lentos. En simulación puedes subirlo a 0,5 sin problema.

### Paso 5 · Usar MoveIt con Gazebo

MoveIt no necesita saber si el robot es real o simulado. Abre dos terminales:

```bash
# Terminal 1 · la simulación (sin su RViz, porque MoveIt abrirá el suyo)
ros2 launch brazo_gazebo gazebo.launch.py rviz:=false
```

```bash
# Terminal 2 · solo MoveIt, usando el reloj de la simulación
ros2 launch brazo_moveit_config move_group.launch.py use_sim_time:=true
```

### Resumen de modos

| Quiero… | Terminal 1 | Terminal 2 |
|---|---|---|
| Aprender MoveIt | `ros2 launch brazo_moveit_config demo.launch.py` | — |
| MoveIt + Gazebo | `ros2 launch brazo_gazebo gazebo.launch.py rviz:=false` | `ros2 launch brazo_moveit_config move_group.launch.py use_sim_time:=true` |
| MoveIt + robot real (cap. 10) | `ros2 launch brazo_bringup brazo.launch.py modo_hardware:=real rviz:=false` | `ros2 launch brazo_moveit_config move_group.launch.py` |

---

## 6. Ejemplos

### Ejemplo 1 · Añadir un obstáculo desde RViz

1. En el panel MotionPlanning, abre la pestaña **Scene Objects**.
2. Elige *Box*, pulsa el botón **+** y colócala entre el brazo y el objetivo.
3. Pulsa **Publish** (si aparece) y vuelve a **Plan**: la ruta rodeará la caja.

### Ejemplo 2 · Crear tu propia postura con nombre

1. Copia el bloque `listo` en `brazo.srdf`.
2. Cámbiale el nombre (por ejemplo `saludo`) y los ángulos.
3. Vuelve a lanzar la demo: aparecerá en *Goal State*.

### Ejemplo 3 · Programar MoveIt

El [capítulo 11](../11_programacion_ejemplos/README.md) incluye un programa en C++
que hace todo esto sin tocar RViz: va a `listo`, se desplaza 10 cm, dibuja un
cuadrado en línea recta y evita una caja.

### Para más adelante: el Setup Assistant

Esta configuración está escrita a mano para que cada archivo sea fácil de leer.
Si cambias mucho el robot (otra cinemática o una pinza), MoveIt tiene un
asistente gráfico que regenera la configuración:

```bash
ros2 launch moveit_setup_assistant setup_assistant.launch.py
```

Elige *Edit Existing MoveIt Configuration Package*, selecciona
`src/brazo_moveit_config`, ve a **Self-Collisions → Generate Collision Matrix**
y, al guardar, desmarca los archivos que no quieras que sobrescriba (como los launch).

---

## 7. Cierre

### Resumen

- **MoveIt** calcula los ángulos necesarios (cinemática inversa) y una ruta sin choques.
- El **SRDF** define el grupo `brazo` y las posturas `home`, `listo` y `plegado`.
- **`demo.launch.py`** sirve para aprender; **`move_group.launch.py`** se usa con Gazebo o el robot real.
- La velocidad empieza al **10 %** por seguridad.

### Comprueba que lo tienes

- [ ] Has planificado y ejecutado un movimiento arrastrando el marcador.
- [ ] Has usado las posturas `listo` y `plegado`.
- [ ] Has hecho que MoveIt esquive una caja.
- [ ] Has usado MoveIt con Gazebo.

### Si algo falla

| Síntoma | Causa probable | Solución |
|---|---|---|
| `No motion plan found` | Objetivo inalcanzable o dentro de un obstáculo | Acerca el objetivo al robot o quita el obstáculo |
| El plan sale pero **Execute** falla | El controlador no está activo | `ros2 control list_controllers` |
| Error "start state differs from current state" | El robot real va retrasado respecto al plan | Revisa velocidades; en Gazebo usa `use_sim_time:=true` |
| No aparece el marcador azul | Grupo no seleccionado | *Planning Group* → `brazo` |

➡️ **Siguiente capítulo:** [10 · Hardware real](../10_hardware_paso_a_paso/README.md),
donde pasaremos de la pantalla a los motores.
