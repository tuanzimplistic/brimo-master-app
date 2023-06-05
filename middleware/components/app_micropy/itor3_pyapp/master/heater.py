"""
Heater (HT) Class
"""

from collections import namedtuple
import uasyncio
from itor3_pyapp.app.log import *
from itor3_pyapp import *
from itor3_pyapp.master.itor3command import *

class HTConstants():
    ## WP Request Code
    HT_REQ_CODE = 0x20

    ## Command timeout in ms
    HT_COMMAND_TOUT = 500

    ## Get state subcode
    HT_SUBCODE_GET_STATE              = 0x00

    ## Clear error subcode
    HT_SUBCODE_CLEAR_ERROR            = 0x01

    ## Set state subcode
    HT_SUBCODE_SET_STATE              = 0x02

    ## Set temperature reference sucode
    HT_SUBCODE_SET_TEMP_REF           = 0x03

    ## Set PID Coeffient subcode
    HT_SUBCODE_SET_PID_COEF           = 0x04

    ## Set Overtemperature subcode
    HT_SUBCODE_SET_SUPERV_OVERTEMP    = 0x05

    ## Set Fan state subcode
    HT_SUBCODE_SET_FAN                = 0x06

    ## Set Fan state with id subcode
    HT_SUBCODE_SET_FAN_ID             = 0x07

    ## Set Fan with PWM
    HT_SUBCODE_SET_FAN_PWM            = 0x08

    ## Heater OFF state
    HT_STATE_OFF                      = 0x00

    ## Heater ON Open Loop state
    HT_STATE_ON_OPEN_LOOP             = 0x01

    ## Heater ON Close Loop state
    HT_STATE_ON_CLOSE_LOOP            = 0x02

    ## Heater Error state
    HT_STATE_ERROR                    = 0x03

    ## Heater ID of Top
    HT_ID_TOP     = 0x00

    ## Heater ID of Bottom
    HT_ID_BTM     = 0x01

    ## Heater ID of Char Top
    HT_ID_CHARTOP = 0x00

    ## Heater ID of Char Bottom
    HT_ID_CHARBTM = 0x01

    ## ACK
    ACK = 0x00

    ## NACK
    NACK = 0x01

    ## Heater check period
    HT_CHECK_TIME = 500

    ## Charring ID
    CHARRING_TOP_ID = 0
    CHARRING_BTM_ID = 1

    ## Charring Sub-code
    REQ20_CHAR_GET_STATE				= 0x10
    REQ20_CHAR_SET_ON_STATE				= 0x11
    REQ20_CHAR_SET_OFF_STATE			= 0x12
    REQ20_CHAR_SET_SAFE_STATE			= 0x13


class Fan():
    ## FAN1 OFF bit value
    FAN1_OFF = 0x00

    ## FAN1 ON bit value
    FAN1_ON = 0x01

    ## FAN2 OFF bit value
    FAN2_OFF = 0x00

    ## FAN2 ON bit value
    FAN2_ON = 0x02

    ## FAN3 OFF bit value
    FAN3_OFF = 0x00

    ## FAN3 ON bit value
    FAN3_ON = 0x04

    ## FAN4 OFF bit value
    FAN4_OFF = 0x00

    ## FAN4 ON bit value
    FAN4_ON = 0x08

class HT(HTConstants):
    ##
    # @brief
    #      Constructor
    # ##
    def __init__(self):
        ## namedtyple to store the Module State
        self._HeaterStatus = namedtuple("HeaterStatus", 
            "ht1_state ht1_temp \
            ht2_state ht2_temp \
            coldjunc \
            line_vol_type line_freq")
        self._CharStatus = namedtuple("CharringStatus","chtop_state chtop_available_time chbtm_state chbtm_available_time")

    ##
    # @brief
    #      Get the status and temperature of heaters.
    #
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() and waits for
    #       the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #      The function gets the HT module state and temperature, and returns the namedtuple as below: 
    # @code
    #   self._HeaterStatus = namedtuple("HeaterStatus", 
    #        "ht1_state ht1_temp \
    #        ht2_state ht2_temp \
    #        ht3_state ht3_temp \
    #        ht4_state ht4_temp \
    #        top_coldjunc btm_coldjunc \
    #        line_vol_type line_freq")
    # @endcode
    #   - Note: This is the information applicable for slaveboard POC6 which has 4 heaters. The information inside the namedtupe is 
    #       described as below:
    #       - ht1_state: Heater1 State (#HT_STATE_OFF/#HT_STATE_ON_OPEN_LOOP/#HT_STATE_ON_CLOSE_LOOP/#HT_STATE_ERROR)
    #       - ht1_temp: Heater1 Temperature, Unit is degree Celsius.
    #       - ht2_state: See description of ht1_state
    #       - ht2_temp: Heater2 Temperature, Unit is degree Celsius.
    #       - ht3_state: See description of ht1_state
    #       - ht3_temp: Heater3 Temperature, Unit is degree Celsius.
    #       - ht4_state: See description of ht1_state
    #       - ht4_temp: Heater4 Temperature, Unit is degree Celsius.
    #       - top_coldjunc: Top Cold Junction Temperature, Unit is degree Celsius.
    #       - btm_coldjunc: Bottom Cold Junction Temperature, Unit is degree Celsius.
    #       - line_vol_type: Line Voltage Type
    #           - 0x00 : LINE_220V
    #           - 0x01 : LINE_110V
    #           - 0x02 : LINE_UNDEFINED
    #       - line_freq: Line Frequency, Unit is Hertz
    # @return
    #      @arg namedtuple("HeaterStatus", 
    #        "ht1_state ht1_temp 
    #        ht2_state ht2_temp 
    #        ht3_state ht3_temp 
    #        ht4_state ht4_temp 
    #        top_coldjunc btm_coldjunc 
    #        line_vol_type line_freq")
    #
    # ##
    def get_status(self) -> namedtuple:
        # Create an array to store parsed values
        heaters_state = []
        heaters_temp = []
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.HT_SUBCODE_GET_STATE)
        response = send_command(builder.build(), self.HT_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('HT get_status has no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.HT_SUBCODE_GET_STATE)
        
        _ = dec.decode_1byte_uint() # ACK, not used

        for i in range(2):
            _state = dec.decode_1byte_uint()
            _temp = dec.decode_4bytes_q16()
            heaters_state.append(_state)
            heaters_temp.append(_temp)
        
        coldjunc = dec.decode_4bytes_q16()
        line_type = dec.decode_1byte_uint()
        line_freq = dec.decode_4bytes_q16()
        status = self._HeaterStatus(heaters_state[0], heaters_temp[0],
            heaters_state[1], heaters_temp[1],
            coldjunc,
            line_type, line_freq
            )
        return status

    ##
    # @brief
    #       Clear Error Flags
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() 
    #       and waits for the response from slave; then, parses the response.
    #
    #      The function clears the error flag of HT module. When the previous command to control 
    #      HT module failed, this command should be sent befor any other commands.
    #
    # @param [in]
    #       id: #HT_ID_TOP/#HT_ID_BTM/#HT_ID_CHARTOP/#HT_ID_CHARBTM
    #
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    # ##
    def clear_error_flags(self, id):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.HT_SUBCODE_CLEAR_ERROR)
        builder.add_1byte_uint(id)
        response = send_command(builder.build(), self.HT_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('HT get_status has no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.HT_SUBCODE_CLEAR_ERROR)
        
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
        
        return retVal
    
    ##
    # @brief
    #       Set State
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() 
    #       and waits for the response from slave; then, parses the response.
    #
    #      The function set the state of the heater with referenced temperature, maximum duty and maximum duty temperature
    #       threshold.
    #
    # @param [in]
    #       id: #HT_ID_TOP/#HT_ID_BTM/#HT_ID_CHARTOP/#HT_ID_CHARBTM
    # @param [in]
    #       state: 0: OFF; 1: ON. If OFF, all referenced values are not applicable.
    # @param [in]
    #       refTemp: Reference Temperature
    # @param [in]
    #       refMaxDuty: Reference maximum duty
    # @param [in]
    #       refMaxDutyTempRef: Reference maximum duty temperature threshold.
    #
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    # ##
    def set_state(self, id, state, refTemp, refMaxDuty, refMaxDutyTempRef):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.HT_SUBCODE_SET_STATE)
        builder.add_1byte_uint(id)
        builder.add_1byte_uint(state)
        builder.add_4bytes_float(refTemp)
        builder.add_4bytes_float(refMaxDuty)
        builder.add_4bytes_float(refMaxDutyTempRef)
        response = send_command(builder.build(), self.HT_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('HT get_status has no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.HT_SUBCODE_SET_STATE)
        
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
        
        return retVal

    ##
    # @brief
    #       Set Temperature Reference
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() 
    #       and waits for the response from slave; then, parses the response.
    #
    #      This command set the heater temperature reference.
    #
    # @param [in]
    #       id: #HT_ID_TOP/#HT_ID_BTM/#HT_ID_CHARTOP/#HT_ID_CHARBTM
    # @param [in]
    #       refTemp: Reference Temperature
    # @param [in]
    #       refMaxDuty: Reference maximum duty
    # @param [in]
    #       refMaxDutyTempRef: Reference maximum duty temperature threshold.
    #
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    # ##
    def set_refTemp(self, id, refTemp, refMaxDuty, refMaxDutyTempRef):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.HT_SUBCODE_SET_TEMP_REF)
        builder.add_1byte_uint(id)
        builder.add_4bytes_float(refTemp)
        builder.add_4bytes_float(refMaxDuty)
        builder.add_4bytes_float(refMaxDutyTempRef)
        response = send_command(builder.build(), self.HT_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('HT get_status has no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.HT_SUBCODE_SET_TEMP_REF)
        
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
        
        return retVal
    
    ##
    # @brief
    #       Set PID coefficient (Dev Use Only)
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() 
    #       and waits for the response from slave; then, parses the response.
    #
    #      For Developer use only.  This is for tuning purpose.
    #
    # @param [in]
    #       id: #HT_ID_TOP/#HT_ID_BTM/#HT_ID_CHARTOP/#HT_ID_CHARBTM
    # @param [in]
    #       coefA: Control Coefficient A
    # @param [in]
    #       coefB: Control Coefficient B
    # @param [in]
    #       coefC: Control Coefficient C
    # @param [in]
    #       ctrlOutSaturation: Control Output Saturation Limit
    #
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    # ##
    def set_pid(self, id, coefA, coefB, coefC, ctrlOutSaturation):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.HT_SUBCODE_SET_PID_COEF)
        builder.add_1byte_uint(id)
        builder.add_4bytes_float(coefA)
        builder.add_4bytes_float(coefB)
        builder.add_4bytes_float(coefC)
        builder.add_4bytes_float(ctrlOutSaturation)
        response = send_command(builder.build(), self.HT_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('HT get_status has no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.HT_SUBCODE_SET_PID_COEF)
        
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
        
        return retVal

    ##
    # @brief
    #       Set Over-Temperature Supervisor (Dev Use Only)
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() 
    #       and waits for the response from slave; then, parses the response.
    #
    #      For Developer use only.  This is for tuning purpose.
    #
    # @param [in]
    #       id: #HT_ID_TOP/#HT_ID_BTM/#HT_ID_CHARTOP/#HT_ID_CHARBTM
    # @param [in]
    #       tempThreshold: Over temperature threshold, Unit degree Celsius
    # @param [in]
    #       duration_ms: Duration window, Unit is millisecond
    #
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    # ##
    def set_overTemp(self, id, tempThreshold, duration_ms):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.HT_SUBCODE_SET_SUPERV_OVERTEMP)
        builder.add_1byte_uint(id)
        builder.add_4bytes_float(tempThreshold)
        builder.add_4bytes_uint(duration_ms)
        response = send_command(builder.build(), self.HT_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('HT get_status has no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.HT_SUBCODE_SET_SUPERV_OVERTEMP)
        
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
        
        return retVal

    ##
    # @brief
    #       Set Fan State
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() 
    #       and waits for the response from slave; then, parses the response.
    #
    #      This command is use to set fan state (ON/OFF).
    #
    # @param [in]
    #       fan_mask: If bit is clear, Turn OFF fan. If bit is set, Turn ON fan.
    #            - bit0 : FAN1
    #            - bit1 : FAN2
    #            - bit2 : FAN3
    #            - bit3 : FAN4
    #            - bit4 : FAN5
    #            - bit5:bit7 : unused 
    #
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    # ##
    def set_fan(self, fan_mask):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.HT_SUBCODE_SET_FAN)
        builder.add_1byte_uint(fan_mask)
        response = send_command(builder.build(), self.HT_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('HT set_fan() has no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.HT_SUBCODE_SET_FAN)
        
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
        
        return retVal

    def set_fan_id(self, fan_id, fan_state):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.HT_SUBCODE_SET_FAN_ID)
        builder.add_1byte_uint(fan_id)
        builder.add_1byte_uint(fan_state)
        response = send_command(builder.build(), self.HT_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('HT set_fan_id() has no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.HT_SUBCODE_SET_FAN_ID)
        
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
        
        return retVal

    def set_fan_pwm(self, fan_id, duty):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.HT_SUBCODE_SET_FAN_PWM)
        builder.add_1byte_uint(fan_id)
        builder.add_1byte_uint(duty)
        response = send_command(builder.build(), self.HT_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('HT get_status has no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.HT_SUBCODE_SET_FAN_PWM)
        
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
        
        return retVal

    ##
    # @brief
    #       Less than or equal to
    #
    # @details
    #      This command wait for the heater cooling down to input temperature or time out. The heater temperature is updated 
    #       every #HT_CHECK_TIME
    #
    # @param [in]
    #       id: #HT_ID_TOP/#HT_ID_BTM/#HT_ID_CHARTOP/#HT_ID_CHARBTM
    # @param [in]
    #       temperature: Temperature
    # @param [in]
    #       tout: Time out
    #
    # @return
    #      @arg    CommandRetCode.CMD_RETCODE_NOERR: Temperature cooling down within input time
    #      @arg    CommandRetCode.CMD_RETCODE_GENERR: Input parameter invalid
    #      @arg    CommandRetCode.CMD_RETCODE_TOUTERR: Timeout
    # ##
    async def le(self, id, temperature, tout):
        retVal = CommandRetCode.CMD_RETCODE_NOERR
        status = self.get_status()
        # print(status) # for debugging
        cnt = tout / self.HT_CHECK_TIME
        temp = -1
        if id == self.HT_ID_TOP:
            temp = status.ht1_temp
        elif id == self.HT_ID_BTM:
            temp = status.ht2_temp
        elif id == self.HT_ID_CHARBTM:
            temp = status.ht3_temp
        elif id == self.HT_ID_CHARTOP:
            temp = status.ht4_temp
        else:
            return CommandRetCode.CMD_RETCODE_GENERR
        
        while temp > temperature:
            status = self.get_status()
            # print(status) # for debugging
            if id == self.HT_ID_TOP:
                temp = status.ht1_temp
            elif id == self.HT_ID_BTM:
                temp = status.ht2_temp
            elif id == self.HT_ID_CHARBTM:
                temp = status.ht3_temp
            elif id == self.HT_ID_CHARTOP:
                temp = status.ht4_temp
            else:
                return CommandRetCode.CMD_RETCODE_GENERR
            await uasyncio.sleep_ms(self.HT_CHECK_TIME)
            if cnt == 1:
                return CommandRetCode.CMD_RETCODE_TOUTERR
            cnt -= 1

        return retVal

    ##
    # @brief
    #       Greater than or equal to
    #
    # @details
    #      This command wait for the heater heating up to the input temperature or time out. The heater temperature is updated 
    #       every #HT_CHECK_TIME
    #
    # @param [in]
    #       id: #HT_ID_TOP/#HT_ID_BTM/#HT_ID_CHARTOP/#HT_ID_CHARBTM
    # @param [in]
    #       temperature: Temperature
    # @param [in]
    #       tout: Time out
    #
    # @return
    #      @arg    CommandRetCode.CMD_RETCODE_NOERR: Temperature heating up within input time
    #      @arg    CommandRetCode.CMD_RETCODE_GENERR: Input parameter invalid
    #      @arg    CommandRetCode.CMD_RETCODE_TOUTERR: Timeout
    # ##
    async def ge(self, id, temperature, tout):
        retVal = CommandRetCode.CMD_RETCODE_NOERR
        status = self.get_status()
        # print(status) # for debugging
        cnt = tout / self.HT_CHECK_TIME
        temp = -1
        if id == self.HT_ID_TOP:
            temp = status.ht1_temp
        elif id == self.HT_ID_BTM:
            temp = status.ht2_temp
        elif id == self.HT_ID_CHARBTM:
            temp = status.ht3_temp
        elif id == self.HT_ID_CHARTOP:
            temp = status.ht4_temp
        else:
            return CommandRetCode.CMD_RETCODE_GENERR
        
        while temp < temperature:
            status = self.get_status()
            if id == self.HT_ID_TOP:
                temp = status.ht1_temp
            elif id == self.HT_ID_BTM:
                temp = status.ht2_temp
            elif id == self.HT_ID_CHARBTM:
                temp = status.ht3_temp
            elif id == self.HT_ID_CHARTOP:
                temp = status.ht4_temp
            else:
                return CommandRetCode.CMD_RETCODE_GENERR
            await uasyncio.sleep_ms(self.HT_CHECK_TIME)
            if cnt == 1:
                return CommandRetCode.CMD_RETCODE_TOUTERR
            cnt -= 1

        return retVal

    def temp_eq(self, cur_temp, expect_temp, tol):
        return True if (cur_temp > expect_temp - tol) and (cur_temp < expect_temp + tol) else False

    async def eq(self, top_temp_exp, bot_temp_exp, tol, tout):
        retVal = CommandRetCode.CMD_RETCODE_NOERR
        status = self.get_status()
        # print(status) # for debugging
        cnt = tout / self.HT_CHECK_TIME
        _top_temp = status.ht1_temp
        _bot_temp = status.ht2_temp

        s = "{} {} {} {} {} {}".format(_top_temp, _bot_temp, top_temp_exp, bot_temp_exp, tol, tout)
        logger.debug({'debug':'{0}'}, s)
        while (self.temp_eq(_top_temp, top_temp_exp, tol) and self.temp_eq(_bot_temp, bot_temp_exp, tol)) != True:
            status = self.get_status()
            _top_temp = status.ht1_temp
            _bot_temp = status.ht2_temp
            s = "{} {} {} {}".format(_top_temp, _bot_temp, top_temp_exp, bot_temp_exp)
            logger.debug({'debug':'{0}'}, s)
            
            await uasyncio.sleep_ms(self.HT_CHECK_TIME)
            if cnt == 1:
                return CommandRetCode.CMD_RETCODE_TOUTERR
            cnt -= 1

        return retVal

    def char_get_state(self):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.REQ20_CHAR_GET_STATE)
        response = send_command(builder.build(), self.HT_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('HT char_get_state has no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.REQ20_CHAR_GET_STATE)
        
        _ =  dec.decode_1byte_uint() # ACK is not used
        
        status = self._CharStatus(dec.decode_1byte_uint(), dec.decode_4bytes_uint_time(), dec.decode_1byte_uint(), dec.decode_4bytes_uint_time())
        return status

    def char_set_state_on(self, id, on_time):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.REQ20_CHAR_SET_ON_STATE)
        builder.add_1byte_uint(id)
        builder.add_2bytes_uint(on_time)
        response = send_command(builder.build(), self.HT_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('HT char_set_state_on has no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.REQ20_CHAR_SET_ON_STATE)
        
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal

    def char_set_state_off(self, id):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.REQ20_CHAR_SET_OFF_STATE)
        builder.add_1byte_uint(id)
        response = send_command(builder.build(), self.HT_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('HT char_set_state_off has no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.REQ20_CHAR_SET_OFF_STATE)
        
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal

    def char_set_state_safe(self):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.HT_REQ_CODE)
        builder.add_1byte_uint(self.REQ20_CHAR_SET_SAFE_STATE)
        response = send_command(builder.build(), self.HT_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('HT char_set_state_safe has no response')

        dec = CommandDecoder(response, self.HT_REQ_CODE, self.REQ20_CHAR_SET_SAFE_STATE)
        
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal