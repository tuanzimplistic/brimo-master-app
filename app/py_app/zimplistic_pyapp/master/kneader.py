"""
Kneader Class
"""

from collections import namedtuple
import uasyncio
from zimplistic_pyapp.app.log import *
from zimplistic_pyapp import *
from zimplistic_pyapp.master.itor3command import *

class KNConstants():
    ## Kneader Request Code.
    KNEADER_REQ_CODE    = 0x24

    ## Kneader Module ID. This ID is used to get the corresponding state memory buffer.
    MAL_KN_ID           = 0x06

    ## Get Status Subcode.
    GET_STATUS = 0x00

    ## Clear Error Subcode.
    CLEAR_ERR_FLAGS = 0x01

    ## Run Subcode.
    RUN = 0x02

    ## Stop Subcode.
    STOP = 0x03

    ## Set Dispense Position Subcode.
    SET_POS = 0x04

    ## Set Dispense Position with Speed Subcode.
    DS_POS_SPD = 0x05

    ## Eject Subcode, used for fishing function.
    EJECT = 0x06

    ## Get encoder counter
    GET_ENCODER_CNT = 0x07

    ## Command ACK
    ACK = 0

    ## Command NACK
    NACK = 1

    ## IDLE State
    IDLE_STATE = 0x00

    ## Busy State
    BUSY_STATE = 0x01

    ## FLOUR ID
    FLOUR_ID = 0x00

    ## WATER ID
    WATER_ID = 0x01

    ## OIL ID
    OIL_ID = 0x02

    ## Command Sending time out
    KNEADER_COMMAND_TOUT = 500

    ## Kneader Command Execution time out; maximum wait time for a command is 1 minute
    KNEADER_CMD_EXETOUT = 60000

    ## Command check period
    KNEADER_CMD_CHECK_TIME = 100

    ## Dispense Position Error flag
    KN_BIT_DISPENSE_POS_ERR = 0x01

    ## Motor Error flag
    KN_BIT_MOTOR_ERR        = 0x02

    ## Dislodge Posion Error flag
    KN_BIT_DISLODGE_POS_ERR = 0x04

    ## TimeOut Error flag
    KN_BIT_TIMEOUT_ERR      = 0x08

    ## Kneader Run Error flag
    KN_BIT_RUN_ERR          = 0x10

    ## Index is known flag
    KN_BIT_INDEX_KNOWN      = 0x20

    ## All bit sets
    KN_BIT_ALL_ERRS         = (KN_BIT_DISPENSE_POS_ERR | KN_BIT_MOTOR_ERR \
                                             | KN_BIT_DISLODGE_POS_ERR | KN_BIT_TIMEOUT_ERR \
                                             | KN_BIT_RUN_ERR)

class KN(KNConstants, Itor3Command):
    ##
    # @brief
    #      Constructor
    # ##
    def __init__(self):
        ## namedtyple to store the Module State
        self._KneaderStatus = namedtuple("KneaderStatus", "state flags pos current")
        Itor3Command.__init__(self, self.KNEADER_CMD_EXETOUT, self.KNEADER_CMD_CHECK_TIME)

    ##
    # @brief
    #      Get the status of Kneader module.
    #
    #
    # @details
    #      This function read the memory buffer in C layer and parse the informations
    #    (state, flag, current) and return the namedtuple type.
    #
    #
    # @return
    #      @arg    namedtuple: ("KneaderStatus", "state flags current")
    # ##
    def get_status(self):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.KNEADER_REQ_CODE)
        builder.add_1byte_uint(self.GET_STATUS)
        response = send_command(builder.build(), self.KNEADER_COMMAND_TOUT)
        
        if not response:
            raise ValueError('KN get_status has no response')

        dec = CommandDecoder(response, self.KNEADER_REQ_CODE, self.GET_STATUS)

        _ = dec.decode_1byte_uint() # ACK, not used

        self._state = dec.decode_1byte_uint()
        self._flags = dec.decode_1byte_uint()
        self._pos = dec.decode_4bytes_q16()
        self._current = dec.decode_4bytes_q16()
        return self._KneaderStatus(self._state, self._flags, self._pos, self._current)

    def get_motor_encoder(self):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.KNEADER_REQ_CODE)
        builder.add_1byte_uint(self.GET_ENCODER_CNT)
        response = send_command(builder.build(), self.KNEADER_COMMAND_TOUT)
        
        if not response:
            raise ValueError('KN get_motor_encoder has no response')

        dec = CommandDecoder(response, self.KNEADER_REQ_CODE, self.GET_ENCODER_CNT)

        _ = dec.decode_1byte_uint() # ACK, not used

        return dec.decode_1byte_uint(), round(dec.decode_4bytes_q16()), dec.decode_4bytes_q16()

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

    ##
    # @brief
    #      Start the kneader.
    #
    # @details
    #      This function creates a sending buffer and sent it to the modbus.
    #
    # @param [in]
    #       speed: Speed to run the kneader (rad/s)
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    # ##
    def run(self, speed):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.KNEADER_REQ_CODE)
        builder.add_1byte_uint(self.RUN)
        builder.add_4bytes_float(speed)
        response = send_command(builder.build(), self.KNEADER_COMMAND_TOUT)
        # print(response) # For debugging
        if not response:
            raise ValueError("KN run command has no response")
        
        ## Check that the response command is correct
        dec = CommandDecoder(response, self.KNEADER_REQ_CODE, self.RUN)
        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal

    ##
    # @brief
    #      Stop the kneader.
    #
    # @details
    #      This function creates a sending buffer and sent it to the modbus.
    #
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    # ##
    def stop(self):
        retVal = True
        buider = CommandBuilder()
        buider.add_1byte_uint(self.KNEADER_REQ_CODE)
        buider.add_1byte_uint(self.STOP)
        response = send_command(buider.build(), self.KNEADER_COMMAND_TOUT)
        # print(response) #for debugging
        if not response:
            raise ValueError("KN stop command has no response")
        
        ## Check that the response command is correct
        dec = CommandDecoder(response, self.KNEADER_REQ_CODE, self.STOP)
        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal

    ##
    # @brief
    #      Set the kneader to the set dispensing position (FLOUR, WATER, OIL)
    #
    # @details
    #      This function creates a sending buffer and sent it to the modbus.
    #      The input ID should be set position. Set positions are as below:
    #   - #FLOUR_ID: Set the kneader to Flour dispense positon
    #   - #WATER_ID: Set the kneader to Water dispense positon
    #   - #OIL_ID: Set the kneader to Oil dispense position
    #
    #@code
    #       # Code example to set the kneader to flour postion
    #       kn = Kneader()
    #       kn.set_dis_pos(KNConstants.FLOUR_ID)
    # @endcode
    #
    # @param [in]
    #       id: dispense ID
    #
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    #
    # ##
    def set_dis_pos(self, id):
        retVal = True
        buider = CommandBuilder()
        buider.add_1byte_uint(self.KNEADER_REQ_CODE)
        buider.add_1byte_uint(self.SET_POS)
        buider.add_1byte_uint(id)
        response = send_command(buider.build(), self.KNEADER_COMMAND_TOUT)
        if not response:
            raise ValueError("KN set_dis_pos command has no response")
        # print(response) #for debugging
        ## Check that the response command is correct
        dec = CommandDecoder(response, self.KNEADER_REQ_CODE, self.SET_POS)
        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
            self._curState = self.IDLE_STATE
        else:
            self._curState = self.BUSY_STATE

        return retVal

    ##
    # @brief
    #      Set the kneader to any position with input speed.
    #
    # @details
    #      This function creates a sending buffer and sent it to the modbus.
    #      The unit of input position is rad counted from the flag. The
    #      unit of input speed is rad/s.
    # @code
    #       kn = Kneader()
    #       if kn.set_pos_spd(0.3, 3):
    #           print("Set kneader position to WATER successful")
    # @endcode
    # @param [in]
    #       pos: position (rad)
    #
    # @param [in]
    #       spd: speed (rad/s)
    #
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    #
    # ##
    def set_pos_spd(self, pos, spd):
        retVal = True
        buider = CommandBuilder()
        buider.add_1byte_uint(self.KNEADER_REQ_CODE)
        buider.add_1byte_uint(self.DS_POS_SPD)
        buider.add_4bytes_float(pos)
        buider.add_4bytes_float(spd)
        response = send_command(buider.build(), self.KNEADER_COMMAND_TOUT)

        if not response:
            raise ValueError("KN set_dis_pos command has no response")
        
        ## Check that the response command is correct
        dec = CommandDecoder(response, self.KNEADER_REQ_CODE, self.DS_POS_SPD)
        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal

    ##
    # @brief
    #      Set Stirrer State
    #
    # @details
    #      This function sets stirrer state to eject or fishing stirrer.
    #
    # @param [in]
    #       state: state of stirrer, 0: eject, 1: fishing
    #
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    #
    # ##
    def set_stirrer_state(self, state):
        retVal = True
        buider = CommandBuilder()
        buider.add_1byte_uint(self.KNEADER_REQ_CODE)
        buider.add_1byte_uint(self.EJECT)
        buider.add_1byte_uint(state)
        response = send_command(buider.build(), self.KNEADER_COMMAND_TOUT)

        if not response:
            raise ValueError("KN set_dis_pos command has no response")
        
        ## Check that the response command is correct
        dec = CommandDecoder(response, self.KNEADER_REQ_CODE, self.EJECT)
        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal

    async def wait(self):
        retVal = await super().wait()
        if retVal != CommandRetCode.CMD_RETCODE_NOERR:
            #print(self._KneaderStatus(self._state, self._flags, self._pos, self._current))
            logger.debug({'kneader status ':'{0}'}, self._KneaderStatus(self._state, self._flags, self._pos, self._current))

        # Currently applied for set_dis_pos only
        if self._flags & self.KN_BIT_DISPENSE_POS_ERR:
            #print(self._KneaderStatus(self._state, self._flags, self._pos, self._current))
            logger.debug({'kneader status ':'{0}'}, self._KneaderStatus(self._state, self._flags, self._pos, self._current))
            retVal = CommandRetCode.CMD_RETCODE_EXEERR

        return retVal