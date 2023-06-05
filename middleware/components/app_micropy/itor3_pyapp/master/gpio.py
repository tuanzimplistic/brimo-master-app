"""
GPIO Class
"""

from collections import namedtuple
import uasyncio
from itor3_pyapp.app.log import *
from itor3_pyapp import *
from itor3_pyapp.master.itor3command import *

class GPIOConstants():
    MB_ZPL_REQ2F                 = 0x2F
    GPIO_CMD_TMOUT               = 500 #in ms
    ## Subcode
    REQ2F_DOUT_GET_STATE         = 0x00 ## Get Dout state
    REQ2F_DIN_GET_STATE			 = 0x01 ## Get Din state
    REQ2F_DOUT_SET_STATE         = 0x02 ## Set Dout state

    ACK = 0x00  ## ACK
    NACK = 0x01 ## NACK

class GPIO(GPIOConstants):
    ##
    # @brief
    #      Constructor
    # ##
    def __init__(self):
        pass

    def get_dout_state(self, id):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.MB_ZPL_REQ2F)
        builder.add_1byte_uint(self.REQ2F_DOUT_GET_STATE)
        builder.add_1byte_uint(id)
        response = send_command(builder.build(), self.GPIO_CMD_TMOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('GPIO get_dout_get_state() has no response')

        dec = CommandDecoder(response, self.MB_ZPL_REQ2F, self.REQ2F_DOUT_GET_STATE)
        
        _ =  dec.decode_1byte_uint() # ACK is not used
        
        return dec.decode_1byte_uint()

    def get_din_state(self, id):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.MB_ZPL_REQ2F)
        builder.add_1byte_uint(self.REQ2F_DIN_GET_STATE)
        builder.add_1byte_uint(id)
        response = send_command(builder.build(), self.GPIO_CMD_TMOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('GPIO get_dout_get_state() has no response')

        dec = CommandDecoder(response, self.MB_ZPL_REQ2F, self.REQ2F_DIN_GET_STATE)
        
        _ =  dec.decode_1byte_uint() # ACK is not used
        
        return dec.decode_1byte_uint()

    def set_dout_state(self, id, value):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.MB_ZPL_REQ2F)
        builder.add_1byte_uint(self.REQ2F_DOUT_SET_STATE)
        builder.add_1byte_uint(id)
        builder.add_1byte_uint(value)
        response = send_command(builder.build(), self.GPIO_CMD_TMOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('GPIO set_dout_state() has no response')

        dec = CommandDecoder(response, self.MB_ZPL_REQ2F, self.REQ2F_DOUT_SET_STATE)
        
        _ =  dec.decode_1byte_uint() # ACK is not used
        
        return True