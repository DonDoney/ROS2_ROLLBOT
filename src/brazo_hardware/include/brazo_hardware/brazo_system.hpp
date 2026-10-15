// =============================================================================
// brazo_system.hpp — Plugin de ros2_control para el brazo ROLLBOT real
// =============================================================================
// ros2_control llama a esta clase en cada ciclo del controller_manager:
//
//   read()  : pide al microcontrolador la posición de cada motor (en pasos)
//             y la convierte a radianes -> state_interfaces
//   write() : toma la posición deseada de cada articulación (radianes) de las
//             command_interfaces, la convierte a pasos y la envía al micro
//
// Ciclo de vida (lifecycle) del hardware:
//   on_init      -> lee parámetros del URDF (<ros2_control> ... </ros2_control>)
//   on_configure -> abre el puerto serie y comprueba que el micro responde
//   on_activate  -> habilita los drivers y, si se pide, hace el homing
//   on_deactivate-> deshabilita los drivers (los motores quedan libres)
//   on_cleanup   -> cierra el puerto serie
//
// El protocolo serie está documentado en firmware/PROTOCOLO.md
// =============================================================================
#ifndef BRAZO_HARDWARE__BRAZO_SYSTEM_HPP_
#define BRAZO_HARDWARE__BRAZO_SYSTEM_HPP_

#include <cstdint>
#include <string>
#include <vector>

#include "brazo_hardware/puerto_serie.hpp"
#include "hardware_interface/handle.hpp"
#include "hardware_interface/hardware_info.hpp"
#include "hardware_interface/system_interface.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"

namespace brazo_hardware
{

class BrazoSystemHardware : public hardware_interface::SystemInterface
{
public:
  RCLCPP_SHARED_PTR_DEFINITIONS(BrazoSystemHardware)

  hardware_interface::CallbackReturn on_init(
    const hardware_interface::HardwareInfo & info) override;

  hardware_interface::CallbackReturn on_configure(
    const rclcpp_lifecycle::State & previous_state) override;

  hardware_interface::CallbackReturn on_cleanup(
    const rclcpp_lifecycle::State & previous_state) override;

  hardware_interface::CallbackReturn on_activate(
    const rclcpp_lifecycle::State & previous_state) override;

  hardware_interface::CallbackReturn on_deactivate(
    const rclcpp_lifecycle::State & previous_state) override;

  std::vector<hardware_interface::StateInterface> export_state_interfaces() override;

  std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;

  hardware_interface::return_type read(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

  hardware_interface::return_type write(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

private:
  // ---------- Bits del campo "estado" que envía el micro (ver PROTOCOLO.md) ----------
  static constexpr uint32_t BIT_HABILITADO = 1u << 0;
  static constexpr uint32_t BIT_HOMING_HECHO = 1u << 1;
  static constexpr uint32_t BIT_ALARMA_DRIVER = 1u << 2;
  static constexpr uint32_t BIT_HOMING_EN_CURSO = 1u << 3;

  // Interpreta una línea "E p1 p2 p3 p4 p5 p6 estado". Devuelve true si es válida.
  bool interpretarEstado(const std::string & linea);

  // Lee líneas hasta recibir una que empiece por `prefijo` o agotar el tiempo.
  bool esperarRespuesta(const std::string & prefijo, double timeout_s);

  // Conversión radianes <-> pasos de motor para la articulación i
  int32_t radianesAPasos(size_t i, double radianes) const;
  double pasosARadianes(size_t i, int32_t pasos) const;

  rclcpp::Logger logger_ = rclcpp::get_logger("BrazoSystemHardware");

  // ---------- Parámetros leídos del URDF ----------
  std::string puerto_;
  int baudios_ {115200};
  double timeout_s_ {0.5};
  bool homing_al_activar_ {true};
  double timeout_homing_s_ {120.0};
  std::vector<double> pasos_por_radian_;  // uno por articulación
  std::vector<int> sentido_;              // +1 o -1

  // ---------- Memoria compartida con los controladores ----------
  std::vector<double> pos_estado_;     // rad
  std::vector<double> vel_estado_;     // rad/s
  std::vector<double> pos_comando_;    // rad

  // ---------- Datos crudos del micro ----------
  std::vector<int32_t> pasos_leidos_;
  uint32_t estado_micro_ {0};
  bool hay_lectura_nueva_ {false};
  rclcpp::Time ultima_respuesta_ {0, 0, RCL_STEADY_TIME};
  rclcpp::Clock reloj_ {RCL_STEADY_TIME};

  PuertoSerie serie_;
};

}  // namespace brazo_hardware

#endif  // BRAZO_HARDWARE__BRAZO_SYSTEM_HPP_
