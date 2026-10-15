# =============================================================================
# CMakeLists.txt — brazo_moveit_config
# Instala launch/ y config/. El archivo .setup_assistant permite reabrir esta
# configuración con el MoveIt Setup Assistant.
# =============================================================================
cmake_minimum_required(VERSION 3.8)
project(brazo_moveit_config)

find_package(ament_cmake REQUIRED)

install(
  DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
)
install(FILES .setup_assistant DESTINATION share/${PROJECT_NAME})

ament_package()
