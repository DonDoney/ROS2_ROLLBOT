#!/usr/bin/env bash
# =============================================================================
# instalar_ros2_jazzy.sh — Instala ROS 2 Jazzy Jalisco en Ubuntu 24.04 (Noble)
# =============================================================================
# Sigue los pasos de la documentación oficial (docs.ros.org/en/jazzy):
#   1. Configura el idioma (locale) en UTF-8
#   2. Activa el repositorio "universe" de Ubuntu
#   3. Añade el repositorio de ROS 2 (paquete ros2-apt-source)
#   4. Instala ros-jazzy-desktop (ROS + RViz + demos) y ros-dev-tools (colcon, rosdep…)
#   5. Inicializa rosdep y añade ROS al ~/.bashrc
#
# Uso:
#   chmod +x instalar_ros2_jazzy.sh
#   ./instalar_ros2_jazzy.sh
# =============================================================================
set -e  # si cualquier comando falla, el script se detiene

# ---------- Comprobación: Jazzy se distribuye para Ubuntu 24.04 ----------
. /etc/os-release
if [ "${VERSION_CODENAME}" != "noble" ]; then
  echo "ATENCIÓN: ROS 2 Jazzy está pensado para Ubuntu 24.04 (noble). Tienes: ${PRETTY_NAME}"
  read -r -p "¿Continuar de todas formas? [s/N] " resp
  [[ "$resp" =~ ^[sS]$ ]] || exit 1
fi

echo "==> [1/5] Configurando locale UTF-8"
sudo apt update
sudo apt install -y locales
sudo locale-gen en_US en_US.UTF-8
sudo update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
export LANG=en_US.UTF-8

echo "==> [2/5] Activando el repositorio universe"
sudo apt install -y software-properties-common
sudo add-apt-repository -y universe

echo "==> [3/5] Añadiendo el repositorio de ROS 2"
sudo apt install -y curl
ROS_APT_SOURCE_VERSION=$(curl -s https://api.github.com/repos/ros-infrastructure/ros-apt-source/releases/latest \
  | grep -F "tag_name" | awk -F\" '{print $4}')
curl -L -o /tmp/ros2-apt-source.deb \
  "https://github.com/ros-infrastructure/ros-apt-source/releases/download/${ROS_APT_SOURCE_VERSION}/ros2-apt-source_${ROS_APT_SOURCE_VERSION}.${UBUNTU_CODENAME:-${VERSION_CODENAME}}_all.deb"
sudo dpkg -i /tmp/ros2-apt-source.deb

echo "==> [4/5] Instalando ROS 2 Jazzy (puede tardar un rato)"
sudo apt update
sudo apt upgrade -y
sudo apt install -y ros-jazzy-desktop ros-dev-tools

echo "==> [5/5] Inicializando rosdep y configurando el terminal"
if [ ! -f /etc/ros/rosdep/sources.list.d/20-default.list ]; then
  sudo rosdep init
fi
rosdep update

# Añadimos "source" al .bashrc solo si no está ya
LINEA='source /opt/ros/jazzy/setup.bash'
grep -qxF "$LINEA" ~/.bashrc || echo "$LINEA" >> ~/.bashrc

echo ""
echo "======================================================================"
echo " ROS 2 Jazzy instalado. Abre una terminal NUEVA y prueba:"
echo "   ros2 run demo_nodes_cpp talker      (en una terminal)"
echo "   ros2 run demo_nodes_py listener     (en otra)"
echo "======================================================================"
