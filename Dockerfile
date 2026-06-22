FROM ghcr.io/screamlab/wildbot_base_image:latest

# 使用新的 ENV key=value 格式消除警告
ENV ROS_DISTRO=jazzy ROS2_WS=/workspaces

COPY . /tmp
WORKDIR /tmp


# 複製 workspace 並安裝 ROS 2 核心套件與大腦/眼睛所需的依賴
RUN mkdir -p /workspaces && \
    cp -r ./workspaces/* /workspaces && \
    apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y --no-install-recommends \
        python3-pip \
        ros-${ROS_DISTRO}-cv-bridge \
        ros-${ROS_DISTRO}-sensor-msgs \
        ros-${ROS_DISTRO}-demo-nodes-py \
        ros-${ROS_DISTRO}-demo-nodes-cpp \
        ros-${ROS_DISTRO}-behaviortree-cpp \
        ros-${ROS_DISTRO}-nav2-bringup \
        ros-${ROS_DISTRO}-nav2-msgs \
        ros-${ROS_DISTRO}-rclcpp-action \
        ros-${ROS_DISTRO}-tf2-ros \
        ros-${ROS_DISTRO}-tf2-geometry-msgs \
        ros-${ROS_DISTRO}-robot-localization \
        ros-${ROS_DISTRO}-cartographer \
        ros-${ROS_DISTRO}-cartographer-ros \
        ros-${ROS_DISTRO}-laser-filters \
        ros-${ROS_DISTRO}-slam-toolbox && \
    rosdep update --rosdistro ${ROS_DISTRO} && \
    colcon mixin update && \
    colcon metadata update

# 強制安裝 YOLO 視覺所需的 Python 套件
RUN pip3 install --break-system-packages --no-cache-dir --ignore-installed ultralytics opencv-python "numpy<2" scipy

WORKDIR /workspaces

# 🌟 【修正 2】：在容器內建置前，先清除可能殘留的舊編譯快取檔，避免污染
RUN rm -rf build/ install/ log/ && \
    /bin/bash -c "source /opt/ros/${ROS_DISTRO}/setup.bash && \
    rosdep install -q -y -r --from-paths src --ignore-src && \
    colcon build --symlink-install --cmake-args -DBUILD_TESTING=OFF"
    
# 【修正：雙重保險】把 source 寫進 .bashrc，這樣連 docker exec 進來都會有環境！
RUN echo "source /opt/ros/\${ROS_DISTRO}/setup.bash" >> /root/.bashrc && \
    echo "source /workspaces/install/setup.bash" >> /root/.bashrc

# 清理垃圾以縮小 Image 體積
RUN rm -rf /tmp/* && \
    rm -rf /temp/* && \
    rm -rf /var/lib/apt/lists/*

ENTRYPOINT ["/ros_entrypoint.bash"]
CMD ["bash", "-l"]