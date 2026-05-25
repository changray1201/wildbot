#ifndef EXECUTE_SCRIPT_HPP
#define EXECUTE_SCRIPT_HPP

#include <string>
#include "behaviortree_cpp/action_node.h"
#include "rclcpp/rclcpp.hpp"

namespace robot_behavior {

class ExecuteScript : public BT::SyncActionNode {
public:
    // 建構子：接收積木名稱、設定，以及 ROS 2 的節點指標
    ExecuteScript(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node);

    // 定義這顆積木對外開放的 Port (接收 XML 傳來的字串)
    static BT::PortsList providedPorts();

    // 積木的主要執行邏輯
    BT::NodeStatus tick() override;

private:
    rclcpp::Node::SharedPtr ros_node_; // 儲存 ROS 2 節點，用來發送 Log 與建立通訊
};

} // namespace robot_behavior

#endif // EXECUTE_SCRIPT_HPP