// =============================================================================
// config.h — CONFIGURACIÓN DEL FIRMWARE (pines, mecánica, velocidades)
// =============================================================================
// Todo lo que depende de TU montaje está aquí. Revisa cada sección antes de
// cargar el firmware en la placa.
//
// Placa por defecto: ESP32 DevKit V1 (ESP32-WROOM-32, 38 pines), económica y
// capaz de generar los pulsos de 6 motores por hardware con FastAccelStepper.
//
// Drivers: de bucle cerrado tipo HBS860H / HBS86H / CL86T (NEMA 34 y 23),
// controlados por STEP (PUL), DIR y ENA, con salida de alarma ALM.
// =============================================================================
#pragma once
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Comunicación con el PC (debe coincidir con "baudios" en el URDF)
// -----------------------------------------------------------------------------
#define BAUDIOS_SERIE 115200

// Si no llega ningún comando "M" o "?" en este tiempo con los motores
// habilitados, se paran todos los ejes (protección si ROS se cuelga).
#define WATCHDOG_MS 500

// -----------------------------------------------------------------------------
// Número de ejes
// -----------------------------------------------------------------------------
#define NUM_EJES 6

// -----------------------------------------------------------------------------
// PINES (ESP32 DevKit V1)
// -----------------------------------------------------------------------------
// Pines EVITADOS a propósito:
//   0, 2, 12           -> de arranque (strapping): pueden impedir que el ESP32 arranque
//   1, 3               -> puerto serie USB
//   6-11               -> conectados a la memoria flash interna
//   34, 35, 36, 39     -> SOLO entrada y SIN pull-up interno: necesitan una
//                         resistencia de 10 kΩ a 3,3 V (los usamos para finales de carrera)
//
// IMPORTANTE: el ESP32 trabaja a 3,3 V y las entradas opto-aisladas de los
// drivers suelen pedir 5 V (o 24 V con su selector). Usa un buffer 74HCT245
// alimentado a 5 V entre el ESP32 y los drivers (ver capítulo 10).
const uint8_t PIN_STEP[NUM_EJES] = {13, 14, 16, 17, 18, 19};
const uint8_t PIN_DIR[NUM_EJES]  = {21, 22, 23, 25, 26, 27};

// Un único pin de habilitación compartido por los 6 drivers
#define PIN_ENABLE 4

// En muchos drivers de bucle cerrado, ACTIVAR la señal ENA (que circule
// corriente por el optoacoplador) DESHABILITA el motor. Compruébalo en el
// manual de tu driver y ajusta:
//   true  -> para habilitar los motores el pin se pone a NIVEL BAJO
//   false -> para habilitar los motores el pin se pone a NIVEL ALTO
#define ENABLE_ACTIVO_EN_BAJO false

// Salidas de alarma (ALM) de los 6 drivers cableadas en paralelo a UN pin
// (son de colector abierto, así que se pueden unir: "OR cableado").
// GPIO15 es de arranque pero tolera la pull-up interna sin problemas.
#define PIN_ALARMA 15
// true -> el pin se pone a nivel BAJO cuando un driver está en alarma
#define ALARMA_ACTIVA_EN_BAJO true

// Finales de carrera, uno por eje (-1 = este eje no tiene final de carrera)
// Recomendado: interruptores NORMALMENTE CERRADOS a GND -> si se rompe un
// cable se detecta como "pulsado" (seguridad ante fallos).
const int8_t PIN_FINAL[NUM_EJES] = {32, 33, 34, 35, 36, 39};
// true -> el pin lee ALTO cuando el final de carrera está pulsado (NC a GND + pull-up)
#define FINAL_ACTIVO_EN_ALTO true

// -----------------------------------------------------------------------------
// MECÁNICA (deben coincidir con brazo.ros2_control.xacro)
// -----------------------------------------------------------------------------
// pasos por radián = pasos_por_vuelta_driver * reducción / (2π)
// No hace falta aquí para mover (ROS ya envía pasos), pero sí para entender
// las velocidades de abajo y para el homing.
const float PASOS_POR_VUELTA[NUM_EJES] = {3200, 3200, 3200, 3200, 3200, 3200};
const float REDUCCION[NUM_EJES]        = {20.0, 20.0, 15.0, 10.0, 10.0, 10.0};

// Invertir el sentido de un eje sin recablear (true = invertido)
const bool INVERTIR_DIR[NUM_EJES] = {false, false, false, false, false, false};

// -----------------------------------------------------------------------------
// VELOCIDADES Y ACELERACIONES (en pasos/s y pasos/s²)
// -----------------------------------------------------------------------------
// ROS envía 100 posiciones por segundo ya suavizadas por el planificador; el
// micro solo tiene que seguirlas. Pon la velocidad máxima un poco por ENCIMA
// del límite del URDF para que el seguimiento no se quede atrás:
//   j1: 1.0 rad/s * 3200*20/(2π) ≈ 10 186 pasos/s  -> 12 000
// No superes la frecuencia máxima de entrada de tu driver (p. ej. 200 kHz).
const uint32_t VEL_MAX[NUM_EJES]  = {12000, 12000, 9000, 9000, 9000, 9000};
const uint32_t ACEL_MAX[NUM_EJES] = {40000, 40000, 30000, 40000, 40000, 40000};

// -----------------------------------------------------------------------------
// HOMING (búsqueda de cero con finales de carrera)
// -----------------------------------------------------------------------------
// Orden en que se referencian los ejes (índices 0..5). Primero los que
// levantan el brazo para alejarlo de la mesa, por último la base.
const uint8_t ORDEN_HOMING[NUM_EJES] = {1, 2, 4, 3, 5, 0};

// Sentido de búsqueda del final de carrera: +1 o -1
const int8_t DIR_HOMING[NUM_EJES] = {-1, -1, -1, -1, -1, -1};

// Velocidades de homing (lentas): búsqueda rápida y re-aproximación fina
const uint32_t VEL_HOMING_RAPIDA[NUM_EJES] = {3000, 3000, 2500, 2500, 2500, 2500};
const uint32_t VEL_HOMING_LENTA[NUM_EJES]  = {500, 500, 400, 400, 400, 400};

// Pasos que retrocede después de tocar el final de carrera
const int32_t RETROCESO_HOMING[NUM_EJES] = {2000, 2000, 1500, 1000, 1000, 1000};

// Posición (en pasos) a la que está el final de carrera respecto al cero del
// modelo (brazo vertical). Mídela en tu robot: por ejemplo, si el final de
// carrera del hombro está a -90°:  -PI/2 * 3200*20/(2π) = -16000
const int32_t POSICION_FINAL[NUM_EJES] = {-30222, -16000, -18000, -15111, -10222, -16000};

// Distancia máxima de búsqueda (pasos). Si no encuentra el final de carrera
// en este recorrido, se aborta el homing (¿cable roto? ¿sentido equivocado?).
const int32_t BUSQUEDA_MAXIMA[NUM_EJES] = {70000, 40000, 50000, 40000, 40000, 40000};
