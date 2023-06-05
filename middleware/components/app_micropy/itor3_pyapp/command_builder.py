"""
Contains functions to build commands sent to the modbus.
"""

##
# @brief 
# This class is used to create command sent to the modbus.
# ##
class CommandBuilder(object):
    def __init__(self):
        """
        Constructor
        """
        self._payload = []
        
    def build(self):
        """
        To return the payload which can be sent to the modbus.
        """
        return self._payload
    
    def add_1byte_uint(self, value):
        """
        This function converts 1 byte value into byte and add it into the payload.
        """
        self._payload.append(value)

    def add_1byte_bool(self, value):
        """
        This function converts 1 byte value with boolean type into byte and add it into the payload.
        """
        if value:
            self._payload.append(0x01)
        else:
            self._payload.append(0x00)
    
    def add_4bytes_uint(self, uint32_value):
        """
        This function converts uint number into 4 bytes and add it into the payload.
        """
        self._payload.append( uint32_value        & 0xFF)
        self._payload.append((uint32_value >> 8)  & 0xFF)
        self._payload.append((uint32_value >> 16) & 0xFF)
        self._payload.append((uint32_value >> 24) & 0xFF)

    def add_2bytes_uint(self, q16_value):
        """
        This function converts uint number into 2 bytes and add it into the payload.
        """
        self._payload.append( q16_value        & 0xFF)
        self._payload.append((q16_value >> 8)  & 0xFF)

    def add_4bytes_float(self, fvalue):
        """
        This function converts float number into 4 bytes and add it into the payload.
        """
        q16_value = round(fvalue * 65536.0)
        # print("add_4bytes_float: {} {}".format(fvalue, q16_value)) ## Debug only
        self._payload.append( q16_value        & 0xFF)
        self._payload.append((q16_value >> 8)  & 0xFF)
        self._payload.append((q16_value >> 16) & 0xFF)
        self._payload.append((q16_value >> 24) & 0xFF)
