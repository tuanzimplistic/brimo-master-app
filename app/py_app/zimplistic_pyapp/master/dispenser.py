"""
Dispenser (DS) Class
"""
from zimplistic_pyapp import *
from collections import namedtuple
import uasyncio


class DSConstants():
    ## Kicker Request Code.
    DS_REQ_CODE    = 0x21

    ## Command Time out
    DS_COMMAND_TOUT = 500

    DS_IS_INGREDIENT_PRESENT    = 0x01

    DS_QUANTITY_INFO            = 0x02

    DS_INGREDIENT_BY_WEIGHT     = 0x03

    DS_INGREDIENT_ABORT         = 0x04

    DS_GET_CURRENT_LOADCELL     = 0x05
    
    DS_CLEAR_ERROR_FLAG         = 0x06

    DS_GET_STATUS               = 0x07
    
    ## Motor Error Bit
    DS_BIT_MOTOR1_ERR            = 0x01

    DS_BIT_MOTOR2_ERR            = 0x02

    DS_BIT_MOTOR3_ERR            = 0x04

    DS_BIT_PUMP_ERR              = 0x06

    DS_BIT_SOLENOID1_ERR         = 0x08

    DS_BIT_SOLENOID2_ERR         = 0x10

    ## All Error bit
    DS_BIT_ALL_ERRS         = (DS_BIT_MOTOR1_ERR | DS_BIT_MOTOR2_ERR | DS_BIT_MOTOR3_ERR | DS_BIT_PUMP_ERR | DS_BIT_SOLENOID1_ERR | DS_BIT_SOLENOID2_ERR)

    ## ACK
    ACK = 0x00

    ## NACK
    NACK = 0x01

    ## DS Command execution timeout; maximum execution time of any command is 3 minutes
    DS_CMD_EXE_TOUT     = 180000

    ## DS Command check period
    DS_CMD_CHECK_TIME = 100

class DS(DSConstants, CommandStatus):
    ##
    # @brief
    #      Constructor
    # ##
    def __init__(self):
        CommandStatus.__init__(self, self.DS_CMD_EXE_TOUT, self.DS_CMD_CHECK_TIME)

    def get_status(self):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_GET_STATUS)

        response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('DS get_status has no response')

        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_GET_STATUS)
        
        _ = dec.decode_1byte_uint()

        id = dec.decode_1byte_uint()
        is_going = dec.decode_1byte_uint()
        flags = dec.decode_1byte_uint()
        return ('status', id, is_going, flags)

    def clear_error_flags(self):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_CLEAR_ERROR_FLAG)

        response = send_command(builder.build(), self.DS_COMMAND_TOUT)

        if not response:
            raise ValueError('DS clear_error_flags has no response')

        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_CLEAR_ERROR_FLAG)

        # Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal
    
    def is_ingredient_present (self, id_of_ingredient):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_IS_INGREDIENT_PRESENT)
        builder.add_1byte_uint(id_of_ingredient)

        response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        #print(response) # for debugging
        if not response:
            raise ValueError('DS clear_error_flags has no response')

        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_IS_INGREDIENT_PRESENT)

        # Get ACK
        _ = dec.decode_1byte_uint()

        status = dec.decode_1byte_uint()

        return status

    def get_quantity_info (self, id_of_ingredient):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_QUANTITY_INFO)
        builder.add_1byte_uint(id_of_ingredient)

        response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        #print(response) # for debugging
        if not response:
            raise ValueError('DS clear_error_flags has no response')

        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_QUANTITY_INFO)

        # Get ACK value
        _ = dec.decode_1byte_uint()
        #returns original qty of ingredients and qty remaining after use
        origin_quantity = dec.decode_4bytes_q16()
        remaining_quantity = dec.decode_4bytes_q16()

        return ('origin_quantity', origin_quantity, ' remaining_quantity', remaining_quantity)


    def dispense_ingredient_by_weight(self, id_of_ingredient, expected_weight, timeout):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_INGREDIENT_BY_WEIGHT)
        builder.add_1byte_uint(id_of_ingredient)
        builder.add_4bytes_float(expected_weight)
        builder.add_2bytes_uint(timeout)

        response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        if not response:
            raise ValueError("HT run command has no response")
        
        ## Check that the response command is correct
        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_INGREDIENT_BY_WEIGHT)

        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal

    def dispense_ingredient_abort(self, id_of_ingredient):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_INGREDIENT_ABORT)
        builder.add_1byte_uint(id_of_ingredient)

        response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        if not response:
            raise ValueError("HT run command has no response")
        
        ## Check that the response command is correct
        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_INGREDIENT_ABORT)

        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal
    
    def get_current_loadcell(self):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_GET_CURRENT_LOADCELL)

        response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        #print(response) # for debugging
        if not response:
            raise ValueError('DS clear_error_flags has no response')

        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_GET_CURRENT_LOADCELL)

        # Get ACK value
        _ = dec.decode_1byte_uint()

        current_loadcell = dec.decode_4bytes_q16()

        return ('current_loadcell', current_loadcell)

    ##
    # @brief
    #      Coroutine to wait until complete
    #
    # @details
    #       This function wait should be called after function dispense_by_weight() or dispense_by_time() called.
    #       The function waits until the dispensing is completed.
    # @param [in]
    #      id: DSConstants.DS_ID_FLOUR, DSConstants.DS_ID_WATER or DSConstants.DS_ID_OIL
    # @return
    #      @arg    mater.CommandStatus.CommandRetCode
    # ##
    async def wait(self):
        retVal = self.CMD_RETCODE_NOERR
        state = self._curState
        cnt = self._cmd_exe_tout / self._cmd_check_period
        while cnt > 0:
            # Update state every _cmd_check_period
            state = self.get_status().is_going
            if state != self._curState:
                break
            cnt -= 1
            await uasyncio.sleep_ms(self._cmd_check_period)
        
        if state == self._curState:
            retVal = self.CMD_RETCODE_TOUTERR
            logger.debug(self.get_status().flags)

        return retVal