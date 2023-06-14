"""
Dispenser (DS) Class
"""
from zimplistic_pyapp import *
from zimplistic_pyapp.master.itor3command import *
from zimplistic_pyapp.app.log import *
from collections import namedtuple
import uasyncio

class DSConstants():
    ## Kicker Request Code.
    DS_REQ_CODE    = 0x21

    ## Command Time out
    DS_COMMAND_TOUT = 500

    ## Get State subcode
    DS_SUBCODE_GET_STATE                   = 0x00

    ## Flour ID
    DS_ID_FLOUR                            = 0x00

    ## Water ID
    DS_ID_WATER                            = 0x01

    ## Oil ID
    DS_ID_OIL                              = 0x02
    
    ## Clear Error Flags subcode
    DS_SUBCODE_CLEAR_ERROR_FLAG            = 0x01
    
    ## Abort dispenser subcode
    DS_SUBCODE_ABORT_DISPENSE              = 0x02

    ## Close shutter subcode
    DS_SUBCODE_CLOSE_SHUTTER               = 0x03

    ## Find Home subcode
    DS_SUBCODE_FIND_HOME                   = 0x04

    ## Dispense subcode
    DS_SUBCODE_DISPENSE                    = 0x05

    ## Set Parameters subcode
    DS_SUBCODE_SET_PARAMETERS              = 0x06

    ## Get Dispenser Pos subcode
    DS_SUBCODE_GET_DISPENSER_POS           = 0x07

    ## Weight Tare subcode
    DS_SUBCODE_TARE_WEIGHTSCALE            = 0x08

    ## Get Weight Scale parameter subcode
    DS_SUBCODE_GET_WEIGHTSCALE_PARAMETERS  = 0x09

    ## Position KP PARAMETERS_FLAG
    DS_PARAM_POSITION_KP                       = 0x00

    ## Weight KP PARAMETERS_FLAG
    DS_PARAM_WEIGHT_KP                         = 0x01

    ## Accelerate factor PARAMETERS_FLAG
    DS_PARAM_ACCEL_FACTOR                      = 0x02
    
    ## Stop tollerance PARAMETERS_FLAG
    DS_PARAM_STOP_TOLERANCE                    = 0x03

    ## Weight scale gain PARAMETERS_FLAG
    DS_PARAM_WEIGHTSCALE_GAIN                  = 0x04

    ## IDLE State 
    DS_STATE_IDLE = 0x00

    ## Busy State
    DS_STATE_BUSY = 0x01

    ## Home Error Bit
    DS_BIT_HOME_ERR             = 0x01

    ## Motor Error Bit
    DS_BIT_MOTOR_ERR            = 0x02

    ## Dispensation Error Bit
    DS_BIT_DISPENSATION_ERR     = 0x04

    ## Timeout error bit
    DS_BIT_TIMEOUT_ERR          = 0x08

    ## Shutter error bit
    DS_BIT_SHUTTER_ERR          = 0x10

    ## Index is known bit
    DS_BIT_INDEX_KNOWN          = 0x20

    ## All Error bit
    DS_BIT_ALL_ERRS         = (DS_BIT_HOME_ERR | DS_BIT_MOTOR_ERR | DS_BIT_DISPENSATION_ERR | DS_BIT_TIMEOUT_ERR | DS_BIT_SHUTTER_ERR)

    ## ACK
    ACK = 0x00

    ## NACK
    NACK = 0x01

    ## DS Command execution timeout; maximum execution time of any command is 3 minutes
    DS_CMD_EXE_TOUT     = 180000

    ## DS Command check period
    DS_CMD_CHECK_TIME = 100

class DS(DSConstants, Itor3Command):
    ##
    # @brief
    #      Constructor
    # ##
    def __init__(self):
        ## namedtyple to store the Module State
        self._DispenserStatus = namedtuple("DispenserStatus", "id state flags weight current")
        Itor3Command.__init__(self, self.DS_CMD_EXE_TOUT, self.DS_CMD_CHECK_TIME)
        
        self._ds_state = [-1, -1, -1]
        self._ds_flags = [-1, -1, -1]

    ##
    # @brief
    #      Get the status of DS module.
    #
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() to get the DS module status and waits for
    #       the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    # @param [in]
    #       id: #DS_ID_FLOUR or #DS_ID_WATER or #DS_ID_OIL
    #
    # @return
    #      @arg    namedtuple: ("DispenserStatus", "id state flags weight current") where weight is the current filterred value of the loadcell and current is the 
    #           flour motor's current
    # ##
    def get_status(self, id):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_SUBCODE_GET_STATE)
        
        builder.add_1byte_uint(id)
        response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('DS get_status has no response')

        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_SUBCODE_GET_STATE)
        
        if dec.decode_1byte_uint() != self.ACK:
            #raise ValueError('DS get_status return False')
            pass

        if dec.decode_1byte_uint() != id:
            raise ValueError('DS ID is invalid')

        self._ds_state[id] = dec.decode_1byte_uint()
        self._ds_flags[id] = dec.decode_1byte_uint()
        weight = dec.decode_4bytes_q16()
        current = dec.decode_4bytes_q16()
        status = self._DispenserStatus(id, self._ds_state[id], self._ds_flags[id], weight, current)
        return status

    ##
    # @brief
    #      Clear the error flags.
    #
    #
    # @details
    #       This function sends command to slave by calling the function command_sender.send_command() waits for
    #           the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #       The function clear the error flags of the DS module. If the previous command to DS module has any issue, this command
    #           needs to be sent before other any commands.
    #
    # @param [in]
    #       id: #DS_ID_FLOUR or #DS_ID_WATER or #DS_ID_OIL
    #
    # @return
    #      @arg    True: if the command sent successfully.
    # @return
    #      @arg    False: if the command sent failed.
    # ##
    def clear_error_flags(self, id):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_SUBCODE_CLEAR_ERROR_FLAG)

        builder.add_1byte_uint(id)
        response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        #print(response) # for debugging
        if not response:
            raise ValueError('DS clear_error_flags has no response')

        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_SUBCODE_CLEAR_ERROR_FLAG)

        # Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        # Get ID
        if dec.decode_1byte_uint() != id:
            raise ValueError('DS ID is incorrect')
        
        return retVal


    ##
    # @brief
    #      Abort dispensing.
    #
    #
    # @details
    #       This function sends command to slave by calling the function command_sender.send_command() waits for
    #           the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #       The function aborts the current dispensing.
    #
    # @param [in]
    #       id: #DS_ID_FLOUR or #DS_ID_WATER or #DS_ID_OIL
    #
    # @return
    #      @arg    True: if the command sent successfully.
    # @return
    #      @arg    False: if the command sent failed.
    # ##
    def abort_dispense(self, id):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_SUBCODE_ABORT_DISPENSE)
        
        builder.add_1byte_uint(id)
        response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('DS abort_dispense has no response')

        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_SUBCODE_ABORT_DISPENSE)

        # Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        # Get ID
        if dec.decode_1byte_uint() != id:
            raise ValueError('DS ID is incorrect')

        return retVal


    ##
    # @brief
    #      Close shutter.
    #
    #
    # @details
    #       This function sends command to slave by calling the function command_sender.send_command() waits for
    #           the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #       The function closes the flour shutter with the input speed in rad/s
    #
    # @param [in]
    #       spd: speed to close the shutter (rad/s)
    #
    # @return
    #      @arg    True: if the command sent successfully.
    # @return
    #      @arg    False: if the command sent failed.
    # ##
    def close_shutter(self, spd):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_SUBCODE_CLOSE_SHUTTER)
        builder.add_1byte_uint(self.DS_ID_FLOUR)
        builder.add_4bytes_float(spd)
        response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('DS close_shutter has no response')

        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_SUBCODE_CLOSE_SHUTTER)

        # Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        # Get ID
        if dec.decode_1byte_uint() != self.DS_ID_FLOUR:
            raise ValueError('DS ID is incorrect')

        return retVal

    ##
    # @brief
    #      Find home.
    #
    #
    # @details
    #       This function sends command to slave by calling the function command_sender.send_command() waits for
    #           the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #       The function finds the home position (when the flag is detected) on flour sensor with the input speed.
    #
    # @param [in]
    #       spd: speed to find home (rad/s)
    #
    # @return
    #      @arg    True: if the command sent successfully.
    # @return
    #      @arg    False: if the command sent failed.
    # ##
    def find_home(self, spd):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_SUBCODE_FIND_HOME)
        builder.add_1byte_uint(self.DS_ID_FLOUR)
        builder.add_4bytes_float(spd)
        response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('DS find_home has no response')

        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_SUBCODE_FIND_HOME)

        # Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        # Get ID
        if dec.decode_1byte_uint() != self.DS_ID_FLOUR:
            raise ValueError('DS ID is incorrect')

        return retVal

    ##
    # @brief
    #      Dispense by weight
    #
    #
    # @details
    #       This function sends command to slave by calling the function command_sender.send_command() waits for
    #           the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #       The function dispense FLOUR/WATER/OIL by weight based on input paramters.
    #
    # @param [in]
    #       id: #DS_ID_FLOUR or #DS_ID_WATER or #DS_ID_OIL
    # @param [in]
    #       weight: weight to dispense in gram
    # @param [in]
    #       spd: dispensing speed (rad/s), applicable for flour only
    #
    # @code
    #   # To dispense 10 gram of flour with motor speed at 3 rad/s
    #   ds = DS()
    #   ds.dispense_by_weight(ds.DS_ID_FLOUR, 10, 3)
    #   status = ds.get_status(ds.DS_ID_FLOUR)
    #   print("Flour dispensed: %f" % status.weight)
    # @endcode
    # @return
    #      @arg    True: if the command sent successfully.
    # @return
    #      @arg    False: if the command sent failed.
    # ##
    def dispense_by_weight(self, id, weight, spd, toCloseShutter = False):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_SUBCODE_DISPENSE)

        builder.add_1byte_uint(id)
        builder.add_1byte_uint(0x00)
        builder.add_4bytes_float(weight)
        builder.add_4bytes_float(spd)
        builder.add_1byte_bool(toCloseShutter)
        response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('DS dispense_by_weight has no response')

        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_SUBCODE_DISPENSE)

        # Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
            self._curState = self.DS_STATE_IDLE
        else:
            self._curState = self.DS_STATE_BUSY

        # Get ID
        if dec.decode_1byte_uint() != id:
            raise ValueError('DS ID is incorrect')

        return retVal

    ##
    # @brief
    #      Dispense by time
    #
    #
    # @details
    #       This function sends command to slave by calling the function command_sender.send_command() waits for
    #           the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #       The function dispense FLOUR/WATER/OIL by time based on input paramters.
    #
    # @param [in]
    #       id: #DS_ID_FLOUR or #DS_ID_WATER or #DS_ID_OIL
    # @param [in]
    #       time: time to dispense in millisecond (ms)
    # @param [in]
    #       spd: dispensing speed (rad/s), applicable for flour only
    #
    # @code
    #   # To dispense flour with motor speed at 3 rad/s in 1 second
    #   ds = DS()
    #   ds.dispense_by_time(ds.DS_ID_FLOUR, 100, 3)
    #   status = ds.get_status(ds.DS_ID_FLOUR)
    #   print("Flour dispensed: %f" % status.weight)
    # @endcode
    # @return
    #      @arg    True: if the command sent successfully.
    # @return
    #      @arg    False: if the command sent failed.
    # ##
    def dispense_by_time(self, id, time, spd):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_SUBCODE_DISPENSE)
        
        builder.add_1byte_uint(id)
        builder.add_1byte_uint(0x01)
        builder.add_4bytes_uint(time)
        builder.add_4bytes_float(spd)
        response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('DS dispense_by_time has no response')

        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_SUBCODE_DISPENSE)

        # Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
            self._curState = self.DS_STATE_IDLE
        else:
            self._curState = self.DS_STATE_BUSY

        # Get ID
        if dec.decode_1byte_uint() != id:
            raise ValueError('DS ID is incorrect')

        return retVal

    ##
    # @brief
    #      Set dispensing paramters.
    #
    #
    # @details
    #       This function sends command to slave by calling the function command_sender.send_command() waits for
    #           the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #       The function sets the dispensing parameters.
    #
    # @param [in]
    #       id: #DS_ID_FLOUR or #DS_ID_WATER or #DS_ID_OIL
    # @param [in]
    #       param: #DS_PARAM_POSITION_KP, #DS_PARAM_WEIGHT_KP, #DS_PARAM_ACCEL_FACTOR, #DS_PARAM_STOP_TOLERANCE 
    #               or #DS_PARAM_WEIGHTSCALE_GAIN
    # @param [in]
    #       value: value to be set
    #
    # @code
    #   # To set the weight gain to 0.5 for flour
    #   ds = DS()
    #   ds.set_param(ds.DS_ID_FLOUR, ds.DS_PARAM_WEIGHTSCALE_GAIN, 0.5)
    # @endcode
    # @return
    #      @arg    True: if the command sent successfully.
    # @return
    #      @arg    False: if the command sent failed.
    # ##
    def set_param(self, id, param, value):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_SUBCODE_SET_PARAMETERS)

        builder.add_1byte_uint(id)
        builder.add_1byte_uint(param)
        builder.add_4bytes_float(value)
        response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('DS set_param has no response')

        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_SUBCODE_SET_PARAMETERS)

        # Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
        
        # Get ID
        if dec.decode_1byte_uint() != id:
            raise ValueError('DS ID is incorrect')

        return retVal

    ##
    # @brief
    #      Get flour encoder position
    #
    #
    # @details
    #       This function sends command to slave by calling the function command_sender.send_command() waits for
    #           the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #       The function get the current position of flour encoder. This function is for debugging only, Roti application will not
    #           use this function.
    #
    # @return
    #      @arg    pos: larger or equal to 0 if the encoder is got correctly.
    # @return
    #      @arg    pos: -1 (invalid value)
    # ##
    def get_flrmotor_pos(self):
        retVal = -1 # invalid value
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_SUBCODE_GET_DISPENSER_POS)
        builder.add_1byte_uint(self.DS_ID_FLOUR)
        response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('DS get_flour_motor_position has no response')

        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_SUBCODE_GET_DISPENSER_POS)

        # Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            return retVal

        # Get ID
        if dec.decode_1byte_uint() != self.DS_ID_FLOUR:
            raise ValueError('DS ID is incorrect')
        
        retVal = dec.decode_4bytes_uint()
        return retVal

    ##
    # @brief
    #      Weight tare.
    #
    #
    # @details
    #       This function sends command to slave by calling the function command_sender.send_command() waits for
    #           the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #       The function tares the loadcell.    
    #
    # @return
    #      @arg    True: if the command is sent successfully.
    # @return
    #      @arg    False: if the command is sent failed.
    # ##
    def weight_tare(self):
        #print("ds.weight_tare(): This function does nothing and will be deleted soon")
        logger.debug({'ds.weight_tare()':'This function does nothing and will be deleted soon'})
        return True
        # retVal = True
        # builder = CommandBuilder()
        # builder.add_1byte_uint(self.DS_REQ_CODE)
        # builder.add_1byte_uint(self.DS_SUBCODE_TARE_WEIGHTSCALE)
        # builder.add_1byte_uint(self.DS_ID_FLOUR) # for dummy
        # response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        # # print(response) # for debugging
        # if not response:
        #     raise ValueError('DS get_flour_motor_position has no response')

        # dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_SUBCODE_TARE_WEIGHTSCALE)

        # # Get ACK
        # if dec.decode_1byte_uint() != self.ACK:
        #     retVal = False

        # # Get ID
        # if dec.decode_1byte_uint() != self.DS_ID_FLOUR:
        #     raise ValueError('DS ID is incorrect')
        
        # return retVal

    ##
    # @brief
    #      Get weight scale parameters.
    #
    #
    # @details
    #       This function sends command to slave by calling the function command_sender.send_command() waits for
    #           the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #       The function get the weight scale paramters and put them into a namedtuple.
    #
    # @return
    #      @arg    namedtuple("DispenserWtScale", "gain offset") if all values are -1 (invalid value), otherwise, valid values
    # ##
    def get_wtscale_para(self):
        _DispenserWtScale = namedtuple("DispenserWtScale", "gain offset")
        builder = CommandBuilder()
        builder.add_1byte_uint(self.DS_REQ_CODE)
        builder.add_1byte_uint(self.DS_SUBCODE_GET_WEIGHTSCALE_PARAMETERS)
        builder.add_1byte_uint(self.DS_ID_FLOUR) # for dummy
        response = send_command(builder.build(), self.DS_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('DS get_flour_motor_position has no response')

        dec = CommandDecoder(response, self.DS_REQ_CODE, self.DS_SUBCODE_GET_WEIGHTSCALE_PARAMETERS)

        # Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            return _DispenserWtScale(-1, -1) # invalid value

        # Get ID
        if dec.decode_1byte_uint() != self.DS_ID_FLOUR:
            raise ValueError('DS ID is incorrect')
        
        wt_gain = dec.decode_4bytes_q16()
        wt_offset = dec.decode_4bytes_q16()
        return _DispenserWtScale(wt_gain, wt_offset)

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
    #      @arg    mater.itor3command.CommandRetCode
    # ##
    async def wait(self, id):
        retVal = self.CMD_RETCODE_NOERR
        state = self._curState
        cnt = self._cmd_exe_tout / self._cmd_check_period
        while cnt > 0:
            # Update state every _cmd_check_period
            state = self.get_status(id).state
            if state != self._curState:
                break
            cnt -= 1
            await uasyncio.sleep_ms(self._cmd_check_period)
        
        if state == self._curState:
            retVal = self.CMD_RETCODE_TOUTERR
        elif state != self._idle_state: 
            retVal = self.CMD_RETCODE_EXEERR

        ## Checking flag
        if retVal == self.CMD_RETCODE_NOERR:
            if self._ds_flags[id] & self.DS_BIT_MOTOR_ERR or \
                self._ds_flags[id] & self.DS_BIT_DISPENSATION_ERR or \
                self._ds_flags[id] & self.DS_BIT_TIMEOUT_ERR:
                retVal = CommandRetCode.CMD_RETCODE_EXEERR

        return retVal