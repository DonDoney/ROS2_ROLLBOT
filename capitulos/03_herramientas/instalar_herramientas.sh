#!/usr/bin/env bash
# =============================================================================
# instalar_herramientas.sh — Herramientas para desarrollar el brazo
# =============================================================================
# Instala por apt (binarios precompilados para Jazzy):
#   - Modelado:     xacro, joint_state_publisher(_gui), check_urdf
#   - Visualización: RViz2 (ya viene con ros-jazzy-desktop), rqt
#   - Control:      ros2_control + ros2_controllers
#   - Simulación:   Gazebo Harmonic + ros_gz + gz_ros2_control
#   - Planificación: MoveIt 2 completo (incluye el Setup Assistant)
#   - Hardware:     pyserial (para probar el ESP32)
#
# Uso:  ./instalar_herramientas.sh
# =============================================================================
set -e

sudo apt update

echo "==> Modelado y visualización"
sudo apt install -y \
  ros-jazzy-xacro \
  ros-jazzy-joint-state-publisher \
  ros-jazzy-joint-state-publisher-gui \
  ros-jazzy-robot-state-publisher \
  ros-jazzy-rviz2 \
  ros-jazzy-rqt-graph \
  ros-jazzy-rqt-tf-tree \
  ros-jazzy-tf2-tools \
  liburdfdom-tools

echo "==> ros2_control"
sudo apt install -y \
  ros-jazzy-ros2-control \
  ros-jazzy-ros2-controllers

echo "==> Gazebo Harmonic e integración con ROS 2"
# ros-jazzy-ros-gz instala Gazebo Harmonic (la versión emparejada con Jazzy)
sudo apt install -y \
  ros-jazzy-ros-gz \
  ros-jazzy-gz-ros2-control

echo "==> MoveIt 2"
sudo apt install -y ros-jazzy-moveit

echo "==> Utilidades para el hardware"
sudo apt install -y python3-serial
# Permiso para usar /dev/ttyUSB* sin sudo (efectivo tras cerrar sesión)
sudo usermod -aG dialout "$USER"

echo ""
echo "======================================================================"
echo " Herramientas instaladas. CIERRA SESIÓN y vuelve a entrar para que el"
echo " permiso del puerto serie (grupo dialout) tenga efecto."
echo "======================================================================"
