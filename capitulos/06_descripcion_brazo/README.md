# Capítulo 06 · Descripción del brazo ROLLBOT de 6 GDL

> **Objetivo:** conocer cómo está organizado el modelo del brazo en el paquete
> [`brazo_description`](../../src/brazo_description) y cómo adaptarlo a tu diseño.

## Cinemática

Brazo antropomórfico de 6 ejes rotativos. En la **posición cero** queda
completamente vertical.

| Articulación | Eje | Función | Límites | Motor previsto |
|---|---|---|---|---|
| `joint_1` | Z | Giro de la torre | ±170° | NEMA 34 + reductora |
| `joint_2` | Y | Hombro | ±90° | NEMA 34 + reductora |
| `joint_3` | Y | Codo | ±135° | NEMA 23/34 + reductora |
| `joint_4` | Z | Giro de muñeca | ±170° | NEMA 23 |
| `joint_5` | Y | Cabeceo de muñeca | ±115° | NEMA 23 |
| `joint_6` | Z | Brida | ±180° | NEMA 23 |

```
            tool0 ●            ← punto de la herramienta (eslabón vacío)
          link_6 ▬  joint_6 (Z)
          link_5 █  joint_5 (Y)
          link_4 ║  joint_4 (Z)
                 ║
          link_3 ║  (0,30 m)
                 ●  joint_3 (Y, codo)
          link_2 ║  (0,35 m)
                 ║
                 ●  joint_2 (Y, hombro)
          link_1 ▓  joint_1 (Z)
       base_link ▀▀▀
           world ─────────
```

Los ejes 4, 5 y 6 se cortan casi en un punto (muñeca esférica aproximada), lo
que facilita la cinemática inversa.

## Organización de los archivos

```
brazo_description/urdf/
├── brazo.urdf.xacro           ← ARCHIVO PRINCIPAL (el que se procesa)
├── brazo_propiedades.xacro    ← medidas, masas y límites (edita aquí)
├── brazo_macro.xacro          ← eslabones y articulaciones
├── inercias.xacro             ← macros de inercia (cilindro y caja)
├── materiales.xacro           ← colores de RViz
├── brazo.ros2_control.xacro   ← interfaz con ros2_control (cap. 07)
└── brazo.gazebo.xacro         ← plugin de Gazebo (cap. 08)
```

**¿Por qué tantos archivos?** Para que cada cambio se haga en un solo sitio:

- ¿Cambia una medida? → `brazo_propiedades.xacro`
- ¿Cambia la reductora de un eje? → `brazo.ros2_control.xacro`
- ¿Añades una pinza? → nueva macro colgando de `tool0`

### Argumentos del modelo

| Argumento | Valores | Efecto |
|---|---|---|
| `modo_hardware` | `mock` · `gazebo` · `real` | Qué plugin de hardware usa ros2_control |
| `puerto_serie` | `/dev/ttyUSB0`… | Puerto del ESP32 (modo real) |
| `baudios` | `115200`… | Velocidad del puerto (modo real) |
| `archivo_controladores` | ruta | YAML de controladores (modo gazebo) |
| `prefijo` | texto | Antepone un prefijo a todos los nombres (útil con dos brazos) |

## Probarlo

```bash
ros2 launch brazo_description visualizar.launch.py

# Generar y validar el URDF
xacro $(ros2 pkg prefix brazo_description)/share/brazo_description/urdf/brazo.urdf.xacro > /tmp/brazo.urdf
check_urdf /tmp/brazo.urdf
```

## Adaptarlo a tu brazo real

1. Mide en tu CAD la **distancia entre ejes** consecutivos y cámbiala en
   `brazo_propiedades.xacro` (`l1_altura`, `l2_largo`, `l3_largo`…).
2. Pon las **masas reales** (el CAD las calcula con el material asignado).
3. Ajusta los **límites** a los topes mecánicos reales, dejando unos grados de margen.
4. Sustituye cilindros y cajas por tus **mallas STL** (ver [`meshes/README.md`](../../src/brazo_description/meshes/README.md)).
5. Si tu cinemática es distinta (p. ej. hombro desplazado del eje de la torre),
   cambia los `origin` de los joints en `brazo_macro.xacro`.

> 💡 Regla de oro: el **origen de cada eslabón está en el eje de su
> articulación**. Exporta las mallas desde el CAD con ese origen y te
> ahorrarás muchos dolores de cabeza.

⬅️ [Capítulo 05](../05_visualizacion_rviz/README.md) · ➡️ [Capítulo 07 · ros2_control](../07_ros2_control/README.md)
