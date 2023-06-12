"""
Kicker (KR) Class
"""

from collections import namedtuple
import uasyncio
from itor3_pyapp.app.log import *
from itor3_pyapp import *
from itor3_pyapp.master.itor3command import *

class KRConstants():
    ## Kicker Request Code.
    KR_REQ_CODE    = 0x25

    ## Command Time out
    KR_COMMAND_TOUT = 500

    ## GET_STATE subcode
    KR_SUBCODE_GET_STATE = 0x00

    ## CLEAR_ERROR_FLAG subcode
    KR_SUBCODE_CLEAR_ERROR_FLAG   = 0x01
    
    ## FIND_DATUM subcode
    KR_SUBCODE_FIND_DATUM         = 0x02

    ## MOVE subcode
    KR_SUBCODE_MOVE               = 0x03

    ## Move error bit
    KR_BIT_MOVE_ERR        = 0x01

    ## Find datum error bit
    KR_BIT_FIND_DATUM_ERR  = 0x02

    ## Motor error bit
    KR_BIT_MOTOR_ERR       = 0x04

    ## Find Limit error bit
    KR_BIT_FIND_LIMIT_ERR  = 0x08

    ## Datum is known bit
    KR_BIT_DATUM_KNOWN     = 0x10

    ## IDLE State
    KR_STATE_IDLE               = 0x00

    ## MOVING State
    KR_STATE_MOVING             = 0x01

    ## FINDING DATUM State
    KR_STATE_FINDING_DATUM      = 0x02

    ## FINDING LIMIT State
    KR_STATE_FINDING_LIMIT      = 0x03

    ## ACK
    ACK = 0x00

    ## NACK
    NACK = 0x01

    ## Kicker Command execution timeout; maximum execution time of any command is 3 minutes\
    KR_CMD_EXE_TOUT     = 180000

    ## KR command check period
    KR_CMD_CHECK_TIME = 100

class KR(KRConstants, Itor3Command):
    ##
    # @brief
    #      Constructor
    # ##
    def __init__(self):
        ## namedtyple to store the Module State
        self._KickerStatus = namedtuple("KickerStatus", "state flags pos current home_state")
        Itor3Command.__init__(self, self.KR_CMD_EXE_TOUT, self.KR_CMD_CHECK_TIME)

    ##
    # @brief
    #      Get the status of KR module.
    #
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() to get the KR module status and waits for
    #       the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #
    # @return
    #      @arg    namedtuple: ("KickerStatus", "state flags pos current home_state")
    # ##
    def get_status(self):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.KR_REQ_CODE)
        builder.add_1byte_uint(self.KR_SUBCODE_GET_STATE)
        response = send_command(builder.build(), self.KR_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('KR get_status has no response')

        dec = CommandDecoder(response, self.KR_REQ_CODE, self.KR_SUBCODE_GET_STATE)

        _ = dec.decode_1byte_uint() # ACK, not used

        self._state = dec.decode_1byte_uint() 
        self._flags = dec.decode_1byte_uint()
        self._pos = dec.decode_4bytes_q16()
        self._current = dec.decode_4bytes_q16()
        self._home_state = dec.decode_1byte_uint()
        status = self._KickerStatus(self._state, self._flags, self._pos, self._current, self._home_state)
        return status

    ##
    # @brief
    #       Clear Error Flags
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() 
    #       and waits for the response from slave; then, parses the response.
    #
    #      The function clears the error flag of KR module. When the previous command to control 
    #      KR module failed, this command should be sent befor any other commands.
    #
    #
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    # ##
    def clear_error_flags(self):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.KR_REQ_CODE)
        builder.add_1byte_uint(self.KR_SUBCODE_CLEAR_ERROR_FLAG)
        response = send_command(builder.build(), self.KR_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('KR get_status has no response')

        dec = CommandDecoder(response, self.KR_REQ_CODE, self.KR_SUBCODE_CLEAR_ERROR_FLAG)

        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal

    ##
    # @brief
    #      Move the KR
    #
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() 
    #       to move the KR and waits for the response from slave; then, parses the response. find_home() function
    #       needs to be done successfully befer calling this function.
    #
    # @param [in]
    #       is_rel: True if relative move; otherwise absoblute move.
    # @param [in]
    #       pos: position (mm)
    # @param [in]
    #       spd: speed (mm/s)
    #
    # @return
    #      @arg    True: if the command is sent successfully
    # @return
    #      @arg    False: if the command is sent failed
    # ##
    def move(self, is_rel, pos, spd):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.KR_REQ_CODE)
        builder.add_1byte_uint(self.KR_SUBCODE_MOVE)
        builder.add_1byte_bool(is_rel)
        builder.add_4bytes_float(pos)
        builder.add_4bytes_float(spd)
        response = send_command(builder.build(), self.KR_COMMAND_TOUT)
        
        if not response:
            raise ValueError('KR move has no response')

        dec = CommandDecoder(response, self.KR_REQ_CODE, self.KR_SUBCODE_MOVE)

        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
            self._curState = self.KR_STATE_IDLE
        else:
            self._curState = self.KR_STATE_MOVING

        return retVal

    ##
    # @brief
    #      Find home
    #
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() and waits for
    #       the response from slave; then, parses the response.
    #   
    #      The function finds home position when the kicker hit the KR's switch on the back. At the time the switch is hit,
    #       the KR encoder is reset and this position is condered as "home" position.
    #
    # @return
    #      @arg    True: if the command is sent successfully
    # @return
    #      @arg    False: if the command is sent failed
    # ##
    def find_home(self):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.KR_REQ_CODE)
        builder.add_1byte_uint(self.KR_SUBCODE_FIND_DATUM)
        response = send_command(builder.build(), self.KR_COMMAND_TOUT)
        
        if not response:
            raise ValueError('KR move has no response')

        dec = CommandDecoder(response, self.KR_REQ_CODE, self.KR_SUBCODE_FIND_DATUM)

        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal

    async def wait(self):
        retVal = await super().wait()
        logger.debug({'kicker status ':'{0}'}, self._KickerStatus(self._state, self._flags, self._pos, self._current, self._home_state)) # RM2-2065
        if retVal != CommandRetCode.CMD_RETCODE_NOERR:
            # logger.debug({'kicker status ':'{0}'}, self._KickerStatus(self._state, self._flags, self._pos, self._current, self._home_state))
            #print(self._KickerStatus(self._state, self._flags, self._pos, self._current, self._home_state)) # For debugging
            return retVal
        
        ## Check Flag
        if self._flags & self.KR_BIT_MOVE_ERR:
            retVal = CommandRetCode.CMD_RETCODE_EXEERR
            #print(self._KickerStatus(self._state, self._flags, self._pos, self._current, self._home_state)) # For debugging
            # logger.debug({'kicker status ':'{0}'}, self._KickerStatus(self._state, self._flags, self._pos, self._current, self._home_state))
        return retVal