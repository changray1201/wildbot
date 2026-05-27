#ifndef ROBOT_BEHAVIOR_DOCK_ROBOT_HPP_
#define ROBOT_BEHAVIOR_DOCK_ROBOT_HPP_

#include <string>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "behaviortree_cpp/action_node.h"
#include "opennav_docking_msgs/action/dock_robot.hpp"

namespace robot_behavior {

class DockRobot : public BT::StatefulActionNode {
public:
    using DockAction = opennav_docking_msgs::action::DockRobot;
    using GoalHandle = rclcpp_action::ClientGoalHandle<DockAction>;

    DockRobot(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node);
    static BT::PortsList providedPorts();
    
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override;

private:
    rclcpp::Node::SharedPtr ros_node_;
    rclcpp_action::Client<DockAction>::SharedPtr action_client_;
    std::shared_ptr<GoalHandle> goal_handle_;
};

} // namespace robot_behavior

#endif  // ROBOT_BEHAVIOR_DOCK_ROBOT_HPP_