#!/usr/bin/env python3
# =============================================================================
# moveit_ejemplo.launch.py — Lanza el ejemplo C++ de MoveIt con sus parámetros
# =============================================================================
# El nodo necesita conocer el robot (URDF, SRDF y cinemática), igual que
# move_group. Se los pasamos con MoveItConfigsBuilder.
#
# Requisito: move_group ya en marcha (demo.launch.py o move_group.launch.py).
#   ros2 launch brazo_examples moveit_ejemplo.launch.py
#   ros2 launch brazo_examples moveit_ejemplo.launch.py use_sim_time:=true   # con Gazebo
# =============================================================================

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from moveit_configs_utils import MoveItConfigsBuilder


def generate_launch_description():
    desc_share = get_package_share_directory('brazo_description')

    moveit_config = (
        MoveItConfigsBuilder('brazo', package_name='brazo_moveit_config')
        .robot_description(file_path=os.path.join(desc_share, 'urdf', 'brazo.urdf.xacro'))
        .robot_description_semantic(file_path='config/brazo.srdf')
        .robot_description_kinematics(file_path='config/kinematics.yaml')
        .joint_limits(file_path='config/joint_limits.yaml')
        .to_moveit_configs()
    )

    nodo_ejemplo = Node(
        package='brazo_examples',
        executable='moveit_ejemplo',
        output='screen',
        parameters=[
            moveit_config.robot_description,
            moveit_config.robot_description_semantic,
            moveit_config.robot_description_kinematics,
            moveit_config.joint_limits,
            {'use_sim_time': ParameterValue(LaunchConfiguration('use_sim_time'), value_type=bool)},
        ])

    return LaunchDescription([
        DeclareLaunchArgument('use_sim_time', default_value='false'),
        nodo_ejemplo,
    ])
