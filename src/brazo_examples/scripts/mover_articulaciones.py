#!/usr/bin/env python3
# =============================================================================
# mover_articulaciones.py — Ejemplo 2: mover el brazo SIN MoveIt
# =============================================================================
# Envía una trayectoria articular directamente al joint_trajectory_controller
# mediante la ACCIÓN /brazo_controller/follow_joint_trajectory.
#
# Una acción es como un servicio de larga duración: mandas un objetivo,
# recibes feedback mientras se ejecuta y un resultado al terminar.
#
# Ojo: aquí NO se comprueban colisiones. El controlador interpola en línea
# recta en el espacio articular entre los puntos que le des.
#
# Uso:
#   ros2 run brazo_examples mover_articulaciones.py
# =============================================================================

import math

import rclpy
from builtin_interfaces.msg import Duration
from control_msgs.action import FollowJointTrajectory
from rclpy.action import ActionClient
from rclpy.node import Node
from trajectory_msgs.msg import JointTrajectoryPoint

ARTICULACIONES = ['joint_1', 'joint_2', 'joint_3', 'joint_4', 'joint_5', 'joint_6']

# Secuencia de posturas en GRADOS y el instante (s desde el inicio) en que
# el brazo debe alcanzar cada una.
SECUENCIA = [
    ([0, 0, 0, 0, 0, 0], 3.0),         # vertical
    ([45, 20, 60, 0, 60, 0], 7.0),     # inclinado a la izquierda
    ([-45, 20, 60, 0, 60, 90], 12.0),  # inclinado a la derecha, girando la brida
    ([0, 0, 0, 0, 0, 0], 16.0),        # vuelta a vertical
]


def segundos_a_duration(segundos: float) -> Duration:
    entero = int(segundos)
    return Duration(sec=entero, nanosec=int((segundos - entero) * 1e9))


class MoverArticulaciones(Node):

    def __init__(self):
        super().__init__('mover_articulaciones')
        self.cliente = ActionClient(
            self, FollowJointTrajectory, '/brazo_controller/follow_joint_trajectory')

    def enviar(self):
        self.get_logger().info('Esperando al servidor de la acción...')
        if not self.cliente.wait_for_server(timeout_sec=10.0):
            self.get_logger().error('No hay servidor. ¿Está lanzado brazo_controller?')
            return False

        objetivo = FollowJointTrajectory.Goal()
        objetivo.trajectory.joint_names = ARTICULACIONES
        for grados, t in SECUENCIA:
            punto = JointTrajectoryPoint()
            punto.positions = [math.radians(g) for g in grados]
            punto.velocities = [0.0] * len(ARTICULACIONES)  # parar en cada punto
            punto.time_from_start = segundos_a_duration(t)
            objetivo.trajectory.points.append(punto)

        self.get_logger().info(f'Enviando trayectoria de {len(SECUENCIA)} puntos...')
        futuro = self.cliente.send_goal_async(objetivo, feedback_callback=self.al_feedback)
        rclpy.spin_until_future_complete(self, futuro)

        manejador = futuro.result()
        if not manejador.accepted:
            self.get_logger().error('Trayectoria RECHAZADA (¿límites? ¿nombres?)')
            return False

        futuro_resultado = manejador.get_result_async()
        rclpy.spin_until_future_complete(self, futuro_resultado)
        resultado = futuro_resultado.result().result
        if resultado.error_code == FollowJointTrajectory.Result.SUCCESSFUL:
            self.get_logger().info('¡Trayectoria completada!')
            return True
        self.get_logger().error(f'Fallo: código {resultado.error_code} {resultado.error_string}')
        return False

    def al_feedback(self, feedback_msg):
        # Cada feedback trae la posición deseada y la real; mostramos el error de joint_1
        fb = feedback_msg.feedback
        if fb.error.positions:
            self.get_logger().debug(f'error joint_1 = {math.degrees(fb.error.positions[0]):.2f}°')


def main():
    rclpy.init()
    nodo = MoverArticulaciones()
    nodo.enviar()
    nodo.destroy_node()
    rclpy.try_shutdown()


if __name__ == '__main__':
    main()
