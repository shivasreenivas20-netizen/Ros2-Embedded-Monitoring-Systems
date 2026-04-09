#!/bin/bash

# Get current script directory
BASE_DIR=$(dirname "$(realpath "$0")")

echo "Base directory: $BASE_DIR"

# Driver paths
MCP3008_KO="$BASE_DIR/mcp3008/driver_MCP3008.ko"
BMP280_KO="$BASE_DIR/bmp280/driver_BMP280_char.ko"
HCSR04_KO="$BASE_DIR/hcsr04/driver_HC-SR04.ko"

#unload drivers
sudo rmmod driver_MCP3008
sudo rmmod driver_BMP280_char
sudo rmmod driver_HC-SR04

# Load drivers
sudo insmod "$MCP3008_KO"
sudo insmod "$BMP280_KO"
sudo insmod "$HCSR04_KO"

# I2C binding
echo bmp280_char 0x76 | sudo tee /sys/bus/i2c/devices/i2c-1/new_device

# SPI binding
echo spi0.0 | sudo tee /sys/bus/spi/drivers/spidev/unbind
echo mcp3008 | sudo tee /sys/bus/spi/devices/spi0.0/driver_override
echo spi0.0 | sudo tee /sys/bus/spi/drivers/mcp3008/bind

# Get majors
MCP_MAJOR=$(awk '/mcp3008/ {print $1}' /proc/devices)
BMP_MAJOR=$(awk '/bmp280/ {print $1}' /proc/devices)
HCSR04_MAJOR=$(awk '/hcsr04/ {print $1}' /proc/devices)

echo "MCP3008 Major: $MCP_MAJOR"
echo "BMP280 Major: $BMP_MAJOR"
echo "HCSR04 Major: $HCSR04_MAJOR"

# Remove old nodes (avoid duplicate error)
sudo rm -f /dev/mcp3008
sudo rm -f /dev/bmp280
sudo rm -f /dev/hcsr04

# Create nodes
sudo mknod /dev/mcp3008 c $MCP_MAJOR 0
sudo mknod /dev/bmp280 c $BMP_MAJOR 0
sudo mknod /dev/hcsr04 c $HCSR04_MAJOR 0

# Permissions
sudo chmod 666 /dev/mcp3008
sudo chmod 666 /dev/bmp280
sudo chmod 666 /dev/hcsr04
