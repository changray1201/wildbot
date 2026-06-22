#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/odometry.hpp"

class OdomBridge : public rclcpp::Node
{
public:
    OdomBridge() : Node("odom_topic_bridge")
    {
        // 1. 建立大腦要聽的 /odom 發布者
        //pub_ = this->create_publisher<nav_msgs::msg::Odometry>(
            //"/odom", 10
            //rclcpp::SensorDataQoS().keep_last(10)
        //);
        pub_ = this->create_publisher<nav_msgs::msg::Odometry>("/odom", 10);

        // 2. 訂閱系統中真正有數據的里程計
        sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
            "/odometry/filtered", 10,
            //rclcpp::SensorDataQoS().keep_last(10), // 👈 BEST_EFFORT
            std::bind(&OdomBridge::topic_callback, this, std::placeholders::_1));

        RCLCPP_INFO(this->get_logger(), "【里程計轉接頭啟動】正在將 /odometry/filtered 數據餵給 /odom...");
    }

private:
    void topic_callback(const nav_msgs::msg::Odometry::SharedPtr msg) const
    {
        // 收到什麼就直接原封不動轉發出去
        pub_->publish(*msg);
    }

    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr pub_;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr sub_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<OdomBridge>());
    rclcpp::shutdown();
    return 0;
}