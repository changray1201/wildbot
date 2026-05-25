#include <chrono>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "tf2_ros/transform_listener.h"
#include "tf2_ros/buffer.h"

using namespace std::chrono_literals;

class RobotPosePublisher : public rclcpp::Node
{
public:
  RobotPosePublisher() : Node("robot_pose_publisher")
  {
    // 1. 初始化 TF2 的 Buffer 與 Listener，用來動態監聽 TF 樹
    tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    // 2. 建立發佈者，Topic 名稱為 /robot_pose，隊列大小為 10
    pose_pub_ = this->create_publisher<geometry_msgs::msg::PoseStamped>("robot_pose", 10);

    // 3. 建立定時器，以 20Hz (每 50 毫秒) 的頻率執行一次回呼函式
    timer_ = this->create_wall_timer(50ms, std::bind(&RobotPosePublisher::timer_callback, this));

    RCLCPP_INFO(this->get_logger(), "Robot Pose Publisher Node 已啟動，開始監聽 map -> base_link");
  }

private:
  void timer_callback()
  {
    geometry_msgs::msg::TransformStamped transform_stamped;

    try {
      // 4. 監聽 map 到 base_link 的座標變換
      // ⭐【修正大寫】：將 tf2::duration 改為 tf2::Duration
      transform_stamped = tf_buffer_->lookupTransform(
        "map", 
        "base_link", 
        tf2::TimePointZero,
        tf2::Duration(std::chrono::milliseconds(50))
      );
    }
    catch (const tf2::TransformException & ex) {
      // 剛開機或模擬器剛啟動時 TF 可能還沒完全建立，限制每 1000 毫秒最多印一次警告，避免洗畫面
      RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000,
                           "無法獲取 map -> base_link 的 TF 轉換: %s", ex.what());
      return;
    }

    // 5. 封裝成 geometry_msgs::msg::PoseStamped 格式
    geometry_msgs::msg::PoseStamped pose_msg;
    
    // 帶入 Header 資訊（包含 frame_id: "map" 以及當時的歷史時間戳）
    pose_msg.header = transform_stamped.header; 

    // 填入平移座標 (Position)
    pose_msg.pose.position.x = transform_stamped.transform.translation.x;
    pose_msg.pose.position.y = transform_stamped.transform.translation.y;
    pose_msg.pose.position.z = transform_stamped.transform.translation.z;

    // 填入旋轉四元數 (Orientation)
    pose_msg.pose.orientation = transform_stamped.transform.rotation;

    // 6. 正式發布到 /robot_pose
    pose_pub_->publish(pose_msg);
  }

  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr pose_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<RobotPosePublisher>());
  rclcpp::shutdown();
  return 0;
}