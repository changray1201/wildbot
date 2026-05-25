#include "robot_behavior/navigate_to_pose.hpp"
#include <iostream>

namespace robot_behavior {

NavigateToPose::NavigateToPose(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node)
  : BT::StatefulActionNode(name, config), ros_node_(node) {}

BT::NodeStatus NavigateToPose::onStart() {
    if (!getInput<std::string>("pose", current_target_)) {
        throw BT::RuntimeError("缺少導航目標");
    }
    
    std::cout << "\033[1;33m[Nav] 發送導航目標至 Nav2: " << current_target_ << "\033[0m" << std::endl;
    
    // 記錄開始導航的當下時間
    start_time_ = std::chrono::system_clock::now();
    
    return BT::NodeStatus::RUNNING; 
}

BT::NodeStatus NavigateToPose::onRunning() {
    // 計算從 onStart 到現在經過了幾秒
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();

    // 如果超過 3 秒，就當作走到了
    if (elapsed >= 3) {
        std::cout << "\033[1;32m[Nav] 模擬導航完成！已抵達目標：" << current_target_ << "\033[0m" << std::endl;
        return BT::NodeStatus::SUCCESS;
    }
    
    // 還沒 3 秒，繼續回傳 RUNNING
    return BT::NodeStatus::RUNNING; 
}

void NavigateToPose::onHalted() {
    std::cout << "\033[1;35m[Nav] 導航被中斷！取消 Nav2 Goal。\033[0m" << std::endl;
}
}