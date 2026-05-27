#ifndef ROBOT_BEHAVIOR_FIND_VALID_DOLL_HPP_
#define ROBOT_BEHAVIOR_FIND_VALID_DOLL_HPP_

#include <mutex>
#include "rclcpp/rclcpp.hpp"
#include "behaviortree_cpp/action_node.h"
#include "geometry_msgs/msg/point.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp" // 新增 PoseStamped 標頭檔

namespace robot_behavior {

class FindValidDoll : public BT::StatefulActionNode {
public:
    FindValidDoll(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node);
    
    // 必須宣告這個靜態函式
    static BT::PortsList providedPorts();

    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override;

private:
    void targetCallback(const geometry_msgs::msg::Point::SharedPtr msg);

    rclcpp::Node::SharedPtr ros_node_;
    rclcpp::Subscription<geometry_msgs::msg::Point>::SharedPtr target_sub_;
    
    geometry_msgs::msg::Point latest_target_;
    bool msg_received_;
    std::mutex data_mutex_; // 新增：保護多執行緒資料安全的鎖
};

} // namespace robot_behavior

#endif