# Capítulo 04 · Describir un robot: URDF y Xacro

> **Objetivo:** aprender a escribir el modelo de un robot con ejemplos
> progresivos, antes de abordar el brazo de 6 GDL.

## ¿Qué es el URDF?

**URDF** (*Unified Robot Description Format*) es un archivo XML que describe:

- **Eslabones** (`<link>`): las piezas rígidas. Cada uno puede tener
  `visual` (cómo se dibuja), `collision` (forma para choques) e `inertial`
  (masa e inercia, para simular la física).
- **Articulaciones** (`<joint>`): cómo se une un eslabón *hijo* a uno *padre*
  y cómo se mueve.

| Tipo de joint | Movimiento |
|---|---|
| `fixed` | Ninguno (unión rígida) |
| `revolute` | Giro con límites (lo normal en un brazo) |
| `continuous` | Giro sin límites (ruedas) |
| `prismatic` | Desplazamiento lineal (ejes de CNC, pinzas) |

### Marcos de referencia y unidades

- Distancias en **metros**, ángulos en **radianes**, masas en **kg**.
- `origin xyz="x y z" rpy="roll pitch yaw"`: posición y orientación de un
  elemento respecto al marco de su eslabón (o del padre, en un joint).
- Convención de colores en RViz: **X = rojo, Y = verde, Z = azul**.

## Ejemplos de este capítulo

Todos se visualizan con el launch del paquete `brazo_description` pasando la
ruta del archivo (tras compilar el workspace del capítulo 02):

```bash
cd ~/rollbot_ws/capitulos/04_urdf_xacro/ejemplos
ros2 launch brazo_description visualizar.launch.py modelo:=$(pwd)/01_un_eslabon.urdf
```

| Archivo | Qué aprendes |
|---|---|
| [`01_un_eslabon.urdf`](ejemplos/01_un_eslabon.urdf) | Estructura mínima: link, visual, collision, joint fijo |
| [`02_brazo_2gdl.urdf`](ejemplos/02_brazo_2gdl.urdf) | Articulaciones `revolute`, ejes, límites, inercias |
| [`03_brazo_2gdl.urdf.xacro`](ejemplos/03_brazo_2gdl.urdf.xacro) | El mismo brazo con **xacro**: propiedades, macros, argumentos y condicionales |

## Xacro en 1 minuto

```xml
<xacro:property name="largo" value="0.3"/>               <!-- variable -->
<origin xyz="0 0 ${largo / 2}"/>                           <!-- expresión -->

<xacro:macro name="barra" params="nombre largo">           <!-- "función" -->
  <link name="${nombre}"> ... </link>
</xacro:macro>
<xacro:barra nombre="eslabon_1" largo="0.3"/>              <!-- llamada -->

<xacro:arg name="modo" default="mock"/>                    <!-- argumento externo -->
<xacro:if value="${'$(arg modo)' == 'gazebo'}"> ... </xacro:if>

<xacro:include filename="$(find brazo_description)/urdf/inercias.xacro"/>
```

## Comprobar un modelo

```bash
# 1) Generar el URDF a partir del xacro
xacro 03_brazo_2gdl.urdf.xacro > /tmp/prueba.urdf

# 2) Validarlo y ver el árbol de eslabones
check_urdf /tmp/prueba.urdf

# 3) Diagrama del árbol en PDF
urdf_to_graphviz /tmp/prueba.urdf    # genera brazo_2gdl_xacro.pdf
```

## Ejercicios propuestos

1. En `02_brazo_2gdl.urdf`, cambia el eje de `joint_1` a `0 0 1`. ¿Qué pasa?
2. En el ejemplo 03, añade un tercer eslabón con una sola línea de cada macro.
3. Lanza el ejemplo 03 con `color_brazo:=azul` (pista: el launch acepta solo
   la ruta, prueba primero con `xacro ... color_brazo:=azul > /tmp/a.urdf`).

⬅️ [Capítulo 03](../03_herramientas/README.md) · ➡️ [Capítulo 05 · RViz](../05_visualizacion_rviz/README.md)
