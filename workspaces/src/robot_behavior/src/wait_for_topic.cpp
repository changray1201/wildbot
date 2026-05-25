#include "robot_behavior/wait_for_topic.hpp"
#include <iostream>

namespace robot_behavior {

WaitForTopic::WaitForTopic(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node)
  : BT::StatefulActionNode(name, config), ros_node_(node) 
{
    // 建立訂閱者，訂閱名稱為 "/mission_command"
    sub_ = ros_node_->create_subscription<std_msgs::msg::String>(
        "/mission_command", 10, 
        std::bind(&WaitForTopic::commandCallback, this, std::placeholders::_1)
    );
}

// 【背景執行緒】當收到指令時觸發
void WaitForTopic::commandCallback(const std_msgs::msg::String::SharedPtr msg) {
    std::lock_guard<std::mutex> lock(mutex_); // 上鎖確保安全
    received_cmd_ = msg->data; // 將收到的字串存起來 (例如 "mode2")
}

// 【行為樹】當節點第一次被執行時
BT::NodeStatus WaitForTopic::onStart() {
    std::lock_guard<std::mutex> lock(mutex_);
    received_cmd_ = ""; // 每次重新開始等待時，先清空舊指令
    
    // 提示目前正在等待指令
    std::cout << "\033[1;34m[守門員] 正在等待 /mission_command 發送指令...\033[0m" << std::endl;
    
    return BT::NodeStatus::RUNNING; 
}

// 【行為樹】當節點處於 RUNNING 狀態時，會不斷被 Tick
BT::NodeStatus WaitForTopic::onRunning() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 檢查是否收到指令了？
    if (!received_cmd_.empty()) {
        std::cout << "\n\033[1;32m[指揮中心] 收到任務指令: [" << received_cmd_ << "]\033[0m" << std::endl;
        
        // 將指令寫入黑板，供後面的 StageSelector (Fallback) 判斷
        setOutput("command_out", received_cmd_);
        
        return BT::NodeStatus::SUCCESS; // 成功放行！
    }

    return BT::NodeStatus::RUNNING; // 還沒收到，繼續卡在這裡等
}

// 【行為樹】當節點被外部強制中斷時
void WaitForTopic::onHalted() { 
    std::lock_guard<std::mutex> lock(mutex_);
    received_cmd_ = ""; 
}

} // namespace robot_behavior