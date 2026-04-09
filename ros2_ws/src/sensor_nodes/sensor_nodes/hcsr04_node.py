import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32

from example_interfaces.srv import Trigger

class HCSR04Node(Node):

    def __init__(self):
        super().__init__("hcsr04_node")
        self.publisher_ = self.create_publisher(Float32, '/distance',10)
        self.timer = self.create_timer(0.1, self.read_and_publish)

        self.distance_service = self.create_service(Trigger,'get_distance',self.handle_distance)

    def read_and_publish(self):
        try:
            with open('/dev/hcsr04', 'r') as fd:
            #self.fd.seek(0)
                data = fd.readline().strip()

                if data:
                    distance = float(data)
                    msg = Float32()
                    msg.data = distance

                    self.publisher_.publish(msg)

                    self.get_logger().info(f'Distance: {distance} cm')
        except Exception as e:
            self.get_logger().error(f'Read error: {e}')

    def handle_distance(self, request, response):
        try:
            with open('/dev/hcsr04', 'r') as fd:
                data = fd.readline().strip()

                if data:
                    distance = float(data)

                    response.success = True
                    response.message = f"{distance}"

                else:
                    response.success = False
                    response.message = "No data"

        except Exception as e:
            response.success = False
            response.message = str(e)

        return response

def main(args=None):
    rclpy.init(args=args)

    node = HCSR04Node()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass

    node.destroy_node()
    rclpy.shutdown()
