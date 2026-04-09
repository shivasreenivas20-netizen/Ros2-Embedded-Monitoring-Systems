import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32
from example_interfaces.srv import Trigger

class MCP3008Node(Node):

    def __init__(self):
        super().__init__('mcp3008_node')

        self.publisher_ = self.create_publisher(Float32, '/lux', 10)
        self.timer = self.create_timer(1.0, self.read_adc)

        self.lux_service = self.create_service(Trigger,'get_lux',self.handle_lux)

        self.get_logger().info("MCP3008 Node Started")

    def read_adc(self):
        try:
            with open('/dev/mcp3008', 'r') as fd:
                data = fd.readline().strip()

                if data:
                    value = data.split('=')[1]
                    adc_value = int(value)

                    msg = Float32()
                    msg.data = float(adc_value)

                    self.publisher_.publish(msg)

                    self.get_logger().info(f"LUX: {adc_value}")

        except Exception as e:
            self.get_logger().error(f"Read error: {e}")

    def handle_lux(self, request, response):
        try:
            with open('/dev/mcp3008', 'r') as fd:
                data = fd.readline().strip()

                if data:
                    value = data.split('=')[1]
                    adc_value = int(value)

                    response.success = True
                    response.message = f"{adc_value}"

                else:
                    response.success = False
                    response.message = "No data"

        except Exception as e:
            response.success = False
            response.message = str(e)

        return response


def main(args=None):
    rclpy.init(args=args)
    node = MCP3008Node()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()
