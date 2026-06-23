#include "robot_behavior/navigate_to_pose.hpp"
#include <iostream>
#include <chrono>

namespace robot_behavior {

NavigateToPose::NavigateToPose(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node)
  : BT::StatefulActionNode(name, config), ros_node_(node) 
{
    // 建立 Action Client，連接到 Nav2 的伺服器
    action_client_ = rclcpp_action::create_client<NavAction>(ros_node_, "/navigate_to_pose");
}

// 宣告這個積木需要讀取什麼輸入
BT::PortsList NavigateToPose::providedPorts()
{
    // 這裡我們不指定嚴格型態，讓它可以收字串 ("WAYPOINT_A") 或座標物件 ({target_pose})
    return { BT::InputPort<BT::Any>("pose") };
}

BT::NodeStatus NavigateToPose::onStart() {
    if (!action_client_->wait_for_action_server(std::chrono::seconds(3))) {
        RCLCPP_ERROR(ros_node_->get_logger(), "🚨 找不到 /navigate_to_pose 伺服器！確認 Nav2 是否啟動");
        return BT::NodeStatus::FAILURE;
    }

    auto goal_msg = NavAction::Goal();
    BT::Any pose_any;

    if (!getInput("pose", pose_any)) {
        RCLCPP_ERROR(ros_node_->get_logger(), "缺少導航目標參數！");
        return BT::NodeStatus::FAILURE;
    }

    // 🧠 智慧判斷：如果輸入的是字串 (例如 "HOME", "WAYPOINT_A")
    if (pose_any.isType<std::string>()) {
        std::string location_name = pose_any.cast<std::string>();
        RCLCPP_INFO(ros_node_->get_logger(), "🗺️ 準備導航至預設點位: [%s]", location_name.c_str());
        
        // TODO: 實戰中這裡要寫一個 Switch/Case，把 "HOME" 換成真實的 XYZ 座標
        // 這裡先隨便給個假座標當作範例，讓程式能編譯
        goal_msg.pose.header.frame_id = "map";
        goal_msg.pose.pose.position.x = 0.0;
        goal_msg.pose.pose.orientation.w = 1.0; 
    }
    // 🧠 智慧判斷：如果輸入的是黑板傳來的真實座標物件 (例如從相機傳來的)
    else if (pose_any.isType<geometry_msgs::msg::PoseStamped>()) {
        goal_msg.pose = pose_any.cast<geometry_msgs::msg::PoseStamped>();
        RCLCPP_INFO(ros_node_->get_logger(), "🎯 接收到黑板動態座標！準備前往目標 X:%.2f, Y:%.2f", 
                    goal_msg.pose.pose.position.x, goal_msg.pose.pose.position.y);
    } 
    else {
        RCLCPP_ERROR(ros_node_->get_logger(), "不支援的導航目標格式！");
        return BT::NodeStatus::FAILURE;
    }
    /*
    // 發送導航請求
    auto send_goal_options = rclcpp_action::Client<NavAction>::SendGoalOptions();
    auto goal_handle_future = action_client_->async_send_goal(goal_msg, send_goal_options);
    
    if (rclcpp::spin_until_future_complete(ros_node_, goal_handle_future) != rclcpp::FutureReturnCode::SUCCESS) {
        return BT::NodeStatus::FAILURE;
    }

    goal_handle_ = goal_handle_future.get();
    if (!goal_handle_) {
        RCLCPP_ERROR(ros_node_->get_logger(), "導航請求被 Nav2 拒絕！");
        return BT::NodeStatus::FAILURE;
    }

    return BT::NodeStatus::RUNNING; 
    */
    
    // ✅ 改成這樣
    goal_rejected_ = false;
    goal_handle_ = nullptr;

    auto send_goal_options = rclcpp_action::Client<NavAction>::SendGoalOptions();
    send_goal_options.goal_response_callback =
        [this](const rclcpp_action::ClientGoalHandle<NavAction>::SharedPtr & handle) {
            goal_handle_ = handle;
            if (!goal_handle_) {
                RCLCPP_ERROR(ros_node_->get_logger(), "導航請求被 Nav2 拒絕！");
                goal_rejected_ = true;
            }
        };

    action_client_->async_send_goal(goal_msg, send_goal_options);
    return BT::NodeStatus::RUNNING;
}
/*
BT::NodeStatus NavigateToPose::onRunning() {
    if (action_client_->wait_for_action_server(std::chrono::milliseconds(0))) {
        auto status = goal_handle_->get_status();
        if (status == action_msgs::msg::GoalStatus::STATUS_SUCCEEDED) {
            RCLCPP_INFO(ros_node_->get_logger(), "✅ 成功抵達目標點！");
            return BT::NodeStatus::SUCCESS;
        } else if (status == action_msgs::msg::GoalStatus::STATUS_ABORTED || status == action_msgs::msg::GoalStatus::STATUS_CANCELED) {
            RCLCPP_ERROR(ros_node_->get_logger(), "❌ 導航失敗或中途取消！");
            return BT::NodeStatus::FAILURE;
        }
    }
    return BT::NodeStatus::RUNNING; 
}
*/

BT::NodeStatus NavigateToPose::onRunning() {
    if (goal_rejected_) return BT::NodeStatus::FAILURE;
    if (!goal_handle_) return BT::NodeStatus::RUNNING; // 還在等 callback

    auto status = goal_handle_->get_status();
    if (status == action_msgs::msg::GoalStatus::STATUS_SUCCEEDED) {
        RCLCPP_INFO(ros_node_->get_logger(), "✅ 成功抵達目標點！");
        return BT::NodeStatus::SUCCESS;
    } else if (status == action_msgs::msg::GoalStatus::STATUS_ABORTED ||
               status == action_msgs::msg::GoalStatus::STATUS_CANCELED) {
        RCLCPP_ERROR(ros_node_->get_logger(), "❌ 導航失敗或中途取消！");
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::RUNNING;
}

void NavigateToPose::onHalted() {
    RCLCPP_WARN(ros_node_->get_logger(), "⚠️ 收到中斷指令，緊急煞車並取消導航！");
    if (goal_handle_) {
        action_client_->async_cancel_goal(goal_handle_);
    }
}

} // namespace robot_behavior