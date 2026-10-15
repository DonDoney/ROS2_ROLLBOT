# Protocolo serie PC ↔ ESP32

Protocolo de **texto**, una orden por línea terminada en `\n`, a **115200 baudios 8N1**.
Se eligió texto (en vez de binario) porque se puede probar con cualquier monitor
serie y es muy fácil de depurar.

Las posiciones siempre van en **pasos absolutos** de motor. La conversión
radianes ↔ pasos la hace el PC (plugin `brazo_hardware`), así el firmware no
necesita saber nada de la cinemática.

## Del PC al ESP32

| Orden | Ejemplo | Qué hace | Respuesta |
|---|---|---|---|
| `M p1 … p6` | `M 1200 -350 0 0 800 0` | Fija la posición objetivo de los 6 ejes | `E …` |
| `?` | `?` | Pide el estado sin mover nada | `E …` |
| `A` | `A` | Habilita los drivers (señal ENA) | `OK A` |
| `D` | `D` | Para todos los ejes y deshabilita los drivers | `OK D` |
| `H` | `H` | Homing de todos los ejes con finales de carrera | `OK H` o `ERR …` |

Mientras dura el homing se puede enviar `D` para abortarlo.

## Del ESP32 al PC

| Mensaje | Ejemplo | Significado |
|---|---|---|
| `E p1 … p6 estado` | `E 1200 -350 0 0 800 0 3` | Posición actual de cada eje (pasos) y campo de estado |
| `OK x` | `OK H` | Orden `x` completada |
| `ERR texto` | `ERR eje 2: final de carrera no encontrado` | Error (el plugin lo muestra en rojo) |
| `LOG texto` | `LOG homing eje 3` | Mensaje informativo |

### Campo `estado` (máscara de bits)

| Bit | Valor | Significado |
|---|---|---|
| 0 | 1 | Drivers habilitados |
| 1 | 2 | Homing completado |
| 2 | 4 | **Alarma** de algún driver (error de seguimiento del bucle cerrado, sobrecorriente…) |
| 3 | 8 | Homing en curso |

Ejemplo: `estado = 3` → habilitado (1) + homing hecho (2).

## Ciclo normal con ROS 2

```
ros2_control (100 Hz)            ESP32
   write()  ── M 1200 -350 … ──▶  moveTo() en cada eje
   read()   ◀── E 1195 -348 … 3 ─  posición actual
```

## Seguridades del firmware

- Arranca **siempre con los drivers deshabilitados**.
- **Watchdog**: si en 500 ms no llega ninguna orden `M` o `?`, frena todos los ejes.
- Si la salida **ALM** de cualquier driver se activa, para todos los ejes al momento
  e ignora órdenes `M` hasta que se rearme el driver.

> ⚠️ Estas seguridades **no sustituyen** a una seta de emergencia física que corte
> la alimentación de potencia de los drivers.
