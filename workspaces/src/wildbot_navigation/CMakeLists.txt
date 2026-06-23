cmake_minimum_required(VERSION 3.8)
project(wildbot_navigation)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 1. 尋找所有必要的依賴包
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)

# 2. 宣告並編譯第一個節點：里程計轉接
add_executable(odom_bridge_node bridge/odom_bridge_node.cpp)
ament_target_dependencies(odom_bridge_node rclcpp nav_msgs)

# 3. 宣告並編譯第二個節點：速度轉接 
add_executable(cmd_vel_bridge_node bridge/cmd_vel_bridge_node.cpp)
ament_target_dependencies(cmd_vel_bridge_node rclcpp geometry_msgs)

# 4. 安裝執行檔
install(TARGETS
  odom_bridge_node
  cmd_vel_bridge_node 
  DESTINATION lib/${PROJECT_NAME}
)

# 5. 安裝資源資料夾
install(DIRECTORY
  launch
  config
  map
  DESTINATION share/${PROJECT_NAME}
)

# 6. 測試區塊（徹底阻斷它影響底部的可能性）
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()