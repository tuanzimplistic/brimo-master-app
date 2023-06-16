"""
Kneader Class
"""

from collections import namedtuple
import uasyncio
from zimplistic_pyapp import *


class KNConstants():
    ## Kneader Request Code.
    KNEADER_REQ_CODE    = 0x24

    ## Clear Error Subcode.
    CLEAR_ERR_FLAGS = 0x01

    IS_POT_PRESENT = 0x02

    IS_STIRRER_PRESENT = 0x03

    GET_STIRRER_STATUS = 0x04

    STIRRER_ATTACH = 0x05

    STIRRER_DETACH = 0x06

    STIRRER_ON = 0x07

    STIRRER_OFF	= 0x08

    ## Command ACK
    ACK = 0

    ## Command NACK
    NACK = 1

    ## Command Sending time out
    KNEADER_COMMAND_TOUT = 500

    ## Kneader Command Execution time out; maximum wait time for a command is 1 minute
    KNEADER_CMD_EXETOUT = 60000

    ## Command check period
    KNEADER_CMD_CHECK_TIME = 100

    ## Motor Error flag
    KN_BIT_STIRRER_MOTOR_ERR = 0x01

    KN_BIT_ATTACH_MOTOR_ERR = 0x02

    ## All bit sets
    KN_BIT_ALL_ERRS         = (KN_BIT_STIRRER_MOTOR_ERR | KN_BIT_ATTACH_MOTOR_ERR)

class KN(KNConstants, CommandStatus):
    def __init__(self):
        CommandStatus.__init__(self, self.KNEADER_CMD_EXETOUT, self.KNEADER_CMD_CHECK_TIME)

    ##
    # @brief
    #      Clear the error flag of Kneader module. When the previous command to control 
    #      kneader module failed, this command should be sent befor any other commands.
    #
    # @details
    #      This function creates a sending buffer and sent it to the modbus.
    #
    #
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    # ##
    def clear_error_flags(self):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.KNEADER_REQ_CODE)
        builder.add_1byte_uint(self.CLEAR_ERR_FLAGS)
        response = send_command(builder.build(), self.KNEADER_COMMAND_TOUT)
        if not response:
            raise ValueError('KN clear_error_flags has no response')
        
        ## Check that the response command is correct 
        dec = CommandDecoder(response, self.KNEADER_REQ_CODE, self.CLEAR_ERR_FLAGS)
        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal
    
    def is_pot_present(self):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.KNEADER_REQ_CODE)
        builder.add_1byte_uint(self.IS_POT_PRESENT)
        response = send_command(builder.build(), self.KNEADER_COMMAND_TOUT)
        
        if not response:
            raise ValueError('KN get_status has no response')
        
        dec = CommandDecoder(response, self.KNEADER_REQ_CODE, self.IS_POT_PRESENT)

        _ = dec.decode_1byte_uint() # ACK, not used

        state = dec.decode_1byte_uint()
        return state
    
    def is_stirrer_present(self):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.KNEADER_REQ_CODE)
        builder.add_1byte_uint(self.IS_STIRRER_PRESENT)
        response = send_command(builder.build(), self.KNEADER_COMMAND_TOUT)
        
        if not response:
            raise ValueError('KN get_status has no response')

        dec = CommandDecoder(response, self.KNEADER_REQ_CODE, self.IS_STIRRER_PRESENT)

        _ = dec.decode_1byte_uint() # ACK, not used

        state = dec.decode_1byte_uint()
        return state
    
    def get_stirrer_status(self):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.KNEADER_REQ_CODE)
        builder.add_1byte_uint(self.GET_STIRRER_STATUS)
        response = send_command(builder.build(), self.KNEADER_COMMAND_TOUT)
        
        if not response:
            raise ValueError('KN get_status has no response')

        dec = CommandDecoder(response, self.KNEADER_REQ_CODE, self.GET_STIRRER_STATUS)

        _ = dec.decode_1byte_uint() # ACK, not used

        state = dec.decode_1byte_uint()
        flags = dec.decode_1byte_uint()
        rpm = dec.decode_4bytes_q16()
        current = dec.decode_4bytes_q16()
        return ('stirrer_status', state, flags, rpm, current)

    def stirrer_attach(self):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.KNEADER_REQ_CODE)
        builder.add_1byte_uint(self.STIRRER_ATTACH)
        response = send_command(builder.build(), self.KNEADER_COMMAND_TOUT)
        
        if not response:
            raise ValueError('KN get_motor_encoder has no response')

        dec = CommandDecoder(response, self.KNEADER_REQ_CODE, self.STIRRER_ATTACH)

        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal

    def stirrer_detach(self):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.KNEADER_REQ_CODE)
        builder.add_1byte_uint(self.STIRRER_DETACH)
        response = send_command(builder.build(), self.KNEADER_COMMAND_TOUT)
        
        if not response:
            raise ValueError('KN get_motor_encoder has no response')

        dec = CommandDecoder(response, self.KNEADER_REQ_CODE, self.STIRRER_DETACH)

        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal

    def stirrer_on(self, rpm):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.KNEADER_REQ_CODE)
        builder.add_1byte_uint(self.STIRRER_ON)
        builder.add_4bytes_float(rpm)
        response = send_command(builder.build(), self.KNEADER_COMMAND_TOUT)
        # print(response) # For debugging
        if not response:
            raise ValueError("KN run command has no response")
        
        ## Check that the response command is correct
        dec = CommandDecoder(response, self.KNEADER_REQ_CODE, self.STIRRER_ON)
        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal

    def stirrer_off(self):
        retVal = True
        buider = CommandBuilder()
        buider.add_1byte_uint(self.KNEADER_REQ_CODE)
        buider.add_1byte_uint(self.STIRRER_OFF)
        response = send_command(buider.build(), self.KNEADER_COMMAND_TOUT)
        # print(response) #for debugging
        if not response:
            raise ValueError("KN stop command has no response")
        
        ## Check that the response command is correct
        dec = CommandDecoder(response, self.KNEADER_REQ_CODE, self.STIRRER_OFF)
        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal

    async def wait(self):
        retVal = await super().wait()
        return retVal