## ROS2 Workspace

This directory contains the ROS2-based middleware layer that interfaces with custom Linux device drivers,
and provides real-time sensor monitoring and control.

The system is built using ROS2 (Humble) on Raspberry Pi and follows a modular architecture with publishers, services, and actions.

---

## Architecture Overview

The ROS2 layer sits above the device drivers and performs:

- Sensor data acquisition from `/dev` interfaces
- Publishing real-time data over ROS2 topics
- Service-based data access
- Action-based continuous monitoring and decision making
- Hardware control (LED) based on sensor thresholds

---

## Packages

### 1. sensor_nodes (ament_python)

Handles direct interaction with device drivers and publishes sensor data.

**Nodes:**

- 'hcsr04_node'
  - Reads from '/dev/hcsr04'
  - Publishes:
    - '/distance' ('Float32')

- 'bmp280_node'
  - Reads from '/dev/bmp280'
  - Publishes:
    - '/temp_pressure' ('String')
  - Services:
    - '/get_temp'
    - '/get_pressure'

- 'mcp3008_node'
  - Reads from '/dev/mcp3008'
  - Publishes:
    - '/lux' ('Float32')
  - Services:
    - '/get_lux'

---

### 2. process_node (ament_cmake)

Implements decision-making logic using ROS2 Actions.

**Action Servers:**

- 'distance_action_server'
  - Subscribes: '/distance'
  - Goal: target distance
  - Feedback: current distance
  - Result: threshold reached
  - Controls LED via GPIO

- 'lux_action_server'
  - Subscribes: '/lux'
  - Goal: threshold lux
  - Feedback: current lux
  - Result: threshold reached
  - Controls LED via GPIO

---

## Features

- Custom Linux character driver integration
- Real-time sensor streaming using ROS2 topics
- Service-based synchronous data retrieval
- Action-based continuous monitoring
- Multi-process node execution
- GPIO-based hardware control (LED indication)
- Modular and extensible architecture

---

## Build and Run

### 1. Build Workspace

cd ros2_ws
colcon build
source install/setup.bash

### 2. Run Sensor Nodes

ros2 run sensor_nodes hcsr04_node
ros2 run sensor_nodes bmp280_node
ros2 run sensor_nodes mcp3008_node

### 3. Run Action Servers

ros2 run process_node distance_action_server.py
ros2 run process_node lux_action_server.py

### 4. Test Actions

ros2 action send_goal /distance_action process_node/action/Distance "{target_distance: 5.0}"
ros2 action send_goal /lux_action process_node/action/Lux "{threshold_lux: 300.0}"
