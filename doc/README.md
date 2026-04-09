# ROS2 Embedded Monitoring System

## Overview

This project implements a real-time embedded monitoring system on Raspberry Pi using custom Linux character device drivers integrated with ROS2.
The system interfaces with multiple sensors (ultrasonic, light, environmental) through low-level drivers and exposes data through ROS2 topics, services, and actions. 
Decision-making is performed using ROS2 action servers, enabling real-time control of hardware outputs.

---

## System Architecture

The system follows a layered architecture:

- **Hardware Layer** → Sensors and actuators
- **Driver Layer** → Custom Linux character drivers (/dev interface)
- **ROS2 Layer** → sensor_nodes and process_node packages
- **Control Layer** → Action-based decision making and GPIO control

---

## Hardware Schematic

![Hardware Schematic](docs/HW_Schematics/hardware_schematic.png)

---

## Component Diagram

![Component Diagram](docs/Flowchat/component_diagram.png)

---

## Sequence Diagram

![Sequence Diagram](docs/Flowchart/sequence_diagram.png)
