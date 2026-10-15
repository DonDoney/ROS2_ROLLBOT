#!/usr/bin/env python3
# =============================================================================
# visualizar.launch.py — Ver el brazo en RViz y moverlo con deslizadores
# =============================================================================
# Arranca tres nodos:
#   1) robot_state_publisher  : lee el URDF y publica las transformadas (TF)
#                               de cada eslabón según el estado de las articulaciones.
#   2) joint_state_publisher_gui : ventana con un deslizador por articulación;
#                               publica /joint_states con los valores elegidos.
#   3) rviz2                  : visualizador 3D.
#
# Uso:
#   ros2 launch brazo_description visualizar.launch.py
#   ros2 launch brazo_description visualizar.launch.py gui:=false
#   ros2 launch brazo_description visualizar.launch.py modelo:=/ruta/a/otro.urdf.xacro
# =============================================================================

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import Command, FindExecutable, LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    pkg_share = get_package_share_directory('brazo_description')

    # ---------------- Argumentos que se pueden cambiar desde la terminal ----------------
    arg_modelo = DeclareLaunchArgument(
        'modelo',
        default_value=os.path.join(pkg_share, 'urdf', 'brazo.urdf.xacro'),
        description='Ruta absoluta al archivo .urdf o .urdf.xacro a visualizar')

    arg_gui = DeclareLaunchArgument(
        'gui', default_value='true',
        description='true = ventana de deslizadores; false = articulaciones a cero')

    arg_rviz = DeclareLaunchArgument(
        'config_rviz',
        default_value=os.path.join(pkg_share, 'rviz', 'brazo.rviz'),
        description='Archivo de configuración de RViz')

    # ---------------- Procesar el xacro -> texto URDF ----------------
    # Command() ejecuta "xacro <archivo>" al lanzar y devuelve el URDF como texto.
    # ParameterValue(..., value_type=str) evita que ROS intente interpretarlo como YAML.
    robot_description = ParameterValue(
        Command([FindExecutable(name='xacro'), ' ', LaunchConfiguration('modelo')]),
        value_type=str)

    # ---------------- Nodos ----------------
    nodo_rsp = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[{'robot_description': robot_description}])

    # Con interfaz gráfica (deslizadores)
    nodo_jsp_gui = Node(
        package='joint_state_publisher_gui',
        executable='joint_state_publisher_gui',
        name='joint_state_publisher_gui',
        condition=IfCondition(LaunchConfiguration('gui')))

    # Sin interfaz gráfica (publica todas las articulaciones a 0)
    nodo_jsp = Node(
        package='joint_state_publisher',
        executable='joint_state_publisher',
        name='joint_state_publisher',
        condition=UnlessCondition(LaunchConfiguration('gui')))

    nodo_rviz = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='log',
        arguments=['-d', LaunchConfiguration('config_rviz')])

    return LaunchDescription([
        arg_modelo, arg_gui, arg_rviz,
        nodo_rsp, nodo_jsp_gui, nodo_jsp, nodo_rviz,
    ])
