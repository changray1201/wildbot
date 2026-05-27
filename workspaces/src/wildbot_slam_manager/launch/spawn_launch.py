import os
import xacro
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import Node
from launch.conditions import IfCondition

from launch.actions import RegisterEventHandler, ExecuteProcess
from launch.event_handlers import OnProcessStart
from launch_ros.event_handlers import OnStateTransition

def generate_launch_description():
    # 1. 宣告與獲取 Launch 參數設定
    use_sim_time = LaunchConfiguration('use_sim_time', default='false')
    localization_mode = LaunchConfiguration('localization_mode')

    # 2. 定義各個 ROS 2 節點 (Nodes)
    
    # EKF 濾波器節點 (Robot Localization)
    ekf_node = Node(
        package='robot_localization',#發布 nav_msgs/msg/Odometry 和 TFTree
        executable='ekf_node',
        name='ekf_filter_node',
        output='screen',
        parameters=[
            PathJoinSubstitution([
                FindPackageShare('wildbot_slam_manager'), 
                'config', 
                'ekf_config.yaml'
            ]),
            #{'use_sim_time': use_sim_time}
            {'use_sim_time': False}
        ]
    )

    # SLAM 建圖節點 (Mapping Mode)
    mapping_node = Node(
        condition=IfCondition(PythonExpression(["'", localization_mode, "' == 'mapping'"])),
        package='slam_toolbox',
        executable='sync_slam_toolbox_node',
        #executable='async_slam_toolbox_node',
        name='slam_toolbox',
        output='screen',
        parameters=[
            PathJoinSubstitution([
                FindPackageShare('wildbot_slam_manager'), 
                'config', 
                'mapper_params_online_async.yaml'
            ]),
            {
                #'use_sim_time': use_sim_time,
                'use_sim_time': False,
                'autostart': True
            }
        ]
    )

    # SLAM 純定位節點 (Localization Mode)
    localization_node = Node(
        condition=IfCondition(PythonExpression(["'", localization_mode, "' == 'localization'"])),
        package='slam_toolbox',
        executable='localization_slam_toolbox_node',
        name='slam_toolbox',
        output='screen',
        parameters=[
            PathJoinSubstitution([
                FindPackageShare('wildbot_slam_manager'), 
                'config', 
                'slam_toolbox_params.yaml'
            ]),
            {
                'mode': PythonExpression(["'mapping' if '", localization_mode, "' == 'mapping' else 'localization'"]),
                'use_sim_time': False
            }
        ]
    )

    # 建立你的動態座標發布節點
    robot_pose_publisher_node = Node(
        package='wildbot_slam_manager',     
        executable='robot_pose_publisher', 
        name='robot_pose_publisher',
        output='screen'
    )

    # 雷射濾波器節點
    #laser_filter_node = Node(
     #   package='laser_filters',
     #   executable='scan_to_scan_filter_chain',
     #   name='laser_filter_node',
     #   output='screen',
     #   parameters=[PathJoinSubstitution([FindPackageShare('wildbot_slam_manager'), 'config', 'filter_config.yaml']),
     #       {'use_sim_time': use_sim_time}]
    #)

    polling_script = (
        "until ros2 lifecycle set /slam_toolbox configure > /dev/null 2>&1; do "
        "  echo '[Launch Manager] 正在等待 /slam_toolbox 節點誕生，1秒後重試 configure...'; "
        "  sleep 1; "
        "done; "
        "echo '[Launch Manager] 配置成功 (Configured)！'; "
        "until ros2 lifecycle set /slam_toolbox activate > /dev/null 2>&1; do "
        "  echo '[Launch Manager] 求解器與地圖載入中，1秒後重試 activate...'; "
        "  sleep 1; "
        "done; "
        "echo '[Launch Manager] 節點活化成功 (Activated)！開始發布地圖 TF。'"
    )

    # 綁定事件：不管是建圖模式還是定位模式啟動，只要目標節點誕生，就啟動無限重試敲門
    auto_activate_slam = RegisterEventHandler(
        OnProcessStart(
            target_action=mapping_node,
            on_start=[ExecuteProcess(cmd=['bash', '-c', polling_script], output='screen')]
        )
    )

    auto_activate_localization = RegisterEventHandler(
        OnProcessStart(
            target_action=localization_node,
            on_start=[ExecuteProcess(cmd=['bash', '-c', polling_script], output='screen')]
        )
    )

    # 3. 回傳 Launch 說明物件，並載入所有參數與節點（完全移除 nav2 依賴與內部 threading）
    return LaunchDescription([
        DeclareLaunchArgument('use_sim_time', default_value='false'),
        #DeclareLaunchArgument('localization_mode', default_value='mapping'),
        DeclareLaunchArgument('localization_mode', default_value='localization'),


        ekf_node,
        mapping_node,
        localization_node,
        #laser_filter_node,
        auto_activate_slam,
        auto_activate_localization,
        robot_pose_publisher_node
    ])