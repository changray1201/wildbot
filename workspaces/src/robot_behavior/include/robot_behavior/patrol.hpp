#pragma once
#include "behaviortree_cpp/action_node.h"
#include "rclcpp/rclcpp.hpp"

namespace robot_behavior {

class Patrol : public BT::StatefulActionNode {
public:
    Patrol(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node);
    
    static BT::PortsList providedPorts() { return {}; }

    // StatefulActionNode 必須實作的三大生命週期函數
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override;

private:
    rclcpp::Node::SharedPtr ros_node_;
    int current_waypoint_index_ = 1;
};

} // namespace robot_behavior