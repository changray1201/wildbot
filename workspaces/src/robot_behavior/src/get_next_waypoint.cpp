#include "robot_behavior/get_next_waypoint.hpp"
#include "tf2/LinearMath/Quaternion.h"
#include "yaml-cpp/yaml.h"
#include <iostream>

namespace robot_behavior
{

GetNextWaypoint::GetNextWaypoint(const std::string & name, const BT::NodeConfig & config)
: BT::SyncActionNode(name, config), currentindex(0)
{
  std::string file_path;
  if (!getInput("file_path", file_path)) {
    throw std::runtime_error("GetNextWaypoint 缺少 file_path 參數！");
  }
  loadWaypoints(file_path);
}

BT::PortsList GetNextWaypoint::providedPorts()
{
  return {
    BT::InputPort<std::string>("file_path", "YAML 巡邏點檔案的絕對路徑"),
    BT::OutputPort<geometry_msgs::msg::PoseStamped>("output_pose", "吐給 Nav2 的下一個目標座標")
  };
}

BT::NodeStatus GetNextWaypoint::tick()
{
  if (waypoints_.empty()) {
    return BT::NodeStatus::FAILURE;
  }

  auto pt = waypoints_[currentindex];

  geometry_msgs::msg::PoseStamped pose;
  pose.header.stamp = rclcpp::Clock().now();
  pose.header.frame_id = "map";
  pose.pose.position.x = pt.x;
  pose.pose.position.y = pt.y;
  pose.pose.position.z = 0.0;

  tf2::Quaternion q;
  q.setRPY(0, 0, pt.yaw);
  pose.pose.orientation.x = q.x();
  pose.pose.orientation.y = q.y();
  pose.pose.orientation.z = q.z();
  pose.pose.orientation.w = q.w();

  setOutput("output_pose", pose);
  currentindex = (currentindex + 1) % waypoints_.size();

  return BT::NodeStatus::SUCCESS;
}

void GetNextWaypoint::loadWaypoints(const std::string & path)
{
  try {
    YAML::Node config = YAML::LoadFile(path);
    if (config["waypoints"]) {
      for (const auto & node : config["waypoints"]) {
        waypoints_.push_back({
          node["x"].as<double>(),
          node["y"].as<double>(),
          node["yaw"].as<double>()
        });
      }
    }
  } catch (const std::exception & e) {
    std::cerr << "🛑 無法讀取巡邏點檔案: " << e.what() << std::endl;
  }
}

} // namespace robot_behavior