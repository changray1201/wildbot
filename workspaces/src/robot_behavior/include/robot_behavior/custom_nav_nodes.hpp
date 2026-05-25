#pragma once

#include "behaviortree_cpp/behavior_tree.h"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "std_msgs/msg/string.hpp"
#include "opennav_docking_msgs/action/dock_robot.hpp"

using DockRobot = opennav_docking_msgs::action::DockRobot;
using GoalHandleDockRobot = rclcpp_action::ClientGoalHandle<DockRobot>;

// =========================================================
// 積木 1：強制煞車暗號 (瞬間完成的同步節點)
// =========================================================
class TriggerDidilong : public BT::SyncActionNode
{
public:
    TriggerDidilong(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node)
      : BT::SyncActionNode(name, config), ros_node_(node)
    {
        publisher_ = ros_node_->create_publisher<std_msgs::msg::String>("/controller_function", 10);
    }

    static BT::PortsList providedPorts() { return {}; }

    BT::NodeStatus tick() override
    {
        RCLCPP_INFO(ros_node_->get_logger(), "大老闆 BT：Didilong！立刻煞車！");
        auto msg = std_msgs::msg::String();
        msg.data = "Didilong";
        publisher_->publish(msg);
        return BT::NodeStatus::SUCCESS;
    }

private:
    rclcpp::Node::SharedPtr ros_node_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
};

// =========================================================
// 積木 2：泊車小弟 (需要等待的非同步節點)
// =========================================================
class DockRobotNode : public BT::StatefulActionNode
{
public:
    DockRobotNode(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node)
      : BT::StatefulActionNode(name, config), ros_node_(node)
    {
        dock_client_ = rclcpp_action::create_client<DockRobot>(ros_node_, "/dock_robot");
    }

    // 定義可以從 XML 接收的參數
    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<double>("target_x"),
            BT::InputPort<double>("target_y")
        };
    }

    // 當節點第一次被執行時
    BT::NodeStatus onStart() override
    {
        if (!dock_client_->wait_for_action_server(std::chrono::seconds(2))) {
            RCLCPP_ERROR(ros_node_->get_logger(), "連不上泊車小弟 Server！");
            return BT::NodeStatus::FAILURE;
        }

        double tx = 0.0, ty = 0.0;
        getInput("target_x", tx);
        getInput("target_y", ty);

        auto goal_msg = DockRobot::Goal();
        goal_msg.use_dock_id = false;
        goal_msg.dock_pose.header.frame_id = "odom";
        goal_msg.dock_pose.pose.position.x = tx;
        goal_msg.dock_pose.pose.position.y = ty;

        auto send_goal_options = rclcpp_action::Client<DockRobot>::SendGoalOptions();
        
        // 發送目標
        goal_handle_future_ = dock_client_->async_send_goal(goal_msg, send_goal_options);
        RCLCPP_INFO(ros_node_->get_logger(), "泊車任務已發送：X=%.2f, Y=%.2f", tx, ty);
        
        return BT::NodeStatus::RUNNING; // 告訴行為樹：我還在執行中，請稍後再來問我
    }

    // 行為樹在 RUNNING 狀態下，會不斷呼叫這個函數檢查進度
    BT::NodeStatus onRunning() override
    {
        // 檢查目標是否被伺服器接受
        if (goal_handle_future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            auto goal_handle = goal_handle_future_.get();
            if (!goal_handle) {
                RCLCPP_ERROR(ros_node_->get_logger(), "泊車任務被伺服器拒絕！");
                return BT::NodeStatus::FAILURE;
            }

            // 檢查任務最終結果
            auto result_future = dock_client_->async_get_result(goal_handle);
            if (result_future.wait_for(std::chrono::milliseconds(100)) == std::future_status::ready) {
                auto result = result_future.get();
                if (result.code == rclcpp_action::ResultCode::SUCCEEDED) {
                    RCLCPP_INFO(ros_node_->get_logger(), "泊車完美停妥！");
                    return BT::NodeStatus::SUCCESS;
                } else {
                    RCLCPP_ERROR(ros_node_->get_logger(), "泊車任務失敗或被取消！");
                    return BT::NodeStatus::FAILURE;
                }
            }
        }
        return BT::NodeStatus::RUNNING; // 還沒到，繼續等
    }

    // 當任務被行為樹強制中斷時 (例如觸發了 Fallback)
    void onHalted() override
    {
        RCLCPP_INFO(ros_node_->get_logger(), "泊車任務被行為樹強制中斷！");
        // 實戰中這裡可以呼叫 action_client 的 cancel_goal
    }

private:
    rclcpp::Node::SharedPtr ros_node_;
    rclcpp_action::Client<DockRobot>::SharedPtr dock_client_;
    std::shared_future<GoalHandleDockRobot::SharedPtr> goal_handle_future_;
};

// =========================================================
// 積木 3：等待特定 Topic 訊號 (升級版：可以讀取指令內容)
// =========================================================
class WaitForTopicNode : public BT::StatefulActionNode
{
public:
    WaitForTopicNode(const std::string& name, const BT::NodeConfig& config, rclcpp::Node::SharedPtr node)
      : BT::StatefulActionNode(name, config), ros_node_(node), message_received_(false)
    {
    }

    // 新增：定義 OutputPort，用來把收到的指令傳給行為樹
    static BT::PortsList providedPorts()
    {
        return { 
            BT::InputPort<std::string>("topic_name"),
            BT::OutputPort<std::string>("command_out")  // <--- 這裡新增了輸出
        };
    }

    BT::NodeStatus onStart() override
    {
        std::string topic_name;
        if (!getInput("topic_name", topic_name)) {
            RCLCPP_ERROR(ros_node_->get_logger(), "缺少 topic_name 參數！");
            return BT::NodeStatus::FAILURE;
        }

        message_received_ = false;

        // 建立訂閱者，這次我們要把 msg->data 存起來
        sub_ = ros_node_->create_subscription<std_msgs::msg::String>(
            topic_name, 10,
            [this](const std_msgs::msg::String::SharedPtr msg) {
                this->received_command_ = msg->data;  // <--- 記下指令內容
                this->message_received_ = true;
                RCLCPP_INFO(this->ros_node_->get_logger(), "✅ 收到指令: [%s]", msg->data.c_str());
            });

        RCLCPP_INFO(ros_node_->get_logger(), "⏳ 等待長官從 [%s] 下達任務指令...", topic_name.c_str());
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        if (message_received_) {
            // 把收到的指令寫入黑板 (Blackboard)，讓行為樹的其他節點可以讀取
            setOutput("command_out", received_command_);
            sub_.reset(); // 任務完成，取消訂閱
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override
    {
        sub_.reset();
    }

private:
    rclcpp::Node::SharedPtr ros_node_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_;
    bool message_received_;
    std::string received_command_; // 用來暫存收到的字串
};