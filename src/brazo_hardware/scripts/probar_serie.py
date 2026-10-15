#!/usr/bin/env python3
# =============================================================================
# probar_serie.py — Consola para probar el firmware del ESP32 SIN ROS
# =============================================================================
# Sirve para la primera puesta en marcha del hardware: comprobar el cableado,
# el sentido de giro de cada motor, los finales de carrera y las alarmas,
# antes de meter ros2_control en medio.
#
# Uso:
#   ros2 run brazo_hardware probar_serie.py --puerto /dev/ttyUSB0
#   (o directamente: python3 probar_serie.py --puerto /dev/ttyUSB0)
#
# Comandos dentro de la consola:
#   ?                     -> pide el estado (posición en pasos de cada eje)
#   A / D                 -> habilitar / deshabilitar drivers
#   H                     -> homing con finales de carrera
#   M p1 p2 p3 p4 p5 p6   -> mover a posiciones absolutas en PASOS
#   g <eje 1-6> <grados>  -> mover UN eje a un ángulo absoluto (usa la tabla de abajo)
#   salir                 -> terminar
#
# ¡Seguridad! Empieza con la alimentación de potencia limitada y el brazo sin
# carga, con la seta de emergencia a mano.
# =============================================================================

import argparse
import math
import threading
import time

import serial  # paquete python3-serial (pyserial)

# Pasos por vuelta del driver y reducción de cada eje (igual que en el URDF:
# brazo_description/urdf/brazo.ros2_control.xacro)
MECANICA = [
    (3200, 20.0),   # joint_1
    (3200, 20.0),   # joint_2
    (3200, 15.0),   # joint_3
    (3200, 10.0),   # joint_4
    (3200, 10.0),   # joint_5
    (3200, 10.0),   # joint_6
]

ultima_posicion = [0] * 6  # pasos, actualizada con cada línea "E ..."


def pasos_por_grado(eje):
    pasos_vuelta, reduccion = MECANICA[eje]
    return pasos_vuelta * reduccion / 360.0


def hilo_lector(puerto):
    """Imprime todo lo que llega del micro y guarda la última posición."""
    global ultima_posicion
    while True:
        try:
            linea = puerto.readline().decode(errors='replace').strip()
        except serial.SerialException:
            print('\n[!] Puerto desconectado')
            return
        if not linea:
            continue
        if linea.startswith('E '):
            partes = linea.split()
            try:
                ultima_posicion = [int(x) for x in partes[1:7]]
                estado = int(partes[7])
            except (ValueError, IndexError):
                print(f'<< {linea}')
                continue
            grados = [p / pasos_por_grado(i) for i, p in enumerate(ultima_posicion)]
            texto = '  '.join(f'j{i+1}={g:7.2f}°' for i, g in enumerate(grados))
            banderas = []
            if estado & 1:
                banderas.append('HABILITADO')
            if estado & 2:
                banderas.append('HOMING_OK')
            if estado & 4:
                banderas.append('¡ALARMA!')
            if estado & 8:
                banderas.append('HOMING...')
            print(f'<< {texto}  [{", ".join(banderas)}]')
        else:
            print(f'<< {linea}')


def main():
    parser = argparse.ArgumentParser(description='Consola de pruebas del firmware ROLLBOT')
    parser.add_argument('--puerto', default='/dev/ttyUSB0')
    parser.add_argument('--baudios', type=int, default=115200)
    args = parser.parse_args()

    puerto = serial.Serial(args.puerto, args.baudios, timeout=0.1)
    print(f'Conectado a {args.puerto}. Esperando a que arranque el ESP32...')
    time.sleep(2.0)
    puerto.reset_input_buffer()

    threading.Thread(target=hilo_lector, args=(puerto,), daemon=True).start()
    puerto.write(b'?\n')

    print("Escribe un comando ('salir' para terminar).")
    while True:
        try:
            orden = input('>> ').strip()
        except (EOFError, KeyboardInterrupt):
            break
        if not orden:
            continue
        if orden.lower() == 'salir':
            break

        # Atajo: g <eje> <grados>
        if orden.lower().startswith('g '):
            try:
                _, eje_txt, grados_txt = orden.split()
                eje = int(eje_txt) - 1
                grados = float(grados_txt)
                assert 0 <= eje < 6
            except (ValueError, AssertionError):
                print('Uso: g <eje 1-6> <grados>')
                continue
            objetivo = list(ultima_posicion)
            objetivo[eje] = int(round(grados * pasos_por_grado(eje)))
            orden = 'M ' + ' '.join(str(p) for p in objetivo)
            print(f'   ({grados}° = {objetivo[eje]} pasos, '
                  f'{math.radians(grados):.3f} rad)')

        puerto.write((orden + '\n').encode())

    puerto.write(b'D\n')  # por seguridad, deshabilitamos al salir
    puerto.close()
    print('Drivers deshabilitados. Adiós.')


if __name__ == '__main__':
    main()
