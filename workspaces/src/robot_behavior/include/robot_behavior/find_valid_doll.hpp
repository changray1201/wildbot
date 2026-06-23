#ifndef FIND_VALID_DOLL_HPP
#define FIND_VALID_DOLL_HPP

#include <string>
#include <memory>
#include <mutex>
#include <chrono>

#include "behaviortree_cpp/condition_node.h"
#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"

namespace robot_behavior
{

class FindValidDoll : public BT::ConditionNode
{
public:
  // 🌟 完美接收 main.cpp 傳遞過來的 Node
  FindValidDoll(const std::string& name, const BT::NodeConfig& config, std::shared_ptr<rclcpp::Node> node);

  static BT::PortsList providedPorts();

  BT::NodeStatus tick() override;

private:
  void poseCallback(const geometry_msgs::msg::PoseStamped::SharedPtr msg);

  rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr sub_;
  geometry_msgs::msg::PoseStamped latestpose;
  std::chrono::steady_clock::time_point last_msgtime;
  bool haspose;
  std::mutex mutex_;
};

}  // namespace robot_behavior

#endif // FIND_VALID_DOLL_HPP