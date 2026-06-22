#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"

class CmdVelBridge : public rclcpp::Node
{
public:
    CmdVelBridge() : Node("bypass_monitor_bridge")
    {
        // 1. 建立送往底盤的 TwistStamped 發布者
        pub_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("/base_controller/cmd_vel", 10);

        // 2. 訂閱平滑器輸出的 Twist 速度
        sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
            //"/cmd_vel_smoothed", 10,
            "/cmd_vel", 10,
            std::bind(&CmdVelBridge::topic_callback, this, std::placeholders::_1));

        RCLCPP_INFO(this->get_logger(), "【暴力直連】已繞過 collision_monitor，直接將平滑速度送往底盤！");
    }

private:
    void topic_callback(const geometry_msgs::msg::Twist::SharedPtr msg) const
    {
        auto stamped_msg = geometry_msgs::msg::TwistStamped();
        
        // 填入時間戳與 Frame ID
        stamped_msg.header.stamp = this->get_clock()->now();
        stamped_msg.header.frame_id = "base_link";
        stamped_msg.twist = *msg;

        pub_->publish(stamped_msg);
    }

    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr pub_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr sub_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<CmdVelBridge>());
    rclcpp::shutdown();
    return 0;
}