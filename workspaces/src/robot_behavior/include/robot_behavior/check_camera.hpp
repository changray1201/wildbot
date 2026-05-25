#pragma once
#include <string>
#include "behaviortree_cpp/condition_node.h"
#include "rclcpp/rclcpp.hpp"

namespace robot_behavior {

class CheckCamera : public BT::ConditionNode {
public:
    CheckCamera(const std::string& name, const BT::NodeConfig& config);
    
    // 宣告需要一個名為 status 的輸入參數
    static BT::PortsList providedPorts();
    
    BT::NodeStatus tick() override;
};

} // namespace robot_behavior