# Capítulo 02 · El workspace: la carpeta de trabajo del proyecto

| Nivel | Tiempo aproximado | Necesitas |
|---|---|---|
| 🟢 Principiante | 20-40 minutos | Haber terminado el [capítulo 01](../01_instalacion_ros2/README.md) |

---

## 1. Introducción

Ya tenemos ROS 2 instalado. Ahora necesitamos un sitio donde guardar **nuestro
propio código**: el modelo del brazo, la configuración de los motores, los
programas de ejemplo… Ese sitio se llama **workspace** (espacio de trabajo).

Piensa en el workspace como el **taller** del proyecto:

- En una estantería guardas las **piezas** (el código fuente).
- Cuando "compilas", se **montan** esas piezas y el resultado queda listo para usar.

Este repositorio ya está organizado como un workspace, así que solo hay que
descargarlo, instalar lo que necesita y compilarlo.

---

## 2. Objetivos

Al terminar este capítulo serás capaz de:

- [ ] Explicar qué es un workspace y qué es un paquete de ROS 2.
- [ ] Reconocer las carpetas `src`, `build`, `install` y `log`.
- [ ] Instalar dependencias con `rosdep` y compilar con `colcon`.
- [ ] Activar el workspace con `source` y comprobar que ROS encuentra nuestros paquetes.

---

## 3. Conceptos clave

| Concepto | Explicación sencilla | Analogía |
|---|---|---|
| **Workspace** | Carpeta que contiene todos los paquetes de tu proyecto | El taller |
| **Paquete** | Unidad mínima de código en ROS 2. Cada uno tiene una tarea concreta | Una caja con las piezas de un mueble |
| **`package.xml`** | Ficha del paquete: nombre, versión, autor y **de qué otros paquetes depende** | La etiqueta de la caja |
| **`CMakeLists.txt`** | Instrucciones de montaje: qué se compila y qué se copia | El manual de instrucciones |
| **Dependencia** | Otro paquete que el tuyo necesita para funcionar | El mueble necesita tornillos que vienen en otra caja |
| **`rosdep`** | Lee las dependencias de cada `package.xml` y las instala con `apt` | Un ayudante que va a comprar los tornillos que faltan |
| **`colcon`** | La herramienta que compila (monta) todos los paquetes | El montador |
| **Compilar** | Convertir el código en programas listos para ejecutar | Montar el mueble |
| **Underlay / overlay** | ROS instalado (`/opt/ros/jazzy`) es la base; tu workspace va **encima** | La planta baja y el primer piso |

---

## 4. Estructura

### Las cuatro carpetas de un workspace

```
rollbot_ws/
├── src/        ← TU código. Es lo único que se guarda en GitHub
├── build/      ← archivos temporales de la compilación      (se crea al compilar)
├── install/    ← resultado final: lo que ROS ejecuta        (se crea al compilar)
└── log/        ← registros de cada compilación              (se crea al compilar)
```

`build/`, `install/` y `log/` **no se suben a GitHub** (están en el archivo
[`.gitignore`](../../.gitignore)) porque se pueden regenerar en cualquier
momento y ocupan mucho.

### Lo que hay en este repositorio

```
rollbot_ws/                       (= el repositorio ROS2_ROLLBOT)
├── capitulos/                    ← la guía que estás leyendo
├── firmware/                     ← código del microcontrolador (no es ROS)
└── src/                          ← los 6 paquetes de ROS 2
    ├── brazo_description/            cómo ES el robot (forma, medidas)
    ├── brazo_bringup/                cómo se ARRANCA el robot
    ├── brazo_gazebo/                 cómo se SIMULA con física
    ├── brazo_moveit_config/          cómo se PLANIFICAN sus movimientos
    ├── brazo_hardware/               cómo se HABLA con los motores reales
    └── brazo_examples/               programas de EJEMPLO
```

> 💡 **¿Por qué tantos paquetes y no uno solo?** Porque así cada uno hace una
> única cosa y se puede reutilizar. Por ejemplo, si un día quieres usar el
> modelo del brazo en otro proyecto, te llevas solo `brazo_description` sin
> arrastrar todo lo demás. Es la forma en que lo hacen los fabricantes
> profesionales de robots.

### Por dentro de un paquete

```
brazo_description/
├── package.xml       ← ficha: nombre y dependencias
├── CMakeLists.txt    ← instrucciones de compilación/instalación
├── urdf/             ← archivos del modelo del robot
├── launch/           ← "lanzadores": arrancan varios programas a la vez
├── rviz/             ← configuración del visor 3D
└── meshes/           ← piezas 3D (STL) cuando tengas el diseño CAD
```

---

## 5. Proceso detallado

### Opción A · Script automático

```bash
cd ~/rollbot_ws/capitulos/02_workspace
chmod +x crear_workspace.sh
./crear_workspace.sh
```

El script [`crear_workspace.sh`](crear_workspace.sh) hace los mismos pasos que
la opción B. Si ya descargaste el repositorio en el capítulo 01, lo detecta y
solo lo actualiza.

### Opción B · Paso a paso

**Paso 1 · Descargar el proyecto** (sáltalo si ya lo hiciste en el capítulo 01).

```bash
git clone https://github.com/DonDoney/ROS2_ROLLBOT.git ~/rollbot_ws
#         └── dirección del repositorio ──────────┘  └── carpeta donde se guarda
```

**Paso 2 · Entrar en el workspace y activar ROS.**

```bash
cd ~/rollbot_ws                        # IMPORTANTE: compilar siempre desde la raíz del workspace
source /opt/ros/jazzy/setup.bash       # activa ROS (por si esta terminal no lo tiene)
```

**Paso 3 · Instalar las dependencias.** `rosdep` revisa los `package.xml` de
`src/` e instala lo que falte. La primera vez descarga bastantes cosas
(MoveIt, Gazebo, ros2_control…).

```bash
rosdep install --from-paths src --ignore-src -r -y
#              │                │            │  └─ -y: responde "sí" a todo
#              │                │            └──── -r: si algo falla, sigue con lo demás
#              │                └───────────────── no intentes instalar NUESTROS paquetes con apt
#              └────────────────────────────────── busca package.xml dentro de src/
```

**Paso 4 · Compilar.**

```bash
colcon build --symlink-install
#            └─ en vez de COPIAR los archivos (URDF, launch, Python), crea "accesos
#               directos". Así, si los editas, los cambios se ven sin recompilar.
```

Al terminar verás algo como `Summary: 6 packages finished`. Si sale algún
`stderr` en amarillo suelen ser avisos (*warnings*), no errores.

**Paso 5 · Activar el workspace.**

```bash
source install/setup.bash              # activa NUESTROS paquetes en esta terminal
```

Para no tener que hacerlo en cada terminal, añádelo al `.bashrc` (el script de
la opción A ya lo hace):

```bash
echo "source ~/rollbot_ws/install/setup.bash" >> ~/.bashrc
```

> ⚠️ **El orden importa:** primero se activa ROS (`/opt/ros/jazzy/setup.bash`)
> y después tu workspace. Es como construir el primer piso: necesita que
> exista la planta baja.

### Comandos de colcon que usarás a menudo

| Comando | Para qué sirve |
|---|---|
| `colcon build --symlink-install` | Compila todo |
| `colcon build --packages-select brazo_hardware` | Compila solo un paquete (más rápido) |
| `colcon build --packages-up-to brazo_bringup` | Compila un paquete y los que necesita |
| `rm -rf build install log` | Borra todo lo compilado para empezar de cero |

---

## 6. Ejemplos

### Ejemplo 1 · ¿Encuentra ROS nuestros paquetes?

```bash
ros2 pkg list | grep brazo
#             └─ "grep" filtra y muestra solo las líneas que contienen "brazo"
```

Deben aparecer los seis:

```
brazo_bringup
brazo_description
brazo_examples
brazo_gazebo
brazo_hardware
brazo_moveit_config
```

### Ejemplo 2 · ¿Dónde está instalado un paquete?

```bash
ros2 pkg prefix brazo_description
# → /home/tu_usuario/rollbot_ws/install/brazo_description
```

### Ejemplo 3 · Crear tu propio paquete (para practicar)

```bash
cd ~/rollbot_ws/src
ros2 pkg create mi_primer_paquete --build-type ament_python --dependencies rclpy
#               └─ nombre          └─ paquete de Python        └─ necesita la librería de ROS para Python
cd ~/rollbot_ws
colcon build --packages-select mi_primer_paquete
```

Abre la carpeta `src/mi_primer_paquete` y mira qué archivos ha creado. Cuando
termines de curiosear, puedes borrarla.

---

## 7. Cierre

### Resumen

- El **workspace** es la carpeta del proyecto; el código va en `src/`.
- Cada **paquete** tiene un `package.xml` (ficha) y un `CMakeLists.txt` (instrucciones).
- `rosdep install` instala dependencias, `colcon build` compila y `source install/setup.bash` activa.
- Este proyecto tiene 6 paquetes, cada uno con una responsabilidad clara.

### Comprueba que lo tienes

- [ ] `colcon build` termina sin errores.
- [ ] `ros2 pkg list | grep brazo` muestra los 6 paquetes.

### Si algo falla

| Síntoma | Causa probable | Solución |
|---|---|---|
| `Package 'brazo_description' not found` | No has activado el workspace | `source ~/rollbot_ws/install/setup.bash` |
| `colcon: command not found` | Falta `ros-dev-tools` | `sudo apt install ros-dev-tools` |
| Error de compilación "Could not find a package configuration file…" | Falta una dependencia | Repite el paso 3 (`rosdep install …`) |
| Se crean `build/` e `install/` dentro de `src/` | Compilaste desde la carpeta equivocada | Bórralas y compila desde `~/rollbot_ws` |

➡️ **Siguiente capítulo:** [03 · Herramientas](../03_herramientas/README.md),
donde conocerás RViz, Gazebo, ros2_control y MoveIt.
