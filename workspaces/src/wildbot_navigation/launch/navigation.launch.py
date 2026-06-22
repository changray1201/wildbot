import os

from launch_ros.actions import Node  # 👈 確保有這行！

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource


def generate_launch_description():
    nav2_bringup_dir = get_package_share_directory('nav2_bringup')
    wildbot_bringup_dir = get_package_share_directory('wildbot_navigation')

    default_params_file = os.path.join(
        wildbot_bringup_dir, 'config', 'nav2_params.yaml'
    )

    nav2_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(nav2_bringup_dir, 'launch', 'navigation_launch.py')
        ),
        launch_arguments={
            'params_file': default_params_file,
            'use_sim_time': 'False',
        }.items()
    )

    return LaunchDescription([
        nav2_launch,

        Node(
            package='wildbot_navigation',
            executable='odom_bridge_node',  # 👈 這裡填你在 CMakeLists.txt 裡設定的名稱（不加 .cpp 或 .py）
            name='odom_bridge_node',
            output='screen'
        ),

        Node(
            package='wildbot_navigation',
            executable='cmd_vel_bridge_node', # 👈 這是你在 CMakeLists.txt 設定的二進位執行檔名稱
            name='bypass_monitor_bridge',     # 這是節點執行時註冊的名稱
            output='screen'
        )

    ])