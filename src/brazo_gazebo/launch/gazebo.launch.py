#!/usr/bin/env python3
# =============================================================================
# gazebo.launch.py — Simulación del brazo en Gazebo Harmonic
# =============================================================================
# Secuencia:
#   1) Abre Gazebo con el mundo mundo_brazo.sdf
#   2) robot_state_publisher publica el URDF (generado con modo_hardware:=gazebo)
#   3) "create" (ros_gz_sim) lee /robot_description y crea el robot en Gazebo.
#      Al crearse, el plugin gz_ros2_control arranca un controller_manager.
#   4) Puente /clock para que ROS use el tiempo simulado
#   5) Cuando el robot ya existe, se activan los controladores
#   6) RViz (opcional)
#
# Uso:
#   ros2 launch brazo_gazebo gazebo.launch.py
#   ros2 launch brazo_gazebo gazebo.launch.py rviz:=false
# =============================================================================

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, RegisterEventHandler
from launch.conditions import IfCondition
from launch.event_handlers import OnProcessExit
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command, FindExecutable, LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    desc_share = get_package_share_directory('brazo_description')
    bringup_share = get_package_share_directory('brazo_bringup')
    gazebo_share = get_package_share_directory('brazo_gazebo')
    ros_gz_sim_share = get_package_share_directory('ros_gz_sim')

    archivo_xacro = os.path.join(desc_share, 'urdf', 'brazo.urdf.xacro')
    archivo_controladores = os.path.join(bringup_share, 'config', 'brazo_controladores.yaml')
    archivo_rviz = os.path.join(desc_share, 'rviz', 'brazo.rviz')

    argumentos = [
        DeclareLaunchArgument('mundo',
                              default_value=os.path.join(gazebo_share, 'worlds', 'mundo_brazo.sdf'),
                              description='Mundo .sdf que se abre en Gazebo'),
        DeclareLaunchArgument('rviz', default_value='true', description='Abrir RViz'),
    ]

    # ---------------- 1) Gazebo ----------------
    # -r : empieza la simulación ya en marcha (sin pulsar "play")
    # -v 3 : nivel de mensajes en la terminal
    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(ros_gz_sim_share, 'launch', 'gz_sim.launch.py')),
        launch_arguments={
            'gz_args': ['-r -v 3 ', LaunchConfiguration('mundo')],
            'on_exit_shutdown': 'true',
        }.items())

    # ---------------- 2) URDF en modo gazebo ----------------
    robot_description = {
        'robot_description': ParameterValue(
            Command([
                FindExecutable(name='xacro'), ' ', archivo_xacro,
                ' modo_hardware:=gazebo',
                ' archivo_controladores:=', archivo_controladores,
            ]),
            value_type=str)
    }

    nodo_rsp = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        output='screen',
        parameters=[robot_description, {'use_sim_time': True}])

    # ---------------- 3) Crear el robot dentro de Gazebo ----------------
    crear_robot = Node(
        package='ros_gz_sim',
        executable='create',
        output='screen',
        arguments=['-topic', 'robot_description',
                   '-name', 'brazo',
                   '-allow_renaming', 'true'])

    # ---------------- 4) Puente del reloj de simulación ----------------
    # Sintaxis: topic@tipo_ROS[tipo_Gazebo   ( "[" = de Gazebo hacia ROS )
    puente_reloj = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        arguments=['/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock'],
        output='screen')

    # ---------------- 5) Controladores ----------------
    spawner_jsb = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['joint_state_broadcaster'],
        parameters=[{'use_sim_time': True}])

    spawner_brazo = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['brazo_controller'],
        parameters=[{'use_sim_time': True}])

    # Orden: robot creado -> joint_state_broadcaster -> brazo_controller
    tras_crear = RegisterEventHandler(
        OnProcessExit(target_action=crear_robot, on_exit=[spawner_jsb]))
    tras_jsb = RegisterEventHandler(
        OnProcessExit(target_action=spawner_jsb, on_exit=[spawner_brazo]))

    # ---------------- 6) RViz ----------------
    nodo_rviz = Node(
        package='rviz2',
        executable='rviz2',
        output='log',
        arguments=['-d', archivo_rviz],
        parameters=[{'use_sim_time': True}],
        condition=IfCondition(LaunchConfiguration('rviz')))

    return LaunchDescription(argumentos + [
        gazebo,
        nodo_rsp,
        crear_robot,
        puente_reloj,
        tras_crear,
        tras_jsb,
        nodo_rviz,
    ])
