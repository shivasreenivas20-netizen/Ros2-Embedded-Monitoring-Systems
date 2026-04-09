# ROS2 Embedded Monitoring System

A real-time embedded system built on Raspberry Pi integrating **custom Linux device drivers** with **ROS2 middleware** for sensor monitoring and hardware control.

---

## What this project demonstrates

- Low-level **Linux character driver development** (GPIO, I2C, SPI)
- Direct **/dev interface integration with ROS2**
- Real-time **sensor data streaming using topics**
- **Service-based access** for sensor data
- **Action-based decision making** (threshold monitoring)
- GPIO-based **hardware control (LED)**

---

## System Overview

This system follows a full-stack embedded architecture:

	Sensors → Device Drivers → /dev Interface → ROS2 Nodes → Actions → GPIO Control

## Repository Structure

├── Device_char_drivers
├── doc
│   ├── Flowchart
│   │   ├── Component_Diagram.png
│   │   └── Sequence_Diagram.png
│   ├── HW_Schematics
│   │   └── schematics.png
│   └── README.md
├── LICENSE
├── README.md
└── ros2_ws

## ⚡ Quick Start

# Load drivers
cd Device_char_drivers
./char_activate.sh

# Run ROS2 system
cd ../ros2_ws
colcon build
source install/setup.bash
