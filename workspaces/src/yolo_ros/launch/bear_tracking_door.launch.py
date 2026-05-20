from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        # 1. 啟動靜態 TF 廣播：把相機掛在車體上 (加入下傾角)
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            name='base_link_to_camera_tf',
            arguments=[
                '--x', '0.08',       # 相機在車體中心「前方」多少公尺 (請拿尺量)
                '--y', '0.015',        # 左右偏移 (照片看起來在正中間，所以是 0)
                '--z', '0.14',       # 相機離車體中心「多高」 (請拿尺量)
                '--roll', '0.0',     # 翻滾角 (沒歪頭就是 0)
                '--pitch', '0.349',  # 🌟 俯仰角：往下傾斜約 20 度 (請依實際角度微調)
                '--yaw', '0.0',      # 偏航角 (沒左右轉頭就是 0)
                '--frame-id', 'base_link',
                '--child-frame-id', 'camera_link'
            ]
        ),

        # 2. (可選) 啟動你的相機驅動 (假設你用 RealSense)
        # Node(
        #     package='realsense2_camera',
        #     executable='rs_launch.py',
        #     name='realsense_camera',
        #     parameters=[{'align_depth.enable': True}]
        # ),

        # 3. 啟動你的 YOLO 視覺節點 (眼睛)
        Node(
            package='yolo_ros',
            executable='bear_tracker_node_door.py',
            name='bear_tracker'
        ),

        # # 4. 啟動你的控制節點 (大腦)
        # Node(
        #     package='bear_control', # 請換成你實際放 controller 的 package 名字
        #     executable='bear_controller_node.py',
        #     name='bear_controller'
        # )
    ])