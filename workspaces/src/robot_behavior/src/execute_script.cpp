#include "robot_behavior/execute_script.hpp"
#include "std_srvs/srv/trigger.hpp"
#include <iostream>
#include <chrono>

namespace robot_behavior {

ExecuteScript::ExecuteScript(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node)
  : BT::SyncActionNode(name, config), ros_node_(node) {}

// 告訴 BehaviorTree，這顆積木需要讀取名為 "action" 的變數
BT::PortsList ExecuteScript::providedPorts() {
    return { BT::InputPort<std::string>("action") };
}

BT::NodeStatus ExecuteScript::tick() {
    std::string action_cmd;
    
    // 1. 嘗試從 XML 讀取指令 (例如 action="grab")
    if (!getInput("action", action_cmd)) {
        RCLCPP_ERROR(ros_node_->get_logger(), "ExecuteScript 缺少 action 參數！");
        return BT::NodeStatus::FAILURE;
    }

    // 2. 防呆與廣播：如果是 "print" 開頭的指令，直接印在終端機，不呼叫機器手臂
    if (action_cmd.find("print") == 0) {
        std::cout << "\033[1;36m[大腦廣播] " << action_cmd << "\033[0m" << std::endl;
        return BT::NodeStatus::SUCCESS;
    }

    // 3. 轉換為 Service 名稱 (例如 "grab" -> "/script/grab")
    std::string service_name = "/script/" + action_cmd;
    
    // 4. 建立臨時節點與 Client 來呼叫 Python 服務 (使用臨時節點可避免與主迴圈死結)
    auto temp_node = std::make_shared<rclcpp::Node>("temp_script_client");
    auto client = temp_node->create_client<std_srvs::srv::Trigger>(service_name);
    
    RCLCPP_INFO(ros_node_->get_logger(), "正在呼叫手臂動作: [%s]...", action_cmd.c_str());

    // 5. 等待 Python 腳本伺服器上線 (最多等 2 秒)
    if (!client->wait_for_service(std::chrono::seconds(2))) {
        RCLCPP_ERROR(ros_node_->get_logger(), "找不到 Python 腳本服務: %s (你是不是忘記開 competition_node.py？)", service_name.c_str());
        return BT::NodeStatus::FAILURE;
    }

    // 6. 發送請求給 Python 腳本
    auto request = std::make_shared<std_srvs::srv::Trigger::Request>();
    auto future = client->async_send_request(request);

    // 7. 等待手臂動作執行完畢
    if (rclcpp::spin_until_future_complete(temp_node, future) == rclcpp::FutureReturnCode::SUCCESS) {
        auto response = future.get();
        if (response->success) {
            RCLCPP_INFO(ros_node_->get_logger(), "手臂動作 [%s] 執行成功！", action_cmd.c_str());
            return BT::NodeStatus::SUCCESS;
        } else {
            RCLCPP_ERROR(ros_node_->get_logger(), "手臂動作 [%s] 失敗或遭到拒絕: %s", action_cmd.c_str(), response->message.c_str());
            return BT::NodeStatus::FAILURE;
        }
    } else {
        RCLCPP_ERROR(ros_node_->get_logger(), "手臂動作 [%s] 通訊超時或崩潰！", action_cmd.c_str());
        return BT::NodeStatus::FAILURE;
    }
}

} // namespace robot_behavior