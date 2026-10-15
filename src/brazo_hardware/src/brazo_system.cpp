// =============================================================================
// brazo_system.cpp — Implementación del plugin de hardware del brazo ROLLBOT
// =============================================================================
#include "brazo_hardware/brazo_system.hpp"

#include <chrono>
#include <cmath>
#include <limits>
#include <sstream>
#include <thread>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "pluginlib/class_list_macros.hpp"

namespace brazo_hardware
{

using hardware_interface::CallbackReturn;
using hardware_interface::return_type;

// -----------------------------------------------------------------------------
// on_init: se llama una vez al cargar el plugin. Leemos y validamos parámetros.
// -----------------------------------------------------------------------------
CallbackReturn BrazoSystemHardware::on_init(const hardware_interface::HardwareInfo & info)
{
  // La clase base guarda la información del URDF en info_
  if (hardware_interface::SystemInterface::on_init(info) != CallbackReturn::SUCCESS) {
    return CallbackReturn::ERROR;
  }

  try {
    // ---- Parámetros generales (<hardware><param .../></hardware>) ----
    puerto_ = info_.hardware_parameters.at("puerto_serie");
    baudios_ = std::stoi(info_.hardware_parameters.at("baudios"));
    timeout_s_ = std::stod(info_.hardware_parameters.at("timeout_ms")) / 1000.0;
    homing_al_activar_ = info_.hardware_parameters.at("homing_al_activar") == "true";
    timeout_homing_s_ = std::stod(info_.hardware_parameters.at("timeout_homing_s"));
  } catch (const std::exception & e) {
    RCLCPP_FATAL(logger_, "Falta o es incorrecto un parámetro de <hardware>: %s", e.what());
    return CallbackReturn::ERROR;
  }

  const size_t n = info_.joints.size();
  pos_estado_.assign(n, 0.0);
  vel_estado_.assign(n, 0.0);
  pos_comando_.assign(n, std::numeric_limits<double>::quiet_NaN());
  pasos_leidos_.assign(n, 0);
  pasos_por_radian_.resize(n);
  sentido_.resize(n);

  for (size_t i = 0; i < n; ++i) {
    const auto & joint = info_.joints[i];

    // ---- Validamos que la articulación tiene las interfaces esperadas ----
    if (joint.command_interfaces.size() != 1 ||
      joint.command_interfaces[0].name != hardware_interface::HW_IF_POSITION)
    {
      RCLCPP_FATAL(logger_, "La articulación '%s' debe tener UNA command_interface 'position'",
        joint.name.c_str());
      return CallbackReturn::ERROR;
    }
    if (joint.state_interfaces.size() != 2 ||
      joint.state_interfaces[0].name != hardware_interface::HW_IF_POSITION ||
      joint.state_interfaces[1].name != hardware_interface::HW_IF_VELOCITY)
    {
      RCLCPP_FATAL(logger_, "La articulación '%s' debe tener state_interfaces 'position' y "
        "'velocity' (en ese orden)", joint.name.c_str());
      return CallbackReturn::ERROR;
    }

    // ---- Parámetros mecánicos de la articulación ----
    try {
      const double pasos_vuelta = std::stod(joint.parameters.at("pasos_por_vuelta"));
      const double reduccion = std::stod(joint.parameters.at("reduccion"));
      pasos_por_radian_[i] = pasos_vuelta * reduccion / (2.0 * M_PI);
      sentido_[i] = std::stoi(joint.parameters.at("sentido")) >= 0 ? 1 : -1;
    } catch (const std::exception & e) {
      RCLCPP_FATAL(logger_, "Parámetros mecánicos incorrectos en '%s': %s",
        joint.name.c_str(), e.what());
      return CallbackReturn::ERROR;
    }

    RCLCPP_INFO(logger_, "%s: %.1f pasos/rad (%.2f pasos/grado), sentido %+d",
      joint.name.c_str(), pasos_por_radian_[i], pasos_por_radian_[i] * M_PI / 180.0, sentido_[i]);
  }

  return CallbackReturn::SUCCESS;
}

// -----------------------------------------------------------------------------
// on_configure: abrimos el puerto y comprobamos que el micro contesta.
// -----------------------------------------------------------------------------
CallbackReturn BrazoSystemHardware::on_configure(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(logger_, "Abriendo %s a %d baudios...", puerto_.c_str(), baudios_);
  if (!serie_.abrir(puerto_, baudios_)) {
    RCLCPP_FATAL(logger_, "%s", serie_.ultimoError().c_str());
    return CallbackReturn::ERROR;
  }

  // Muchas placas ESP32 se reinician al abrir el puerto (señal DTR).
  // Esperamos a que arranque el firmware antes de hablarle.
  std::this_thread::sleep_for(std::chrono::seconds(2));
  serie_.vaciarEntrada();

  serie_.escribirLinea("?");
  if (!esperarRespuesta("E", 2.0)) {
    RCLCPP_FATAL(logger_, "El microcontrolador no responde. ¿Firmware cargado? ¿Puerto correcto?");
    serie_.cerrar();
    return CallbackReturn::ERROR;
  }

  RCLCPP_INFO(logger_, "Microcontrolador detectado (estado = 0x%02X)", estado_micro_);
  return CallbackReturn::SUCCESS;
}

// -----------------------------------------------------------------------------
// on_cleanup: cerramos el puerto.
// -----------------------------------------------------------------------------
CallbackReturn BrazoSystemHardware::on_cleanup(const rclcpp_lifecycle::State &)
{
  serie_.cerrar();
  return CallbackReturn::SUCCESS;
}

// -----------------------------------------------------------------------------
// on_activate: habilitamos drivers, homing opcional y sincronizamos comandos.
// -----------------------------------------------------------------------------
CallbackReturn BrazoSystemHardware::on_activate(const rclcpp_lifecycle::State &)
{
  serie_.escribirLinea("A");  // habilitar drivers (señal ENA)
  if (!esperarRespuesta("OK A", 1.0)) {
    RCLCPP_ERROR(logger_, "El micro no confirmó la habilitación de los drivers");
    return CallbackReturn::ERROR;
  }

  if (homing_al_activar_) {
    RCLCPP_WARN(logger_, "Iniciando HOMING. Mantén la zona despejada y la seta de emergencia a mano.");
    serie_.escribirLinea("H");
    if (!esperarRespuesta("OK H", timeout_homing_s_)) {
      RCLCPP_ERROR(logger_, "El homing no terminó correctamente");
      serie_.escribirLinea("D");
      return CallbackReturn::ERROR;
    }
    RCLCPP_INFO(logger_, "Homing completado");
  }

  // Leemos la posición actual y hacemos comando = estado, para que el robot
  // NO salte al activarse los controladores.
  serie_.escribirLinea("?");
  if (!esperarRespuesta("E", 1.0)) {
    RCLCPP_ERROR(logger_, "Sin respuesta de estado tras activar");
    return CallbackReturn::ERROR;
  }
  for (size_t i = 0; i < pos_estado_.size(); ++i) {
    pos_estado_[i] = pasosARadianes(i, pasos_leidos_[i]);
    vel_estado_[i] = 0.0;
    pos_comando_[i] = pos_estado_[i];
  }

  RCLCPP_INFO(logger_, "Hardware activado");
  return CallbackReturn::SUCCESS;
}

// -----------------------------------------------------------------------------
// on_deactivate: deshabilitamos los drivers.
// -----------------------------------------------------------------------------
CallbackReturn BrazoSystemHardware::on_deactivate(const rclcpp_lifecycle::State &)
{
  serie_.escribirLinea("D");
  esperarRespuesta("OK D", 1.0);
  RCLCPP_INFO(logger_, "Drivers deshabilitados");
  return CallbackReturn::SUCCESS;
}

// -----------------------------------------------------------------------------
// Exportamos las interfaces: punteros a nuestras variables que los
// controladores leerán (state) o escribirán (command).
// -----------------------------------------------------------------------------
std::vector<hardware_interface::StateInterface> BrazoSystemHardware::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> interfaces;
  for (size_t i = 0; i < info_.joints.size(); ++i) {
    interfaces.emplace_back(
      info_.joints[i].name, hardware_interface::HW_IF_POSITION, &pos_estado_[i]);
    interfaces.emplace_back(
      info_.joints[i].name, hardware_interface::HW_IF_VELOCITY, &vel_estado_[i]);
  }
  return interfaces;
}

std::vector<hardware_interface::CommandInterface> BrazoSystemHardware::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> interfaces;
  for (size_t i = 0; i < info_.joints.size(); ++i) {
    interfaces.emplace_back(
      info_.joints[i].name, hardware_interface::HW_IF_POSITION, &pos_comando_[i]);
  }
  return interfaces;
}

// -----------------------------------------------------------------------------
// read(): se ejecuta en cada ciclo. Procesa las respuestas del micro.
// -----------------------------------------------------------------------------
return_type BrazoSystemHardware::read(const rclcpp::Time &, const rclcpp::Duration & period)
{
  hay_lectura_nueva_ = false;
  for (const auto & linea : serie_.leerLineas()) {
    if (linea.rfind("E ", 0) == 0) {
      interpretarEstado(linea);
    } else if (linea.rfind("ERR", 0) == 0) {
      RCLCPP_ERROR(logger_, "Micro: %s", linea.c_str());
    } else {
      RCLCPP_DEBUG(logger_, "Micro: %s", linea.c_str());
    }
  }

  // ¿Lleva demasiado tiempo sin contestar? -> error (para los controladores)
  if ((reloj_.now() - ultima_respuesta_).seconds() > timeout_s_) {
    RCLCPP_ERROR(logger_, "Sin respuesta del micro durante más de %.0f ms", timeout_s_ * 1000.0);
    return return_type::ERROR;
  }

  // ¿Algún driver ha disparado su alarma (error de posición del bucle cerrado)?
  if (estado_micro_ & BIT_ALARMA_DRIVER) {
    RCLCPP_ERROR(logger_, "ALARMA de driver activa: revisa el motor/carga y rearma el driver");
    return return_type::ERROR;
  }

  if (hay_lectura_nueva_) {
    const double dt = period.seconds();
    for (size_t i = 0; i < pos_estado_.size(); ++i) {
      const double nueva = pasosARadianes(i, pasos_leidos_[i]);
      vel_estado_[i] = dt > 0.0 ? (nueva - pos_estado_[i]) / dt : 0.0;
      pos_estado_[i] = nueva;
    }
  }
  return return_type::OK;
}

// -----------------------------------------------------------------------------
// write(): enviamos las posiciones objetivo en pasos: "M p1 p2 p3 p4 p5 p6"
// El micro contesta a cada "M" con una línea "E ..." que leeremos en read().
// -----------------------------------------------------------------------------
return_type BrazoSystemHardware::write(const rclcpp::Time &, const rclcpp::Duration &)
{
  std::ostringstream msg;
  msg << "M";
  for (size_t i = 0; i < pos_comando_.size(); ++i) {
    // Si aún no hay comando válido mantenemos la posición actual
    const double objetivo = std::isfinite(pos_comando_[i]) ? pos_comando_[i] : pos_estado_[i];
    msg << ' ' << radianesAPasos(i, objetivo);
  }

  if (!serie_.escribirLinea(msg.str())) {
    RCLCPP_ERROR(logger_, "Error escribiendo en el puerto: %s", serie_.ultimoError().c_str());
    return return_type::ERROR;
  }
  return return_type::OK;
}

// -----------------------------------------------------------------------------
// Funciones auxiliares
// -----------------------------------------------------------------------------
bool BrazoSystemHardware::interpretarEstado(const std::string & linea)
{
  std::istringstream iss(linea);
  char tipo;
  iss >> tipo;  // 'E'
  std::vector<int32_t> pasos(pasos_leidos_.size());
  for (auto & p : pasos) {
    if (!(iss >> p)) {
      RCLCPP_WARN(logger_, "Línea de estado incompleta: '%s'", linea.c_str());
      return false;
    }
  }
  uint32_t estado = 0;
  if (!(iss >> estado)) {
    RCLCPP_WARN(logger_, "Línea de estado sin campo 'estado': '%s'", linea.c_str());
    return false;
  }
  pasos_leidos_ = pasos;
  estado_micro_ = estado;
  hay_lectura_nueva_ = true;
  ultima_respuesta_ = reloj_.now();
  return true;
}

bool BrazoSystemHardware::esperarRespuesta(const std::string & prefijo, double timeout_s)
{
  const auto inicio = reloj_.now();
  while ((reloj_.now() - inicio).seconds() < timeout_s) {
    for (const auto & linea : serie_.leerLineas()) {
      if (linea.rfind("E ", 0) == 0) {
        // Línea de estado: la guardamos; solo termina la espera si era lo que buscábamos
        if (interpretarEstado(linea) && prefijo == "E") {
          return true;
        }
        continue;
      }
      if (linea.rfind("ERR", 0) == 0) {
        RCLCPP_ERROR(logger_, "Micro: %s", linea.c_str());
        return false;
      }
      if (linea.rfind(prefijo, 0) == 0) {
        ultima_respuesta_ = reloj_.now();
        return true;
      }
      RCLCPP_INFO(logger_, "Micro: %s", linea.c_str());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return false;
}

int32_t BrazoSystemHardware::radianesAPasos(size_t i, double radianes) const
{
  return static_cast<int32_t>(std::lround(radianes * pasos_por_radian_[i] * sentido_[i]));
}

double BrazoSystemHardware::pasosARadianes(size_t i, int32_t pasos) const
{
  return static_cast<double>(pasos) * sentido_[i] / pasos_por_radian_[i];
}

}  // namespace brazo_hardware

// Registro del plugin para que pluginlib lo pueda cargar por su nombre
PLUGINLIB_EXPORT_CLASS(brazo_hardware::BrazoSystemHardware, hardware_interface::SystemInterface)
