# Capítulo 06 · El modelo del brazo ROLLBOT de 6 GDL

| Nivel | Tiempo aproximado | Necesitas |
|---|---|---|
| 🟡 Medio | 1 hora | Capítulos [04](../04_urdf_xacro/README.md) y [05](../05_visualizacion_rviz/README.md) |

---

## 1. Introducción

Ya sabes escribir un URDF sencillo y verlo en RViz. Ahora vamos a abrir el
modelo **real** del proyecto: un brazo de **6 articulaciones**, parecido a
los brazos industriales.

Este modelo está pensado para crecer contigo:

- Hoy usa **formas simples** (cilindros y cajas) para poder empezar ya.
- Cuando tengas el diseño en CAD, cambiarás medidas y piezas **en un solo archivo**.
- El mismo modelo sirve para **RViz, Gazebo, MoveIt y el robot real**.

En este capítulo recorreremos sus archivos, entenderemos por qué está
dividido así y aprenderemos a adaptarlo a tu brazo.

---

## 2. Objetivos

Al terminar este capítulo serás capaz de:

- [ ] Describir las 6 articulaciones del brazo y qué movimiento hace cada una.
- [ ] Saber en qué archivo cambiar cada cosa (medidas, límites, motores…).
- [ ] Entender los argumentos del modelo (`modo_hardware`, `puerto_serie`…).
- [ ] Generar y validar el URDF del brazo.
- [ ] Planificar cómo pasar de formas simples a tus piezas reales en STL.

---

## 3. Conceptos clave

| Concepto | Explicación sencilla |
|---|---|
| **Brazo antropomórfico** | Brazo con forma parecida al humano: torre, hombro, codo y muñeca |
| **Posición cero** | Postura del brazo cuando todos los ángulos valen 0. Aquí: **completamente vertical** |
| **Muñeca esférica** | Las 3 últimas articulaciones se cruzan casi en un punto. Facilita calcular los movimientos |
| **`tool0`** | Eslabón vacío en la punta del brazo. Es el "punto de trabajo" donde irá la herramienta |
| **Mallas (meshes)** | Archivos 3D (STL/DAE) exportados de tu CAD para que el modelo tenga la forma real |
| **Propiedad (property)** | Variable de xacro, por ejemplo `l2_largo = 0.35` |
| **Macro** | Plantilla de xacro que genera un bloque de URDF (por ejemplo, todo el brazo) |
| **Argumento (arg)** | Valor que se pasa al generar el URDF, por ejemplo `modo_hardware:=gazebo` |

---

## 4. Estructura

### Las 6 articulaciones

| Articulación | Gira alrededor de | Función | Límites | Motor previsto |
|---|---|---|---|---|
| `joint_1` | Z (vertical) | Gira toda la torre | ±170° | NEMA 34 + reductora |
| `joint_2` | Y | Hombro | ±90° | NEMA 34 + reductora |
| `joint_3` | Y | Codo | ±135° | NEMA 23/34 + reductora |
| `joint_4` | Z | Gira el antebrazo | ±170° | NEMA 23 |
| `joint_5` | Y | Dobla la muñeca | ±115° | NEMA 23 |
| `joint_6` | Z | Gira la brida | ±180° | NEMA 23 |

```
            tool0 ●            ← punto de la herramienta
          link_6 ▬  joint_6 (Z) · brida
          link_5 █  joint_5 (Y) · muñeca
          link_4 ║  joint_4 (Z) · giro antebrazo
                 ║
          link_3 ║  0,30 m · antebrazo
                 ●  joint_3 (Y) · codo
          link_2 ║  0,35 m · brazo
                 ║
                 ●  joint_2 (Y) · hombro
          link_1 ▓  joint_1 (Z) · torre
       base_link ▀▀▀
           world ───────── suelo
```

### Los archivos del modelo y qué cambiar en cada uno

```
src/brazo_description/urdf/
├── brazo.urdf.xacro           ← ARCHIVO PRINCIPAL: une todas las piezas
├── brazo_propiedades.xacro    ← ✏️ medidas, masas y límites   ← AQUÍ cambiarás casi todo
├── brazo_macro.xacro          ← eslabones y articulaciones (la "forma" del árbol)
├── inercias.xacro             ← fórmulas para calcular la inercia
├── materiales.xacro           ← colores para RViz
├── brazo.ros2_control.xacro   ← ✏️ motores: pasos por vuelta y reductoras (cap. 07 y 10)
└── brazo.gazebo.xacro         ← configuración para el simulador (cap. 08)
```

| Si quieres cambiar… | Edita… |
|---|---|
| La longitud de un eslabón | `brazo_propiedades.xacro` |
| El peso de una pieza | `brazo_propiedades.xacro` |
| Hasta dónde gira una articulación | `brazo_propiedades.xacro` |
| Los colores en RViz | `materiales.xacro` |
| La reductora o los pasos de un motor | `brazo.ros2_control.xacro` |
| La forma (pasar a piezas STL) | `brazo_macro.xacro` |
| Añadir una pinza | Nueva macro colgada de `tool0` |

> 💡 **¿Por qué dividirlo en tantos archivos?** Para que cada cambio se haga
> en **un solo sitio**. Si las medidas estuvieran repetidas por todo el
> modelo, tarde o temprano olvidarías cambiar alguna.

### Cómo se unen las piezas

```
brazo.urdf.xacro
 ├── include → brazo_macro.xacro
 │              ├── include → brazo_propiedades.xacro
 │              ├── include → inercias.xacro
 │              └── include → materiales.xacro
 ├── include → brazo.ros2_control.xacro
 ├── include → brazo.gazebo.xacro      (solo se usa si modo_hardware = gazebo)
 │
 ├── <link name="world"/>              ← el suelo
 ├── xacro:brazo_rollbot               ← "dibuja" el brazo encima del suelo
 └── xacro:brazo_ros2_control          ← conecta con los motores
```

---

## 5. Proceso detallado

### Paso 1 · Leer las propiedades

Abre [`brazo_propiedades.xacro`](../../src/brazo_description/urdf/brazo_propiedades.xacro).
Verás bloques como este:

```xml
<!-- ===================== ESLABÓN 2: brazo (hombro, eje Y) ===================== -->
<xacro:property name="l2_ancho"     value="0.06"/>   <!-- 6 cm de ancho -->
<xacro:property name="l2_largo"     value="0.35"/>   <!-- 35 cm de largo -->
<xacro:property name="l2_masa"      value="2.5"/>    <!-- 2,5 kg -->
```

Y los límites, escritos en grados para que sean fáciles de leer:

```xml
<xacro:property name="j2_min" value="${ -90 * pi / 180}"/>   <!-- -90° convertidos a radianes -->
<xacro:property name="j2_max" value="${  90 * pi / 180}"/>   <!-- +90° -->
```

### Paso 2 · Ver cómo se usa una propiedad

En [`brazo_macro.xacro`](../../src/brazo_description/urdf/brazo_macro.xacro),
la articulación del codo se coloca **en la punta del brazo**, usando la
propiedad `l2_largo`:

```xml
<joint name="${prefijo}joint_3" type="revolute">
  <parent link="${prefijo}link_2"/>
  <child  link="${prefijo}link_3"/>
  <origin xyz="0 0 ${l2_largo}" rpy="0 0 0"/>     <!-- 0,35 m por encima del hombro -->
  <axis xyz="0 1 0"/>                              <!-- gira alrededor de Y -->
  <limit lower="${j3_min}" upper="${j3_max}" velocity="${vel_ejes_grandes}" effort="${esfuerzo_grande}"/>
</joint>
```

Si cambias `l2_largo` a `0.40`, el eslabón se alarga **y** el codo se mueve
solo a la nueva punta.

> `${prefijo}` está vacío normalmente. Sirve por si algún día montas dos
> brazos: uno con `izq_` y otro con `der_`, para que los nombres no se repitan.

### Paso 3 · Conocer los argumentos

El modelo acepta argumentos al generarse. Los launch de los siguientes
capítulos ya los pasan por ti, pero conviene saber que existen:

| Argumento | Valores | Para qué |
|---|---|---|
| `modo_hardware` | `mock` · `gazebo` · `real` | Quién mueve las articulaciones (cap. 07) |
| `puerto_serie` | `/dev/ttyUSB0`… | Puerto USB de la placa (modo real) |
| `baudios` | `115200`… | Velocidad de comunicación (modo real) |
| `archivo_controladores` | ruta a un YAML | Configuración de controladores (modo gazebo) |
| `prefijo` | texto | Antepone un texto a todos los nombres |

### Paso 4 · Generar y validar el URDF

```bash
# Ruta del archivo principal dentro del workspace instalado
XACRO=$(ros2 pkg prefix brazo_description)/share/brazo_description/urdf/brazo.urdf.xacro

xacro $XACRO > /tmp/brazo.urdf                       # modo por defecto (mock)
check_urdf /tmp/brazo.urdf                            # valida y muestra el árbol

xacro $XACRO modo_hardware:=real > /tmp/brazo_real.urdf   # con otro argumento
grep -A3 "<hardware>" /tmp/brazo_real.urdf                # ¿qué plugin usa?
```

### Paso 5 · Adaptarlo a tu brazo real

Sigue este orden cuando tengas el diseño mecánico:

1. **Medidas.** Mide en tu CAD la distancia **entre ejes de giro**
   consecutivos y ponla en `brazo_propiedades.xacro` (`l1_altura`, `l2_largo`, `l3_largo`…).
2. **Masas.** Tu CAD calcula el peso de cada pieza si le asignas el material.
3. **Límites.** Pon los topes mecánicos reales **con unos grados de margen**.
4. **Velocidades.** Calcula la velocidad máxima de cada eje:
   `velocidad (rad/s) = rpm_motor ÷ reducción × 2π ÷ 60`.
5. **Formas reales.** Sustituye cilindros y cajas por tus STL (ver
   [`meshes/README.md`](../../src/brazo_description/meshes/README.md)).
6. **Cinemática distinta.** Si tu hombro no está justo encima de la torre
   (por ejemplo, desplazado hacia delante), cambia los `origin` de los joints
   en `brazo_macro.xacro`.

> 🏆 **Regla de oro:** el **origen de cada pieza** debe estar en el **eje de
> su articulación**. Exporta así las piezas desde el CAD y todo encajará a la
> primera.

---

## 6. Ejemplos

### Ejemplo 1 · Alargar el brazo

1. Abre `brazo_propiedades.xacro` y cambia `l2_largo` de `0.35` a `0.45`.
2. Como compilamos con `--symlink-install`, **no hace falta recompilar**.
3. Lanza y compara:

```bash
ros2 launch brazo_description visualizar.launch.py
```

4. Comprueba la nueva altura con `ros2 run tf2_ros tf2_echo world tool0`
   (debería ser 1,15 m). Después **deja el valor como estaba**.

### Ejemplo 2 · Limitar el codo

Cambia `j3_max` a `${ 45 * pi / 180}`. Al lanzar, el deslizador de `joint_3`
no pasará de 45°.

### Ejemplo 3 · Pasar una pieza a STL (cuando tengas el CAD)

En `brazo_macro.xacro`, dentro de `link_2`, sustituye la geometría del bloque `visual`:

```xml
<!-- Antes: una caja -->
<geometry><box size="${l2_ancho} ${l2_ancho} ${l2_largo}"/></geometry>

<!-- Después: tu pieza real (exportada en milímetros → escala 0.001) -->
<geometry>
  <mesh filename="package://brazo_description/meshes/visual/link_2.stl" scale="0.001 0.001 0.001"/>
</geometry>
```

Si cambias el `origin` del visual, recuerda que la pieza STL ya trae su propio
origen: normalmente pondrás `xyz="0 0 0"`.

---

## 7. Cierre

### Resumen

- El brazo tiene 6 articulaciones; en posición cero queda vertical y mide 1,05 m.
- Las **medidas, masas y límites** están todas en `brazo_propiedades.xacro`.
- Los **motores** (pasos y reductoras) se configuran en `brazo.ros2_control.xacro`.
- El argumento **`modo_hardware`** decide si el brazo es simulado o real.
- Para tu diseño real: medidas → masas → límites → STL.

### Comprueba que lo tienes

- [ ] Sabes qué archivo tocar para cambiar una longitud, un límite o un color.
- [ ] `check_urdf` valida el modelo sin errores.
- [ ] Has hecho el ejemplo 1 y has dejado el valor original.

### Si algo falla

| Síntoma | Causa probable | Solución |
|---|---|---|
| Un cambio no se nota | Compilaste sin `--symlink-install` | `colcon build --symlink-install` |
| `Undefined substitution argument` | Falta un argumento o tiene un error de escritura | Revisa los `xacro:arg` de `brazo.urdf.xacro` |
| La pieza STL no aparece | Ruta o escala incorrecta | Comprueba `package://...` y prueba `scale="0.001 0.001 0.001"` |
| Error `name 'xxx' is not defined` | Propiedad con un nombre mal escrito | Busca el nombre exacto en `brazo_propiedades.xacro` |

➡️ **Siguiente capítulo:** [07 · ros2_control](../07_ros2_control/README.md),
donde el brazo dejará de moverse con deslizadores y lo harán controladores de verdad.
