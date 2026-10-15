#!/usr/bin/env python3
# =============================================================================
# brazo.launch.py — Arranca el brazo con ros2_control (modo mock o real)
# =============================================================================
# Pone en marcha:
#   1) robot_state_publisher  -> publica /robot_description y las TF
#   2) ros2_control_node      -> controller_manager + plugin de hardware
#   3) spawner joint_state_broadcaster -> publica /joint_states
#   4) spawner brazo_controller        -> acepta trayectorias (action FollowJointTrajectory)
#   5) rviz2 (opcional)
#
# Ejemplos:
#   # Sin hardware (las articulaciones "obedecen" al instante):
#   ros2 launch brazo_bringup brazo.launch.py
#
#   # Con el brazo real conectado al ESP32 por USB:
#   ros2 launch brazo_bringup brazo.launch.py modo_hardware:=real puerto_serie:=/dev/ttyUSB0
#
# Para la simulación en Gazebo usa brazo_gazebo/launch/gazebo.launch.py
# (allí el controller_manager lo crea el propio Gazebo).
# =============================================================================

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, RegisterEventHandler
from launch.conditions import IfCondition
from launch.event_handlers import OnProcessExit
from launch.substitutions import Command, FindExecutable, LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    desc_share = get_package_share_directory('brazo_description')
    bringup_share = get_package_share_directory('brazo_bringup')

    archivo_xacro = os.path.join(desc_share, 'urdf', 'brazo.urdf.xacro')
    archivo_controladores = os.path.join(bringup_share, 'config', 'brazo_controladores.yaml')
    archivo_rviz = os.path.join(desc_share, 'rviz', 'brazo.rviz')

    # ---------------- Argumentos ----------------
    argumentos = [
        DeclareLaunchArgument('modo_hardware', default_value='mock',
                              description='mock (sin hardware) | real (ESP32 + drivers)'),
        DeclareLaunchArgument('puerto_serie', default_value='/dev/ttyUSB0',
                              description='Puerto del microcontrolador (solo modo real)'),
        DeclareLaunchArgument('baudios', default_value='115200',
                              description='Velocidad del puerto serie (solo modo real)'),
        DeclareLaunchArgument('rviz', default_value='true',
                              description='Abrir RViz'),
    ]

    # ---------------- URDF generado desde xacro con los argumentos elegidos ----------------
    robot_description = {
        'robot_description': ParameterValue(
            Command([
                FindExecutable(name='xacro'), ' ', archivo_xacro,
                ' modo_hardware:=', LaunchConfiguration('modo_hardware'),
                ' puerto_serie:=', LaunchConfiguration('puerto_serie'),
                ' baudios:=', LaunchConfiguration('baudios'),
            ]),
            value_type=str)
    }

    nodo_rsp = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        output='screen',
        parameters=[robot_description])

    # ---------------- controller_manager ----------------
    # En Jazzy el controller_manager recibe el URDF por el topic /robot_description
    # (lo publica robot_state_publisher), por eso remapeamos ~/robot_description.
    nodo_control = Node(
        package='controller_manager',
        executable='ros2_control_node',
        output='screen',
        parameters=[archivo_controladores],
        remappings=[('~/robot_description', '/robot_description')])

    # ---------------- Spawners: cargan y activan los controladores ----------------
    spawner_jsb = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['joint_state_broadcaster', '--controller-manager', '/controller_manager'])

    spawner_brazo = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['brazo_controller', '--controller-manager', '/controller_manager'])

    # El controlador del brazo se carga cuando ya existe joint_state_broadcaster
    retrasar_brazo = RegisterEventHandler(
        event_handler=OnProcessExit(target_action=spawner_jsb, on_exit=[spawner_brazo]))

    nodo_rviz = Node(
        package='rviz2',
        executable='rviz2',
        output='log',
        arguments=['-d', archivo_rviz],
        condition=IfCondition(LaunchConfiguration('rviz')))

    return LaunchDescription(argumentos + [
        nodo_rsp,
        nodo_control,
        spawner_jsb,
        retrasar_brazo,
        nodo_rviz,
    ])
