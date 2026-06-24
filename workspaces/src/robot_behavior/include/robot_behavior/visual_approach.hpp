#ifndef VISUAL_APPROACH_HPP
#define VISUAL_APPROACH_HPP

#include "behaviortree_cpp/action_node.h"
#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
// ❌ 已經移除 tf2_ros 相關標頭檔，因為 C++ 端不再需要自己算座標轉換了

namespace robot_behavior
{

class VisualApproach : public BT::StatefulActionNode
{
public:
  VisualApproach(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node);

  static BT::PortsList providedPorts();

  BT::NodeStatus onStart() override;
  BT::NodeStatus onRunning() override;
  void onHalted() override;

private:
  rclcpp::Node::SharedPtr ros_node_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;

  // ✨ 【全新架構】：直接訂閱 Python 發布的即時對接座標
  rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr dock_pose_sub_;
  geometry_msgs::msg::PoseStamped::SharedPtr latest_dock_pose_;
};

} // namespace robot_behavior

#endif // VISUAL_APPROACH_HPP