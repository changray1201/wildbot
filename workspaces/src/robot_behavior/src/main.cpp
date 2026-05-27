#include <iostream>
#include <thread>
#include <chrono>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "behaviortree_cpp/bt_factory.h"

// 引入你的自訂節點標頭檔 (請確保這些 .hpp 檔案存在於 include/robot_behavior/ 中)
#include "robot_behavior/wait_for_topic.hpp"
#include "robot_behavior/find_valid_doll.hpp"
#include "robot_behavior/navigate_to_pose.hpp"
#include "robot_behavior/dock_robot.hpp"  // 對應你截圖中的 docking_test.cpp
#include "robot_behavior/check_camera.hpp"
#include "robot_behavior/execute_script.hpp"
#include "behaviortree_cpp/loggers/groot2_publisher.h"

namespace robot_behavior {

// ==========================================================
// 1. 輕量級腳本執行器 (用來取代被刪除的 execute_script.cpp)
// ==========================================================
BT::NodeStatus ExecuteScriptFunction(BT::TreeNode& self) {
    std::string action_type;
    
    // 從 XML 讀取 action 參數
    if (!self.getInput("action", action_type)) {
        std::cerr << "\033[1;31m[腳本錯誤] ExecuteScript 缺少 'action' 參數！\033[0m" << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // 處理單純印出文字的指令 (例如：print('啟動模式一'))
    if (action_type.find("print(") == 0) {
        // 簡單的字串處理，把 print('...') 裡面的文字切出來
        size_t start = action_type.find("'");
        size_t end = action_type.rfind("'");
        if (start != std::string::npos && end != std::string::npos && end > start) {
            std::string msg = action_type.substr(start + 1, end - start - 1);
            std::cout << "\n\033[1;36m[大腦廣播] " << msg << "\033[0m" << std::endl;
        }
        return BT::NodeStatus::SUCCESS;
    }

    // 處理真實的硬體與控制腳本
    std::cout << "\033[1;35m[硬體控制] 正在執行腳本: " << action_type << "...\033[0m" << std::endl;

    if (action_type == "grab" || action_type == "grab_special") {
        // 實戰：在這裡呼叫夾爪的 ROS 2 Service 或 Topic
        std::this_thread::sleep_for(std::chrono::seconds(2)); // 模擬夾取時間
    } 
    else if (action_type == "drop") {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 模擬放下時間
    }
    else if (action_type == "pull_handle_down" || action_type == "push_door_forward") {
        std::this_thread::sleep_for(std::chrono::seconds(2)); // 模擬開門動作時間
    }
    else if (action_type == "start_slam") {
        // 實戰：呼叫系統指令啟動 SLAM (例如：system("ros2 launch slam_toolbox..."); )
        std::cout << ">> 已發送啟動 SLAM 訊號" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    else {
        std::cout << "\033[1;33m[警告] 未知的腳本指令: " << action_type << "\033[0m" << std::endl;
    }

    std::cout << "\033[1;35m[硬體控制] " << action_type << " 執行完畢。\033[0m" << std::endl;
    return BT::NodeStatus::SUCCESS;
}

// 宣告這個 Function 需要接收的 Ports


} // namespace robot_behavior

// ==========================================================
// 2. 主程式入口 (Main)
// ==========================================================
int main(int argc, char **argv) {
    // 初始化 ROS 2
    rclcpp::init(argc, argv);
    auto ros_node = std::make_shared<rclcpp::Node>("competition_bt_node");
    
    // 【極度重要】開啟背景執行緒來處理 ROS 2 的 Callback (如相機、定位的 Topic 接收)
    // 這樣才不會跟行為樹的 Tick 迴圈互相卡死
    std::thread spin_thread([ros_node]() { rclcpp::spin(ros_node); });

    // 建立行為樹工廠
    BT::BehaviorTreeFactory factory;
    
    // 註冊需要傳入 ROS Node 的自訂節點 (因為這些節點需要收發 Topic 或 Action)
    factory.registerNodeType<robot_behavior::WaitForTopic>("WaitForTopic", ros_node);
    factory.registerNodeType<robot_behavior::FindValidDoll>("FindValidDoll", ros_node);
    factory.registerNodeType<robot_behavior::NavigateToPose>("NavigateToPose", ros_node);
    factory.registerNodeType<robot_behavior::ExecuteScript>("ExecuteScript", ros_node);
    
    // 注意這裡：你的檔案叫 docking_test.cpp，但 XML 裡面叫 DockRobot
    // 我們在這裡直接把它註冊成 "DockRobot"，這樣 XML 就不用改了！
    factory.registerNodeType<robot_behavior::DockRobot>("DockRobot", ros_node);
    
    // 註冊不需要 ROS Node 的純邏輯節點
    factory.registerNodeType<robot_behavior::CheckCamera>("CheckCamera");

    // 從檔案載入行為樹
    std::cout << "正在載入行為樹..." << std::endl;
    auto tree = factory.createTreeFromFile("src/robot_behavior/config/bt_tree.xml");
    std::cout << "\033[1;32m--- 機器人行為樹已啟動，等待指令中 ---\033[0m" << std::endl;

    BT::Groot2Publisher publisher(tree); // 開啟 Groot2 監聽通道
    //RCLCPP_INFO(node->get_logger(), "👀 Groot2 監聽器已啟動...");
    tree.tickWhileRunning();

    // 自訂 Tick 迴圈：只要 ROS 2 還活著，就以 100Hz (10ms) 的頻率不斷 Tick 行為樹
    while (rclcpp::ok()) {
        tree.tickExactlyOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 關閉與清理資源
    rclcpp::shutdown();
    spin_thread.join();
    return 0;
}