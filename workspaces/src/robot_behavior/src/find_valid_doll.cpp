#include "robot_behavior/find_valid_doll.hpp"

namespace robot_behavior
{

// 🌟 建構子修正：名稱對齊，且直接使用傳進來的 node 建立訂閱者
FindValidDoll::FindValidDoll(const std::string& name, const BT::NodeConfig& config, std::shared_ptr<rclcpp::Node> node)
: BT::ConditionNode(name, config),
  haspose(false)
{
  // 直接訂閱相機發出的對接座標，不需要再去黑板抓取了！
  sub_ = node->create_subscription<geometry_msgs::msg::PoseStamped>(
    "/detected_dock_pose", 10,
    std::bind(&FindValidDoll::poseCallback, this, std::placeholders::_1));
}

// 定義這個積木的輸出介面
BT::PortsList FindValidDoll::providedPorts()
{
  return {
    BT::OutputPort<geometry_msgs::msg::PoseStamped>("target_pose", "取得最新娃娃的座標")
  };
}

// 行為樹每個運算週期都會呼叫 tick()
BT::NodeStatus FindValidDoll::tick()
{
  std::lock_guard<std::mutex> lock(mutex_);

  // 如果從未收到過座標，直接回傳失敗 (讓外層的 Fallback 繼續去巡邏)
  if (!haspose) {
    return BT::NodeStatus::FAILURE;
  }

  // 防呆機制：如果相機超過 1 秒沒有看到娃娃，視為目標丟失，中斷對接回退到巡邏
  auto now = std::chrono::steady_clock::now();
  if (std::chrono::duration_cast<std::chrono::seconds>(now - last_msgtime).count() > 1) {
    haspose = false;
    return BT::NodeStatus::FAILURE;
  }

  // 依然看見目標，把座標寫入黑板，並回傳 SUCCESS 觸發對接！
  setOutput("target_pose", latestpose);
  return BT::NodeStatus::SUCCESS;
}

void FindValidDoll::poseCallback(const geometry_msgs::msg::PoseStamped::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(mutex_);
  latestpose = *msg;
  last_msgtime = std::chrono::steady_clock::now();
  haspose = true;
}

}  // namespace robot_behavior

