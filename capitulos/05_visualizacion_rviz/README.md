# Capítulo 05 · Visualización en RViz y transformadas (TF)

> **Objetivo:** entender qué nodos hacen falta para "ver" un robot y cómo
> fluye la información entre ellos.

## Lanzar el brazo de 6 GDL

```bash
ros2 launch brazo_description visualizar.launch.py
```

Se abrirán RViz y una ventana con **seis deslizadores**, uno por articulación.
Al moverlos, el brazo se mueve en RViz.

## ¿Qué está pasando por dentro?

```
   brazo.urdf.xacro
        │  xacro (al lanzar)
        ▼
 ┌──────────────────────┐   /robot_description   ┌────────┐
 │ robot_state_publisher│ ─────────────────────▶ │  RViz  │
 │                      │   /tf, /tf_static      │        │
 │                      │ ─────────────────────▶ │        │
 └──────────▲───────────┘                        └────────┘
            │ /joint_states (ángulos)
 ┌──────────┴───────────┐
 │ joint_state_publisher│  ← deslizadores
 │        _gui          │
 └──────────────────────┘
```

1. **`joint_state_publisher_gui`** publica en `/joint_states` el ángulo de cada
   articulación (los valores de los deslizadores).
2. **`robot_state_publisher`** combina el URDF con esos ángulos y calcula la
   posición de cada eslabón (cinemática directa). La publica como **TF**.
3. **RViz** dibuja el modelo colocando cada eslabón donde dicen las TF.

Más adelante, `joint_state_publisher_gui` se sustituye por el
`joint_state_broadcaster` de ros2_control, que publica los ángulos **reales**
(del simulador o de los motores). El resto no cambia.

## Explorar con la terminal

Con el launch en marcha, en otra terminal:

```bash
ros2 node list                        # nodos activos
ros2 topic echo /joint_states         # ángulos (radianes)
ros2 run tf2_ros tf2_echo world tool0 # posición de la herramienta respecto al mundo
ros2 run tf2_tools view_frames        # genera frames_*.pdf con el árbol TF
rqt_graph                             # grafo visual de nodos y topics
```

> 🧪 Prueba: pon todos los deslizadores a 0 y ejecuta `tf2_echo world tool0`.
> La traslación en Z debe ser **1,05 m** = 0,05 (base) + 0,15 + 0,35 + 0,30 + 0,10 + 0,08 + 0,02.

## Paneles útiles de RViz

| Display | Para qué |
|---|---|
| **RobotModel** | El modelo 3D (activa *Collision Enabled* para ver las formas de colisión) |
| **TF** | Ejes de cada eslabón (activa *Show Names*) |
| **Grid** | Rejilla del suelo, cada celda 10 cm |

`Fixed Frame` debe ser `world`. Si cambias la vista y quieres conservarla,
guarda con **Ctrl+S** (se sobrescribe `brazo_description/rviz/brazo.rviz`).

## Problemas frecuentes

- **El robot sale blanco o no aparece** → mira el panel *Displays*: si
  RobotModel indica error, revisa que *Description Topic* sea `/robot_description`.
- **"No transform from X to world"** → falta algún valor en `/joint_states`
  (¿está en marcha el joint_state_publisher?).

⬅️ [Capítulo 04](../04_urdf_xacro/README.md) · ➡️ [Capítulo 06 · Brazo de 6 GDL](../06_descripcion_brazo/README.md)
