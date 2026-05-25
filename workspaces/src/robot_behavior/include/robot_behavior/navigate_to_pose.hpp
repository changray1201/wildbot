#pragma once
#include "behaviortree_cpp/action_node.h"
#include "rclcpp/rclcpp.hpp"
#include <chrono> // 【重要】必須引入時間函式庫

namespace robot_behavior {

class NavigateToPose : public BT::StatefulActionNode {
public:
    NavigateToPose(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node);
    static BT::PortsList providedPorts() {
        return { BT::InputPort<std::string>("pose") };
    }

    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override;

private:
    rclcpp::Node::SharedPtr ros_node_;
    std::string current_target_;
    
    // 【重要】用來記錄導航開始時間的變數
    std::chrono::system_clock::time_point start_time_; 
};
}