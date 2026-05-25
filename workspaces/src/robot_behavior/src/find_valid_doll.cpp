#include "robot_behavior/find_valid_doll.hpp"

namespace robot_behavior {

FindValidDoll::FindValidDoll(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node)
  : BT::StatefulActionNode(name, config), ros_node_(node), msg_received_(false)
{
    // 📻 建立訂閱，頻道名稱對應你 Python 寫的 '/yolo/targets_info'
    target_sub_ = ros_node_->create_subscription<geometry_msgs::msg::Point>(
        "/yolo/targets_info", 10,
        std::bind(&FindValidDoll::targetCallback, this, std::placeholders::_1)
    );
}

void FindValidDoll::targetCallback(const geometry_msgs::msg::Point::SharedPtr msg)
{
    latest_target_ = *msg;
    msg_received_ = true; // 標記已收到熱騰騰的新資料
}

BT::NodeStatus FindValidDoll::onStart()
{
    RCLCPP_INFO(ros_node_->get_logger(), "👀 開始尋找目標娃娃...");
    msg_received_ = false; // 每次開始找娃娃時，都把舊資料清掉，確保拿到最新的
    return BT::NodeStatus::RUNNING;
}

BT::NodeStatus FindValidDoll::onRunning()
{
    // 如果收到 YOLO 傳來的新資料了
    if (msg_received_) {
        
        // 📝 把資料寫進行為樹的黑板 (Blackboard)，供後面的積木使用
        setOutput("target_dist", latest_target_.x);
        setOutput("target_angle", latest_target_.y);
        setOutput("target_z", latest_target_.z);

        RCLCPP_INFO(ros_node_->get_logger(), "🎯 鎖定目標！距離: %.3fm, 角度: %.1f°, 高度差: %.3fm", 
                    latest_target_.x, latest_target_.y, latest_target_.z);
        
        return BT::NodeStatus::SUCCESS;
    }

    // 如果還沒收到，就繼續等 (大腦保持 RUNNING 狀態不卡死)
    return BT::NodeStatus::RUNNING;
}

void FindValidDoll::onHalted()
{
    RCLCPP_INFO(ros_node_->get_logger(), "🛑 尋找娃娃任務被中斷");
}

} // namespace robot_behavior