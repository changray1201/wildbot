#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from std_msgs.msg import String          # 🌟 改用 String 發布 JSON 資料
from geometry_msgs.msg import PointStamped
from cv_bridge import CvBridge
from rclpy.qos import qos_profile_sensor_data
from ultralytics import YOLO
import cv2
import numpy as np
import message_filters
import json                              # 🌟 處理多物件資料打包
import math

# 🌟 引入 TF2 相關套件，用於座標轉換
from tf2_ros import Buffer, TransformListener
import tf2_geometry_msgs

import sys

# 1. 這裡必須填寫「包含 models 資料夾」的那個目錄 (因為你在 Docker 裡，所以用 /workspaces)
sys.path.append('/workspaces/src/yolo_ros/weights') 

try:
    # 2. 這裡必須是 from "models.scconv" (迎合 PyTorch 的記憶)
    from models.scconv import SCConv  
    import ultralytics.nn.modules as modules
    import ultralytics.nn.tasks as tasks

    # 雙重魔法註冊
    setattr(modules, 'SCConv', SCConv)
    setattr(tasks, 'SCConv', SCConv)
    print("✅ 成功註冊 SCConv 模組！")
except Exception as e:
    print(f"❌ 找不到 SCConv 模組！錯誤訊息: {e}")
    
class MultiObjectTrackerNode(Node):
    def __init__(self):
        super().__init__('multi_object_tracker_node')

        # 1. 模型路徑設定
        self.model_path = "/workspaces/src/yolo_ros/weights/best.pt"
        
        try:
            self.model = YOLO(self.model_path)
            self.get_logger().info(f"Loaded YOLO model from: {self.model_path}")
        except Exception as e:
            self.get_logger().error(f"Failed to load YOLO model: {e}")
            raise e

        # 2. 影像同步訂閱
        self.bridge = CvBridge()
        self.rgb_sub = message_filters.Subscriber(self, Image, '/camera/color/image_raw', qos_profile=qos_profile_sensor_data)
        self.dep_sub = message_filters.Subscriber(self, Image, '/camera/depth/image_raw', qos_profile=qos_profile_sensor_data)
        
        self.ts = message_filters.ApproximateTimeSynchronizer([self.rgb_sub, self.dep_sub], queue_size=5, slop=0.05)
        self.ts.registerCallback(self.image_callback)

        # 3. 發布 JSON 格式的多目標情報
        self.targets_pub = self.create_publisher(String, '/yolo/targets_info', 10)

        # 4. 🌟 設定 TF 監聽器，用來取得 base_link -> camera_color_optical_frame 的關係
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)

        # 相機內參 (請依據實際 /camera/color/camera_info 微調，這裡用 720p 預設估值)
        self.fx = 640.0  
        self.fy = 640.0
        self.cx = 640.0
        self.cy = 360.0
        
        self.get_logger().info("進化版多物件視覺節點已啟動，開始發布 /yolo/targets_info")

    def image_callback(self, rgb_msg, dep_msg):
        try:
            # --- A. 嘗試取得最新的車體與相機座標轉換矩陣 ---
            try:
                # 詢問 ROS：請告訴我從 base_link 看 camera_link 的座標關係
                trans = self.tf_buffer.lookup_transform(
                    'base_link', 
                    rgb_msg.header.frame_id, # 通常是 'camera_color_optical_frame'
                    rclpy.time.Time()
                )
            except Exception as e:
                self.get_logger().warn(f"等待 TF 座標樹建立中... {e}")
                return # 如果還沒拿到 TF，就先跳過這一幀照片不處理

            # --- B. 影像處理與 YOLO 辨識 ---
            rgb_img = self.bridge.imgmsg_to_cv2(rgb_msg, "bgr8")
            dep_img = self.bridge.imgmsg_to_cv2(dep_msg, "16UC1")
            
            img_height, img_width = rgb_img.shape[:2]

            # 🌟 labels=True 讓畫面上顯示出辨識到的物件名稱
            results = self.model(rgb_img, conf=0.5, verbose=False, device=0)
            annotated_frame = results[0].plot(labels=True, conf=True, line_width=2)

            targets_list = [] # 準備一個空陣列，用來裝所有看到的東西

            # --- C. 迴圈處理每一個看到的物件 ---
            if len(results[0].boxes) > 0:
                for box in results[0].boxes:
                    # 取得物件類別名稱 (例如 'bear', 'bottle')
                    cls_id = int(box.cls[0].item())
                    cls_name = self.model.names[cls_id] 
                    
                    xyxy = box.xyxy[0].cpu().numpy()
                    u = int((xyxy[0] + xyxy[2]) / 2)
                    v = int((xyxy[1] + xyxy[3]) / 2)

                    # 取深度值
                    u_min, u_max = max(0, u-2), min(img_width, u+3)
                    v_min, v_max = max(0, v-2), min(img_height, v+3)
                    depth_roi = dep_img[v_min:v_max, u_min:u_max]
                    valid_depths = depth_roi[depth_roi > 0]

                    if valid_depths.size > 0:
                        depth_m = float(np.median(valid_depths) / 1000.0)
                        
                        # 📐 1. 計算在「相機座標」下的 3D 位置
                        x_cam = (u - self.cx) * depth_m / self.fx
                        y_cam = (v - self.cy) * depth_m / self.fy
                        z_cam = depth_m

                        # 🌟 2. 準備透過 TF 轉換成「車體座標」
                        pt_cam = PointStamped()
                        pt_cam.header.frame_id = rgb_msg.header.frame_id
                        pt_cam.point.x = float(x_cam)
                        pt_cam.point.y = float(y_cam)
                        pt_cam.point.z = float(z_cam)

                        # 施展魔法：直接轉換為 base_link 座標！
                        pt_car = tf2_geometry_msgs.do_transform_point(pt_cam, trans)
                        
                        # 📐 3. 計算相對於車體的絕對距離與角度
                        car_x = pt_car.point.x
                        car_y = pt_car.point.y
                        car_z = pt_car.point.z
                        
                        # 真實距離 (畢氏定理)
                        real_dist = math.hypot(car_x, car_y)
                        # 真實角度 (相對於車頭的左右偏角，左正右負)
                        real_angle_rad = math.atan2(car_y, car_x)
                        
                        # 打包該物件的情報
                        target_info = {
                            "class": cls_name,
                            "distance": round(real_dist, 3),    # 相對於車子的距離(m)
                            "angle_rad": round(real_angle_rad, 3), # 相對於車子的角度(弧度)
                            "angle_deg": round(math.degrees(real_angle_rad), 1) ,# 方便人類看的度數
                            "height": round(car_z, 3)
                        }
                        targets_list.append(target_info)

                        # 在畫面上印出「車體視角」的數據
                        text = f"{cls_name} | Dist:{real_dist:.2f}m | Ang:{math.degrees(real_angle_rad):.0f} | H:{car_z:.2f}m"
                        cv2.putText(annotated_frame, text, (u-40, v-20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 2)
                        cv2.circle(annotated_frame, (u, v), 5, (0, 255, 255), -1)

            # --- D. 將所有情報轉成 JSON 並發布 ---
            msg = String()
            msg.data = json.dumps(targets_list) 
            self.targets_pub.publish(msg)

            cv2.imshow("Multi-Object Tracker", annotated_frame)
            cv2.waitKey(1)

        except Exception as e:
            self.get_logger().error(f"Image processing error: {e}")

def main(args=None):
    rclpy.init(args=args)
    node = MultiObjectTrackerNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        cv2.destroyAllWindows()
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()