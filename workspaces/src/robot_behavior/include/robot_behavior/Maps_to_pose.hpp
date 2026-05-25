#ifndef ROBOT_BEHAVIOR_NAVIGATE_TO_POSE_HPP
#define ROBOT_BEHAVIOR_NAVIGATE_TO_POSE_HPP

#include <string>
#include <map>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "behaviortree_cpp/action_node.h"
#include "nav2_msgs/action/navigate_to_pose.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"

namespace robot_behavior {

// 繼承 StatefulActionNode，這是 BT.CPP 專門用來處理「需要花時間執行的非同步動作」的積木
class NavigateToPose : public BT::StatefulActionNode
{
public:
    using Nav2Action = nav2_msgs::action::NavigateToPose;
    using GoalHandle = rclcpp_action::ClientGoalHandle<Nav2Action>;

    // 建構子：需要接收 ROS 2 Node 來建立 Action Client
    NavigateToPose(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node);

    // 定義 XML 裡面可以輸入的參數 (例如: pose="HOME")
    static BT::PortsList providedPorts() {
        return { BT::InputPort<std::string>("pose") };
    }

    // 行為樹執行到這個節點時的生命週期
    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override;

private:
    rclcpp::Node::SharedPtr ros_node_;
    rclcpp_action::Client<Nav2Action>::SharedPtr action_client_;
    
    // 用來記錄目標是否完成、是否成功
    bool goal_done_;
    bool goal_success_;

    // Action Client 的回呼函數
    void resultCallback(const GoalHandle::WrappedResult& result);
    
    // 將字串 (例如 "HOME") 轉換為實體坐標的工具函數
    bool getPoseFromString(const std::string& pose_name, geometry_msgs::msg::PoseStamped& pose);
};

} // namespace robot_behavior

#endif // ROBOT_BEHAVIOR_NAVIGATE_TO_POSE_HPP