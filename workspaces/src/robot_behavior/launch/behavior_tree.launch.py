import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import Node
from launch.conditions import IfCondition

def generate_launch_description():

    # ==========================================
    # 0. ⚙️ 宣告與獲取 Launch 參數設定
    # ==========================================
    use_sim_time = LaunchConfiguration('use_sim_time', default='false')
    localization_mode = LaunchConfiguration('localization_mode', default='mapping')

    # ==========================================
    # 1. 🧠 啟動大腦 (BehaviorTree 主程式)
    # ==========================================
    bt_main_node = Node(
        package='robot_behavior',
        executable='bt_main',
        name='robot_brain_node',
        output='screen',
        emulate_tty=True
    )

    # ==========================================
    # 2. 🦴 啟動骨架 (靜態 TF 廣播：把相機掛在車體上)
    # ==========================================
    camera_tf_node = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='base_link_to_camera_tf',
        arguments=[
            '--x', '0.08',       # 前後距離
            '--y', '0.015',      # 左右偏移
            '--z', '0.14',       # 上下高度
            '--roll', '0.0',     
            '--pitch', '0.349',  # 下傾角約 20 度
            '--yaw', '0.0',      
            '--frame-id', 'base_link',
            '--child-frame-id', 'camera_link'
        ]
    )

    # ==========================================
    # 3. 👀 啟動眼睛 (YOLO 視覺追蹤節點)
    # ==========================================
    yolo_node = Node(
        package='yolo_ros',
        executable='bear_tracker_node_bear1.py',
        name='bear_tracker',
        output='screen'
    )

    # ==========================================
    # 4. 🛰️ SLAM 核心：EKF 濾波器節點 (狀態估測)
    # ==========================================
    ekf_node = Node(
        package='robot_localization',
        executable='ekf_node',
        name='ekf_filter_node',
        output='screen',
        parameters=[
            PathJoinSubstitution([
                FindPackageShare('wildbot_slam_manager'), 
                'config', 
                'ekf_config.yaml'
            ]),
            {'use_sim_time': use_sim_time}
        ]
    )

    # ==========================================
    # 5. 🗺️ SLAM 核心：建圖節點 (僅在 mapping 模式啟動)
    # ==========================================
    mapping_node = Node(
        condition=IfCondition(PythonExpression(["'", localization_mode, "' == 'mapping'"])),
        package='slam_toolbox',
        executable='sync_slam_toolbox_node',
        name='slam_toolbox',
        output='screen',
        parameters=[
            PathJoinSubstitution([
                FindPackageShare('wildbot_slam_manager'), 
                'config', 
                'mapper_params_online_async.yaml'
            ]),
            {'use_sim_time': use_sim_time}
        ]
    )

    # ==========================================
    # 6. 📍 SLAM 核心：純定位節點 (僅在 localization 模式啟動)
    # ==========================================
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
                'use_sim_time': use_sim_time
            }
        ]
    )

    # ==========================================
    # 7. 📌 座標與雷達濾波輔助節點
    # ==========================================
    robot_pose_publisher_node = Node(
        package='wildbot_slam_manager',     
        executable='robot_pose_publisher', 
        name='robot_pose_publisher',
        output='screen'
    )

    laser_filter_node = Node(
        package='laser_filters',
        executable='scan_to_scan_filter_chain',
        name='laser_filter_node',
        output='screen',
        parameters=[
            PathJoinSubstitution([
                FindPackageShare('wildbot_slam_manager'), 
                'config', 
                'filter_config.yaml'
            ]),
            {'use_sim_time': use_sim_time}
        ]
    )

    # ==========================================
    # 8. 📷 (可選) 啟動實體相機驅動
    # ==========================================
    # realsense_node = Node(
    #     package='realsense2_camera',
    #     executable='rs_launch.py',
    #     name='realsense_camera',
    #     parameters=[{'align_depth.enable': True}]
    # )

    # ==========================================
    # 9. 回傳集合 (包含參數宣告與節點)
    # ==========================================
    return LaunchDescription([
        DeclareLaunchArgument('use_sim_time', default_value='false', description='Use simulation clock if true'),
        DeclareLaunchArgument('localization_mode', default_value='mapping', description='SLAM mode: mapping or localization'),

        # 核心節點清單
        bt_main_node,
        camera_tf_node,
        yolo_node,
        ekf_node,
        mapping_node,
        localization_node,
        laser_filter_node,
        robot_pose_publisher_node
        # realsense_node
    ])