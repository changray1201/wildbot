#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient
from rclpy.executors import MultiThreadedExecutor
from rclpy.callback_groups import ReentrantCallbackGroup
from control_msgs.action import FollowJointTrajectory
from trajectory_msgs.msg import JointTrajectoryPoint
from std_srvs.srv import Trigger  # 🌟 引入標準觸發服務
import time

class PickPlaceClient(Node):
    def __init__(self):
        super().__init__('competition_client')
        
        # 🌟 使用多執行緒群組，防止 Action 跟 Service 互相卡死 (死結)
        self.cb_group = ReentrantCallbackGroup()
        
        self._action_client = ActionClient(
            self, FollowJointTrajectory, '/arm_controller/follow_joint_trajectory',
            callback_group=self.cb_group)
            
        self.get_logger().info('正在等待 arm_controller Action Server...')
        self._action_client.wait_for_server()
        self.get_logger().info('Action Server 已連線！準備接收大腦指令...')
        self.joint_names = ['arm_1_joint', 'arm_2_joint', 'gripper_joint']

        # 🌟 建立 Service 伺服器，等待大腦透過 /script/grab 或 /script/drop 呼叫
        self.srv_grab = self.create_service(Trigger, '/script/grab', self.grab_callback, callback_group=self.cb_group)
        self.srv_drop = self.create_service(Trigger, '/script/drop', self.drop_callback, callback_group=self.cb_group)
        
        # (如果你還有 grab_special 等等，就在這裡繼續加 create_service)

    # 收到大腦抓取指令時觸發
    def grab_callback(self, request, response):
        self.get_logger().info('🟢 收到大腦指令：開始執行 grab (夾取)')
        self.run_pick()
        response.success = True
        response.message = "Grab completed successfully"
        return response

    # 收到大腦放下指令時觸發
    def drop_callback(self, request, response):
        self.get_logger().info('🔴 收到大腦指令：開始執行 drop (放下)')
        self.run_place()
        response.success = True
        response.message = "Drop completed successfully"
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
        rclpy.spin_until_future_complete(self, send_goal_future)
        goal_handle = send_goal_future.result()
        if not goal_handle.accepted:
            return False
        get_result_future = goal_handle.get_result_async()
        rclpy.spin_until_future_complete(self, get_result_future)
        return True

    def run_pick(self):
        self.get_logger().info('=== [Pick 1/3] 移動至初始位置 ===')
        self.send_trajectory_goal([2.7, 1.93, 4.0], duration_sec=3)
        time.sleep(1.0)
        self.get_logger().info('=== [Pick 2/3] 夾爪閉合至 3.0 ===')
        self.send_trajectory_goal([2.7, 1.93, 3.0], duration_sec=2)
        time.sleep(1.0)
        self.get_logger().info('=== [Pick 3/3] 手臂移動至 (1.75, 1.93, 3.0) ===')
        self.send_trajectory_goal([1.75, 1.93, 3.0], duration_sec=3)
        time.sleep(1.5)

    def run_place(self):
        self.get_logger().info('=== [Place 1/2] 抬升/張開至 4.0 ===')
        self.send_trajectory_goal([1.75, 1.93, 4.0], duration_sec=2)
        time.sleep(1.0)
        self.get_logger().info('=== [Place 2/2] 放回初始位置 (2.7, 1.93, 4.0) ===')
        self.send_trajectory_goal([2.7, 1.93, 4.0], duration_sec=3)

def main(args=None):
    rclpy.init(args=args)
    node = PickPlaceClient()
    # 🌟 必須使用多執行緒，否則 Action 跟 Service 會互相打架卡死
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