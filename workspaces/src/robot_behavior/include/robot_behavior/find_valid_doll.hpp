#ifndef ROBOT_BEHAVIOR_FIND_VALID_DOLL_HPP
#define ROBOT_BEHAVIOR_FIND_VALID_DOLL_HPP

#include "rclcpp/rclcpp.hpp"
#include "behaviortree_cpp/action_node.h"
#include "geometry_msgs/msg/point.hpp" // YOLO 傳來的資料型態

namespace robot_behavior {

class FindValidDoll : public BT::StatefulActionNode
{
public:
    FindValidDoll(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node);

    // 🌟 定義這個積木的「輸出端口」，用來把資料寫進黑板
    static BT::PortsList providedPorts() {
        return {
            BT::OutputPort<double>("target_dist"),
            BT::OutputPort<double>("target_angle"),
            BT::OutputPort<double>("target_z")
        };
    }

    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override;

private:
    rclcpp::Node::SharedPtr ros_node_;
    // 建立一個訂閱者，準備接收 YOLO 的廣播
    rclcpp::Subscription<geometry_msgs::msg::Point>::SharedPtr target_sub_;
    
    bool msg_received_;
    geometry_msgs::msg::Point latest_target_;

    // 收到 YOLO 訊息時的處理函數
    void targetCallback(const geometry_msgs::msg::Point::SharedPtr msg);
};

} // namespace robot_behavior

#endif // ROBOT_BEHAVIOR_FIND_VALID_DOLL_HPP