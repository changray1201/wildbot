#include "robot_behavior/dock_robot.hpp"

namespace robot_behavior {

// 1. 建構子實作
DockRobot::DockRobot(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node)
    : BT::StatefulActionNode(name, config), ros_node_(node) 
{
    // 連接真實的 /dock_robot Action Server
    action_client_ = rclcpp_action::create_client<DockAction>(ros_node_, "/dock_robot");
}

// 2. 宣告輸入屬性
BT::PortsList DockRobot::providedPorts() {
    return { 
        BT::InputPort<std::string>("target_item") 
    };
}

// 3. 啟動對接任務
BT::NodeStatus DockRobot::onStart() {
    if (!action_client_->wait_for_action_server(std::chrono::seconds(3))) {
        RCLCPP_ERROR(ros_node_->get_logger(), "🚨 找不到 /dock_robot 伺服器！確認底層是否啟動");
        return BT::NodeStatus::FAILURE;
    }

    std::string target_item;
    if (!getInput("target_item", target_item)) {
        RCLCPP_ERROR(ros_node_->get_logger(), "缺少 target_item 參數！");
        return BT::NodeStatus::FAILURE;
    }

    auto goal_msg = DockAction::Goal();
    goal_msg.use_dock_id = true;
    goal_msg.dock_id = target_item;

    RCLCPP_INFO(ros_node_->get_logger(), "🔥 發送精準對接請求，目標: [%s]", target_item.c_str());

    auto send_goal_options = rclcpp_action::Client<DockAction>::SendGoalOptions();
    auto goal_handle_future = action_client_->async_send_goal(goal_msg, send_goal_options);
    
    if (rclcpp::spin_until_future_complete(ros_node_, goal_handle_future) != rclcpp::FutureReturnCode::SUCCESS) {
        return BT::NodeStatus::FAILURE;
    }

    goal_handle_ = goal_handle_future.get();
    if (!goal_handle_) {
        RCLCPP_ERROR(ros_node_->get_logger(), "對接請求被伺服器拒絕！");
        return BT::NodeStatus::FAILURE;
    }

    return BT::NodeStatus::RUNNING;
}

// 4. 監聽執行狀態
BT::NodeStatus DockRobot::onRunning() {
    if (action_client_->wait_for_action_server(std::chrono::milliseconds(0))) {
        auto status = goal_handle_->get_status();
        if (status == action_msgs::msg::GoalStatus::STATUS_SUCCEEDED) {
            RCLCPP_INFO(ros_node_->get_logger(), "✅ 對接完美達成！準備執行下一步...");
            return BT::NodeStatus::SUCCESS;
        } else if (status == action_msgs::msg::GoalStatus::STATUS_ABORTED || status == action_msgs::msg::GoalStatus::STATUS_CANCELED) {
            RCLCPP_ERROR(ros_node_->get_logger(), "❌ 對接失敗或被取消！");
            return BT::NodeStatus::FAILURE;
        }
    }
    return BT::NodeStatus::RUNNING;
}

// 5. 中斷任務處理
void DockRobot::onHalted() {
    RCLCPP_WARN(ros_node_->get_logger(), "⚠️ 收到中斷指令，停止對接！");
    if (goal_handle_) {
        action_client_->async_cancel_goal(goal_handle_);
    }
}

} // namespace robot_behavior