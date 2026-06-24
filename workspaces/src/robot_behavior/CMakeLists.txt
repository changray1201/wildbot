cmake_minimum_required(VERSION 3.8)
project(robot_behavior)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()


# ==========================================
# 尋找所有需要的套件
# ==========================================
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(behaviortree_cpp REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(opennav_docking_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(lifecycle_msgs REQUIRED)

# ==========================================
# 大腦主程式：bt_main (名稱改為 bt_main 以符合你的 ros2 run 指令)
# ==========================================
add_executable(bt_main 
  src/main.cpp
  src/wait_for_topic.cpp
  src/find_valid_doll.cpp
  src/navigate_to_pose.cpp
  src/dock_robot.cpp
  src/check_camera.cpp
  src/execute_script.cpp
  src/get_next_waypoint.cpp
  src/visual_approach.cpp
)

# 告訴編譯器去 include 資料夾找你的 .hpp 標頭檔
target_include_directories(bt_main PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# 連結所有 ROS 2 依賴庫
# 注意：yaml-cpp 不是 standard ament package，通常不放在這裡
ament_target_dependencies(bt_main
  rclcpp
  behaviortree_cpp
  std_msgs
  geometry_msgs
  rclcpp_action
  nav2_msgs
  opennav_docking_msgs
  std_srvs
  tf2
  tf2_ros
  tf2_geometry_msgs
  lifecycle_msgs
)


# 關鍵修正：確保 yaml-cpp 的連結被強制執行
target_link_libraries(bt_main yaml-cpp)

# ==========================================
# 安裝與打包設定
# ==========================================
install(TARGETS 
  bt_main
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME} 
)

# 確保 config 和 launch 檔案會被安裝過去
install(DIRECTORY
  launch
  config
  DESTINATION share/${PROJECT_NAME}
)

# install(PROGRAMS
#   scripts/competition_node.py
#   DESTINATION lib/${PROJECT_NAME}
# )

ament_package()