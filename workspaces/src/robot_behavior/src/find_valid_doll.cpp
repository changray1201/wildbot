#include "robot_behavior/find_valid_doll.hpp"

namespace robot_behavior
{

FindValidDoll::FindValidDoll(const std::string& name, const BT::NodeConfig& config, std::shared_ptr<rclcpp::Node> node)
: BT::ConditionNode(name, config), ros_node_(node), has_pose_(false)
{
  // 🌟 關鍵修正：將監聽的 Topic 變更為 "detected_dock_pose" (移除開頭斜線)，與 Python 端完全一致
  sub_ = ros_node_->create_subscription<geometry_msgs::msg::PoseStamped>(
    "detected_dock_pose", 10,
    std::bind(&FindValidDoll::poseCallback, this, std::placeholders::_1));
}

BT::PortsList FindValidDoll::providedPorts()
{
  return {
    // 🌟 關鍵修正：Port 名稱設定為 "target_pose"，完美對接 XML 中的 target_pose="{target_pose}"
    BT::OutputPort<geometry_msgs::msg::PoseStamped>("target_pose", "取得相機計算出的扣除手臂對接座標")
  };
}

BT::NodeStatus FindValidDoll::tick()
{
  std::lock_guard<std::mutex> lock(mutex_);
  
  // 如果從未收到過相機座標，回傳 FAILURE 讓機器人繼續巡邏
  if (!has_pose_) {
    return BT::NodeStatus::FAILURE;
  }

  // 防呆機制：如果相機超過 1 秒沒有更新畫面（可能斷訊或被擋住），視為目標遺失
  auto now = std::chrono::steady_clock::now();
  if (std::chrono::duration_cast<std::chrono::seconds>(now - last_msg_time_).count() > 1) {
    has_pose_ = false;
    return BT::NodeStatus::FAILURE;
  }

  // 🌟 核心動作：將收到的 PoseStamped 完整寫入黑板的 {target_pose} 變數中
  setOutput("target_pose", latest_pose_);
  
  // 回傳 SUCCESS，這會觸發外層 Sequence 的下一個積木：DockRobot！
  return BT::NodeStatus::SUCCESS;
}

void FindValidDoll::poseCallback(const geometry_msgs::msg::PoseStamped::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(mutex_);
  latest_pose_ = *msg;
  last_msg_time_ = std::chrono::steady_clock::now();
  has_pose_ = true;
}

}  // namespace robot_behavior