import os
import xacro
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import Node
from launch.conditions import IfCondition

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

    # 3. 回傳 Launch 說明物件，並載入所有參數與節點（完全移除 nav2 依賴與內部 threading）
    return LaunchDescription([
        DeclareLaunchArgument('use_sim_time', default_value='false'),
        DeclareLaunchArgument('localization_mode', default_value='mapping'),

        ekf_node,
        mapping_node,
        localization_node,
        #laser_filter_node,
        robot_pose_publisher_node
    ])BT::PublisherZMQ publisher_zmq(tree); // 開啟 Groot2 監聽通道
    RCLCPP_INFO(node->get_logger(), "👀 Groot2 監聽器已啟動...");