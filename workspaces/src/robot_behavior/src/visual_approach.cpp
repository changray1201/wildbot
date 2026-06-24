#include "robot_behavior/visual_approach.hpp"
#include <cmath>
#include <algorithm>

namespace robot_behavior
{

// 1. 建構子：建立速度發布器，並「訂閱」對接 Topic
VisualApproach::VisualApproach(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node)
: BT::StatefulActionNode(name, config), ros_node_(node)
{
  cmd_vel_pub_ = ros_node_->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);
  
  // ✨ 建立訂閱者：監聽 Python 視覺節點算好的即時直角座標
  dock_pose_sub_ = ros_node_->create_subscription<geometry_msgs::msg::PoseStamped>(
    "detected_dock_pose", 10,
    [this](const geometry_msgs::msg::PoseStamped::SharedPtr msg) {
      this->latest_dock_pose_ = msg; // 只要有新資料就更新暫存
    });
}

// 2. 宣告行為樹輸出入埠口
BT::PortsList VisualApproach::providedPorts()
{
  // ✨ 不再需要從黑板讀取 target_pose，回傳空列表即可
  return {};
}

// 3. 節點啟動時的初始化
BT::NodeStatus VisualApproach::onStart()
{
  RCLCPP_INFO(ros_node_->get_logger(), "🎯 VisualApproach 啟動！開始聆聽即時視覺對接資料...");
  latest_dock_pose_ = nullptr; // 每次啟動時先清空舊資料，避免用到上一次夾娃娃的殘留數據
  return BT::NodeStatus::RUNNING;
}

// 4. 核心控制迴圈
BT::NodeStatus VisualApproach::onRunning()
{
  // ==========================================================
  // ✨ 步驟 A：檢查是否有最新視覺資料
  // ==========================================================
  if (!latest_dock_pose_) {
    // 限制警告頻率 (每 1000 毫秒印一次)，避免洗版
    RCLCPP_WARN_THROTTLE(ros_node_->get_logger(), *ros_node_->get_clock(), 1000, 
                         "⏳ 等待 Python 視覺節點發布對接數據...");
    
    // 沒看到目標時先踩煞車比較安全
    auto stop_msg = geometry_msgs::msg::Twist();
    cmd_vel_pub_->publish(stop_msg);
    return BT::NodeStatus::RUNNING; 
  }

  // 讀取 Python 端已經幫我們轉換好 (base_link) 且扣除臂長的相對距離
  double target_x = latest_dock_pose_->pose.position.x;
  double target_y = latest_dock_pose_->pose.position.y;

  RCLCPP_INFO(ros_node_->get_logger(), "👀 [即時視覺反饋] 剩餘對接距離 -> 前方 X: %+.3f, 左右 Y: %+.3f", target_x, target_y);

  // ==========================================================
  // 🎯 步驟 B：抵達終點的檢查 (容許誤差區間)
  // ==========================================================
  const double DISTANCE_TOLERANCE_X = 0.015; // 前後容許誤差 1.5 公分
  const double DISTANCE_TOLERANCE_Y = 0.04; // 左右容許誤差 4 公分

  if (std::abs(target_x) < DISTANCE_TOLERANCE_X && std::abs(target_y) < DISTANCE_TOLERANCE_Y) {
    RCLCPP_INFO(ros_node_->get_logger(), "🎉 [成功] 已進入完美對接範圍！發布煞車並結束節點。");
    
    auto stop_msg = geometry_msgs::msg::Twist(); 
    cmd_vel_pub_->publish(stop_msg);
    
    return BT::NodeStatus::SUCCESS; // 動作完成，行為樹可跳下一顆節點
  }

  // ==========================================================
  // 🚗 步驟 C：P 控制器計算與發布 (使用距離與 atan2 角度控制)
  // ==========================================================
  auto twist_msg = geometry_msgs::msg::Twist();

  // 1. 計算目標與機器人正前方的夾角誤差 (-PI 到 +PI)
  double angle_error = std::atan2(target_y, target_x);
  
  // 2. 計算到目標的直線距離
  double distance = std::hypot(target_x, target_y);

  // 3. 旋轉 Z 控制：根據夾角誤差轉向
  twist_msg.angular.z = 1.2 * angle_error;

  // 4. 線性 X 控制：如果角度偏差超過 30 度 (約 0.52 弧度)，先原地對準不前進
  if (std::abs(angle_error) > 0.52) {
    twist_msg.linear.x = 0.0; 
    RCLCPP_INFO(ros_node_->get_logger(), "🔄 角度偏差過大 (%.1f度)，原地旋轉對準中...", angle_error * 180.0 / M_PI);
  } else {
    // 角度已經大致朝前了，根據直線距離前進
    twist_msg.linear.x = 0.4 * distance;
  }

  // 5. 限速保護
  twist_msg.linear.x = std::clamp(twist_msg.linear.x, -0.12, 0.12);
  twist_msg.angular.z = std::clamp(twist_msg.angular.z, -0.3, 0.3);

  RCLCPP_INFO(ros_node_->get_logger(), "🔒 [發布] 速度 X: %.3f, 轉向 Z: %.3f\n-------------------", twist_msg.linear.x, twist_msg.angular.z);

  cmd_vel_pub_->publish(twist_msg);
  return BT::NodeStatus::RUNNING;
}

// 5. 當行為樹要求中斷時
void VisualApproach::onHalted()
{
  RCLCPP_WARN(ros_node_->get_logger(), "⚠️ 視覺逼近中斷，緊急煞車！");
  auto twist_msg = geometry_msgs::msg::Twist();
  cmd_vel_pub_->publish(twist_msg); 
}

} // namespace robot_behavior