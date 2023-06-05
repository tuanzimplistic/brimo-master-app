"""
Wedge & Press (WP) Class
"""

from collections import namedtuple

from itor3_pyapp import *
from itor3_pyapp.master.itor3command import *
from itor3_pyapp.app.log import *

class WPConstants():
    ## WP Request Code
    WP_REQ_CODE = 0x23

    ## Command timeout in ms
    WP_COMMAND_TOUT = 500

    ## Get state subcode
    WP_SUBCODE_GET_STATE              = 0x00

    ## Clear error flag subcode
    WP_SUBCODE_CLEAR_ERROR_FLAG       = 0x01

    ## Find datum subcode
    WP_SUBCODE_FIND_DATUM             = 0x02

    ## Absolute linear move subcode
    WP_SUBCODE_MOVE                   = 0x03

    ## Angular move subcode
    WP_SUBCODE_ANGLE_MOVE             = 0x04

    WP_CALIBRATE_FLAG                 = 0x05
    WP_GET_FLAG_DIM                   = 0x06
    WP_STORE_NVS                      = 0x07

    
    ## Calibrate flag
    WP_SUBCODE_CALIBRATE              = 0x05

    ## Get Flag/Slot Dimension
    WP_SUBCODE_GET_DIMESION           = 0x06

    ## Get Save Flag Dimension into NVS 
    WP_SUBCODE_SAVE_DIMESION          = 0x07
    
    ## Absolute linear move press subcode
    WP_SUBCODE_MOVE_PRESS             = 0x08

    ## Absolute linear move press subcode
    WP_SUBCODE_MOVE_PIVOT             = 0x09

    ## Move error bit flag
    WP_BIT_MOVE_ERR        = 0x01

    ## Find datum error bit flag
    WP_BIT_FIND_DATUM_ERR  = 0x02

    ## Press link error bit flag
    WP_BIT_PRESS_LINK_ERR  = 0x04

    ## Pivot link error bit flag
    WP_BIT_PIVOT_LINK_ERR  = 0x08

    ## Press datum is known bit flag
    WP_BIT_PRESS_DATUM_KNOWN   = 0x10

    ## Pivot datum is known bit flag
    WP_BIT_PIVOT_DATUM_KNOWN   = 0x20

    ## Idle state
    WP_STATE_IDLE = 0x00

    ## Moving state
    WP_STATE_MOVING = 0x01

    ## Finding datum state
    WP_STATE_FINDING_DATUM = 0x02

    WP_STATE_CALIBRATING_FLAG = 0x03

    ## ACK
    ACK = 0x00

    ## NACK
    NACK = 0x01

    ## WP Command execution timeout; maximum execution time of any command is 3 minutes
    WP_CMD_EXE_TOUT     = 180000

    ## WP Command check period
    WP_CMD_CHECK_TIME = 100

class WP(WPConstants, Itor3Command):
    ##
    # @brief
    #      Constructor
    # ##
    def __init__(self):
        ## namedtyple to store the Module State
        self._WedgePressStatus = namedtuple("WedgePressStatus", "state flags press_gap pivot_gap press_current pivot_current")
        Itor3Command.__init__(self, self.WP_CMD_EXE_TOUT, self.WP_CMD_CHECK_TIME)

        self._press_pos = None
        self._pivot_pos = None
        self._press_cur = None
        self._pivot_cur = None

    ##
    # @brief
    #      Get the status of WP module.
    #
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() and waits for
    #       the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #      The function gets the WP module state and return the namedtuple: ("WedgePressStatus", "state flags press_gap pivot_gap press_current pivot_current")
    #       where press and pivot position is in mm; press and pivot motor current is in Ampere.
    # @return
    #      @arg    namedtuple: ("WedgePressStatus", "state flags press_gap pivot_gap press_current pivot_current") 
    #
    # ##
    def get_status(self) -> namedtuple:
        builder = CommandBuilder()
        builder.add_1byte_uint(self.WP_REQ_CODE)
        builder.add_1byte_uint(self.WP_SUBCODE_GET_STATE)
        response = send_command(builder.build(), self.WP_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('WP get_status has no response')

        dec = CommandDecoder(response, self.WP_REQ_CODE, self.WP_SUBCODE_GET_STATE)
        
        _ = dec.decode_1byte_uint() # ACK, not used

        self._state = dec.decode_1byte_uint()
        self._flags = dec.decode_1byte_uint()
        self._press_pos = dec.decode_4bytes_q16()
        self._pivot_pos = dec.decode_4bytes_q16()
        self._press_cur = dec.decode_4bytes_q16()
        self._pivot_cur = dec.decode_4bytes_q16()
        status = self._WedgePressStatus(self._state, self._flags, self._press_pos, self._pivot_pos, self._press_cur, self._pivot_cur)
        return status

    ##
    # @brief
    #      Clear error flags.
    #
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() and waits for
    #       the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #      The function clear error flags of WP module. This command needs to be sent before any command if the previous 
    #       command has any error.
    #
    # @return
    #      @arg    True: if the command is sent successfully.
    # @return
    #      @arg    False: if the command is sent failed.
    #
    # ##
    def clear_error_flags(self) -> bool:
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.WP_REQ_CODE)
        builder.add_1byte_uint(self.WP_SUBCODE_CLEAR_ERROR_FLAG)
        response = send_command(builder.build(), self.WP_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('WP get_status has no response')

        dec = CommandDecoder(response, self.WP_REQ_CODE, self.WP_SUBCODE_CLEAR_ERROR_FLAG)
        
        ## Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal

    ##
    # @brief
    #      Find datum.
    #
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() and waits for
    #       the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #      The function find the WP Press Linkage and/or Pivot Linkage position with respect to a datum.
    #
    # @param [in]
    #       flag: 0x00: find datum on Press Linkage; 0x01: find datum on Pivot Linkage; 0x03: Simultaneous motion on finding datum of both Linkage;
    #           other values are invalid.
    #
    # @return
    #      @arg    True: if the command is sent successfully.
    # @return
    #      @arg    False: if the command is sent failed.
    #
    # ##
    def find_datum(self, flag) -> bool:
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.WP_REQ_CODE)
        builder.add_1byte_uint(self.WP_SUBCODE_FIND_DATUM)
        builder.add_1byte_uint(flag)
        response = send_command(builder.build(), self.WP_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('WP get_status has no response')

        dec = CommandDecoder(response, self.WP_REQ_CODE, self.WP_SUBCODE_FIND_DATUM)
        
        ## Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
            self._curState = self.WP_STATE_IDLE
        else:
            self._curState = self.WP_STATE_FINDING_DATUM

        return retVal

    def calibrate_flag(self) -> bool:
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.WP_REQ_CODE)
        builder.add_1byte_uint(self.WP_SUBCODE_CALIBRATE)
        response = send_command(builder.build(), self.WP_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('WP calibrate_flag has no response')

        dec = CommandDecoder(response, self.WP_REQ_CODE, self.WP_SUBCODE_CALIBRATE)
        
        ## Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
            self._curState = self.WP_STATE_IDLE
        else:
            self._curState = self.WP_STATE_CALIBRATING_FLAG

        return retVal

    def get_flag_dim(self, flag):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.WP_REQ_CODE)
        builder.add_1byte_uint(self.WP_SUBCODE_GET_DIMESION)
        builder.add_1byte_uint(flag)
        response = send_command(builder.build(), self.WP_COMMAND_TOUT)

        if not response:
            raise ValueError('VT vt_calibrate_flag has no response')
        
        # Check response
        dec = CommandDecoder(response, self.WP_REQ_CODE, self.WP_SUBCODE_GET_DIMESION)
        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
        return retVal, dec.decode_1byte_uint(), \
            dec.decode_4bytes_q16(), dec.decode_4bytes_q16(),  \
            dec.decode_4bytes_q16(), dec.decode_4bytes_q16(),  \
            dec.decode_4bytes_q16(), dec.decode_4bytes_q16(),  \
            dec.decode_4bytes_q16(), dec.decode_4bytes_q16(),  \
            dec.decode_4bytes_q16(), dec.decode_4bytes_q16(),  \
            dec.decode_4bytes_q16(), dec.decode_4bytes_q16(),  \
            dec.decode_4bytes_q16(), dec.decode_4bytes_q16()

    ##
    # @brief
    #      Absolute move.
    #
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() and waits for
    #       the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #      The function moves the WP Press Linkage and/or Pivot Linkage to an absolute.
    #       Prerequisite: Datum of Linkage is already known (function find_datum() has been done successfully).
    #
    # @param [in]
    #       press_pos: Press Linkage End effector position in millimeter.
    # @param [in]
    #       pivot_pos: Pivot Linkage End effector position in millimeter.
    # @param [in]
    #       desire_time: Time to perform move in millisecond.
    #
    # @return
    #      @arg    True: if the command is sent successfully.
    # @return
    #      @arg    False: if the command is sent failed.
    #
    # ##
    def move_absolute(self, press_pos, pivot_pos, desire_time) -> bool:
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.WP_REQ_CODE)
        builder.add_1byte_uint(self.WP_SUBCODE_MOVE)
        builder.add_4bytes_float(press_pos)
        builder.add_4bytes_float(pivot_pos)
        builder.add_2bytes_uint(desire_time)
        response = send_command(builder.build(), self.WP_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('WP move_absolute has no response')

        dec = CommandDecoder(response, self.WP_REQ_CODE, self.WP_SUBCODE_MOVE)
        
        ## Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
            self._curState = self.WP_STATE_IDLE
        else:
            self._curState = self.WP_STATE_MOVING

        return retVal

    def move_press(self, press_pos, desire_time) -> bool:
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.WP_REQ_CODE)
        builder.add_1byte_uint(self.WP_SUBCODE_MOVE_PRESS)
        builder.add_4bytes_float(press_pos)
        builder.add_2bytes_uint(desire_time)
        response = send_command(builder.build(), self.WP_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('WP move_press has no response')

        dec = CommandDecoder(response, self.WP_REQ_CODE, self.WP_SUBCODE_MOVE_PRESS)
        
        ## Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
            self._curState = self.WP_STATE_IDLE
        else:
            self._curState = self.WP_STATE_MOVING

        return retVal

    def move_wedge(self, wedge_pos, desire_time) -> bool:
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.WP_REQ_CODE)
        builder.add_1byte_uint(self.WP_SUBCODE_MOVE_PIVOT)
        builder.add_4bytes_float(wedge_pos)
        builder.add_2bytes_uint(desire_time)
        response = send_command(builder.build(), self.WP_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('WP move_press has no response')

        dec = CommandDecoder(response, self.WP_REQ_CODE, self.WP_SUBCODE_MOVE_PIVOT)
        
        ## Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
            self._curState = self.WP_STATE_IDLE
        else:
            self._curState = self.WP_STATE_MOVING

        return retVal

    ##
    # @brief
    #      Angular move.
    #
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() and waits for
    #       the response from slave. Then, the response is parsed and stored into a namedtuple.
    #
    #      The function moves the WP Press Linkage and/or Pivot Linkage to an absolute or relative angle.
    #
    # @param [in]
    #       flag: 0 if absolute angle; 1 if relative angle.
    # @param [in]
    #       press_angle: Press Linkage angle in radian.
    # @param [in]
    #       pivot_angle: Pivot Linkage angle in radian.
    # @param [in]
    #       speed: Linkage rotation speed in radian/second.
    #
    # @return
    #      @arg    True: if the command is sent successfully.
    # @return
    #      @arg    False: if the command is sent failed.
    #
    # ##
    def move_angular(self, flag, press_angle, pivot_angle, speed) -> bool:
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.WP_REQ_CODE)
        builder.add_1byte_uint(self.WP_SUBCODE_ANGLE_MOVE)
        builder.add_1byte_uint(flag)
        builder.add_4bytes_float(press_angle)
        builder.add_4bytes_float(pivot_angle)
        builder.add_4bytes_float(speed)
        response = send_command(builder.build(), self.WP_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('WP get_status has no response')

        dec = CommandDecoder(response, self.WP_REQ_CODE, self.WP_SUBCODE_ANGLE_MOVE)
        
        ## Get ACK
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False

        return retVal

    async def wait(self):
        retVal = await super().wait()
        logger.debug({'WP status':'{0}'}, self._WedgePressStatus(self._state, self._flags, self._press_pos, self._pivot_pos, self._press_cur, self._pivot_cur)) # RM2-2065
        if retVal != CommandRetCode.CMD_RETCODE_NOERR:
            #print(self._WedgePressStatus(self._state, self._flags, self._press_pos, self._pivot_pos, self._press_cur, self._pivot_cur))
            # logger.debug({'WP status':'{0}'}, self._WedgePressStatus(self._state, self._flags, self._press_pos, self._pivot_pos, self._press_cur, self._pivot_cur))
            return retVal

        if self._flags & self.WP_BIT_MOVE_ERR:
            #print(self._WedgePressStatus(self._state, self._flags, self._press_pos, self._pivot_pos, self._press_cur, self._pivot_cur))
            # logger.debug({'WP status':'{0}'}, self._WedgePressStatus(self._state, self._flags, self._press_pos, self._pivot_pos, self._press_cur, self._pivot_cur))
            retVal = CommandRetCode.CMD_RETCODE_EXEERR

        return retVal