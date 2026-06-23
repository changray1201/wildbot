#ifndef ROBOT_BEHAVIOR_NAVIGATE_TO_POSE_HPP_
#define ROBOT_BEHAVIOR_NAVIGATE_TO_POSE_HPP_

#include <string>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "behaviortree_cpp/action_node.h"
#include "nav2_msgs/action/navigate_to_pose.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"

namespace robot_behavior {

class NavigateToPose : public BT::StatefulActionNode {
public:
    using NavAction = nav2_msgs::action::NavigateToPose;
    using GoalHandle = rclcpp_action::ClientGoalHandle<NavAction>;

    NavigateToPose(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node);
    static BT::PortsList providedPorts();

    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override;

private:
    rclcpp::Node::SharedPtr ros_node_;
    rclcpp_action::Client<NavAction>::SharedPtr action_client_;
    std::shared_ptr<GoalHandle> goal_handle_;
    bool goal_rejected_ = false;  // 👈 新增這一行
};

} // namespace robot_behavior

#endif