#ifndef GET_NEXT_WAYPOINT_HPP
#define GET_NEXT_WAYPOINT_HPP

#include <string>
#include <vector>
#include "rclcpp/rclcpp.hpp"
#include "behaviortree_cpp/action_node.h"
#include "geometry_msgs/msg/pose_stamped.hpp"

namespace robot_behavior // 💡 修正：加上底線，與 .cpp 一致
{

class GetNextWaypoint : public BT::SyncActionNode
{
public:
  GetNextWaypoint(const std::string & name, const BT::NodeConfig & config);

  static BT::PortsList providedPorts();

  BT::NodeStatus tick() override;

private:
  struct Waypoint { double x; double y; double yaw; };
  std::vector<Waypoint> waypoints_; // 💡 修正：加上底線，與 .cpp 一致
  size_t currentindex;

  void loadWaypoints(const std::string & path);
};

} // namespace robot_behavior

#endif // GET_NEXT_WAYPOINT_HPP