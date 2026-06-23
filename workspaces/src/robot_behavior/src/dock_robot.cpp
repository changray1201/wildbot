#include "robot_behavior/dock_robot.hpp"

namespace robot_behavior {

DockRobot::DockRobot(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node)
    : BT::StatefulActionNode(name, config), ros_node_(node), goal_done_(false)
{
    action_client_ = rclcpp_action::create_client<DockAction>(ros_node_, "/dock_robot");
}

// 🌟 1. 完美對接你的 XML 屬性 (支援 ID 或 動態座標)
BT::PortsList DockRobot::providedPorts() {
    return { 
        BT::InputPort<geometry_msgs::msg::PoseStamped>("dock_pose", "相機傳來的動態對接座標"),
        BT::InputPort<bool>("use_dock_id", "是否使用內建站點名稱"),
        BT::InputPort<std::string>("dock_id", "站點名稱(若use_dock_id為true)"),
        BT::InputPort<std::string>("dock_type", "對接演算法類型")
    };
}

BT::NodeStatus DockRobot::onStart() {
    if (!action_client_->wait_for_action_server(std::chrono::seconds(2))) {
        RCLCPP_ERROR(ros_node_->get_logger(), "🚨 找不到 /dock_robot 伺服器！");
        return BT::NodeStatus::FAILURE;
    }

    auto goal_msg = DockAction::Goal();

    // 解析 XML 傳入的行為樹參數
    bool use_id = false;
    getInput("use_dock_id", use_id);
    goal_msg.use_dock_id = use_id;

    if (use_id) {
        getInput("dock_id", goal_msg.dock_id);
        RCLCPP_INFO(ros_node_->get_logger(), "🔥 發送對接請求，使用內建 ID: [%s]", goal_msg.dock_id.c_str());
    } else {
        getInput("dock_pose", goal_msg.dock_pose);
        getInput("dock_type", goal_msg.dock_type);
        RCLCPP_INFO(ros_node_->get_logger(), "🔥 發送視覺對接請求，目標類型: [%s]", goal_msg.dock_type.c_str());
    }

    goal_done_ = false;
    auto send_goal_options = rclcpp_action::Client<DockAction>::SendGoalOptions();

    // 🌟 2. 非同步 Callback：當伺服器「接受」或「拒絕」任務時觸發
    send_goal_options.goal_response_callback =
        [this](const GoalHandleDock::SharedPtr & goal_handle) {
            if (!goal_handle) {
                RCLCPP_ERROR(ros_node_->get_logger(), "❌ 對接請求被伺服器拒絕！");
                this->goal_done_ = true; // 強制結束
            } else {
                this->goal_handle_ = goal_handle;
            }
        };

    // 🌟 3. 非同步 Callback：當任務「徹底執行完畢 (成功/失敗)」時觸發
    send_goal_options.result_callback =
        [this](const GoalHandleDock::WrappedResult & result) {
            this->goal_result_code_ = result.code;
            this->goal_done_ = true; // 標記為完成，通知 onRunning 判斷結果
        };

    // 發送請求後立刻回傳 RUNNING，絕對不阻塞執行緒！
    action_client_->async_send_goal(goal_msg, send_goal_options);
    return BT::NodeStatus::RUNNING;
}

BT::NodeStatus DockRobot::onRunning() {
    // 如果 Callback 還沒把 goal_done_ 設為 true，就繼續保持 RUNNING 狀態
    if (!goal_done_) {
        return BT::NodeStatus::RUNNING;
    }

    // 任務結束了，檢查結果
    switch (goal_result_code_) {
        case rclcpp_action::ResultCode::SUCCEEDED:
            RCLCPP_INFO(ros_node_->get_logger(), "✅ 完美進入 Dock！對接達成！");
            return BT::NodeStatus::SUCCESS;
        case rclcpp_action::ResultCode::ABORTED:
            RCLCPP_ERROR(ros_node_->get_logger(), "❌ 對接過程中被終止 (Aborted)！");
            return BT::NodeStatus::FAILURE;
        case rclcpp_action::ResultCode::CANCELED:
            RCLCPP_ERROR(ros_node_->get_logger(), "❌ 對接被取消 (Canceled)！");
            return BT::NodeStatus::FAILURE;
        default:
            RCLCPP_ERROR(ros_node_->get_logger(), "❌ 對接發生未知錯誤！");
            return BT::NodeStatus::FAILURE;
    }
}

void DockRobot::onHalted() {
    RCLCPP_WARN(ros_node_->get_logger(), "⚠️ 收到中斷指令，停止對接！(可能是相機沒看到娃娃了)");
    if (goal_handle_) {
        action_client_->async_cancel_goal(goal_handle_);
    }
}

} // namespace robot_behavior