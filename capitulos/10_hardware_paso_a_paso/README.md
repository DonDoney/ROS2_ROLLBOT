# Capítulo 10 · El brazo real: ESP32, drivers de bucle cerrado y motores NEMA

| Nivel | Tiempo aproximado | Necesitas |
|---|---|---|
| 🟡 Medio → 🔴 Avanzado | Varias sesiones | Capítulos 07-09, conocimientos básicos de electrónica y el material de la lista |

---

> ⚠️ **ANTES DE EMPEZAR: SEGURIDAD**
>
> Un brazo con motores NEMA 34 y reductoras tiene **fuerza suficiente para
> hacer daño** a una persona y para romperse a sí mismo. Sigue siempre estas reglas:
>
> 1. Instala una **seta de emergencia** que corte la **alimentación de potencia** de los drivers.
> 2. Haz las primeras pruebas **sin carga**, con **poca corriente** en los drivers y **a baja velocidad**.
> 3. **Nunca** te pongas dentro del radio de alcance del brazo mientras esté habilitado.
> 4. Trabaja con la alimentación de potencia **apagada** cuando toques cables.

---

## 1. Introducción

Hasta ahora todo ha pasado dentro del ordenador. En este capítulo conectamos
el software con el mundo real. La cadena completa es esta:

1. **MoveIt** planifica un movimiento (capítulo 09).
2. **ros2_control** lo convierte en 100 posiciones por segundo (capítulo 07).
3. Nuestro plugin **`brazo_hardware`** envía esas posiciones por USB a una placa **ESP32**.
4. El **ESP32** genera los **pulsos eléctricos** que mueven cada motor.
5. Los **drivers de bucle cerrado** amplifican esos pulsos y vigilan que el motor no pierda pasos.
6. Los **motores NEMA** mueven las articulaciones.

Lo haremos **por etapas**, probando cada pieza por separado antes de juntarlas.
Es la forma más segura y la que ahorra más tiempo buscando fallos.

---

## 2. Objetivos

Al terminar este capítulo serás capaz de:

- [ ] Explicar el papel de cada componente: PC, ESP32, driver y motor.
- [ ] Elegir el microcontrolador y entender por qué recomendamos el ESP32.
- [ ] Cablear las señales STEP, DIR, ENA, ALM y los finales de carrera.
- [ ] Cargar el firmware en el ESP32 y probar cada motor sin ROS.
- [ ] Hacer el *homing* (búsqueda de la posición cero).
- [ ] Mover el brazo real desde ROS 2 y desde MoveIt.

---

## 3. Conceptos clave

| Concepto | Explicación sencilla |
|---|---|
| **Motor paso a paso (NEMA)** | Motor que gira en "pasitos" fijos. NEMA 23 y NEMA 34 indican el **tamaño** de la cara frontal (2,3" y 3,4") |
| **Driver** | Aparato que recibe señales débiles del microcontrolador y da al motor la corriente que necesita |
| **Bucle cerrado** | El motor lleva un **encoder** (sensor de posición) y el driver corrige solo si el motor se retrasa. Si no puede corregir, activa una **alarma** |
| **Microcontrolador (ESP32)** | Ordenador diminuto que genera los pulsos con precisión de microsegundos |
| **Firmware** | El programa que se carga dentro del microcontrolador |
| **STEP (PUL)** | Señal de pulsos: **cada pulso = un paso** del motor |
| **DIR** | Señal de dirección: nivel alto = un sentido, nivel bajo = el otro |
| **ENA** | Señal de habilitación: activa o desactiva el motor (sin ella, el motor queda "suelto") |
| **ALM** | Salida de alarma del driver: avisa si el motor no ha podido seguir la orden |
| **Pulsos por vuelta** | Cuántos pulsos necesita el motor para dar una vuelta (se configura en el driver, p. ej. 3200) |
| **Reductora** | Engranaje que reduce la velocidad y multiplica la fuerza (20:1 = 20 vueltas de motor por 1 de la articulación) |
| **Final de carrera** | Interruptor que se pulsa cuando una articulación llega a un punto conocido |
| **Homing** | Proceso de buscar los finales de carrera para que el robot sepa dónde está al encenderse |
| **Nivel lógico** | Tensión que representa un "1": el ESP32 usa **3,3 V**, los drivers suelen esperar **5 V** |
| **Puerto serie** | Comunicación por USB entre el PC y el ESP32, línea a línea de texto |
| **Watchdog** | "Perro guardián": si el PC deja de enviar órdenes, el firmware para los motores |

### La cuenta más importante: radianes ↔ pasos

```
pasos por vuelta de la articulación = pasos_por_vuelta_del_driver × reducción

Ejemplo, joint_1: 3200 pasos/vuelta × reductora 20 = 64 000 pasos por vuelta
                  → 1 grado = 64 000 / 360 ≈ 178 pasos
```

El plugin hace esta conversión automáticamente con los valores de
[`brazo.ros2_control.xacro`](../../src/brazo_description/urdf/brazo.ros2_control.xacro).

---

## 4. Estructura

### La cadena completa

```
┌──────────────── PC · Ubuntu 24.04 + ROS 2 Jazzy ────────────────┐
│  MoveIt 2 → brazo_controller → plugin brazo_hardware (C++)      │
└────────────────────────────┬────────────────────────────────────┘
                             │ cable USB · texto a 115200 baudios
                    ┌────────▼────────┐
                    │  ESP32 DevKit   │  firmware: genera los pulsos
                    └──┬───┬───┬───┬──┘
         STEP/DIR/ENA  │   │   │   │   ← ALM (alarma) y finales de carrera
                 ┌─────▼───▼───▼───▼─────┐
                 │ 74HCT245 (3,3 V → 5 V) │  adapta la tensión
                 └─────┬─────────────────┘
        ┌──────────────┼──────────────────────────┐
   ┌────▼────┐    ┌────▼────┐             ┌────────▼┐
   │ HBS860H │    │ HBS860H │     …       │ HBS57   │   drivers de bucle cerrado
   └────┬────┘    └────┬────┘             └────┬────┘
    NEMA 34         NEMA 34                 NEMA 23     motores con encoder
```

### Archivos implicados

```
firmware/
├── PROTOCOLO.md                    ← "idioma" que hablan el PC y el ESP32
└── esp32_brazo/
    ├── platformio.ini              ← configuración del proyecto del firmware
    ├── include/config.h            ← ✏️ pines, mecánica, velocidades, homing (TODO lo ajustable)
    └── src/main.cpp                ← el firmware (comentado)

src/brazo_hardware/
├── src/brazo_system.cpp            ← plugin de ros2_control (lado del PC)
├── src/puerto_serie.cpp            ← lectura/escritura del puerto USB
└── scripts/probar_serie.py         ← consola para probar el ESP32 SIN ROS
```

### El "idioma" entre el PC y el ESP32

Son líneas de texto muy simples (detalle en [`PROTOCOLO.md`](../../firmware/PROTOCOLO.md)):

| El PC envía | Significado | El ESP32 responde |
|---|---|---|
| `A` | Habilitar los motores | `OK A` |
| `H` | Hacer el homing | `OK H` |
| `M 1200 -350 0 0 800 0` | Ir a estas posiciones (en pasos) | `E 1195 -348 0 0 798 0 3` |
| `?` | ¿Dónde estás? | `E …` |
| `D` | Deshabilitar los motores | `OK D` |

En la respuesta `E`, los 6 primeros números son la posición actual de cada
eje y el último es el **estado**: 1 = habilitado, 2 = homing hecho,
4 = **alarma**, 8 = haciendo homing (se suman: `3` = habilitado + homing hecho).

---

## 5. Proceso detallado

### Paso 1 · Elegir el microcontrolador

Hay que generar **6 trenes de pulsos a la vez**, cada uno de hasta unos
15 000 pulsos por segundo, con aceleraciones suaves.

| Placa | Precio aprox. | ¿Sirve para 6 ejes? | Comentario |
|---|---|---|---|
| **ESP32 DevKit V1** | 5-10 € | ✅ Sí | **Recomendada.** Genera los pulsos con su hardware interno. Es la que usa el firmware |
| ESP32-S3 DevKit | 8-15 € | ✅ Sí | Más pines libres; hay que cambiar los pines en `config.h` |
| Arduino Mega | 15-40 € | ❌ No | No tiene suficientes temporizadores para 6 ejes a la vez |
| Teensy 4.1 | 35-45 € | ⚠️ Con otro firmware | Muy potente, pero habría que reescribir el firmware |

### Paso 2 · Configurar los drivers

En cada driver (con sus interruptores DIP o su software):

1. **Pulsos por vuelta:** `3200` para empezar. Debe coincidir con
   `pasos_por_vuelta` en `brazo.ros2_control.xacro` y con `PASOS_POR_VUELTA` en `config.h`.
2. **Corriente:** la que indica la placa del motor. Para las primeras pruebas, algo menos.
3. **Nivel de señal:** 5 V (algunos drivers tienen opción de 24 V).

### Paso 3 · Cablear

**Tabla de pines del ESP32** (se pueden cambiar en `config.h`):

| Señal | Eje 1 | Eje 2 | Eje 3 | Eje 4 | Eje 5 | Eje 6 |
|---|---|---|---|---|---|---|
| STEP | GPIO 13 | GPIO 14 | GPIO 16 | GPIO 17 | GPIO 18 | GPIO 19 |
| DIR | GPIO 21 | GPIO 22 | GPIO 23 | GPIO 25 | GPIO 26 | GPIO 27 |
| Final de carrera | GPIO 32 | GPIO 33 | GPIO 34 * | GPIO 35 * | GPIO 36 * | GPIO 39 * |

| Señal común | Pin |
|---|---|
| ENA (los 6 drivers unidos) | GPIO 4 |
| ALM (alarmas de los 6 drivers) | GPIO 15 |

\* Los GPIO 34 a 39 **no tienen resistencia interna**: pon una resistencia de
**10 kΩ** desde cada uno de esos pines a **3,3 V**.

**Adaptar la tensión (3,3 V → 5 V).** El ESP32 da 3,3 V y los drivers suelen
esperar 5 V. Con 3,3 V directos pueden funcionar "a ratos", que es lo peor que
puede pasar. Pon un chip **74HCT245** alimentado a 5 V entre el ESP32 y los
drivers. Conexión en **cátodo común**:

```
Salida del 74HCT245 (5 V) ──▶ PUL+        GND común ──▶ PUL-
Salida del 74HCT245 (5 V) ──▶ DIR+        GND común ──▶ DIR-
Salida del 74HCT245 (5 V) ──▶ ENA+        GND común ──▶ ENA-
```

> 🔎 Consulta en la hoja de datos de tu driver cuánta corriente necesita cada
> entrada. Si el 74HCT245 no la da, usa un buffer más potente o una placa
> adaptadora para drivers.

**Alarma (ALM).** Conecta `ALM+` al GPIO 15 y `ALM-` a GND. Según el driver, el
transistor de alarma **conduce cuando hay alarma** (entonces puedes unir los
6 en paralelo) o **conduce en estado normal** (entonces únelos en serie).
Ajusta `ALARMA_ACTIVA_EN_BAJO` en `config.h`.

**Finales de carrera.** Usa interruptores **normalmente cerrados (NC)** entre
el pin y GND. Así, si se rompe un cable, el sistema lo ve como "pulsado" y se
detiene en lugar de seguir a ciegas.

**Alimentación.** Separa la fuente de **lógica** (5 V para ESP32 y 74HCT245) de
la de **potencia** (la de los drivers). La seta de emergencia corta la de potencia.

### Paso 4 · Revisar `config.h` antes de cargar el firmware

Abre [`config.h`](../../firmware/esp32_brazo/include/config.h). Lo más importante:

```cpp
// ¿Cómo se HABILITA tu driver? En muchos de bucle cerrado, activar ENA lo DESHABILITA.
#define ENABLE_ACTIVO_EN_BAJO false

// ¿La alarma pone el pin a nivel bajo?
#define ALARMA_ACTIVA_EN_BAJO true

// Mecánica: debe coincidir con brazo.ros2_control.xacro
const float PASOS_POR_VUELTA[NUM_EJES] = {3200, 3200, 3200, 3200, 3200, 3200};
const float REDUCCION[NUM_EJES]        = {20.0, 20.0, 15.0, 10.0, 10.0, 10.0};
```

### Paso 5 · Cargar el firmware

Usaremos **PlatformIO**, que descarga solo todo lo necesario para el ESP32.

```bash
pip install platformio                      # o instala la extensión "PlatformIO IDE" en VS Code
cd ~/rollbot_ws/firmware/esp32_brazo
pio run -t upload                           # compila y carga (la placa conectada por USB)
pio device monitor                          # abre el monitor serie
# Debe aparecer:  LOG ROLLBOT firmware listo
# Sal con Ctrl + C
```

### Paso 6 · Probar cada motor SIN ROS

Con la **potencia encendida**, el brazo sin carga y la mano en la seta:

```bash
ros2 run brazo_hardware probar_serie.py --puerto /dev/ttyUSB0
>> A            # habilita los drivers
>> g 1 10       # eje 1 a +10 grados
>> g 1 0        # eje 1 vuelve a 0
>> ?            # muestra la posición de todos los ejes
>> D            # deshabilita
>> salir
```

Haz esta lista **eje por eje**:

- [ ] **Sentido de giro:** al pedir grados positivos, ¿gira hacia donde dice el
      modelo en RViz? Si gira al revés, cambia `INVERTIR_DIR` en `config.h`.
- [ ] **Escala:** si pides 10°, ¿gira 10° de verdad? Si no, revisa pulsos por
      vuelta y reducción.
- [ ] **Final de carrera:** pulsa el interruptor con la mano y escribe `?`.
- [ ] **Alarma:** si salta `¡ALARMA!`, baja la velocidad o revisa la mecánica.

### Paso 7 · El homing

Configura en `config.h`:

- `DIR_HOMING`: hacia qué lado busca cada eje su final de carrera (+1 o -1).
- `POSICION_FINAL`: dónde está el final de carrera respecto al brazo vertical,
  **en pasos**. Ejemplo: si el final del hombro está a -90°:
  `-90° → -16 000 pasos` (3200 × 20 × 90 / 360).

Prueba en la consola:

```
>> A
>> H            # ¡mano en la seta de emergencia!
```

El firmware busca los finales de carrera eje por eje (primero los que levantan
el brazo, la base al final) y deja el brazo en posición vertical.

### Paso 8 · El brazo real con ROS 2

```bash
ros2 launch brazo_bringup brazo.launch.py modo_hardware:=real puerto_serie:=/dev/ttyUSB0
```

El plugin abre el puerto, habilita los drivers, **hace el homing** y activa los
controladores. Prueba con el programa de siempre:

```bash
ros2 run brazo_examples mover_articulaciones.py
```

### Paso 9 · El brazo real con MoveIt

```bash
# Terminal 1
ros2 launch brazo_bringup brazo.launch.py modo_hardware:=real rviz:=false
# Terminal 2
ros2 launch brazo_moveit_config move_group.launch.py
```

La velocidad empieza al 10 %. Súbela **poco a poco**.

---

## 6. Ejemplos

### Ejemplo 1 · Hablar con el ESP32 "a mano"

Con `pio device monitor` abierto, escribe `?` y pulsa Intro. Verás algo como
`E 0 0 0 0 0 0 0`: los seis ejes en 0 y el estado 0 (deshabilitado, sin homing).

### Ejemplo 2 · Calcular los pasos de una postura

¿Cuántos pasos son 45° en el eje 3 (3200 pasos/vuelta, reductora 15)?

```
pasos = 45 / 360 × 3200 × 15 = 6000 pasos
```

Compruébalo con `probar_serie.py`: `g 3 45` debe mostrar "6000 pasos".

### Ejemplo 3 · Probar el watchdog

Con el brazo en marcha desde ROS, cierra el launch con `Ctrl + C` a mitad de
un movimiento. En menos de medio segundo el firmware frenará todos los ejes.

---

## 7. Cierre

### Resumen

- La cadena es: MoveIt → ros2_control → plugin → USB → ESP32 → driver → motor.
- El **ESP32** genera los pulsos; los **drivers de bucle cerrado** vigilan que el motor no pierda pasos.
- La **mecánica** (pasos y reductoras) debe coincidir en `config.h` y en `brazo.ros2_control.xacro`.
- Se prueba **por etapas**: firmware → cada motor → homing → ROS → MoveIt.
- La **seguridad física** (seta de emergencia) no se sustituye con software.

### Lista de seguridad final

- [ ] Seta de emergencia que **corta la potencia** de los drivers.
- [ ] Fuente de lógica separada de la de potencia.
- [ ] Primeras pruebas sin carga, con poca corriente y a baja velocidad.
- [ ] Ejes 2 y 3 (soportan el peso): plantéate **frenos electromagnéticos** o
      reductoras que no se muevan solas; al deshabilitar, el brazo podría caer.

### Si algo falla

| Síntoma | Causa probable | Solución |
|---|---|---|
| `No se pudo abrir /dev/ttyUSB0: Permission denied` | Tu usuario no está en `dialout` | `sudo usermod -aG dialout $USER` y reinicia sesión |
| `El microcontrolador no responde` | Puerto equivocado o firmware sin cargar | `ls /dev/ttyUSB* /dev/ttyACM*` y repite el paso 5 |
| El motor vibra pero no gira | Señales a 3,3 V, cableado o corriente insuficiente | Revisa el 74HCT245 y la corriente del driver |
| Gira al revés | Sentido invertido | `INVERTIR_DIR` en `config.h` |
| Salta la alarma del driver | Demasiada velocidad/aceleración o mecánica atascada | Baja `VEL_MAX` y `ACEL_MAX`; revisa la mecánica |
| `ERR eje N: final de carrera no encontrado` | Busca en el sentido equivocado o cable roto | Cambia `DIR_HOMING` o revisa el interruptor |

➡️ **Siguiente capítulo:** [11 · Programar el brazo](../11_programacion_ejemplos/README.md),
donde escribirás tus propios programas.
