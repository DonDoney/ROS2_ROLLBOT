// =============================================================================
// puerto_serie.hpp — Clase mínima para leer/escribir líneas por puerto serie
// =============================================================================
// Usa directamente la API POSIX de Linux (termios), así no hace falta
// instalar ninguna librería extra. Funciona con:
//   /dev/ttyUSB0  -> placas con chip CH340 / CP2102 (ESP32 DevKit típico)
//   /dev/ttyACM0  -> placas con USB nativo (ESP32-S3, Arduino, Teensy…)
//
// Recuerda dar permisos a tu usuario una sola vez:
//   sudo usermod -aG dialout $USER     (y cerrar sesión / reiniciar)
// =============================================================================
#ifndef BRAZO_HARDWARE__PUERTO_SERIE_HPP_
#define BRAZO_HARDWARE__PUERTO_SERIE_HPP_

#include <string>
#include <vector>

namespace brazo_hardware
{

class PuertoSerie
{
public:
  PuertoSerie() = default;
  ~PuertoSerie();

  // Abre el puerto en modo "crudo" 8N1 sin bloqueo. Devuelve false si falla.
  bool abrir(const std::string & ruta, int baudios);

  // Cierra el puerto si está abierto.
  void cerrar();

  bool estaAbierto() const {return fd_ >= 0;}

  // Envía una línea de texto; añade '\n' al final. Devuelve false si falla.
  bool escribirLinea(const std::string & linea);

  // Lee todo lo que haya disponible SIN bloquear y devuelve las líneas
  // completas recibidas (sin el '\n'). Las líneas incompletas se guardan
  // en un búfer interno hasta que llegue el resto.
  std::vector<std::string> leerLineas();

  // Descarta todo lo pendiente en el búfer de entrada.
  void vaciarEntrada();

  // Último mensaje de error del sistema (para los logs).
  const std::string & ultimoError() const {return error_;}

private:
  int fd_ {-1};
  std::string bufer_;
  std::string error_;
};

}  // namespace brazo_hardware

#endif  // BRAZO_HARDWARE__PUERTO_SERIE_HPP_
