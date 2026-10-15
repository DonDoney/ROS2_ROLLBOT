# Capítulo 01 · Instalación de ROS 2 Jazzy

> **Objetivo:** tener ROS 2 Jazzy Jalisco funcionando en Ubuntu 24.04.

## ¿Por qué Jazzy y Ubuntu 24.04?

ROS 2 publica una versión cada año. **Jazzy Jalisco** (mayo de 2024) es una
versión **LTS**, con soporte hasta mayo de 2029, y está compilada para
**Ubuntu 24.04 Noble Numbat**. Mezclar versiones (por ejemplo Jazzy en Ubuntu
22.04) obliga a compilar todo desde código fuente, así que no lo recomendamos.

| Componente | Versión |
|---|---|
| Sistema operativo | Ubuntu 24.04 LTS (64 bits) |
| ROS 2 | Jazzy Jalisco |
| Simulador | Gazebo Harmonic (capítulo 03) |
| Planificación | MoveIt 2 (capítulo 03) |

> 💡 Si usas Windows, puedes instalar Ubuntu 24.04 en **arranque dual** (recomendado
> para trabajar con el robot real por USB) o en **WSL2** (válido para simulación).

## Instalación automática

```bash
cd capitulos/01_instalacion_ros2
chmod +x instalar_ros2_jazzy.sh
./instalar_ros2_jazzy.sh
```

El script [`instalar_ros2_jazzy.sh`](instalar_ros2_jazzy.sh) está comentado
paso a paso. Hace exactamente lo mismo que la guía oficial.

## Instalación manual (para entender cada paso)

```bash
# 1) Idioma UTF-8
sudo apt update && sudo apt install -y locales
sudo locale-gen en_US en_US.UTF-8
sudo update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8

# 2) Repositorio universe
sudo apt install -y software-properties-common
sudo add-apt-repository universe

# 3) Repositorio de ROS 2
sudo apt install -y curl
export ROS_APT_SOURCE_VERSION=$(curl -s https://api.github.com/repos/ros-infrastructure/ros-apt-source/releases/latest | grep -F "tag_name" | awk -F\" '{print $4}')
curl -L -o /tmp/ros2-apt-source.deb "https://github.com/ros-infrastructure/ros-apt-source/releases/download/${ROS_APT_SOURCE_VERSION}/ros2-apt-source_${ROS_APT_SOURCE_VERSION}.$(. /etc/os-release && echo ${UBUNTU_CODENAME:-${VERSION_CODENAME}})_all.deb"
sudo dpkg -i /tmp/ros2-apt-source.deb

# 4) ROS 2 + herramientas de desarrollo
sudo apt update && sudo apt upgrade -y
sudo apt install -y ros-jazzy-desktop ros-dev-tools

# 5) rosdep y entorno
sudo rosdep init
rosdep update
echo "source /opt/ros/jazzy/setup.bash" >> ~/.bashrc
```

## Comprobación

Abre **dos terminales nuevas**:

```bash
# Terminal 1
ros2 run demo_nodes_cpp talker
# Terminal 2
ros2 run demo_nodes_py listener
```

Si la segunda terminal muestra `I heard: [Hello World: 1]`, ROS 2 funciona:
un nodo en C++ está hablando con otro en Python a través de un *topic*.

## Conceptos clave de este capítulo

| Concepto | Qué es |
|---|---|
| **Nodo** | Un programa que forma parte del sistema ROS (p. ej. `talker`) |
| **Topic** | Canal con nombre por el que los nodos publican/reciben mensajes (`/chatter`) |
| **`source`** | Carga las variables de entorno de ROS en la terminal actual |
| **rosdep** | Instala automáticamente las dependencias que declara cada paquete |
| **colcon** | Herramienta que compila los paquetes de un workspace |

## Problemas frecuentes

- **`ros2: command not found`** → no has hecho `source /opt/ros/jazzy/setup.bash`
  o abriste la terminal antes de editar el `.bashrc`.
- **El listener no recibe nada** → revisa que ambas terminales tengan el mismo
  `ROS_DOMAIN_ID` (`echo $ROS_DOMAIN_ID`) y que no haya un cortafuegos bloqueando.

➡️ Siguiente: [Capítulo 02 · Workspace](../02_workspace/README.md)
