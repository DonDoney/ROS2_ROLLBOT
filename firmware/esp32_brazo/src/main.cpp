// =============================================================================
// main.cpp — Firmware del brazo ROLLBOT para ESP32
// =============================================================================
// Función: recibir por USB (puerto serie) las posiciones objetivo de los 6 ejes
// que envía ROS 2 (plugin brazo_hardware) y generar los pulsos STEP/DIR para
// los drivers de bucle cerrado. Devuelve la posición actual de cada eje.
//
// La generación de pulsos la hace la librería FastAccelStepper usando los
// periféricos de hardware del ESP32 (MCPWM/PCNT/RMT), así que los pulsos son
// estables aunque el procesador esté ocupado leyendo el puerto serie.
//
// Protocolo de texto, una orden por línea (detalle en ../PROTOCOLO.md):
//   PC -> ESP32 :  "M p1 p2 p3 p4 p5 p6"  "?"  "A"  "D"  "H"
//   ESP32 -> PC :  "E p1 p2 p3 p4 p5 p6 estado"  "OK x"  "ERR texto"  "LOG texto"
// =============================================================================
#include <Arduino.h>
#include <FastAccelStepper.h>

#include "config.h"

// ---------------- Bits del campo "estado" ----------------
static const uint32_t BIT_HABILITADO      = 1u << 0;
static const uint32_t BIT_HOMING_HECHO    = 1u << 1;
static const uint32_t BIT_ALARMA_DRIVER   = 1u << 2;
static const uint32_t BIT_HOMING_EN_CURSO = 1u << 3;

// ---------------- Variables globales ----------------
FastAccelStepperEngine motor_engine;
FastAccelStepper * ejes[NUM_EJES] = {nullptr};

bool habilitado = false;
bool homing_hecho = false;
bool homing_en_curso = false;
uint32_t ultimo_comando_ms = 0;

char linea[128];
uint8_t largo_linea = 0;

// =============================================================================
// Utilidades de E/S
// =============================================================================
bool alarmaActiva()
{
  const int nivel = digitalRead(PIN_ALARMA);
  return ALARMA_ACTIVA_EN_BAJO ? (nivel == LOW) : (nivel == HIGH);
}

bool finalPulsado(uint8_t eje)
{
  if (PIN_FINAL[eje] < 0) {
    return false;
  }
  const int nivel = digitalRead(PIN_FINAL[eje]);
  return FINAL_ACTIVO_EN_ALTO ? (nivel == HIGH) : (nivel == LOW);
}

void escribirEnable(bool habilitar)
{
  // Traduce "habilitar" al nivel eléctrico que espera el driver
  const bool nivel_alto = ENABLE_ACTIVO_EN_BAJO ? !habilitar : habilitar;
  digitalWrite(PIN_ENABLE, nivel_alto ? HIGH : LOW);
  habilitado = habilitar;
}

uint32_t estado()
{
  uint32_t e = 0;
  if (habilitado) e |= BIT_HABILITADO;
  if (homing_hecho) e |= BIT_HOMING_HECHO;
  if (alarmaActiva()) e |= BIT_ALARMA_DRIVER;
  if (homing_en_curso) e |= BIT_HOMING_EN_CURSO;
  return e;
}

// Envía "E p1 p2 p3 p4 p5 p6 estado"
void enviarEstado()
{
  Serial.print('E');
  for (uint8_t i = 0; i < NUM_EJES; i++) {
    Serial.print(' ');
    Serial.print(ejes[i]->getCurrentPosition());
  }
  Serial.print(' ');
  Serial.println(estado());
}

void pararTodo()
{
  for (uint8_t i = 0; i < NUM_EJES; i++) {
    ejes[i]->forceStop();  // parada inmediata (sin rampa)
  }
}

// =============================================================================
// HOMING de un eje: busca el final de carrera, retrocede, se re-aproxima
// despacio y fija la posición conocida del final de carrera.
// Devuelve false si algo falla.
// =============================================================================
bool esperarFinMovimiento(uint8_t eje, bool parar_en_final)
{
  while (ejes[eje]->isRunning()) {
    if (parar_en_final && finalPulsado(eje)) {
      ejes[eje]->forceStop();
      return true;
    }
    if (alarmaActiva()) {
      pararTodo();
      Serial.println("ERR alarma de driver durante el homing");
      return false;
    }
    // Permite abortar desde el PC enviando "D"
    if (Serial.available() && Serial.peek() == 'D') {
      pararTodo();
      Serial.println("ERR homing abortado por el usuario");
      return false;
    }
    delay(1);
  }
  return !parar_en_final;  // si buscábamos el final y se paró solo: no lo encontró
}

bool homingEje(uint8_t eje)
{
  FastAccelStepper * m = ejes[eje];
  Serial.printf("LOG homing eje %u\n", eje + 1);

  if (PIN_FINAL[eje] < 0) {
    // Sin final de carrera: tomamos la posición actual como cero
    m->setCurrentPosition(0);
    Serial.printf("LOG eje %u sin final de carrera: posicion actual = 0\n", eje + 1);
    return true;
  }

  const int8_t dir = DIR_HOMING[eje];

  // 1) Si ya está pulsado, nos separamos primero
  if (finalPulsado(eje)) {
    m->setSpeedInHz(VEL_HOMING_LENTA[eje]);
    m->move(-dir * RETROCESO_HOMING[eje]);
    esperarFinMovimiento(eje, false);
    if (finalPulsado(eje)) {
      Serial.printf("ERR eje %u: el final de carrera sigue pulsado\n", eje + 1);
      return false;
    }
  }

  // 2) Búsqueda rápida
  m->setSpeedInHz(VEL_HOMING_RAPIDA[eje]);
  m->move(dir * BUSQUEDA_MAXIMA[eje]);
  if (!esperarFinMovimiento(eje, true)) {
    Serial.printf("ERR eje %u: final de carrera no encontrado\n", eje + 1);
    return false;
  }

  // 3) Retroceso
  m->setSpeedInHz(VEL_HOMING_LENTA[eje]);
  m->move(-dir * RETROCESO_HOMING[eje]);
  esperarFinMovimiento(eje, false);

  // 4) Aproximación lenta para más precisión
  m->move(dir * (RETROCESO_HOMING[eje] * 2));
  if (!esperarFinMovimiento(eje, true)) {
    Serial.printf("ERR eje %u: fallo en la aproximacion lenta\n", eje + 1);
    return false;
  }

  // 5) Ahora sabemos dónde estamos: en el final de carrera
  m->setCurrentPosition(POSICION_FINAL[eje]);

  // 6) Volver al cero del modelo (brazo vertical en ese eje)
  m->setSpeedInHz(VEL_HOMING_RAPIDA[eje]);
  m->moveTo(0);
  esperarFinMovimiento(eje, false);

  // Restauramos la velocidad normal de trabajo
  m->setSpeedInHz(VEL_MAX[eje]);
  return true;
}

bool homingCompleto()
{
  if (!habilitado) {
    Serial.println("ERR habilita los motores (A) antes del homing");
    return false;
  }
  homing_en_curso = true;
  homing_hecho = false;
  bool ok = true;
  for (uint8_t k = 0; k < NUM_EJES && ok; k++) {
    ok = homingEje(ORDEN_HOMING[k]);
  }
  homing_en_curso = false;
  homing_hecho = ok;
  // Restablecemos velocidades por si el homing se cortó a medias
  for (uint8_t i = 0; i < NUM_EJES; i++) {
    ejes[i]->setSpeedInHz(VEL_MAX[i]);
  }
  return ok;
}

// =============================================================================
// Procesado de órdenes
// =============================================================================
void procesarLinea(char * texto)
{
  switch (texto[0]) {
    case 'M': {
        // "M p1 p2 p3 p4 p5 p6": posiciones absolutas en pasos
        ultimo_comando_ms = millis();
        int32_t objetivo[NUM_EJES];
        char * cursor = texto + 1;
        for (uint8_t i = 0; i < NUM_EJES; i++) {
          char * fin;
          objetivo[i] = strtol(cursor, &fin, 10);
          if (fin == cursor) {
            Serial.println("ERR orden M incompleta");
            return;
          }
          cursor = fin;
        }
        if (habilitado && !homing_en_curso && !alarmaActiva()) {
          for (uint8_t i = 0; i < NUM_EJES; i++) {
            if (objetivo[i] != ejes[i]->targetPos()) {
              ejes[i]->moveTo(objetivo[i]);
            }
          }
        }
        enviarEstado();
        break;
      }

    case '?':
      ultimo_comando_ms = millis();
      enviarEstado();
      break;

    case 'A':
      escribirEnable(true);
      // Fijamos el objetivo en la posición actual para que no haya saltos
      for (uint8_t i = 0; i < NUM_EJES; i++) {
        ejes[i]->moveTo(ejes[i]->getCurrentPosition());
      }
      ultimo_comando_ms = millis();
      Serial.println("OK A");
      break;

    case 'D':
      pararTodo();
      escribirEnable(false);
      Serial.println("OK D");
      break;

    case 'H':
      if (homingCompleto()) {
        Serial.println("OK H");
      }
      break;

    default:
      Serial.printf("ERR orden desconocida: %s\n", texto);
      break;
  }
}

// Lee caracteres del puerto serie y procesa cada línea completa
void leerSerie()
{
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (largo_linea > 0) {
        linea[largo_linea] = '\0';
        procesarLinea(linea);
        largo_linea = 0;
      }
    } else if (largo_linea < sizeof(linea) - 1) {
      linea[largo_linea++] = c;
    } else {
      largo_linea = 0;  // línea demasiado larga: se descarta
    }
  }
}

// =============================================================================
// setup() y loop()
// =============================================================================
void setup()
{
  Serial.begin(BAUDIOS_SERIE);

  pinMode(PIN_ENABLE, OUTPUT);
  escribirEnable(false);  // arrancamos SIEMPRE con los motores deshabilitados

  pinMode(PIN_ALARMA, INPUT_PULLUP);
  for (uint8_t i = 0; i < NUM_EJES; i++) {
    if (PIN_FINAL[i] >= 0) {
      // Los pines 34-39 no tienen pull-up interna: llevan resistencia externa
      pinMode(PIN_FINAL[i], PIN_FINAL[i] >= 34 ? INPUT : INPUT_PULLUP);
    }
  }

  motor_engine.init();
  for (uint8_t i = 0; i < NUM_EJES; i++) {
    ejes[i] = motor_engine.stepperConnectToPin(PIN_STEP[i]);
    if (ejes[i] == nullptr) {
      // Sin esto no podemos funcionar: avisamos sin parar por el puerto
      while (true) {
        Serial.printf("ERR no se pudo asignar el pin STEP %u al eje %u\n", PIN_STEP[i], i + 1);
        delay(1000);
      }
    }
    // dirHighCountsUp: con INVERTIR_DIR cambiamos qué nivel de DIR cuenta hacia arriba.
    // 10 µs de espera tras cambiar DIR: los drivers opto-aislados necesitan
    // unos microsegundos de "setup" antes del siguiente pulso (ver manual del driver).
    ejes[i]->setDirectionPin(PIN_DIR[i], !INVERTIR_DIR[i], 10);
    ejes[i]->setSpeedInHz(VEL_MAX[i]);
    ejes[i]->setAcceleration(ACEL_MAX[i]);
    ejes[i]->setCurrentPosition(0);
  }

  Serial.println("LOG ROLLBOT firmware listo");
}

void loop()
{
  leerSerie();

  // ---- Seguridad 1: alarma de cualquier driver -> parar todo ----
  if (habilitado && alarmaActiva()) {
    pararTodo();
  }

  // ---- Seguridad 2: watchdog de comunicación ----
  if (habilitado && !homing_en_curso && (millis() - ultimo_comando_ms > WATCHDOG_MS)) {
    bool alguno_moviendo = false;
    for (uint8_t i = 0; i < NUM_EJES; i++) {
      if (ejes[i]->isRunning()) {
        ejes[i]->stopMove();  // parada con rampa de deceleración
        alguno_moviendo = true;
      }
    }
    if (alguno_moviendo) {
      Serial.println("LOG watchdog: sin comandos del PC, ejes detenidos");
    }
    ultimo_comando_ms = millis();
  }
}
