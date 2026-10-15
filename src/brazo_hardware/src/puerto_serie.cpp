// =============================================================================
// puerto_serie.cpp — Implementación de PuertoSerie con termios (Linux)
// =============================================================================
#include "brazo_hardware/puerto_serie.hpp"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace brazo_hardware
{

namespace
{
// Traduce un número de baudios a la constante de termios.
speed_t aVelocidadTermios(int baudios)
{
  switch (baudios) {
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
    case 460800: return B460800;
    case 921600: return B921600;
    default: return B0;  // no soportado
  }
}
}  // namespace

PuertoSerie::~PuertoSerie()
{
  cerrar();
}

bool PuertoSerie::abrir(const std::string & ruta, int baudios)
{
  cerrar();

  const speed_t velocidad = aVelocidadTermios(baudios);
  if (velocidad == B0) {
    error_ = "Velocidad no soportada: " + std::to_string(baudios);
    return false;
  }

  // O_NOCTTY: el puerto no se convierte en terminal de control del proceso
  // O_NONBLOCK: las lecturas devuelven al momento aunque no haya datos
  fd_ = ::open(ruta.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd_ < 0) {
    error_ = "No se pudo abrir " + ruta + ": " + std::strerror(errno);
    return false;
  }

  termios tty{};
  if (tcgetattr(fd_, &tty) != 0) {
    error_ = std::string("tcgetattr: ") + std::strerror(errno);
    cerrar();
    return false;
  }

  // Modo crudo: sin eco, sin procesar caracteres especiales, 8 bits, sin paridad
  cfmakeraw(&tty);
  cfsetispeed(&tty, velocidad);
  cfsetospeed(&tty, velocidad);
  tty.c_cflag |= (CLOCAL | CREAD);  // ignora líneas de módem, habilita lectura
  tty.c_cflag &= ~CSTOPB;           // 1 bit de parada
  tty.c_cflag &= ~CRTSCTS;          // sin control de flujo por hardware
  tty.c_cc[VMIN] = 0;               // read() no espera bytes mínimos
  tty.c_cc[VTIME] = 0;              // ni tiempo mínimo

  if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
    error_ = std::string("tcsetattr: ") + std::strerror(errno);
    cerrar();
    return false;
  }

  vaciarEntrada();
  return true;
}

void PuertoSerie::cerrar()
{
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
  bufer_.clear();
}

bool PuertoSerie::escribirLinea(const std::string & linea)
{
  if (fd_ < 0) {
    error_ = "Puerto cerrado";
    return false;
  }
  const std::string datos = linea + "\n";
  size_t enviados = 0;
  int reintentos = 0;
  while (enviados < datos.size()) {
    const ssize_t n = ::write(fd_, datos.data() + enviados, datos.size() - enviados);
    if (n < 0) {
      if ((errno == EAGAIN || errno == EWOULDBLOCK) && reintentos++ < 100) {
        ::usleep(100);  // búfer de salida lleno: esperamos 0,1 ms y reintentamos
        continue;
      }
      error_ = std::string("write: ") + std::strerror(errno);
      return false;
    }
    enviados += static_cast<size_t>(n);
  }
  return true;
}

std::vector<std::string> PuertoSerie::leerLineas()
{
  std::vector<std::string> lineas;
  if (fd_ < 0) {
    return lineas;
  }

  char temporal[256];
  while (true) {
    const ssize_t n = ::read(fd_, temporal, sizeof(temporal));
    if (n <= 0) {
      break;  // no hay más datos ahora mismo (o error no bloqueante)
    }
    bufer_.append(temporal, static_cast<size_t>(n));
  }

  // Separamos por '\n'; quitamos también '\r' por si el micro usa "\r\n"
  size_t pos;
  while ((pos = bufer_.find('\n')) != std::string::npos) {
    std::string linea = bufer_.substr(0, pos);
    bufer_.erase(0, pos + 1);
    if (!linea.empty() && linea.back() == '\r') {
      linea.pop_back();
    }
    if (!linea.empty()) {
      lineas.push_back(linea);
    }
  }

  // Protección: si llega basura sin '\n' no dejamos crecer el búfer sin límite
  if (bufer_.size() > 4096) {
    bufer_.clear();
  }
  return lineas;
}

void PuertoSerie::vaciarEntrada()
{
  if (fd_ >= 0) {
    tcflush(fd_, TCIFLUSH);
  }
  bufer_.clear();
}

}  // namespace brazo_hardware
