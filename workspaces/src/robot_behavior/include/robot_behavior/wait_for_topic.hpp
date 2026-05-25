#pragma once

#include "behaviortree_cpp/action_node.h"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include <mutex>
#include <string>

namespace robot_behavior {

class WaitForTopic : public BT::StatefulActionNode {
public:
    // 建構子，需要傳入 ROS Node 以便建立 Subscriber
    WaitForTopic(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node);
    
    // 定義輸出埠 (對應 XML 裡的 command_out="{cmd}")
    // 🌟 定義這個積木的「輸入與輸出」端口
    static BT::PortsList providedPorts() {
        return {
            // 讓 XML 可以傳入字串給這個積木
            BT::InputPort<std::string>("topic_name"), 
            
            // 讓積木可以把字串寫回黑板的變數裡
            BT::OutputPort<std::string>("command_out") 
        };
    }
    
    // StatefulActionNode 必須實作的三個狀態函式
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override;

private:
    rclcpp::Node::SharedPtr ros_node_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_;
    
    // 收到 Topic 時會觸發的回呼函式
    void commandCallback(const std_msgs::msg::String::SharedPtr msg);

    std::mutex mutex_;
    std::string received_cmd_ = "";
};

} // namespace robot_behavior