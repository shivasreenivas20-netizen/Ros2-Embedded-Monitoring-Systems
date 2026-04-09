import rclpy
from rclpy.node import Node
from std_msgs.msg import String
#from sensor_nodes.srv import Bmp280

from example_interfaces.srv import Trigger

import fcntl
import struct
import array

def _IOR(type_char, nr, size):
    IOC_READ = 2
    return (IOC_READ << 30) | (size << 16) | (ord(type_char) << 8) | nr

GET_TEMP = _IOR('b', 1, struct.calcsize('i'))
GET_PRESSURE = _IOR('b', 2, struct.calcsize('I'))

class BMP280Node(Node):

    def __init__(self):
        super().__init__('bmp280_node')

        # Publisher
        self.publisher_ = self.create_publisher(String, '/temp_pressure', 10)
        self.timer = self.create_timer(1.0, self.publish_data)

        # Service
        #self.srv = self.create_service(Bmp280, 'bmp280_service', self.handle_service)

        self.temp_pressure_service = self.create_service(Trigger,'get_temp_pressure',self.handle_temp_pressure)
        self.temp_service = self.create_service(Trigger,'get_temperature',self.handle_temp)
        self.pressure_service = self.create_service(Trigger,'get_pressure',self.handle_pressure)

        self.get_logger().info("BMP280 Node Started")

    def publish_data(self):
        try:
            with open('/dev/bmp280', 'r') as fd:
                data = fd.readline().strip()

                msg = String()
                msg.data = data

                self.publisher_.publish(msg)

                self.get_logger().info(f'Publish: {data}')

        except Exception as e:
            self.get_logger().error(f'Publish error: {e}')

    def handle_temp_pressure(self, request, response):
        try:
            with open('/dev/bmp280', 'r') as fd:
                data = fd.readline().strip()

                if data:
                    response.success = True
                    response.message = f"{data}"

                else:
                    response.success = False
                    response.message = "No data"

        except Exception as e:
            response.success = False
            response.message = str(e)

        return response

    def handle_temp(self, request, response):
        try:
            with open('/dev/bmp280', 'rb') as fd:
                buf = array.array('i', [0])

                fcntl.ioctl(fd, GET_TEMP, buf, True)

                temp = buf[0] / 100.0

                response.success = True
                response.message = f"{temp:.2f} C"

        except Exception as e:
            response.success = False
            response.message = str(e)

        return response

    def handle_pressure(self, request, response):
        try:
            with open('/dev/bmp280', 'rb') as fd:
                buf = array.array('I', [0])

                fcntl.ioctl(fd, GET_PRESSURE, buf, True)

                pressure = buf[0] / 100.0

                response.success = True
                response.message = f"{pressure:.2f} PA"

        except Exception as e:
            response.success = False
            response.message = str(e)

        return response

    """def handle_service(self, request, response):
        try:
            with open('/dev/bmp280', 'rb') as fd:

                if request.command == "temp":
                    buf = struct.pack('i', 0)
                    fcntl.ioctl(fd, GET_TEMP, buf)
                    value = struct.unpack('i', buf)[0] / 100.0

                elif request.command == "pressure":
                    buf = struct.pack('I', 0)
                    fcntl.ioctl(fd, GET_PRESSURE, buf)
                    value = struct.unpack('I', buf)[0] / 100.0

                else:
                    value = -1.0

                response.value = float(value)

        except Exception as e:
            self.get_logger().error(f'Service error: {e}')
            response.value = -1.0

        return response"""


def main(args=None):
    rclpy.init(args=args)
    node = BMP280Node()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()
