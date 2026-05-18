# FROM ghcr.io/screamlab/wildbot_base_image:latest
# ENV ROS_DISTRO jazzy
# COPY . /tmp

# WORKDIR /tmp

# ### list the ros2 packages you want here
# #copy the workspaces and install ros packages and dependencies
# RUN cp -r ./workspaces/* /workspaces && \
#     apt-get update && \
#     apt-get install -y --no-install-recommends \
#         ros-${ROS_DISTRO}-demo-nodes-py \
#         ros-${ROS_DISTRO}-demo-nodes-cpp && \
#     rosdep update --rosdistro ${ROS_DISTRO} && \
#     colcon mixin update && \
#     colcon metadata update


# #build the workspace
# WORKDIR /workspaces
# RUN source /opt/ros/${ROS_DISTRO}/setup.bash && \
#     rosdep install -q -y -r --from-paths src --ignore-src && \
#     colcon build --symlink-install


# RUN rm -rf /tmp/* && \
#     rm -rf /temp/* && \
#     rm -rf /var/lib/apt/lists/*


# ENTRYPOINT ["/ros_entrypoint.bash"]
# CMD ["bash", "-l"]



FROM ghcr.io/screamlab/wildbot_base_image:latest

# 修正：使用新的 ENV key=value 格式消除警告
ENV ROS_DISTRO=jazzy 
COPY . /tmp

WORKDIR /tmp

### list the ros2 packages you want here
#copy the workspaces and install ros packages and dependencies
RUN cp -r ./workspaces/* /workspaces && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        python3-pip \
        ros-${ROS_DISTRO}-cv-bridge \
        ros-${ROS_DISTRO}-sensor-msgs \
        ros-${ROS_DISTRO}-demo-nodes-py \
        ros-${ROS_DISTRO}-demo-nodes-cpp && \
    rosdep update --rosdistro ${ROS_DISTRO} && \
    colcon mixin update && \
    colcon metadata update

# 🌟 關鍵修正：加入 --ignore-installed 強制忽略 Debian 系統預裝的 numpy 衝突
RUN pip3 install --break-system-packages --no-cache-dir --ignore-installed ultralytics opencv-python "numpy<2" scipy

#build the workspace
WORKDIR /workspaces
RUN /bin/bash -c "source /opt/ros/${ROS_DISTRO}/setup.bash && \
    rosdep install -q -y -r --from-paths src --ignore-src && \
    colcon build --symlink-install"

RUN rm -rf /tmp/* && \
    rm -rf /temp/* && \
    rm -rf /var/lib/apt/lists/*

ENTRYPOINT ["/ros_entrypoint.bash"]
CMD ["bash", "-l"]