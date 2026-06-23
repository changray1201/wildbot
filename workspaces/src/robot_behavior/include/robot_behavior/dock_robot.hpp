#ifndef DOCK_ROBOT_HPP
#define DOCK_ROBOT_HPP

#include <string>
#include <memory>

#include "behaviortree_cpp/action_node.h"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "opennav_docking_msgs/action/dock_robot.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"

namespace robot_behavior
{

class DockRobot : public BT::StatefulActionNode
{
public:
    using DockAction = opennav_docking_msgs::action::DockRobot;
    using GoalHandleDock = rclcpp_action::ClientGoalHandle<DockAction>;

    DockRobot(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node);

    static BT::PortsList providedPorts();

    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override;

private:
    rclcpp::Node::SharedPtr ros_node_;
    rclcpp_action::Client<DockAction>::SharedPtr action_client_;
    GoalHandleDock::SharedPtr goal_handle_;

    // 用來非同步追蹤伺服器執行狀態的標記
    bool goal_done_;
    rclcpp_action::ResultCode goal_result_code_;
};

} // namespace robot_behavior

#endif // DOCK_ROBOT_HPP