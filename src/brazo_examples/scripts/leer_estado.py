#!/usr/bin/env python3
# =============================================================================
# leer_estado.py — Ejemplo 1: suscribirse a /joint_states
# =============================================================================
# Muestra en la terminal la posición de cada articulación EN GRADOS.
# Es el "hola mundo" de un nodo ROS 2 en Python: crear un nodo, suscribirse
# a un topic y procesar los mensajes en una función callback.
#
# Uso (con el robot en marcha en mock, Gazebo o real):
#   ros2 run brazo_examples leer_estado.py
# =============================================================================

import math

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState


class LectorEstado(Node):

    def __init__(self):
        super().__init__('lector_estado')
        # create_subscription(tipo, topic, callback, tamaño de cola)
        self.create_subscription(JointState, '/joint_states', self.al_recibir, 10)
        self.contador = 0
        self.get_logger().info('Escuchando /joint_states ... (Ctrl+C para salir)')

    def al_recibir(self, msg: JointState):
        # /joint_states llega a ~100 Hz; imprimimos solo 2 veces por segundo
        self.contador += 1
        if self.contador % 50 != 0:
            return
        texto = '  '.join(
            f'{nombre}={math.degrees(pos):7.2f}°'
            for nombre, pos in zip(msg.name, msg.position))
        self.get_logger().info(texto)


def main():
    rclpy.init()
    nodo = LectorEstado()
    try:
        rclpy.spin(nodo)
    except KeyboardInterrupt:
        pass
    finally:
        nodo.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
