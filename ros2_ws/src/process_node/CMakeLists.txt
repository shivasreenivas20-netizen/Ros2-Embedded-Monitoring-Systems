cmake_minimum_required(VERSION 3.8)
project(process_node)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(action_msgs REQUIRED)

find_package(rosidl_default_generators REQUIRED)

# Generate actions
rosidl_generate_interfaces(${PROJECT_NAME}
  "action/Distance.action"
  "action/Lux.action"
)

ament_export_dependencies(rosidl_default_runtime)

install(PROGRAMS
  src/distance_action_server.py
  src/lux_action_server.py
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
