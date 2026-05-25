// #include <memory>
// #include <chrono>
// #include "rclcpp/rclcpp.hpp"
// #include "rclcpp_action/rclcpp_action.hpp"
// #include "opennav_docking_msgs/action/dock_robot.hpp"

// using DockRobot = opennav_docking_msgs::action::DockRobot;

// class BlindTestNode : public rclcpp::Node
// {
// public:
//   BlindTestNode() : Node("blind_test_node")
//   {
//     // 建立對講機：呼叫泊車小弟
//     client_ = rclcpp_action::create_client<DockRobot>(this, "/dock_robot");
//   }

//   void send_test_command()
//   {
//     RCLCPP_INFO(this->get_logger(), "正在尋找泊車小弟伺服器...");
//     if (!client_->wait_for_action_server(std::chrono::seconds(10))) {
//       RCLCPP_ERROR(this->get_logger(), "找不到伺服器！請確認大腦 (navigation.xml) 有啟動。");
//       return;
//     }

//     // 設定盲開目標
//     auto goal_msg = DockRobot::Goal();
//     goal_msg.use_dock_id = false;
//     goal_msg.dock_pose.header.frame_id = "odom"; // 關鍵：不看地圖，只看相對里程計
//     goal_msg.dock_pose.pose.position.x = 1.0;    // 指令：直接往前 1 公尺
//     goal_msg.dock_pose.pose.orientation.w = 1.0;

//     RCLCPP_INFO(this->get_logger(), "發射指令：無地圖盲開，模擬往前前進 1 公尺！");
//     client_->async_send_goal(goal_msg);
//   }

// private:
//   rclcpp_action::Client<DockRobot>::SharedPtr client_;
// };

// int main(int argc, char ** argv)
// {
//   rclcpp::init(argc, argv);
//   auto node = std::make_shared<BlindTestNode>();
 
//   // 程式一啟動，立刻發送指令
//   node->send_test_command();
 
//   rclcpp::spin(node);
//   rclcpp::shutdown();
//   return 0;
// }

#include "robot_behavior/docking_test.hpp"
#include <iostream>

namespace robot_behavior {

DockingTest::DockingTest(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node)
  : BT::StatefulActionNode(name, config), ros_node_(node) {}

BT::NodeStatus DockingTest::onStart() {
    std::cout << "\033[1;33m[Docking] 這是空殼節點，假裝我們已經貼近目標了！\033[0m" << std::endl;
    return BT::NodeStatus::SUCCESS; // 假裝瞬間執行成功，讓行為樹繼續往下跑
}

BT::NodeStatus DockingTest::onRunning() {
    return BT::NodeStatus::SUCCESS; 
}

void DockingTest::onHalted() {
}

} // namespace robot_behavior