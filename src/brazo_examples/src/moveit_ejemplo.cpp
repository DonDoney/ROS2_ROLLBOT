// =============================================================================
// moveit_ejemplo.cpp — Ejemplo 3: planificar y mover el brazo con MoveIt 2 (C++)
// =============================================================================
// Muestra las tres formas más habituales de dar un objetivo a MoveIt:
//   1) Una postura con nombre del SRDF ("listo", "home", "plegado")
//   2) Una POSE (posición + orientación) de la herramienta tool0
//   3) Una TRAYECTORIA CARTESIANA (la herramienta se mueve en línea recta)
// Antes de mover, añade una caja (obstáculo) a la escena para que MoveIt la evite.
//
// Necesita move_group en marcha. Uso:
//   Terminal 1: ros2 launch brazo_moveit_config demo.launch.py
//   Terminal 2: ros2 launch brazo_examples moveit_ejemplo.launch.py
// =============================================================================

#include <memory>
#include <thread>
#include <vector>

#include <geometry_msgs/msg/pose.hpp>
#include <moveit/move_group_interface/move_group_interface.h>
#include <moveit/planning_scene_interface/planning_scene_interface.h>
#include <moveit_msgs/msg/collision_object.hpp>
#include <rclcpp/rclcpp.hpp>
#include <shape_msgs/msg/solid_primitive.hpp>

using moveit::planning_interface::MoveGroupInterface;

// Planifica hacia el objetivo ya fijado en `brazo` y, si sale bien, lo ejecuta.
static bool planificarYEjecutar(MoveGroupInterface & brazo, const rclcpp::Logger & log)
{
  MoveGroupInterface::Plan plan;
  if (brazo.plan(plan) != moveit::core::MoveItErrorCode::SUCCESS) {
    RCLCPP_ERROR(log, "No se encontró un plan");
    return false;
  }
  RCLCPP_INFO(log, "Plan encontrado (%.2f s de planificación). Ejecutando...", plan.planning_time);
  return brazo.execute(plan) == moveit::core::MoveItErrorCode::SUCCESS;
}

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  // Los parámetros robot_description, robot_description_semantic, etc. llegan
  // desde el launch; con esta opción el nodo los declara automáticamente.
  auto nodo = std::make_shared<rclcpp::Node>(
    "moveit_ejemplo",
    rclcpp::NodeOptions().automatically_declare_parameters_from_overrides(true));
  const auto log = nodo->get_logger();

  // MoveGroupInterface necesita que el nodo esté "girando" (spin) para
  // recibir /joint_states, así que lo hacemos en un hilo aparte.
  rclcpp::executors::SingleThreadedExecutor ejecutor;
  ejecutor.add_node(nodo);
  std::thread hilo_spin([&ejecutor]() {ejecutor.spin();});

  // "brazo" es el nombre del grupo de planificación definido en brazo.srdf
  MoveGroupInterface brazo(nodo, "brazo");
  brazo.setPlanningTime(5.0);
  brazo.setMaxVelocityScalingFactor(0.3);      // 30 % de la velocidad máxima
  brazo.setMaxAccelerationScalingFactor(0.3);

  RCLCPP_INFO(log, "Marco de planificación: %s | efector: %s",
    brazo.getPlanningFrame().c_str(), brazo.getEndEffectorLink().c_str());

  // ---------------------------------------------------------------------------
  // 0) Añadir un obstáculo a la escena (una caja delante del robot)
  // ---------------------------------------------------------------------------
  moveit::planning_interface::PlanningSceneInterface escena;
  moveit_msgs::msg::CollisionObject caja;
  caja.header.frame_id = brazo.getPlanningFrame();
  caja.id = "caja_obstaculo";
  shape_msgs::msg::SolidPrimitive forma;
  forma.type = shape_msgs::msg::SolidPrimitive::BOX;
  forma.dimensions = {0.3, 0.4, 0.2};  // x, y, z en metros (igual que la mesa de Gazebo)
  geometry_msgs::msg::Pose pose_caja;
  pose_caja.orientation.w = 1.0;
  pose_caja.position.x = 0.45;
  pose_caja.position.z = 0.1;
  caja.primitives.push_back(forma);
  caja.primitive_poses.push_back(pose_caja);
  caja.operation = moveit_msgs::msg::CollisionObject::ADD;
  escena.applyCollisionObject(caja);
  RCLCPP_INFO(log, "Obstáculo añadido a la escena");

  // ---------------------------------------------------------------------------
  // 1) Objetivo con nombre (definido en el SRDF)
  // ---------------------------------------------------------------------------
  RCLCPP_INFO(log, "[1/3] Moviendo a la postura 'listo'");
  brazo.setNamedTarget("listo");
  planificarYEjecutar(brazo, log);

  // ---------------------------------------------------------------------------
  // 2) Objetivo en pose: partimos de la pose actual de tool0 y la desplazamos
  //    10 cm en Y (a la izquierda) manteniendo la orientación.
  // ---------------------------------------------------------------------------
  RCLCPP_INFO(log, "[2/3] Moviendo la herramienta 10 cm a la izquierda");
  geometry_msgs::msg::Pose objetivo = brazo.getCurrentPose().pose;
  objetivo.position.y += 0.10;
  brazo.setPoseTarget(objetivo);
  planificarYEjecutar(brazo, log);

  // ---------------------------------------------------------------------------
  // 3) Trayectoria cartesiana: un cuadrado de 8 cm en el plano horizontal
  // ---------------------------------------------------------------------------
  RCLCPP_INFO(log, "[3/3] Dibujando un cuadrado en línea recta");
  std::vector<geometry_msgs::msg::Pose> puntos;
  geometry_msgs::msg::Pose p = brazo.getCurrentPose().pose;
  p.position.y -= 0.08; puntos.push_back(p);
  p.position.x -= 0.08; puntos.push_back(p);
  p.position.y += 0.08; puntos.push_back(p);
  p.position.x += 0.08; puntos.push_back(p);

  moveit_msgs::msg::RobotTrajectory trayectoria;
  const double paso_efector = 0.005;  // interpolar cada 5 mm
  const double salto_maximo = 0.0;    // 0 = sin comprobación de saltos articulares
  const double fraccion = brazo.computeCartesianPath(puntos, paso_efector, salto_maximo, trayectoria);
  RCLCPP_INFO(log, "Se pudo planificar el %.0f %% de la trayectoria cartesiana", fraccion * 100.0);
  if (fraccion > 0.95) {
    brazo.execute(trayectoria);
  } else {
    RCLCPP_WARN(log, "Trayectoria incompleta: no se ejecuta");
  }

  // Volver a casa y limpiar la escena
  brazo.setNamedTarget("home");
  planificarYEjecutar(brazo, log);
  escena.removeCollisionObjects({"caja_obstaculo"});

  RCLCPP_INFO(log, "Ejemplo terminado");
  rclcpp::shutdown();
  hilo_spin.join();
  return 0;
}
