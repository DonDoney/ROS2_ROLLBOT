# Capítulo 05 · Ver el robot en RViz y entender las transformadas (TF)

| Nivel | Tiempo aproximado | Necesitas |
|---|---|---|
| 🟢 Principiante | 45 minutos | Capítulo [04](../04_urdf_xacro/README.md) |

---

## 1. Introducción

En el capítulo anterior moviste un brazo con deslizadores y lo viste moverse en
3D. Parece magia, pero detrás hay **tres programas** trabajando en equipo y
pasándose información.

Entender este "trabajo en equipo" es una de las cosas más importantes de ROS,
porque **todo el proyecto funciona igual**: cuando más adelante el brazo se
mueva de verdad, solo cambiaremos *quién* dice los ángulos; lo demás seguirá
siendo idéntico.

En este capítulo cargaremos el brazo de 6 articulaciones del proyecto,
veremos qué programas se ejecutan y aprenderemos a "espiar" lo que se dicen.

---

## 2. Objetivos

Al terminar este capítulo serás capaz de:

- [ ] Lanzar el brazo de 6 GDL en RViz y moverlo con deslizadores.
- [ ] Explicar qué hacen `joint_state_publisher_gui`, `robot_state_publisher` y RViz.
- [ ] Entender qué son las **TF** (transformadas) y para qué sirven.
- [ ] Consultar desde la terminal la posición exacta de cualquier pieza.
- [ ] Configurar los paneles básicos de RViz.

---

## 3. Conceptos clave

| Concepto | Explicación sencilla | Analogía |
|---|---|---|
| **GDL** | Grados de libertad: cuántos movimientos independientes tiene el robot. Nuestro brazo tiene 6 | Tu brazo tiene 7 (hombro 3, codo 1, muñeca 3) |
| **`/joint_states`** | Topic donde se publica el ángulo de cada articulación | La lista de "cuánto está doblada cada articulación" |
| **Cinemática directa** | Calcular dónde está cada pieza a partir de los ángulos | Si sé cuánto doblo hombro y codo, sé dónde está mi mano |
| **TF (transformada)** | Posición y orientación de una pieza respecto a otra | "La mano está 60 cm delante del hombro y girada 20°" |
| **Frame (marco)** | Sistema de ejes X-Y-Z pegado a cada pieza | Unas flechas de colores pegadas a cada pieza |
| **Fixed Frame** | El marco que RViz considera "quieto" (aquí, `world`) | El suelo de la habitación |
| **Launch file** | Archivo Python que arranca varios nodos a la vez con su configuración | Un mando que enciende todos los aparatos de golpe |

---

## 4. Estructura

### Archivos implicados

```
src/brazo_description/
├── launch/visualizar.launch.py   ← arranca los 3 nodos (ábrelo: está comentado)
├── rviz/brazo.rviz               ← configuración guardada de RViz (vista, paneles)
└── urdf/brazo.urdf.xacro         ← el modelo del brazo (lo veremos a fondo en el cap. 06)
```

### Cómo fluye la información

```
   brazo.urdf.xacro
        │ (xacro lo convierte en URDF al lanzar)
        ▼
 ┌─────────────────────────┐   /robot_description    ┌──────────┐
 │  robot_state_publisher  │ ──────────────────────▶ │          │
 │  "Calculo dónde está    │   (el modelo completo)   │   RViz   │
 │   cada pieza"           │                          │  "Dibujo │
 │                         │   /tf  (posiciones)      │   todo"  │
 │                         │ ──────────────────────▶ │          │
 └───────────▲─────────────┘                          └──────────┘
             │ /joint_states  (los 6 ángulos)
 ┌───────────┴─────────────┐
 │ joint_state_publisher   │
 │ _gui                    │
 │ "Leo los deslizadores"  │
 └─────────────────────────┘
```

Paso a paso:

1. **`joint_state_publisher_gui`** muestra una ventana con 6 deslizadores y
   publica sus valores en `/joint_states`.
2. **`robot_state_publisher`** tiene el modelo del robot. Cada vez que recibe
   nuevos ángulos, calcula dónde queda cada pieza (cinemática directa) y lo
   publica en `/tf`.
3. **RViz** dibuja cada pieza en la posición que indica `/tf`.

> 🔁 **Lo que cambiará más adelante:** en lugar de los deslizadores, será
> **ros2_control** quien publique `/joint_states` con los ángulos reales del
> simulador o de los motores. Los pasos 2 y 3 no cambian nunca.

---

## 5. Proceso detallado

### Paso 1 · Lanzar el brazo

```bash
source ~/rollbot_ws/install/setup.bash        # por si la terminal no lo tiene activado
ros2 launch brazo_description visualizar.launch.py
```

Se abren dos ventanas: **RViz** con el brazo naranja y gris, y la ventana de
**deslizadores** (*Joint State Publisher*).

### Paso 2 · Mover el brazo

Mueve cada deslizador y observa qué articulación se mueve:

| Deslizador | Movimiento |
|---|---|
| `joint_1` | Gira toda la torre (como girar la cintura) |
| `joint_2` | Hombro: inclina el brazo adelante/atrás |
| `joint_3` | Codo |
| `joint_4` | Gira el antebrazo sobre sí mismo |
| `joint_5` | Dobla la muñeca |
| `joint_6` | Gira la brida (donde iría la herramienta) |

El botón **Center** pone todo a cero (brazo vertical) y **Randomize** elige
una postura al azar.

### Paso 3 · Conocer los paneles de RViz

En el panel izquierdo (**Displays**) hay tres elementos:

| Display | Para qué sirve | Prueba esto |
|---|---|---|
| **Grid** | Rejilla del suelo; cada cuadro mide 10 cm | Cuenta cuántos cuadros mide el brazo |
| **RobotModel** | El modelo 3D | Despliégalo y activa *Collision Enabled* para ver las formas de choque |
| **TF** | Los ejes de cada pieza | Despliégalo y activa *Show Names* para ver el nombre de cada marco |

Para mover la cámara: **clic izquierdo** gira, **rueda** hace zoom, **clic
central** (o Mayúsculas + clic) desplaza.

Si cambias la vista y quieres conservarla, pulsa **Ctrl + S**.

### Paso 4 · Espiar desde la terminal

Deja RViz abierto y abre **otra terminal**:

```bash
# ¿Qué programas están funcionando?
ros2 node list
#   /joint_state_publisher_gui
#   /robot_state_publisher
#   /rviz2

# ¿Qué ángulos se están publicando? (en radianes)
ros2 topic echo /joint_states
#   name: [joint_1, joint_2, ...]
#   position: [0.0, 0.52, ...]
# Pulsa Ctrl + C para parar

# ¿Dónde está la punta del brazo (tool0) respecto al suelo (world)?
ros2 run tf2_ros tf2_echo world tool0
#   Translation: [x, y, z]   ← posición en metros
#   Rotation: ...            ← orientación

# Dibujo de qué nodo habla con cuál
rqt_graph
```

---

## 6. Ejemplos

### Ejemplo 1 · Comprobar la altura del brazo

1. Pulsa **Center** en la ventana de deslizadores (todo a 0).
2. Ejecuta `ros2 run tf2_ros tf2_echo world tool0`.
3. La traslación en Z debe ser **1,05 m**. ¿De dónde sale?

```
   0,05  base
 + 0,15  eslabón 1 (torre)
 + 0,35  eslabón 2 (brazo)
 + 0,30  eslabón 3 (antebrazo)
 + 0,10  eslabón 4 (muñeca)
 + 0,08  eslabón 5
 + 0,02  eslabón 6 (brida)
 ──────
   1,05 m
```

### Ejemplo 2 · Ver el árbol completo de piezas

```bash
cd /tmp
ros2 run tf2_tools view_frames
# Crea un archivo frames_XXXX.pdf. Ábrelo:
xdg-open frames_*.pdf
```

Verás `world → base_link → link_1 → … → link_6 → tool0`.

### Ejemplo 3 · Arrancar sin deslizadores

```bash
ros2 launch brazo_description visualizar.launch.py gui:=false
```

El brazo aparecerá quieto en posición cero. Es útil cuando solo quieres ver el
modelo.

---

## 7. Cierre

### Resumen

- Para ver un robot trabajan juntos tres nodos: quien publica los ángulos,
  `robot_state_publisher` (que calcula las TF) y RViz (que dibuja).
- Las **TF** dicen dónde está cada pieza respecto a otra.
- Con `ros2 topic echo` y `tf2_echo` puedes ver los datos en la terminal.
- Más adelante solo cambiará *quién* publica `/joint_states`.

### Comprueba que lo tienes

- [ ] Has movido las 6 articulaciones con los deslizadores.
- [ ] Has obtenido Z = 1,05 m con `tf2_echo` en posición cero.
- [ ] Sabes activar *Show Names* en el display TF.

### Si algo falla

| Síntoma | Causa probable | Solución |
|---|---|---|
| El robot no aparece y RobotModel está en rojo | RViz no recibe el modelo | Comprueba que *Description Topic* es `/robot_description` |
| Aviso "No transform from link_X to world" | Falta algún ángulo en `/joint_states` | Comprueba que la ventana de deslizadores está abierta |
| `Package 'brazo_description' not found` | Workspace sin activar | `source ~/rollbot_ws/install/setup.bash` |
| Fixed Frame en rojo | Marco fijo incorrecto | Escribe `world` en *Global Options → Fixed Frame* |

➡️ **Siguiente capítulo:** [06 · Descripción del brazo de 6 GDL](../06_descripcion_brazo/README.md),
donde abriremos el modelo del brazo y veremos cómo adaptarlo a tu diseño.
