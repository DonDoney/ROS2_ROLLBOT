#!/usr/bin/env python3
# =============================================================================
# move_group.launch.py — SOLO MoveIt (move_group + RViz)
# =============================================================================
# Úsalo cuando el robot YA está funcionando por otro lado:
#
#   a) Con Gazebo:
#        Terminal 1: ros2 launch brazo_gazebo gazebo.launch.py rviz:=false
#        Terminal 2: ros2 launch brazo_moveit_config move_group.launch.py use_sim_time:=true
#
#   b) Con el brazo real:
#        Terminal 1: ros2 launch brazo_bringup brazo.launch.py modo_hardware:=real rviz:=false
#        Terminal 2: ros2 launch brazo_moveit_config move_group.launch.py
# =============================================================================

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from moveit_configs_utils import MoveItConfigsBuilder


def lanzar(context, *args, **kwargs):
    # OpaqueFunction nos deja leer el valor real de los argumentos (texto)
    use_sim_time = LaunchConfiguration('use_sim_time').perform(context).lower() == 'true'

    desc_share = get_package_share_directory('brazo_description')
    moveit_share = get_package_share_directory('brazo_moveit_config')

    moveit_config = (
        MoveItConfigsBuilder('brazo', package_name='brazo_moveit_config')
        .robot_description(file_path=os.path.join(desc_share, 'urdf', 'brazo.urdf.xacro'))
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

    nodo_move_group = Node(
        package='moveit_ros_move_group',
        executable='move_group',
        output='screen',
        parameters=[moveit_config.to_dict(), {'use_sim_time': use_sim_time}])

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
            {'use_sim_time': use_sim_time},
        ],
        condition=IfCondition(LaunchConfiguration('rviz')))

    return [nodo_move_group, nodo_rviz]


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument('use_sim_time', default_value='false',
                              description='true si el robot está en Gazebo'),
        DeclareLaunchArgument('rviz', default_value='true', description='Abrir RViz'),
        OpaqueFunction(function=lanzar),
    ])
