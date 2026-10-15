# =============================================================================
# CMakeLists.txt — brazo_hardware
# Compila el plugin de ros2_control como librería compartida (.so) y lo
# registra con pluginlib para que el controller_manager lo encuentre por su
# nombre "brazo_hardware/BrazoSystemHardware".
# =============================================================================
cmake_minimum_required(VERSION 3.16)
project(brazo_hardware LANGUAGES CXX)

# Avisos del compilador: ayudan a detectar errores pronto
if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Dependencias de ROS 2
set(DEPENDENCIAS
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
)

find_package(ament_cmake REQUIRED)
foreach(dep IN ITEMS ${DEPENDENCIAS})
  find_package(${dep} REQUIRED)
endforeach()

# ---------------- Librería del plugin ----------------
add_library(brazo_hardware SHARED
  src/brazo_system.cpp
  src/puerto_serie.cpp
)
target_compile_features(brazo_hardware PUBLIC cxx_std_17)
target_include_directories(brazo_hardware PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/brazo_hardware>
)
ament_target_dependencies(brazo_hardware PUBLIC ${DEPENDENCIAS})

# Registra brazo_hardware.xml como descripción de plugins de hardware_interface
pluginlib_export_plugin_description_file(hardware_interface brazo_hardware.xml)

# ---------------- Instalación ----------------
install(DIRECTORY include/ DESTINATION include/brazo_hardware)
install(TARGETS brazo_hardware
  EXPORT export_brazo_hardware
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
# Herramienta para probar el firmware sin ROS (ver capítulo 10)
install(PROGRAMS scripts/probar_serie.py DESTINATION lib/${PROJECT_NAME})

ament_export_targets(export_brazo_hardware HAS_LIBRARY_TARGET)
ament_export_dependencies(${DEPENDENCIAS})
ament_package()
