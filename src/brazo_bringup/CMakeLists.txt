# =============================================================================
# CMakeLists.txt — brazo_bringup
# Solo instala las carpetas launch/ y config/ (no hay código que compilar).
# =============================================================================
cmake_minimum_required(VERSION 3.8)
project(brazo_bringup)

find_package(ament_cmake REQUIRED)

install(
  DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
