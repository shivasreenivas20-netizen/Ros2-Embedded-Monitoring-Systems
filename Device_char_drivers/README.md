## Device Character Drivers

This directory contains custom Linux character device drivers developed for sensor interfacing on Raspberry Pi.

## Directory Structure

Device_char_drivers/
├── char_activate.sh
├── mcp3008/
├── bmp280/
└── hcsr04/

### Drivers Included

- **MCP3008** (SPI)
  - Analog-to-Digital Converter interface
  - Exposes `/dev/mcp3008`

- **BMP280** (I2C)
  - Temperature and Pressure sensor
  - Uses ioctl-based communication
  - Exposes `/dev/bmp280`

- **HCSR04** (GPIO)
  - Ultrasonic distance measurement
  - Exposes `/dev/hcsr04`

Each driver is implemented as a loadable kernel module (`.ko`) with a corresponding Makefile for compilation.

---

## Build and Load Drivers

### 1. Build Drivers

cd Device_char_drivers/mcp3008
make

cd ../bmp280
make

cd ../hcsr04
make

### 2. Load Drivers

cd ..
./char_activate.sh
