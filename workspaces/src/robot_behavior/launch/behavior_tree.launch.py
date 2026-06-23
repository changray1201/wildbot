import os
from launch.actions import TimerAction
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import Node
from launch.conditions import IfCondition
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource

from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, RegisterEventHandler, ExecuteProcess # 👈 關鍵是補上這一個！
from launch.event_handlers import OnProcessExit, OnProcessStart

def generate_launch_description():

    # ==========================================
    # 0. ⚙️ 宣告與獲取 Launch 參數設定 (🌟 救回並修正這兩行)
    # ==========================================
    use_sim_time = LaunchConfiguration('use_sim_time', default='false')
    localization_mode = LaunchConfiguration('localization_mode', default='localization')

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
            '--child-frame-id', 'camera_color_optical_frame'
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
    # 9. 🚗 啟動自訂的 Wildbot 導航系統 (巢狀載入)
    # ==========================================
    # 這裡直接呼叫你自己寫的 wildbot_navigation 中的 navigation.launch.py
    wildbot_nav_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([
                FindPackageShare('wildbot_navigation'), 
                'launch',
                'navigation.launch.py'                 
            ])
        ),
        launch_arguments={
            'use_sim_time': use_sim_time, # 動態變數
            'params_file': PathJoinSubstitution([
                FindPackageShare('wildbot_navigation'), 
                'config', 
                'nav2_params.yaml'
            ])
        }.items()
    )

    # ==========================================
    # 10. 🅿️ 啟動泊車小弟 (OpenNav Docking 伺服器)
    # ==========================================
    docking_node = Node(
        package='opennav_docking',
        executable='opennav_docking',
        name='docking_server',
        output='screen',
        parameters=[
            {
                'use_sim_time': use_sim_time,
                'autostart': True  # 讓 Lifecycle Node 啟動後自動進入 Active 狀態
            }
            # 💡 未來如果有對接用的設定檔 (例如標記娃娃的長寬高、對接速度等)，可以加在這裡：
            # PathJoinSubstitution([FindPackageShare('wildbot_navigation'), 'config', 'docking_params.yaml'])
        ]
    )

    # ==========================================
    # 9. 回傳集合 (包含參數宣告與節點)
    # ==========================================
    return LaunchDescription([
        DeclareLaunchArgument('use_sim_time', default_value='false', description='Use simulation clock if true'),
        DeclareLaunchArgument('localization_mode', default_value='localization', description='SLAM mode: mapping or localization'),

        # 核心節點清單
        bt_main_node,
        camera_tf_node,
        yolo_node,
        ekf_node,
        mapping_node,
        localization_node,
        robot_pose_publisher_node,
        auto_activate_slam,
        auto_activate_localization,
        wildbot_nav_launch,
        # docking_node,

    ])