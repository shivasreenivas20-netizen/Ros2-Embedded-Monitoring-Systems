#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from rclpy.action import ActionServer
from std_msgs.msg import Float32
from process_node.action import Distance
import time
import RPi.GPIO as GPIO

class DistanceActionServer(Node):

    def __init__(self):
        super().__init__('distance_action_server')

        GPIO.setmode(GPIO.BCM)
        GPIO.setwarnings(False)

        self.LED_PIN = 5
        GPIO.setup(self.LED_PIN, GPIO.OUT)

        # Initially ON (before reaching target)
        GPIO.output(self.LED_PIN, GPIO.HIGH)

        self.current_distance = None

        self.create_subscription(
            Float32,
            '/distance',
            self.distance_callback,
            10
        )

        self._action_server = ActionServer(
            self,
            Distance,
            'distance_action',
            self.execute_callback
        )

    def distance_callback(self, msg):
        self.current_distance = msg.data
        #self.get_logger().info(f"[SUB] {msg.data} Dis: {self.current_distance} cm")

    async def execute_callback(self, goal_handle):
        self.get_logger().info('Distance goal received')

        target = goal_handle.request.target_distance

        feedback_msg = Distance.Feedback()
        result = Distance.Result()

        self.get_logger().info(f'Target: {target} cm');

        while rclpy.ok():

            if goal_handle.is_cancel_requested:
                goal_handle.canceled()
                result.reached = False
                return result

            rclpy.spin_once(self, timeout_sec=0.1)

            if self.current_distance is None:
                time.sleep(0.05)
                continue

            GPIO.output(self.LED_PIN, GPIO.HIGH)

            feedback_msg.current_distance = self.current_distance
            goal_handle.publish_feedback(feedback_msg)

            #self.get_logger().info(f'{self.current_distance} cm');

            if self.current_distance <= target:
                result.reached = True
                goal_handle.succeed()
                self.get_logger().info('Target reached')
                GPIO.output(self.LED_PIN, GPIO.LOW)
                return result

            #time.sleep(0.05)

    def destroy_node(self):
        GPIO.output(self.LED_PIN, GPIO.LOW)
        GPIO.cleanup()
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)

    node = DistanceActionServer()
    rclpy.spin(node)

    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
