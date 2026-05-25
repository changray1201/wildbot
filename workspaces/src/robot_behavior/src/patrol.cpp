#include "robot_behavior/patrol.hpp"
#include <iostream>

namespace robot_behavior {

Patrol::Patrol(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node)
  : BT::StatefulActionNode(name, config), ros_node_(node) {}

BT::NodeStatus Patrol::onStart() {
    std::cout << "[Patrol] 開始巡邏！發送目標至第 " << current_waypoint_index_ << " 個巡邏點。" << std::endl;
    // 實戰：在這裡呼叫 Nav2 讓機器人前往巡邏點
    
    return BT::NodeStatus::RUNNING; // 回傳 RUNNING 讓行為樹知道任務正在進行
}

BT::NodeStatus Patrol::onRunning() {
    // 實戰：在這裡檢查 Nav2 的狀態，如果抵達了，就將 current_waypoint_index_ + 1，然後前往下一個點
    
    // 因為它被包在 ReactiveFallback 下面，如果相機沒看到娃娃，這個節點就會一直維持 RUNNING
    return BT::NodeStatus::RUNNING;
}

void Patrol::onHalted() {
    // 【核心邏輯】當 FindValidDoll 回傳 SUCCESS 時，行為樹會立刻強制中斷這個節點！
    std::cout << "[Patrol] ⚠️ 巡邏被緊急中斷！(可能是發現娃娃了)" << std::endl;
    
    // 實戰：非常重要！你必須在這裡呼叫 Nav2 的 Action Client 發送 cancel_goal()，讓輪子馬上停下來
}

} // namespace robot_behavior