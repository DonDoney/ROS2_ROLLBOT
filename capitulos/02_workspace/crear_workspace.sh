#!/usr/bin/env bash
# =============================================================================
# crear_workspace.sh — Descarga este repositorio como workspace y lo compila
# =============================================================================
# El repositorio YA ES un workspace de ROS 2: contiene la carpeta src/ con
# los paquetes. Basta con clonarlo, instalar dependencias y compilar.
#
# Uso:
#   ./crear_workspace.sh                 # lo crea en ~/rollbot_ws
#   ./crear_workspace.sh ~/otra_carpeta  # en otra ruta
# =============================================================================
set -e

WS="${1:-$HOME/rollbot_ws}"
REPO="https://github.com/DonDoney/ROS2_ROLLBOT.git"

# Cargamos ROS (por si esta terminal aún no lo tiene)
source /opt/ros/jazzy/setup.bash

echo "==> [1/4] Clonando el repositorio en ${WS}"
if [ -d "${WS}/.git" ]; then
  echo "    Ya existe: actualizando con git pull"
  git -C "${WS}" pull
else
  git clone "${REPO}" "${WS}"
fi
cd "${WS}"

echo "==> [2/4] Instalando dependencias con rosdep"
# --from-paths src : busca package.xml dentro de src/
# --ignore-src     : no intentes instalar con apt los paquetes que ya están en src/
# -y               : responde "sí" a todo
rosdep install --from-paths src --ignore-src -r -y

echo "==> [3/4] Compilando con colcon"
# --symlink-install : enlaza (en vez de copiar) los archivos Python, launch, URDF…
#                     así los cambios se ven sin recompilar
colcon build --symlink-install

echo "==> [4/4] Añadiendo el workspace al ~/.bashrc"
LINEA="source ${WS}/install/setup.bash"
grep -qxF "$LINEA" ~/.bashrc || echo "$LINEA" >> ~/.bashrc

echo ""
echo "======================================================================"
echo " Workspace listo en ${WS}"
echo " Abre una terminal nueva y prueba:"
echo "   ros2 launch brazo_description visualizar.launch.py"
echo "======================================================================"
