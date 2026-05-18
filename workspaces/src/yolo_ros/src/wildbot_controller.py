#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import String  # 接收 JSON 字串用的格式
import json

class WildbotController(Node):
    def __init__(self):
        super().__init__('wildbot_controller_node')
        
        # 🌟 1. 訂閱 YOLO 發布的情報頻道
        # 注意：請把 '/yolo/targets_info' 換成你實際發布的 Topic 名稱！
        self.subscription = self.create_subscription(
            String,
            '/yolo/targets_info',
            self.yolo_callback,
            10
        )
        
        self.get_logger().info("🧠 Wildbot 大腦已上線，正在監聽視覺情報...")

    # 🌟 2. 只要 YOLO 一發布新情報，這個函數就會自動被觸發！
    def yolo_callback(self, msg):
        try:
            # 將收到的字串解碼回 Python 的 List (裡面包著 Dictionary)
            targets = json.loads(msg.data)
        except Exception as e:
            self.get_logger().error(f"解析 JSON 失敗: {e}")
            return

        # 如果畫面中什麼都沒看到
        if len(targets) == 0:
            # 這裡可以寫：車子原地旋轉找目標，或是停下來
            # self.get_logger().info("沒看到東西，原地待命...")
            return

        # 🌟 3. 戰略分析：尋找最重要的目標 (例如：優先找熊)
        bear_target = None
        road_target = None

        for item in targets:
            if item['class'] == 'bear' or item['class'] == '熊': # 依照你 YAML 裡的命名
                bear_target = item
            elif item['class'] == '橋的路':
                road_target = item

        # 🌟 4. 下達決策 (控制邏輯)
        self.make_decision(bear_target, road_target)

    def make_decision(self, bear, road):
        """在這裡寫下你的動機系硬核控制邏輯"""
        
        # 情況 A：看到了熊！
        if bear:
            dist = bear['distance']
            angle = bear['angle_deg']
            
            if dist > 1.5:
                self.get_logger().info(f"🐻 發現熊！距離 {dist}m，有點遠，全速前進！(角度: {angle}度)")
                # TODO: 發布 Twist 訊息讓馬達往前轉
            elif dist <= 1.5 and dist > 0.5:
                self.get_logger().info(f"🐻 熊就在前方 {dist}m，減速靠近，準備執行任務！")
                # TODO: 發布 Twist 訊息讓馬達減速
            else:
                self.get_logger().info(f"🛑 距離 {dist}m，太近了！煞車！")
                # TODO: 發布 Twist 訊息讓馬達煞車

        # 情況 B：沒看到熊，但看到了橋的路
        elif road:
            # 依據你上一篇算出來的「安全上橋點」角度來微調車頭
            angle = road['angle_deg']
            if angle > 5.0:
                self.get_logger().info(f"🌉 路稍微偏左 ({angle}度)，車頭向左微調...")
            elif angle < -5.0:
                self.get_logger().info(f"🌉 路稍微偏右 ({angle}度)，車頭向右微調...")
            else:
                self.get_logger().info("🌉 完美對準橋面，直直往前開！")

def main(args=None):
    rclpy.init(args=args)
    node = WildbotController()
    
    try:
        rclpy.spin(node) # 讓程式持續運行，隨時準備接收情報
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()