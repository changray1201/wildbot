#include "robot_behavior/check_camera.hpp"
#include <iostream>

namespace robot_behavior {

CheckCamera::CheckCamera(const std::string& name, const BT::NodeConfig& config)
  : BT::ConditionNode(name, config) {}

BT::PortsList CheckCamera::providedPorts() {
    return { BT::InputPort<std::string>("status") };
}

BT::NodeStatus CheckCamera::tick() {
    // 加上 static 關鍵字，讓這個變數在每次呼叫 tick() 時都會記住上一次的數值
    static int attempt_count = 0; 
    
    attempt_count++; // 每次進入這個節點，次數就 +1

    std::cout << "\033[1;36m[Camera] 啟動影像辨識，這是第 " << attempt_count << " 次確認狀態...\033[0m" << std::endl;

    // 前 3 次強制回傳失敗
    if (attempt_count <= 3) {
        std::cout << "\033[1;31m[結果] 影像確認失敗，爪子裡沒有娃娃！\033[0m" << std::endl;
        return BT::NodeStatus::FAILURE;
    } 
    // 第 4 次回傳成功
    else {
        std::cout << "\033[1;32m[結果] 影像確認通過！成功夾到娃娃了！\033[0m" << std::endl;
        
        // 【重要】成功後要把計數器歸零，這樣如果行為樹跑第二圈，才能重新計算
        attempt_count = 0; 
        
        return BT::NodeStatus::SUCCESS;
    }
}

} // namespace robot_behavior