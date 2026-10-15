# Capítulo 01 · Instalación de ROS 2 Jazzy

| Nivel | Tiempo aproximado | Necesitas |
|---|---|---|
| 🟢 Principiante | 30-60 minutos | Un PC con Ubuntu 24.04 e Internet |

---

## 1. Introducción

Antes de mover un brazo robótico necesitamos el "sistema operativo" de los
robots: **ROS 2** (*Robot Operating System 2*).

A pesar del nombre, ROS no es un sistema operativo como Windows o Ubuntu. Es
un **conjunto de programas y herramientas que se instalan encima de Ubuntu** y
que resuelven los problemas que tiene cualquier robot:

- ¿Cómo hace un programa que lee un sensor para pasarle el dato a otro programa que mueve un motor?
- ¿Cómo dibujo el robot en 3D para ver qué está haciendo?
- ¿Cómo calculo qué ángulo debe tener cada motor para llevar la pinza a un punto?

En lugar de programar todo eso desde cero, ROS 2 ya lo trae hecho y probado
por miles de personas. Nosotros solo tendremos que **configurarlo** para
nuestro brazo.

En este capítulo instalaremos ROS 2 y comprobaremos que funciona con un
pequeño ejemplo de dos programas que "hablan" entre sí.

---

## 2. Objetivos

Al terminar este capítulo serás capaz de:

- [ ] Explicar con tus palabras qué es ROS 2 y para qué sirve.
- [ ] Saber por qué usamos **Jazzy** con **Ubuntu 24.04**.
- [ ] Instalar ROS 2 Jazzy con un script automático (o a mano, paso a paso).
- [ ] Comprobar que la instalación funciona con el ejemplo *talker / listener*.

---

## 3. Conceptos clave

| Concepto | Explicación sencilla | Ejemplo cotidiano |
|---|---|---|
| **ROS 2** | Conjunto de herramientas para construir programas de robots | Una caja de herramientas muy completa |
| **Distribución** | Cada "versión" de ROS 2 tiene nombre de tortuga en orden alfabético: Humble, Iron, **Jazzy**, Kilted… | Como Android 13, 14, 15… |
| **LTS** | *Long Term Support*: versión con soporte durante 5 años. Jazzy es LTS (hasta 2029) | Un coche con garantía larga |
| **Terminal** | Ventana donde escribes órdenes de texto. Se abre con `Ctrl + Alt + T` | El "símbolo del sistema" de Windows |
| **`sudo`** | Ejecuta una orden con permisos de administrador. Te pedirá tu contraseña | "Ejecutar como administrador" |
| **`apt`** | El instalador de programas de Ubuntu | La tienda de aplicaciones, pero por texto |
| **Nodo** | Un programa que forma parte de un sistema ROS | Un trabajador de una fábrica |
| **Topic** | Canal con nombre por el que los nodos se envían mensajes | Un grupo de WhatsApp: unos escriben, otros leen |
| **`source`** | Orden que "activa" ROS en la terminal actual | Encender la luz de una habitación antes de trabajar en ella |

### ¿Por qué Jazzy y por qué Ubuntu 24.04?

Cada versión de ROS 2 se prepara para **una versión concreta de Ubuntu**. Jazzy
se diseñó para **Ubuntu 24.04**. Si intentas instalar Jazzy en otro Ubuntu
tendrías que compilarlo todo a mano (horas de trabajo y muchos errores).

| Componente | Versión que usaremos |
|---|---|
| Sistema operativo | Ubuntu 24.04 LTS (64 bits) |
| ROS 2 | Jazzy Jalisco |
| Simulador (capítulo 03) | Gazebo Harmonic |
| Planificador de movimientos (capítulo 03) | MoveIt 2 |

> 💡 **¿Usas Windows?** Tienes dos opciones:
> - **Arranque dual** (Windows y Ubuntu en el mismo PC, eliges al encender). Es lo recomendado si vas a conectar el robot real por USB.
> - **WSL2** (Ubuntu dentro de Windows). Sirve para aprender y para la simulación.

---

## 4. Estructura

En este capítulo solo hay dos archivos:

```
capitulos/01_instalacion_ros2/
├── README.md                 ← esta guía
└── instalar_ros2_jazzy.sh    ← script que hace toda la instalación por ti
```

Así queda tu sistema después de instalar:

```
/opt/ros/jazzy/          ← aquí vive ROS 2 (no hace falta tocar nada)
├── bin/                    programas
├── lib/                    librerías
├── share/                  archivos de configuración de cada paquete
└── setup.bash              ← el archivo que "activa" ROS en la terminal

~/.bashrc                ← archivo que se ejecuta al abrir cada terminal;
                            le añadiremos una línea para activar ROS siempre
```

---

## 5. Proceso detallado

Tienes dos caminos. **Si es tu primera vez, usa la opción A.** La opción B es
para entender qué hace el script por dentro.

### Opción A · Instalación automática (recomendada)

1. Abre una terminal (`Ctrl + Alt + T`).
2. Descarga este repositorio (si aún no tienes `git`, el primer comando lo instala):

   ```bash
   sudo apt install -y git                                   # instala git
   git clone https://github.com/DonDoney/ROS2_ROLLBOT.git ~/rollbot_ws   # descarga el proyecto
   ```

3. Entra en la carpeta del capítulo y ejecuta el script:

   ```bash
   cd ~/rollbot_ws/capitulos/01_instalacion_ros2   # "cd" = cambiar de carpeta
   chmod +x instalar_ros2_jazzy.sh                  # da permiso para ejecutar el script
   ./instalar_ros2_jazzy.sh                         # lo ejecuta
   ```

4. Escribe tu contraseña cuando la pida (al escribirla **no se ve nada**: es normal).
5. Espera. Descargará unos 2-3 GB, así que puede tardar un rato.
6. Cuando termine, **cierra la terminal y abre una nueva**.

### Opción B · Instalación manual, paso a paso

Copia los bloques **uno por uno** y lee el comentario de cada línea.

**Paso 1 · Idioma del sistema en UTF-8.** ROS necesita que el sistema use la
codificación de texto UTF-8 (la que admite tildes, eñes, etc.).

```bash
sudo apt update                                   # actualiza la lista de programas disponibles
sudo apt install -y locales                       # herramienta para gestionar idiomas
sudo locale-gen en_US en_US.UTF-8                 # genera el idioma inglés en UTF-8
sudo update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8   # lo deja configurado
```

**Paso 2 · Activar el repositorio "universe" de Ubuntu.** Es una sección de
la "tienda" de Ubuntu con programas de la comunidad que ROS necesita.

```bash
sudo apt install -y software-properties-common    # herramienta para añadir repositorios
sudo add-apt-repository universe                  # activa "universe"
```

**Paso 3 · Añadir el repositorio de ROS 2.** Le decimos a Ubuntu dónde
descargar los paquetes de ROS. Lo hace un pequeño paquete oficial llamado
`ros2-apt-source`.

```bash
sudo apt install -y curl                          # curl sirve para descargar archivos
# Averigua cuál es la última versión del paquete ros2-apt-source:
export ROS_APT_SOURCE_VERSION=$(curl -s https://api.github.com/repos/ros-infrastructure/ros-apt-source/releases/latest | grep -F "tag_name" | awk -F\" '{print $4}')
# Lo descarga (el nombre incluye la versión de Ubuntu, "noble"):
curl -L -o /tmp/ros2-apt-source.deb "https://github.com/ros-infrastructure/ros-apt-source/releases/download/${ROS_APT_SOURCE_VERSION}/ros2-apt-source_${ROS_APT_SOURCE_VERSION}.$(. /etc/os-release && echo ${UBUNTU_CODENAME:-${VERSION_CODENAME}})_all.deb"
sudo dpkg -i /tmp/ros2-apt-source.deb             # lo instala
```

**Paso 4 · Instalar ROS 2.** `ros-jazzy-desktop` trae ROS, RViz y ejemplos.
`ros-dev-tools` trae las herramientas para compilar (colcon, rosdep…).

```bash
sudo apt update                                   # vuelve a leer la lista (ahora incluye ROS)
sudo apt upgrade -y                               # actualiza lo que ya tienes instalado
sudo apt install -y ros-jazzy-desktop ros-dev-tools
```

**Paso 5 · Preparar rosdep y activar ROS en cada terminal.**

```bash
sudo rosdep init                                  # crea la configuración de rosdep (solo una vez)
rosdep update                                     # descarga la lista de dependencias
echo "source /opt/ros/jazzy/setup.bash" >> ~/.bashrc   # activa ROS en cada terminal nueva
```

> 🔎 **¿Qué hace la última línea?** Añade la orden `source ...` al final del
> archivo `~/.bashrc`. Ese archivo se ejecuta cada vez que abres una terminal,
> así que ROS quedará activado siempre sin que tengas que acordarte.

---

## 6. Ejemplos

### Ejemplo 1 · Dos programas que hablan

Vamos a arrancar dos **nodos**: uno que habla (*talker*) y otro que escucha
(*listener*). Abre **dos terminales nuevas**.

```bash
# Terminal 1: un nodo escrito en C++ que publica "Hello World: 1, 2, 3..."
ros2 run demo_nodes_cpp talker
```

```bash
# Terminal 2: un nodo escrito en Python que escucha esos mensajes
ros2 run demo_nodes_py listener
```

Si en la terminal 2 aparece algo así, **ROS 2 funciona** 🎉:

```
[INFO] [listener]: I heard: [Hello World: 1]
[INFO] [listener]: I heard: [Hello World: 2]
```

Para detener cada programa pulsa `Ctrl + C`.

> 🧠 **¿Qué ha pasado?** El *talker* publica mensajes en un **topic** llamado
> `/chatter` y el *listener* está suscrito a ese topic. Ninguno sabe que el
> otro existe: solo comparten el nombre del canal. Así se comunican todas las
> piezas de nuestro brazo.

### Ejemplo 2 · Espiar el canal

Con el *talker* en marcha, abre una tercera terminal:

```bash
ros2 topic list            # lista todos los topics activos (verás /chatter)
ros2 topic echo /chatter   # muestra en pantalla cada mensaje que pasa por /chatter
ros2 topic hz /chatter     # cuántos mensajes por segundo se publican
```

---

## 7. Cierre

### Resumen

- ROS 2 es una caja de herramientas para programar robots que se instala sobre Ubuntu.
- Usamos **Jazzy** porque es la versión LTS preparada para **Ubuntu 24.04**.
- Los programas de ROS se llaman **nodos** y se comunican por **topics**.
- `source /opt/ros/jazzy/setup.bash` activa ROS en la terminal (lo hemos dejado automático).

### Comprueba que lo tienes

- [ ] `ros2 --help` muestra la ayuda (y no "command not found").
- [ ] El ejemplo *talker / listener* funciona.

### Si algo falla

| Síntoma | Causa probable | Solución |
|---|---|---|
| `ros2: command not found` | ROS no está activado en esta terminal | Abre una terminal nueva o ejecuta `source /opt/ros/jazzy/setup.bash` |
| `Unable to locate package ros-jazzy-desktop` | No se añadió el repositorio de ROS o no es Ubuntu 24.04 | Repite el paso 3 y comprueba tu versión con `lsb_release -a` |
| El *listener* no recibe nada | Las terminales usan distinto `ROS_DOMAIN_ID` | Ejecuta `echo $ROS_DOMAIN_ID` en ambas; debe coincidir |

➡️ **Siguiente capítulo:** [02 · El workspace](../02_workspace/README.md), donde
prepararemos la carpeta de trabajo del proyecto y compilaremos el código.
