# Capítulo 10 · Hardware real: ESP32 + drivers de bucle cerrado + NEMA 23/34

> **Objetivo:** pasar de la simulación al brazo físico de forma segura y por
> etapas, probando cada pieza antes de juntarlas.

> ⚠️ **Seguridad primero.** Un brazo con NEMA 34 y reductoras tiene fuerza
> suficiente para hacer daño. Antes de energizar: seta de emergencia que corte
> la **potencia** de los drivers, zona de trabajo despejada y primeras pruebas
> **sin carga** y a velocidad reducida.

## Arquitectura

```
┌──────────────── PC Ubuntu 24.04 + ROS 2 Jazzy ────────────────┐
│ MoveIt 2 → brazo_controller → plugin brazo_hardware (C++)     │
└───────────────────────────┬───────────────────────────────────┘
                            │ USB · texto 115200 baudios (firmware/PROTOCOLO.md)
                   ┌────────▼────────┐
                   │  ESP32 DevKit   │  FastAccelStepper: pulsos por hardware
                   └──┬───┬───┬───┬──┘
         STEP/DIR/ENA │   │   │   │  ALM (alarma) · finales de carrera
                ┌─────▼───▼───▼───▼──────┐
                │ buffer 74HCT245 (5 V)  │  adapta 3,3 V → 5 V
                └─────┬──────────────────┘
       ┌──────────────┼───────────────────────────┐
  ┌────▼────┐    ┌────▼────┐             ┌────────▼┐
  │ HBS860H │    │ HBS860H │    …        │ HBS57   │   drivers de bucle cerrado
  └────┬────┘    └────┬────┘             └────┬────┘
   NEMA 34 + enc  NEMA 34 + enc           NEMA 23 + enc
```

## Elegir el microcontrolador

Necesitas generar **6 trenes de pulsos** de hasta ~15 kHz cada uno, con
rampas, mientras atiendes el puerto serie. Comparativa de opciones económicas:

| Placa | Precio aprox. | ¿6 ejes con FastAccelStepper? | Comentario |
|---|---|---|---|
| **ESP32 DevKit V1** | 5-10 € | ✅ Sí (periféricos MCPWM/PCNT/RMT) | **Recomendada.** Es la configuración por defecto del firmware |
| ESP32-S3 DevKit | 8-15 € | ✅ Sí | Más pines libres y USB nativo (`/dev/ttyACM0`); ajusta los pines en `config.h` |
| Arduino Mega | 15-40 € | ❌ No (pocos temporizadores) | Se queda corto para 6 ejes simultáneos |
| Teensy 4.1 | 35-45 € | Otra librería (TeensyStep) | Muy potente, pero exige reescribir el firmware |

Por eso el firmware de este repositorio está hecho para **ESP32 DevKit V1**.

## Drivers de bucle cerrado

Los HBS860H (y similares: HBS86H, CL86T para NEMA 34; HBS57, CL57T para NEMA 23)
leen el encoder del motor y corrigen solos los pasos perdidos. Para el ESP32
se comportan como un driver de pasos normal (**PUL/DIR/ENA**) más una salida
de **alarma (ALM)** que se activa si el error de posición es demasiado grande.

Configura en cada driver (interruptores DIP o software del fabricante):

1. **Pulsos por vuelta**: 3200 para empezar (debe coincidir con `pasos_por_vuelta`
   en `brazo.ros2_control.xacro` y con `PASOS_POR_VUELTA` en `config.h`).
2. **Corriente**: según la placa del motor.
3. **Nivel de señal de entrada**: 5 V (algunos traen opción de 24 V).

## Cableado (ESP32 DevKit V1)

| Señal | Eje 1 | Eje 2 | Eje 3 | Eje 4 | Eje 5 | Eje 6 |
|---|---|---|---|---|---|---|
| STEP | GPIO 13 | GPIO 14 | GPIO 16 | GPIO 17 | GPIO 18 | GPIO 19 |
| DIR | GPIO 21 | GPIO 22 | GPIO 23 | GPIO 25 | GPIO 26 | GPIO 27 |
| Final de carrera | GPIO 32 | GPIO 33 | GPIO 34* | GPIO 35* | GPIO 36* | GPIO 39* |

| Señal común | Pin |
|---|---|
| ENA (los 6 drivers en paralelo) | GPIO 4 |
| ALM (los 6 drivers, ver nota) | GPIO 15 |

\* GPIO 34-39 son **solo entrada sin pull-up interna**: pon una resistencia de
10 kΩ de cada pin a 3,3 V.

Todos los pines se cambian en [`firmware/esp32_brazo/include/config.h`](../../firmware/esp32_brazo/include/config.h).

### Niveles lógicos (¡importante!)

El ESP32 da **3,3 V** y las entradas opto-aisladas de estos drivers esperan
normalmente **5 V**. Con 3,3 V directos pueden funcionar de forma
intermitente. Solución recomendada: un **74HCT245** alimentado a 5 V entre el
ESP32 y los drivers (la serie **HCT** acepta 3,3 V como "1" lógico).

Conexión en **cátodo común** (la más simple):

```
74HCT245 salida (5 V) ──▶ PUL+      GND común ──▶ PUL-
74HCT245 salida (5 V) ──▶ DIR+      GND común ──▶ DIR-
74HCT245 salida (5 V) ──▶ ENA+      GND común ──▶ ENA-
```

Comprueba en la hoja de datos de tu driver la corriente que necesita cada
entrada: si el 74HCT245 no da suficiente, usa un buffer con más corriente o
una placa adaptadora de entradas para drivers.

### Alarma (ALM)

La salida ALM es un transistor de colector abierto. Conecta **ALM+** al GPIO 15
(con la pull-up interna activada) y **ALM-** a GND. Si el transistor de tu
driver **conduce cuando hay alarma**, puedes unir las 6 salidas en paralelo
(cualquier alarma pone el pin a 0). Si conduce en estado normal, cablea los 6
en **serie**. Ajusta `ALARMA_ACTIVA_EN_BAJO` en `config.h` según tu caso.

### Finales de carrera

Usa interruptores **normalmente cerrados (NC) a GND**: si se corta un cable,
el sistema lo ve como "pulsado" y no sigue moviéndose (*fail-safe*).

## Puesta en marcha por etapas

### Etapa 1 · Cargar el firmware

```bash
pip install platformio          # o la extensión PlatformIO de VS Code
cd firmware/esp32_brazo
pio run -t upload
pio device monitor              # debe mostrar: LOG ROLLBOT firmware listo
```

### Etapa 2 · Probar cada motor sin ROS

```bash
ros2 run brazo_hardware probar_serie.py --puerto /dev/ttyUSB0
>> A            # habilita drivers
>> g 1 10       # eje 1 a +10 grados
>> g 1 0        # vuelta a 0
>> ?            # estado
>> D            # deshabilita
```

Para cada eje comprueba:

- [ ] Gira en el **sentido correcto** (positivo = regla de la mano derecha sobre
      el eje del URDF). Si no, cambia `INVERTIR_DIR` en `config.h`.
- [ ] **10 grados pedidos son 10 grados reales** (si no, revisa pulsos/vuelta y reducción).
- [ ] El final de carrera se detecta (`?` con el interruptor pulsado a mano).

### Etapa 3 · Homing

Configura en `config.h` el sentido de búsqueda (`DIR_HOMING`) y la posición
real de cada final de carrera respecto al brazo vertical (`POSICION_FINAL`).
Después:

```
>> A
>> H            # mantén la mano en la seta de emergencia
```

### Etapa 4 · ROS 2 con el robot real

```bash
ros2 launch brazo_bringup brazo.launch.py modo_hardware:=real puerto_serie:=/dev/ttyUSB0
```

El plugin abre el puerto, habilita los drivers, hace el homing
(`homing_al_activar: true`) y activa los controladores. Prueba primero:

```bash
ros2 run brazo_examples mover_articulaciones.py
```

### Etapa 5 · MoveIt con el robot real

```bash
# Terminal 1
ros2 launch brazo_bringup brazo.launch.py modo_hardware:=real rviz:=false
# Terminal 2
ros2 launch brazo_moveit_config move_group.launch.py
```

`joint_limits.yaml` limita la velocidad al 10 % por defecto. Súbela poco a poco.

## Lista de seguridad

- [ ] Seta de emergencia que **corta la alimentación de potencia** de los drivers.
- [ ] Fuente lógica (5 V del buffer, ESP32) separada de la de potencia.
- [ ] Masas (GND) de lógica unidas en un punto; potencia aislada por los optoacopladores.
- [ ] Primeras pruebas sin carga y con corriente de driver reducida.
- [ ] Ejes 2 y 3 (los que cargan peso): considera **freno electromagnético**
      o reductoras no reversibles; al deshabilitar, el brazo puede caer.

⬅️ [Capítulo 09](../09_moveit2/README.md) · ➡️ [Capítulo 11 · Programación](../11_programacion_ejemplos/README.md)
