cmake_minimum_required(VERSION 3.8)
project(robot_behavior)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ==========================================
# 1. 尋找依賴套件 (準備物資)
# ==========================================
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(behaviortree_cpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(opennav_docking_msgs REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_srvs REQUIRED)

include_directories(include)

# ==========================================
# 🚢 2. 第一艘船：大腦主程式 (bt_main)
# ==========================================
add_executable(bt_main 
  src/main.cpp
  src/find_valid_doll.cpp
  src/navigate_to_pose.cpp
  src/check_camera.cpp  
  src/patrol.cpp
  src/wait_for_topic.cpp      
  src/docking_test.cpp
  src/execute_script.cpp
)

ament_target_dependencies(bt_main
  rclcpp 
  rclcpp_action
  behaviortree_cpp 
  std_msgs
  opennav_docking_msgs
  nav2_msgs
  geometry_msgs
  std_srvs
)



# ==========================================
# 📦 4. 安裝規則 (打包上車)
# ==========================================
# 安裝標頭檔
install(DIRECTORY include/
  DESTINATION include
)

# 安裝 Launch 檔與設定檔 (已經幫你把重複的合併了)
install(DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
)

# 🌟 把兩艘船合併在同一個 Install 裡，一起送進執行目錄
install(TARGETS 
  bt_main
  DESTINATION lib/${PROJECT_NAME}
)

# 將 Python 腳本安裝到套件的執行檔目錄下
install(PROGRAMS
  scripts/competition_node.py
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()