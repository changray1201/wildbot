#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient
from rclpy.executors import MultiThreadedExecutor
from rclpy.callback_groups import ReentrantCallbackGroup
from control_msgs.action import FollowJointTrajectory
from trajectory_msgs.msg import JointTrajectoryPoint
from std_srvs.srv import Trigger
import time

class PickPlaceClient(Node):
    def __init__(self):
        super().__init__('competition_client')
        
        self.cb_group = ReentrantCallbackGroup()
        
        self._action_client = ActionClient(
            self, FollowJointTrajectory, '/arm_controller/follow_joint_trajectory',
            callback_group=self.cb_group)
            
        self.get_logger().info('正在等待 arm_controller Action Server...')
        self._action_client.wait_for_server()
        self.get_logger().info('Action Server 已連線！準備接收大腦指令...')
        self.joint_names = ['arm_1_joint', 'arm_2_joint', 'gripper_joint']

        # 註冊服務（與 XML 的 action 名稱保持一致）
        self.srv_grab = self.create_service(Trigger, '/script/grab_doll', self.grab_callback, callback_group=self.cb_group)
        self.srv_drop = self.create_service(Trigger, '/script/drop_doll', self.drop_callback, callback_group=self.cb_group)

    # 收到大腦抓取指令時觸發
    def grab_callback(self, request, response):
        self.get_logger().info('🟢 收到大腦指令：開始執行 grab (夾取)')
        success = self.run_pick()
        response.success = success
        response.message = "Grab completed successfully" if success else "Grab failed"
        return response

    # 收到大腦放下指令時觸發
    def drop_callback(self, request, response):
        self.get_logger().info('🔴 收到大腦指令：開始執行 drop (放下)')
        success = self.run_place()
        response.success = success
        response.message = "Drop completed successfully" if success else "Drop failed"
        return response

    def send_trajectory_goal(self, positions, duration_sec):
        goal_msg = FollowJointTrajectory.Goal()
        goal_msg.trajectory.joint_names = self.joint_names
        point = JointTrajectoryPoint()
        point.positions = positions
        point.time_from_start.sec = duration_sec
        point.time_from_start.nanosec = 0
        goal_msg.trajectory.points.append(point)
        
        self.get_logger().info(f'正在發送目標位置: {positions}')
        send_goal_future = self._action_client.send_goal_async(goal_msg)
        
        # 🌟 核心修正：因為使用了 MultiThreadedExecutor，背景本來就在 spin。
        # 這裡絕對不能呼叫 spin_until_future_complete！改用安全的 while 迴圈等待。
        while rclpy.ok() and not send_goal_future.done():
            time.sleep(0.05)
            
        goal_handle = send_goal_future.result()
        if not goal_handle.accepted:
            self.get_logger().error('❌ 軌跡目標被 Action Server 拒絕！')
            return False
            
        self.get_logger().info('✔ 目標已接受，正在等待執行結果...')
        get_result_future = goal_handle.get_result_async()
        
        # 🌟 同理，等待 Action 執行完畢也改用 while 迴圈
        while rclpy.ok() and not get_result_future.done():
            time.sleep(0.05)
            
        self.get_logger().info('✔ Action 動作執行成功！')
        return True

    def run_pick(self):
        self.get_logger().info('=== [Pick 1/3] 移動至初始位置 ===')
        if not self.send_trajectory_goal([2.7, 1.93, 4.0], duration_sec=3): return False
        time.sleep(1.0)
        
        self.get_logger().info('=== [Pick 2/3] 夾爪閉合至 3.0 ===')
        if not self.send_trajectory_goal([2.7, 1.93, 3.0], duration_sec=2): return False
        time.sleep(1.0)
        
        self.get_logger().info('=== [Pick 3/3] 手臂移動至 (1.75, 1.93, 3.0) ===')
        if not self.send_trajectory_goal([1.75, 1.93, 3.0], duration_sec=3): return False
        time.sleep(1.5)
        return True

    def run_place(self):
        self.get_logger().info('=== [Place 1/2] 抬升/張開至 4.0 ===')
        if not self.send_trajectory_goal([1.75, 1.93, 4.0], duration_sec=2): return False
        time.sleep(1.0)
        
        self.get_logger().info('=== [Place 2/2] 放回初始位置 (2.7, 1.93, 4.0) ===')
        if not self.send_trajectory_goal([2.7, 1.93, 4.0], duration_sec=3): return False
        return True

def main(args=None):
    rclpy.init(args=args)
    node = PickPlaceClient()
    
    # 使用多執行緒執行器，配合修正後的 while 迴圈等待，絕不卡死
    executor = MultiThreadedExecutor()
    executor.add_node(node)
    try:
        executor.spin()
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()