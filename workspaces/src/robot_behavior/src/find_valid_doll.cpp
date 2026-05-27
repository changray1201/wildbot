#include "robot_behavior/find_valid_doll.hpp"
#include <cmath>
#include "tf2/LinearMath/Quaternion.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

namespace robot_behavior {

FindValidDoll::FindValidDoll(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node)
  : BT::StatefulActionNode(name, config), ros_node_(node), msg_received_(false)
{
    // 📻 建立訂閱，接收 YOLO 的目標資訊
    target_sub_ = ros_node_->create_subscription<geometry_msgs::msg::Point>(
        "/yolo/targets_info", 10,
        std::bind(&FindValidDoll::targetCallback, this, std::placeholders::_1)
    );
}

// ⚠️ 必須宣告這個節點會輸出什麼變數到黑板上！
BT::PortsList FindValidDoll::providedPorts()
{
    return {
        BT::OutputPort<geometry_msgs::msg::PoseStamped>("target_pose")
    };
}

void FindValidDoll::targetCallback(const geometry_msgs::msg::Point::SharedPtr msg)
{
    // 🔒 加上互斥鎖，防止與行為樹的 tick 迴圈發生資料碰撞
    std::lock_guard<std::mutex> lock(data_mutex_);
    latest_target_ = *msg;
    msg_received_ = true; 
}

BT::NodeStatus FindValidDoll::onStart()
{
    RCLCPP_INFO(ros_node_->get_logger(), "👀 開始尋找目標娃娃...");
    
    std::lock_guard<std::mutex> lock(data_mutex_);
    msg_received_ = false; // 清空舊資料
    return BT::NodeStatus::RUNNING;
}

BT::NodeStatus FindValidDoll::onRunning()
{
    geometry_msgs::msg::Point current_target;
    bool has_new_msg = false;

    {
        // 🔒 安全地把資料拿出來
        std::lock_guard<std::mutex> lock(data_mutex_);
        if (msg_received_) {
            current_target = latest_target_;
            has_new_msg = true;
            msg_received_ = false; // 讀取完就重置，避免重複處理
        }
    }

    if (has_new_msg) {
        // 📐 1. 將 YOLO 的距離(x)與角度(y)轉換為 ROS 的 PoseStamped
        // 假設 YOLO 傳來的角度 (current_target.y) 是度數 (Degree)，將其轉為弧度 (Radian)
        double angle_rad = current_target.y * M_PI / 180.0;
        double distance = current_target.x;

        geometry_msgs::msg::PoseStamped goal_pose;
        goal_pose.header.stamp = ros_node_->now();
        goal_pose.header.frame_id = "base_link"; // 關鍵：告訴導航這是在車體前方的相對座標！

        // 🧮 極座標轉直角座標 (X 前方, Y 左方)
        goal_pose.pose.position.x = distance * std::cos(angle_rad);
        goal_pose.pose.position.y = distance * std::sin(angle_rad);
        goal_pose.pose.position.z = 0.0; // 導航不需要 Z 軸高度

        // 計算車體要轉向娃娃的角度 (四元數)
        tf2::Quaternion q;
        q.setRPY(0, 0, angle_rad);
        goal_pose.pose.orientation.x = q.x();
        goal_pose.pose.orientation.y = q.y();
        goal_pose.pose.orientation.z = q.z();
        goal_pose.pose.orientation.w = q.w();

        // 📝 2. 把算好的完整座標寫入黑板！(這才對得上 XML 裡的 target_pose)
        setOutput("target_pose", goal_pose);

        RCLCPP_INFO(ros_node_->get_logger(), "🎯 鎖定目標！轉換為目標座標 X:%.2f, Y:%.2f，已寫入黑板交接給導航！", 
                    goal_pose.pose.position.x, goal_pose.pose.position.y);
        
        return BT::NodeStatus::SUCCESS;
    }

    return BT::NodeStatus::RUNNING;
}

void FindValidDoll::onHalted()
{
    RCLCPP_INFO(ros_node_->get_logger(), "🛑 尋找娃娃任務被中斷");
}

} // namespace robot_behavior