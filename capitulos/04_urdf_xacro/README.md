# Capítulo 04 · Describir un robot con URDF y Xacro

| Nivel | Tiempo aproximado | Necesitas |
|---|---|---|
| 🟢 Principiante → 🟡 Medio | 1-2 horas | Capítulos 01 a 03 y el workspace compilado |

---

## 1. Introducción

Para que el ordenador pueda dibujar, simular o mover un robot, primero tiene
que **saber cómo es**: cuántas piezas tiene, cuánto mide cada una, cómo están
unidas y hacia dónde puede girar cada articulación.

Esa "ficha técnica" se escribe en un archivo de texto con formato **URDF**.
Es como describir un brazo humano por escrito:

> *"El hombro está unido al tronco y gira hacia delante y hacia atrás. Del
> hombro sale el brazo, de 30 cm. Al final del brazo está el codo, que dobla
> hasta 140°…"*

En este capítulo aprenderás a escribir URDF con **tres ejemplos que van de
menos a más**, antes de pasar al brazo de 6 articulaciones del proyecto.

---

## 2. Objetivos

Al terminar este capítulo serás capaz de:

- [ ] Explicar qué es un eslabón (*link*) y una articulación (*joint*).
- [ ] Leer y modificar un archivo URDF sencillo.
- [ ] Entender las unidades y los ejes que usa ROS.
- [ ] Usar **xacro** para evitar repetir código (variables, macros, argumentos).
- [ ] Visualizar y validar cualquier modelo.

---

## 3. Conceptos clave

| Concepto | Explicación sencilla | En un brazo humano |
|---|---|---|
| **URDF** | Archivo de texto (formato XML) que describe un robot | El plano del cuerpo |
| **Link (eslabón)** | Una pieza rígida del robot | El antebrazo |
| **Joint (articulación)** | La unión entre dos eslabones; dice cómo se mueve uno respecto al otro | El codo |
| **Padre / hijo** | Cada articulación une un eslabón *padre* (más cerca de la base) con un *hijo* | El brazo es padre del antebrazo |
| **`visual`** | La forma que se **dibuja** | La piel |
| **`collision`** | La forma que se usa para **detectar choques** (suele ser más simple) | Una "funda" alrededor |
| **`inertial`** | Masa e inercia: cuánto pesa y cómo se reparte el peso | El peso del antebrazo |
| **`origin`** | Dónde está y cómo está girado algo respecto a su referencia | "5 cm por encima del codo" |
| **`axis`** | Eje alrededor del cual gira la articulación | El codo gira en un solo eje |
| **`limit`** | Ángulo mínimo/máximo, velocidad y fuerza máximas | El codo no dobla hacia atrás |
| **XML** | Formato de texto con etiquetas `<así>...</así>` | Como el HTML de las páginas web |
| **Xacro** | "URDF con superpoderes": variables, fórmulas y plantillas | Una plantilla rellenable |

### Tipos de articulación

| Tipo | Movimiento | Ejemplo |
|---|---|---|
| `fixed` | Ninguno, unión rígida | Una pieza atornillada |
| `revolute` | Gira, con límites | El codo (lo más habitual en brazos) |
| `continuous` | Gira sin límite | Una rueda |
| `prismatic` | Se desliza en línea recta | El eje de una CNC, una pinza |

### Unidades y ejes (¡muy importante!)

| Magnitud | Unidad en ROS | Ejemplo |
|---|---|---|
| Distancia | **metros** | 30 cm → `0.3` |
| Ángulo | **radianes** | 90° → `1.5708` (π/2) · 180° → `3.1416` (π) |
| Masa | kilogramos | 500 g → `0.5` |

> 🔢 **De grados a radianes:** `radianes = grados × π / 180`. Con xacro
> puedes escribir directamente `${90 * pi / 180}`.

Ejes de coordenadas (regla de colores que verás en RViz):

```
        Z (azul) ↑ arriba
                 │
                 │
                 └──────→ Y (verde) izquierda
                /
               ↙ X (rojo) hacia delante
```

`rpy` significa *roll, pitch, yaw*: giro alrededor de X, de Y y de Z (en radianes).

---

## 4. Estructura

### Archivos del capítulo

```
capitulos/04_urdf_xacro/
├── README.md
└── ejemplos/
    ├── 01_un_eslabon.urdf          ← nivel 1: una pieza fija
    ├── 02_brazo_2gdl.urdf          ← nivel 2: brazo de 2 articulaciones, en URDF "a mano"
    └── 03_brazo_2gdl.urdf.xacro    ← nivel 3: el mismo brazo, pero con xacro
```

### Esqueleto de cualquier URDF

```xml
<robot name="mi_robot">            <!-- todo va dentro de <robot> -->

  <link name="pieza_A"> ... </link>  <!-- una pieza -->
  <link name="pieza_B"> ... </link>  <!-- otra pieza -->

  <joint name="union" type="revolute">   <!-- la unión entre ambas -->
    <parent link="pieza_A"/>
    <child  link="pieza_B"/>
    ...
  </joint>

</robot>
```

### Árbol de piezas del ejemplo 02

```
world ──(fixed)── base_link ──(joint_1)── eslabon_1 ──(joint_2)── eslabon_2 ──(fixed)── punta
```

Un robot siempre es un **árbol**: cada pieza tiene un solo padre y no puede
haber "círculos".

---

## 5. Proceso detallado

Vamos a recorrer los tres ejemplos. Ábrelos con un editor de texto (por
ejemplo **VS Code**, `gedit` o `nano`) mientras lees: **cada línea está comentada**.

### Nivel 1 · Una pieza fija — [`01_un_eslabon.urdf`](ejemplos/01_un_eslabon.urdf)

Lo mínimo para entender la estructura. Un eslabón vacío llamado `world` (el
suelo, la referencia) y un cilindro gris pegado a él.

```xml
<link name="base">
  <visual>
    <origin xyz="0 0 0.05" rpy="0 0 0"/>         <!-- subimos el cilindro 5 cm -->
    <geometry>
      <cylinder radius="0.1" length="0.1"/>        <!-- 10 cm de radio, 10 cm de alto -->
    </geometry>
  </visual>
</link>
```

> 🤔 **¿Por qué subirlo 5 cm?** La geometría se dibuja centrada en su origen.
> Un cilindro de 10 cm de alto quedaría medio enterrado; al subirlo la mitad
> de su altura, se apoya justo sobre el suelo.

### Nivel 2 · Brazo de 2 articulaciones — [`02_brazo_2gdl.urdf`](ejemplos/02_brazo_2gdl.urdf)

Aquí aparecen las articulaciones que se mueven:

```xml
<joint name="joint_1" type="revolute">
  <parent link="base_link"/>
  <child  link="eslabon_1"/>
  <origin xyz="0 0 0.05" rpy="0 0 0"/>     <!-- la articulación está en lo alto de la base -->
  <axis xyz="0 1 0"/>                       <!-- gira alrededor del eje Y (verde) -->
  <limit lower="-1.57" upper="1.57"         <!-- de -90° a +90° -->
         velocity="1.0" effort="10"/>       <!-- 1 rad/s máx., 10 N·m máx. -->
</joint>
```

**Regla práctica para colocar piezas:** cada eslabón "crece" hacia arriba desde
su articulación, y la siguiente articulación se coloca **en la punta** del
eslabón anterior. Por eso `joint_2` tiene `origin xyz="0 0 0.3"`: el eslabón 1
mide 0,3 m.

Fíjate en lo **repetitivo** que es este archivo: los dos eslabones son casi
iguales y hay que copiar las medidas en varios sitios (visual, collision,
inertial…). Si cambias la longitud, tienes que acordarte de cambiarla en 4
lugares. Eso lo resuelve xacro.

### Nivel 3 · El mismo brazo con xacro — [`03_brazo_2gdl.urdf.xacro`](ejemplos/03_brazo_2gdl.urdf.xacro)

Xacro añade cinco herramientas al URDF:

```xml
<!-- 1) VARIABLES: se definen una vez y se usan donde quieras -->
<xacro:property name="largo_1" value="0.30"/>

<!-- 2) FÓRMULAS: entre ${ } puedes hacer cuentas -->
<origin xyz="0 0 ${largo_1 / 2}"/>

<!-- 3) MACROS: "plantillas" que generan bloques repetidos -->
<xacro:macro name="barra" params="nombre largo color">
  <link name="${nombre}"> ... </link>
</xacro:macro>
<xacro:barra nombre="eslabon_1" largo="${largo_1}" color="naranja"/>   <!-- usar la plantilla -->

<!-- 4) ARGUMENTOS: valores que se pasan desde fuera al generar el URDF -->
<xacro:arg name="color_brazo" default="naranja"/>

<!-- 5) CONDICIONALES: incluir un bloque solo si se cumple algo -->
<xacro:if value="${largo_2 > 0.1}"> ... </xacro:if>
```

Con la macro `barra`, añadir un eslabón nuevo es **una sola línea**, y la
inercia se calcula sola con su fórmula.

### ¿Cómo se convierte un xacro en URDF?

Xacro es un "preprocesador": lee el `.xacro`, sustituye variables y macros, y
escribe un URDF normal. Puedes verlo en la terminal:

```bash
cd ~/rollbot_ws/capitulos/04_urdf_xacro/ejemplos
xacro 03_brazo_2gdl.urdf.xacro                         # muestra el URDF generado
xacro 03_brazo_2gdl.urdf.xacro color_brazo:=azul       # pasando un argumento
xacro 03_brazo_2gdl.urdf.xacro > /tmp/prueba.urdf      # lo guarda en un archivo
```

### ¿Cómo compruebo que mi modelo está bien?

```bash
check_urdf /tmp/prueba.urdf
# Si está bien, muestra el árbol:
#   robot name is: brazo_2gdl_xacro
#   root Link: world has 1 child(ren)
#       child(1):  base_link
#           child(1):  eslabon_1
#               ...

urdf_to_graphviz /tmp/prueba.urdf    # crea un PDF con el diagrama del árbol
```

---

## 6. Ejemplos

Para ver cada modelo en 3D usamos el lanzador del paquete `brazo_description`,
indicándole la ruta del archivo con `modelo:=`:

```bash
cd ~/rollbot_ws/capitulos/04_urdf_xacro/ejemplos

# Nivel 1
ros2 launch brazo_description visualizar.launch.py modelo:=$(pwd)/01_un_eslabon.urdf

# Nivel 2 (aparecerá una ventana con deslizadores para mover las articulaciones)
ros2 launch brazo_description visualizar.launch.py modelo:=$(pwd)/02_brazo_2gdl.urdf

# Nivel 3
ros2 launch brazo_description visualizar.launch.py modelo:=$(pwd)/03_brazo_2gdl.urdf.xacro
```

> `$(pwd)` se sustituye por la carpeta actual, así le damos la ruta completa al archivo.

### Ejercicios para practicar

1. **Cambia el eje.** En `02_brazo_2gdl.urdf`, cambia el `axis` de `joint_1`
   a `0 0 1`. Vuelve a lanzar. ¿Qué movimiento hace ahora?
2. **Cambia una medida.** En el ejemplo 03, pon `largo_1` a `0.5`. ¿En cuántos
   sitios has tenido que cambiarlo? (Respuesta: en uno.)
3. **Añade un eslabón.** En el ejemplo 03, añade un `joint_3` y un `eslabon_3`
   usando las macros `bisagra` y `barra`. Pista: el nuevo `joint_3` tiene como
   padre `eslabon_2` y su `z` es `${largo_2}`. Recuerda mover la `punta` al
   nuevo eslabón.
4. **Rompe algo a propósito.** Escribe mal el nombre de un `parent` y ejecuta
   `check_urdf`. Lee el mensaje de error: así aprenderás a reconocerlo.

---

## 7. Cierre

### Resumen

- Un robot se describe como un **árbol** de **eslabones** unidos por **articulaciones**.
- Cada eslabón puede tener forma **visual**, de **colisión** e **inercia**.
- En ROS todo va en **metros** y **radianes**; X rojo, Y verde, Z azul.
- **Xacro** evita repetir código con variables, fórmulas, macros y argumentos.
- `xacro` genera el URDF, `check_urdf` lo valida y el launch lo muestra en RViz.

### Comprueba que lo tienes

- [ ] Has visualizado los tres ejemplos.
- [ ] Sabes explicar qué hacen `parent`, `child`, `origin`, `axis` y `limit`.
- [ ] Has hecho al menos el ejercicio 3.

### Si algo falla

| Síntoma | Causa probable | Solución |
|---|---|---|
| `Error: link 'xxx' is not unique` | Dos eslabones con el mismo nombre | Cambia uno de los nombres |
| `Failed to find root link` / más de una raíz | Algún eslabón no está unido a nada | Revisa que cada eslabón (menos `world`) sea `child` de un joint |
| Error de XML ("not well-formed") | Falta cerrar una etiqueta o una comilla | Revisa la línea que indica el error |
| La pieza aparece enterrada o flotando | `origin` mal colocado | Recuerda que la geometría se dibuja centrada |

➡️ **Siguiente capítulo:** [05 · Visualización en RViz](../05_visualizacion_rviz/README.md),
donde veremos qué ocurre "por dentro" cuando mueves los deslizadores.
