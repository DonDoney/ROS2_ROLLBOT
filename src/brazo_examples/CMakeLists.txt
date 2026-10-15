# =============================================================================
# CMakeLists.txt — brazo_examples
# Compila el ejemplo C++ de MoveIt e instala los scripts Python como
# ejecutables (ros2 run brazo_examples <script>.py).
# =============================================================================
cmake_minimum_required(VERSION 3.16)
project(brazo_examples LANGUAGES CXX)

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
  add_compile_options(-Wall -Wextra)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(moveit_msgs REQUIRED)
find_package(shape_msgs REQUIRED)
find_package(moveit_ros_planning_interface REQUIRED)

# ---------------- Ejemplo C++ con MoveIt ----------------
add_executable(moveit_ejemplo src/moveit_ejemplo.cpp)
target_compile_features(moveit_ejemplo PUBLIC cxx_std_17)
ament_target_dependencies(moveit_ejemplo
  rclcpp
  geometry_msgs
  moveit_msgs
  shape_msgs
  moveit_ros_planning_interface
)
install(TARGETS moveit_ejemplo DESTINATION lib/${PROJECT_NAME})

# ---------------- Scripts Python ----------------
install(PROGRAMS
  scripts/leer_estado.py
  scripts/mover_articulaciones.py
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

ament_package()
