# =============================================================================
# CMakeLists.txt — brazo_description
# Este paquete no compila código: solo instala carpetas (urdf, launch, rviz,
# meshes) en install/brazo_description/share/... para que otros paquetes
# puedan encontrarlas con get_package_share_directory("brazo_description")
# o con $(find brazo_description) dentro de un xacro.
# =============================================================================
cmake_minimum_required(VERSION 3.8)
project(brazo_description)

find_package(ament_cmake REQUIRED)

# Copiamos las carpetas de recursos al directorio "share" del paquete
install(
  DIRECTORY urdf launch rviz meshes
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
