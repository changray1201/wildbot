#include "robot_behavior/navigate_to_pose.hpp"

namespace robot_behavior {

NavigateToPose::NavigateToPose(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node)
  : BT::StatefulActionNode(name, config), ros_node_(node)
{
    // 建立聯絡 Nav2 導航伺服器的電話線 (Action Client)
    action_client_ = rclcpp_action::create_client<Nav2Action>(ros_node_, "navigate_to_pose");
}

// 🗺️ 【你的戰術地圖字典】：請在這裡填寫你們場地的真實 X, Y 座標！
bool NavigateToPose::getPoseFromString(const std::string& pose_name, geometry_msgs::msg::PoseStamped& pose)
{
    // 建立一個對應表 (名稱 -> {X, Y})
    std::map<std::string, std::pair<double, double>> waypoint_dict = {
        {"HOME", {0.0, 0.0}},
        {"BRIDGE_START_POSE", {1.5, 2.0}},
        {"BRIDGE_END_POSE", {3.0, 2.0}},
        {"WAYPOINT_A", {1.0, 1.0}},
        {"WAYPOINT_B", {2.0, 1.0}},
        {"WAYPOINT_C", {2.0, -1.0}},
        {"WAYPOINT_D", {1.0, -1.0}}
    };

    // 如果 XML 傳進來的字串在地圖裡找不到，就報錯
    if (waypoint_dict.find(pose_name) == waypoint_dict.end()) {
        RCLCPP_ERROR(ros_node_->get_logger(), "找不到這個地點：[%s]", pose_name.c_str());
        return false;
    }

    // 將找到的座標打包成 ROS 2 規定的格式
    pose.header.frame_id = "map";  // 通常 Nav2 的全局座標系叫做 map
    pose.header.stamp = ros_node_->now();
    pose.pose.position.x = waypoint_dict[pose_name].first;
    pose.pose.position.y = waypoint_dict[pose_name].second;
    pose.pose.position.z = 0.0;
    
    // 假設車頭永遠朝向正前方 (無旋轉)。如果有角度需求，請修改 Quaternion
    pose.pose.orientation.x = 0.0;
    pose.pose.orientation.y = 0.0;
    pose.pose.orientation.z = 0.0;
    pose.pose.orientation.w = 1.0; 

    return true;
}

BT::NodeStatus NavigateToPose::onStart()
{
    std::string pose_name;
    if (!getInput("pose", pose_name)) {
        RCLCPP_ERROR(ros_node_->get_logger(), "NavigateToPose 缺少 'pose' 參數！");
        return BT::NodeStatus::FAILURE;
    }

    geometry_msgs::msg::PoseStamped target_pose;
    if (!getPoseFromString(pose_name, target_pose)) {
        return BT::NodeStatus::FAILURE;
    }

    // 確認 Nav2 司機有沒有上班？(等待 2 秒)
    if (!action_client_->wait_for_action_server(std::chrono::seconds(2))) {
        RCLCPP_ERROR(ros_node_->get_logger(), "連不上 Nav2 導航伺服器 (/navigate_to_pose)！請檢查導航是否啟動。");
        return BT::NodeStatus::FAILURE;
    }

    // 準備發送目標
    goal_done_ = false;
    goal_success_ = false;

    auto send_goal_options = rclcpp_action::Client<Nav2Action>::SendGoalOptions();
    // 設定當車子抵達目的地時，會自動呼叫 resultCallback
    send_goal_options.result_callback = std::bind(&NavigateToPose::resultCallback, this, std::placeholders::_1);
    
    auto goal_msg = Nav2Action::Goal();
    goal_msg.pose = target_pose;
    
    RCLCPP_INFO(ros_node_->get_logger(), "🚗 開始導航前往：[%s] (X=%.2f, Y=%.2f)", 
                pose_name.c_str(), target_pose.pose.position.x, target_pose.pose.position.y);
    
    // 非同步送出目標 (不會卡死主程式)
    action_client_->async_send_goal(goal_msg, send_goal_options);

    // 告訴大腦：「我正在跑，請耐心等我！」
    return BT::NodeStatus::RUNNING;
}

BT::NodeStatus NavigateToPose::onRunning()
{
    // 如果 callback 函數已經被觸發 (代表車子停下來了)
    if (goal_done_) {
        return goal_success_ ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
    }
    // 還沒抵達，繼續跑
    return BT::NodeStatus::RUNNING;
}

void NavigateToPose::onHalted()
{
    // 如果大腦突然決定取消這個任務 (例如 Fallback 觸發)，就通知 Nav2 煞車
    RCLCPP_INFO(ros_node_->get_logger(), "🛑 收到中斷指令，取消導航任務！");
    // 注意：實務上應該抓取 goal_handle 來 cancel，這裡為求穩定簡化，讓 Nav2 保留狀態，
    // 或實作 action_client_->async_cancel_all_goals(); 
}

void NavigateToPose::resultCallback(const GoalHandle::WrappedResult& result)
{
    goal_done_ = true;
    switch (result.code) {
        case rclcpp_action::ResultCode::SUCCEEDED:
            RCLCPP_INFO(ros_node_->get_logger(), "✅ 成功抵達目的地！");
            goal_success_ = true;
            break;
        case rclcpp_action::ResultCode::ABORTED:
            RCLCPP_ERROR(ros_node_->get_logger(), "❌ 導航被放棄 (可能撞牆或算不出路徑)");
            goal_success_ = false;
            break;
        case rclcpp_action::ResultCode::CANCELED:
            RCLCPP_WARN(ros_node_->get_logger(), "⚠️ 導航被手動取消");
            goal_success_ = false;
            break;
        default:
            RCLCPP_ERROR(ros_node_->get_logger(), "❓ 未知的導航錯誤");
            goal_success_ = false;
            break;
    }
}

} // namespace robot_behavior