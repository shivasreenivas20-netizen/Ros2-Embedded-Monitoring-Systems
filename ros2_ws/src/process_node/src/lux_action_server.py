#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from rclpy.action import ActionServer
from std_msgs.msg import Float32
from process_node.action import Lux
import RPi.GPIO as GPIO

class LuxActionServer(Node):

    def __init__(self):
        super().__init__('lux_action_server')

        GPIO.setmode(GPIO.BCM)
        GPIO.setwarnings(False)

        self.LED_PIN = 6
        GPIO.setup(self.LED_PIN, GPIO.OUT)

        # Initially ON (before reaching target)
        GPIO.output(self.LED_PIN, GPIO.HIGH)

        self.current_lux = None

        self.create_subscription(
            Float32,
            '/lux',
            self.lux_callback,
            10
        )

        self._action_server = ActionServer(
            self,
            Lux,
            'lux_action',
            self.execute_callback
        )

    def lux_callback(self, msg):
        self.current_lux = msg.data

    async def execute_callback(self, goal_handle):
        self.get_logger().info('Lux goal received')

        threshold = goal_handle.request.threshold_lux

        feedback_msg = Lux.Feedback()
        result = Lux.Result()

        while rclpy.ok():

            if goal_handle.is_cancel_requested:
                goal_handle.canceled()
                result.reached = False
                return result

            rclpy.spin_once(self, timeout_sec=0.1)

            if self.current_lux is None:
                continue

            GPIO.output(self.LED_PIN, GPIO.HIGH)

            feedback_msg.current_lux = self.current_lux
            goal_handle.publish_feedback(feedback_msg)

            if self.current_lux <= threshold:
                result.reached = True
                GPIO.output(self.LED_PIN, GPIO.LOW)
                goal_handle.succeed()
                self.get_logger().info('Lux threshold reached')
                return result

    def destroy_node(self):
        GPIO.output(self.LED_PIN, GPIO.LOW)
        GPIO.cleanup()
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)

    node = LuxActionServer()
    rclpy.spin(node)

    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
