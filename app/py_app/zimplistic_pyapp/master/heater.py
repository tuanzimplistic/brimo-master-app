"""
Heater (HT) Class
"""

from collections import namedtuple
import uasyncio
from zimplistic_pyapp import *


class HTConstants():
    # WP Request Code
    HT_REQ_CODE = 0x20

    # Command timeout in ms
    HT_COMMAND_TOUT = 500

    # Get state subcode
    HT_GET_STATUS = 0x01

    HT_CLEAR_ERROR = 0x02

    HT_ON = 0x03

    HT_OFF = 0x04

    HT_COOL_ON = 0x05

    HT_COOL_OFF = 0x06

    # ACK
    ACK = 0x00

    # NACK
    NACK = 0x01


class HT(HTConstants):
    ##
    # @brief
    #      Constructor
    # ##
    def __init__(self):
        pass

    def get_status(self, sensor_id):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.HT_GET_STATUS)
        builder.add_1byte_uint(sensor_id)

        response = send_command(builder.build(), self.HT_COMMAND_TOUT)

        if not response:
            raise ValueError('no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.HT_GET_STATUS)

        if dec.decode_1byte_uint() != self.ACK:
            raise ValueError("wrong ACK")

        temp = dec.decode_4bytes_q16()

        return ('sensor_id', sensor_id, 'temp', temp)

    def clear_error_flags(self):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.HT_CLEAR_ERROR)

        response = send_command(builder.build(), self.HT_COMMAND_TOUT)

        if not response:
            raise ValueError('no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.HT_CLEAR_ERROR)

        if dec.decode_1byte_uint() != self.ACK:
            raise ValueError("wrong ACK")

    def heater_on(self, set_temperature):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.HT_ON)
        builder.add_4bytes_float(set_temperature)

        response = send_command(builder.build(), self.HT_COMMAND_TOUT)

        if not response:
            raise ValueError('no response')

        # Check that the response command is correct
        dec = CommandDecoder(response, self.HT_REQ_CODE, self.HT_ON)

        if dec.decode_1byte_uint() != self.ACK:
            raise ValueError("wrong ACK")

    def heater_off(self):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.HT_OFF)

        response = send_command(builder.build(), self.HT_COMMAND_TOUT)

        if not response:
            raise ValueError('no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.HT_OFF)

        if dec.decode_1byte_uint() != self.ACK:
            raise ValueError("wrong ACK")

    def cooling_on(self, fan_id):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.HT_COOL_ON)
        builder.add_1byte_uint(fan_id)

        response = send_command(builder.build(), self.HT_COMMAND_TOUT)

        if not response:
            raise ValueError('no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.HT_COOL_ON)

        if dec.decode_1byte_uint() != self.ACK:
            raise ValueError("wrong ACK")

    def cooling_off(self, fan_id):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.HT_COOL_OFF)
        builder.add_1byte_uint(fan_id)

        response = send_command(builder.build(), self.HT_COMMAND_TOUT)

        if not response:
            raise ValueError('no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.HT_COOL_OFF)

        if dec.decode_1byte_uint() != self.ACK:
            raise ValueError("wrong ACK")
