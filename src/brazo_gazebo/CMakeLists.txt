# =============================================================================
# CMakeLists.txt — brazo_gazebo
# Instala los launch y los mundos (.sdf) de Gazebo.
# =============================================================================
cmake_minimum_required(VERSION 3.8)
project(brazo_gazebo)

find_package(ament_cmake REQUIRED)

install(
  DIRECTORY launch worlds
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
