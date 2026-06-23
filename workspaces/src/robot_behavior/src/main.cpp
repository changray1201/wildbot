#include <iostream>
#include <thread>
#include <chrono>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "behaviortree_cpp/bt_factory.h"
#include "behaviortree_cpp/loggers/groot2_publisher.h"

#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include "tf2/exceptions.h"

#include "rclcpp_action/rclcpp_action.hpp"
#include "nav2_msgs/action/navigate_to_pose.hpp"

#include "nav2_msgs/srv/manage_lifecycle_nodes.hpp"
#include "lifecycle_msgs/srv/get_state.hpp"

// 引入你的自訂節點標頭檔
#include "robot_behavior/wait_for_topic.hpp"
#include "robot_behavior/find_valid_doll.hpp"
#include "robot_behavior/navigate_to_pose.hpp"
#include "robot_behavior/dock_robot.hpp"
#include "robot_behavior/check_camera.hpp"
#include "robot_behavior/execute_script.hpp" // 👈 這是我們新寫好的類別！
#include "robot_behavior/get_next_waypoint.hpp"

// ==========================================================
// 主程式入口 (Main)
// ==========================================================
int main(int argc, char **argv) {
    // 初始化 ROS 2
    rclcpp::init(argc, argv);
    //auto ros_node = std::make_shared<rclcpp::Node>("competition_bt_node");
    auto ros_node = std::make_shared<rclcpp::Node>("competition_bt_node");

    // 開啟背景執行緒來處理 ROS 2 的 Callback (如相機、定位的 Topic 接收)
    std::thread spin_thread([ros_node]() { rclcpp::spin(ros_node); });

    // =========================
    // TF listener
    // =========================
    tf2_ros::Buffer tf_buffer(ros_node->get_clock());
    tf2_ros::TransformListener tf_listener(tf_buffer);


    // 建立行為樹工廠
    BT::BehaviorTreeFactory factory;
    
    // 註冊需要傳入 ROS Node 的自訂節點
    factory.registerNodeType<robot_behavior::WaitForTopic>("WaitForTopic", ros_node);
    factory.registerNodeType<robot_behavior::FindValidDoll>("FindValidDoll", ros_node);
    factory.registerNodeType<robot_behavior::NavigateToPose>("NavigateToPose", ros_node);
    factory.registerNodeType<robot_behavior::ExecuteScript>("ExecuteScript", ros_node);
    factory.registerNodeType<robot_behavior::DockRobot>("DockRobot", ros_node);
    
    // 註冊不需要 ROS Node 的純邏輯節點
    factory.registerNodeType<robot_behavior::CheckCamera>("CheckCamera");
    //factory.registerFromPlugin("libget_next_waypoint_plugin.so");
    // 💡 修正 2：刪除原本的 registerFromPlugin，改用靜態註冊
    factory.registerNodeType<robot_behavior::GetNextWaypoint>("GetNextWaypoint");

    // 從檔案載入行為樹
    std::cout << "正在載入行為樹..." << std::endl;
    auto tree = factory.createTreeFromFile("src/robot_behavior/config/bt_tree.xml");


    // =========================
    // wait TF (map -> base_link)
    // =========================
    //RCLCPP_INFO(ros_node->get_logger(), "Waiting TF...");

    while (rclcpp::ok())
    {
        if (tf_buffer.canTransform("map", "base_link", tf2::TimePointZero))
            break;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    RCLCPP_INFO(
        ros_node->get_logger(),
        "Localization TF detected.");
    
    // =========================
    // wait Nav2 ready
    // =========================
    //wait_for_action_server 只能確認 server 存在，不能確認是否 Active
    /*
    using NavAction = nav2_msgs::action::NavigateToPose;

    auto nav_client =
        rclcpp_action::create_client<NavAction>(ros_node, "/navigate_to_pose");

    RCLCPP_INFO(ros_node->get_logger(), "Waiting Nav2...");

    while (rclcpp::ok())
    {
        if (nav_client->wait_for_action_server(std::chrono::seconds(1)))
            break;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    */

    // 等待 bt_navigator 真正 Active
    auto state_client = ros_node->create_client<lifecycle_msgs::srv::GetState>(
        "/docking_server/get_state");

    RCLCPP_INFO(ros_node->get_logger(), "Waiting for bt_navigator to become active...");

    while (rclcpp::ok())
    {
        if (!state_client->wait_for_service(std::chrono::seconds(1)))
            continue;

        auto request = std::make_shared<lifecycle_msgs::srv::GetState::Request>();
        auto future = state_client->async_send_request(request);

        // 等 future 完成
        auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        bool ready = false;
        while (std::chrono::steady_clock::now() < timeout)
        {
            if (future.wait_for(std::chrono::milliseconds(100)) == std::future_status::ready)
            {
                ready = true;
                break;
            }
        }

        if (ready && future.get()->current_state.id == 3)
        {
            RCLCPP_INFO(ros_node->get_logger(), "bt_navigator is active!");
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    RCLCPP_INFO(ros_node->get_logger(), "System ready → start BT");
    

    std::cout << "\033[1;32m--- 機器人行為樹已啟動，等待指令中 ---\033[0m" << std::endl;


    BT::Groot2Publisher publisher(tree); // 開啟 Groot2 監聽通道

    // 自訂 Tick 迴圈：以 100Hz (10ms) 的頻率不斷掃描行為樹
    while (rclcpp::ok()) {
        tree.tickExactlyOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 關閉與清理資源
    rclcpp::shutdown();
    spin_thread.join();
    return 0;
}

