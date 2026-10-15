#!/usr/bin/env python3
# =============================================================================
# demo.launch.py — MoveIt 2 + RViz con hardware simulado (mock), TODO EN UNO
# =============================================================================
# Es la forma más rápida de probar la planificación sin Gazebo ni robot real.
# Arranca:
#   - robot_state_publisher
#   - ros2_control_node con mock_components/GenericSystem
#   - spawners de joint_state_broadcaster y brazo_controller
#   - move_group  (el "cerebro" de MoveIt: planifica y ejecuta)
#   - RViz con el plugin MotionPlanning
#
# Uso:
#   ros2 launch brazo_moveit_config demo.launch.py
# =============================================================================

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import RegisterEventHandler
from launch.event_handlers import OnProcessExit
from launch_ros.actions import Node
from moveit_configs_utils import MoveItConfigsBuilder


def generate_launch_description():
    desc_share = get_package_share_directory('brazo_description')
    bringup_share = get_package_share_directory('brazo_bringup')
    moveit_share = get_package_share_directory('brazo_moveit_config')

    archivo_controladores = os.path.join(bringup_share, 'config', 'brazo_controladores.yaml')

    # ---------------- Configuración de MoveIt ----------------
    # MoveItConfigsBuilder reúne en un solo objeto: URDF, SRDF, cinemática,
    # límites, planificadores y controladores.
    moveit_config = (
        MoveItConfigsBuilder('brazo', package_name='brazo_moveit_config')
        .robot_description(
            file_path=os.path.join(desc_share, 'urdf', 'brazo.urdf.xacro'),
            mappings={'modo_hardware': 'mock'})
        .robot_description_semantic(file_path='config/brazo.srdf')
        .robot_description_kinematics(file_path='config/kinematics.yaml')
        .joint_limits(file_path='config/joint_limits.yaml')
        .trajectory_execution(file_path='config/moveit_controllers.yaml')
        .planning_scene_monitor(
            publish_robot_description=True,
            publish_robot_description_semantic=True)
        .planning_pipelines(pipelines=['ompl'])
        .to_moveit_configs()
    )

    # ---------------- move_group ----------------
    nodo_move_group = Node(
        package='moveit_ros_move_group',
        executable='move_group',
        output='screen',
        parameters=[moveit_config.to_dict()])

    # ---------------- RViz con MotionPlanning ----------------
    nodo_rviz = Node(
        package='rviz2',
        executable='rviz2',
        output='log',
        arguments=['-d', os.path.join(moveit_share, 'config', 'moveit.rviz')],
        parameters=[
            moveit_config.robot_description,
            moveit_config.robot_description_semantic,
            moveit_config.robot_description_kinematics,
            moveit_config.planning_pipelines,
            moveit_config.joint_limits,
        ])

    # ---------------- Robot (mock) con ros2_control ----------------
    nodo_rsp = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        output='both',
        parameters=[moveit_config.robot_description])

    nodo_control = Node(
        package='controller_manager',
        executable='ros2_control_node',
        output='screen',
        parameters=[archivo_controladores],
        remappings=[('~/robot_description', '/robot_description')])

    spawner_jsb = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['joint_state_broadcaster', '--controller-manager', '/controller_manager'])

    spawner_brazo = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['brazo_controller', '--controller-manager', '/controller_manager'])

    retrasar_brazo = RegisterEventHandler(
        OnProcessExit(target_action=spawner_jsb, on_exit=[spawner_brazo]))

    return LaunchDescription([
        nodo_rsp,
        nodo_control,
        spawner_jsb,
        retrasar_brazo,
        nodo_move_group,
        nodo_rviz,
    ])
