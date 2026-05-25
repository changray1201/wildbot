#pragma once
#include "behaviortree_cpp/action_node.h"
#include "rclcpp/rclcpp.hpp"
#include <string>

namespace robot_behavior {

class DockingTest : public BT::StatefulActionNode {
public:
    DockingTest(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node);
    
    static BT::PortsList providedPorts() { 
        return { BT::InputPort<std::string>("target_item") }; 
    }
    
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override;

private:
    rclcpp::Node::SharedPtr ros_node_;
};

} // namespace robot_behavior