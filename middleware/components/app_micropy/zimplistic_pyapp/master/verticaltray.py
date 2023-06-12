"""
verticaltray
"""

from collections import namedtuple
from itor3_pyapp import *
from itor3_pyapp.master.itor3command import *
from itor3_pyapp.app.log import *
import time
import uasyncio

##
# @brief
#      VT Constant Class
# ##
class VTConstants():
    ## VT Request Code
    VT_REQ_CODE    = 0x22

    ## VT Module ID. This ID is used to get the corresponding state memory buffer.
    MAL_VT_ID           = 0x04

    ## Get state subcode
    VT_GET_STATE          = 0x00

    ## Clear error subcode
    VT_CLEAR_ERROR_FLAG   = 0x01

    ## Find datum subcode
    VT_FIND_DATUM         = 0x02

    ## Move subcode
    VT_MOVE               = 0x03

    ## Find top limit subcode
    VT_FIND_TOP_LIMIT     = 0x04

    ## Find top limit subcode
    VT_GET_TOP_LIMIT     = 0x05

    VT_CALIBRATE_FLAG   = 0x06

    VT_GET_FLAG_DIM = 0x07

    VT_STORE_NVS = 0x08

    ## IDLE state
    VT_IDLE_STATE       = 0x00

    ## Moving state
    VT_MOVING_STATE       = 0x01

    ## Finding datum state
    VT_FINDING_DATUM_STATE       = 0x02

    ## Finding top limit state
    VT_FINDING_TOP_STATE       = 0x03

    ## Calibrate flag state
    VT_CALIBRATE_STATE       = 0x04

    ## Error Flag Bit
    VT_MOVE_ERR_BIT        = 0x01

    ## Find datum Flag Bit
    VT_FIND_DATUM_ERR_BIT  = 0x02

    ## Motor Error Flag Bit
    VT_MOTOR_ERR_BIT       = 0x04

    ## VT Overload Error Flag Bit
    VT_OVERLOAD_ERR_BIT    = 0x08

    ## Datum is known flag bit
    VT_DATUM_KNOWN         = 0x10

    ## Top limit is known flag bit
    VT_LIMIT_KNOWN         = 0x20

    ## VT Command ack response timeout
    VT_COMMAND_TOUT        = 500

    ## VT Command execution timeout; maximum execution time of any command is 3 minutes
    VT_CMD_EXE_TOUT     = 180000

    ## VT Command check period
    VT_CMD_CHECK_TIME = 100

    ## Command ACK
    ACK = 0

    ## Command NACK
    NACK = 1

##
# @brief
#      VT Class
# ##
class VT(VTConstants, Itor3Command):
    ##
    # @brief
    #      Constructor
    # ##
    def __init__(self):
        ## namedtyple to store the Module State
        self._VTStatus = namedtuple("VTStatus", "state flags pos current")
        Itor3Command.__init__(self, self.VT_CMD_EXE_TOUT, self.VT_CMD_CHECK_TIME)

    ##
    # @brief
    #      Get the status of the VT module.
    #
    #
    # @details
    #      This function read the memory buffer in C layer and parse the informations
    #    (state, flag, position, current) and return the namedtuple type.
    #
    #
    # @return
    #      @arg    namedtuple: ("VTStatus", "state flags pos current")
    # ##
    def get_status(self):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.VT_REQ_CODE)
        builder.add_1byte_uint(self.VT_GET_STATE)
        response = send_command(builder.build(), self.VT_COMMAND_TOUT)
        
        ## Check response
        if not response:
            raise ValueError('VT get_status has no response')
        
        dec = CommandDecoder(response, self.VT_REQ_CODE, self.VT_GET_STATE)

        _ = dec.decode_1byte_uint() # ACK, not used

        self._state = dec.decode_1byte_uint()
        self._flags = dec.decode_1byte_uint()
        self._pos = dec.decode_4bytes_q16()
        self._current = dec.decode_4bytes_q16()
        status = self._VTStatus(self._state, self._flags, self._pos, self._current)
        return status

    ##
    # @brief
    #      Find datum
    #
    #
    # @details
    #      This function find datum (positions) based on flag patterns on the VT gear. The VT will stop at 
    #   positions depends on the starting position. If the function is performed successfully, 
    #   the "FINDING DATUM" will be turned ON.
    #
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    # ##
    def find_datum(self):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.VT_REQ_CODE)
        builder.add_1byte_uint(self.VT_FIND_DATUM)
        response = send_command(builder.build(), self.VT_COMMAND_TOUT)

        if not response:
            raise ValueError('VT find_datum has no response')
        
        # Check response
        dec = CommandDecoder(response, self.VT_REQ_CODE, self.VT_FIND_DATUM)
        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
            self._curState = self.VT_IDLE_STATE
        else:
            self._curState = self.VT_FINDING_DATUM_STATE
        return retVal

    ##
    # @brief
    #      Find find_datum_and_limit 
    #
    #
    # @details
    #      This function find datum and top limit position. If the function is performed successfully, 
    #   the "FINDING DATUM" will be turned ON.
    #
    # @return
    #      @arg    True: successful.
    #      @arg    False: failed.
    # ##
    def find_datum_and_limit(self):
        times = 0
        wait_for_datum = 0
        while times < 2:
            logger.debug({'VT':"find_datum_and_limit {} time".format(times+1)})
            retVal = self.find_datum()
            if not retVal:
                logger.debug({'VT':'find_datum error'})
                break
            else:
                ## wait for 10 secs timeout
                while wait_for_datum < 100:
                    logger.debug({'VT':'wait for find_datum'})
                    time.sleep(0.1)
                    if(self.get_status().state == 0):
                        break
                    wait_for_datum += 1
                logger.debug({'VT':"find_datum {}".format(self.get_status().pos)})
                retVal = self.find_top_limit()
                if not retVal:
                    logger.debug({'VT':'find_top_limit error'})
                    break
                retCmdVal = uasyncio.run(self.wait())
                if retCmdVal == CommandRetCode.CMD_RETCODE_NOERR:
                    vtTopLimit = round(self.get_status().pos, 0)
                    logger.debug({'VT':"find_top_limit {}".format(vtTopLimit)})
                    if(vtTopLimit >= 46 and vtTopLimit <= 50):
                        retVal = True
                        break
                    else:
                        retVal = False
                        logger.debug({'VT':'find_datum_and_limit is wrong position'})
                else:
                    retVal = False
                    logger.debug({'VT':'find_top_limit wait execution error'})
                    break
            times += 1
        return retVal

    ##
    # @brief
    #      Find top limit
    #
    #
    # @details
    #      This function moves the VT up to the highest position. The function will stop when the 
    #   motor current is larger than the set threshold. The current is supposed to be larger when the 
    #   VT hits something.
    #
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    # ##
    def find_top_limit(self, jogSpd = 20, ovThres = 0):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.VT_REQ_CODE)
        builder.add_1byte_uint(self.VT_FIND_TOP_LIMIT)
        builder.add_4bytes_float(jogSpd)
        builder.add_4bytes_float(ovThres)
        response = send_command(builder.build(), self.VT_COMMAND_TOUT)

        if not response:
            raise ValueError('VT find_top_limit has no response')

        # Check response
        dec = CommandDecoder(response, self.VT_REQ_CODE, self.VT_FIND_TOP_LIMIT)
        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
            self._curState = self.VT_IDLE_STATE
        else:
            self._curState = self.VT_FINDING_TOP_STATE
        return retVal

    def get_top_limit(self):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.VT_REQ_CODE)
        builder.add_1byte_uint(self.VT_GET_TOP_LIMIT)
        response = send_command(builder.build(), self.VT_COMMAND_TOUT)

        if not response:
            raise ValueError('VT get_top_limit has no response')

        # Check response
        dec = CommandDecoder(response, self.VT_REQ_CODE, self.VT_GET_TOP_LIMIT)
        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            return False, 0.0
            
        else:
            # parsing the top limit position
            return True, dec.decode_4bytes_q16()

    def calibrate_flag(self):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.VT_REQ_CODE)
        builder.add_1byte_uint(self.VT_CALIBRATE_FLAG)
        response = send_command(builder.build(), self.VT_COMMAND_TOUT)

        if not response:
            raise ValueError('VT vt_calibrate_flag has no response')
        
        # Check response
        dec = CommandDecoder(response, self.VT_REQ_CODE, self.VT_CALIBRATE_FLAG)
        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
            self._curState = self.VT_IDLE_STATE
        else:
            self._curState = self.VT_CALIBRATE_STATE
        return retVal

    def get_flag_dim(self):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.VT_REQ_CODE)
        builder.add_1byte_uint(self.VT_GET_FLAG_DIM)
        response = send_command(builder.build(), self.VT_COMMAND_TOUT)

        if not response:
            raise ValueError('VT get_top_limit has no response')

        # Check response
        dec = CommandDecoder(response, self.VT_REQ_CODE, self.VT_GET_FLAG_DIM)
        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
        
        return retVal, dec.decode_4bytes_q16(), dec.decode_4bytes_q16(), dec.decode_4bytes_q16(), dec.decode_4bytes_q16(), \
            dec.decode_4bytes_q16(), dec.decode_4bytes_q16(), dec.decode_4bytes_q16(), dec.decode_4bytes_q16()

    ##
    # @brief
    #      Move
    #
    #
    # @details
    #      This function moves the VT to input position with input speed.
    #
    # @param [in]
    #       is_rel: True: relative move, False: absoblute move
    # @param [in]
    #       pos: Position (mm)
    # @param [in]
    #       spd: Speed to move the VT (mm/s)
    #
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    #
    # ##
    def move(self, is_rel, pos, spd):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.VT_REQ_CODE)
        builder.add_1byte_uint(self.VT_MOVE)
        builder.add_1byte_bool(is_rel)
        builder.add_4bytes_float(pos)
        builder.add_4bytes_float(spd)
        builder.add_4bytes_float(0) #Overload Threshold, set to 0 as default
        response = send_command(builder.build(), self.VT_COMMAND_TOUT)

        if not response:
            raise ValueError('VT move has no response')

        # Check response
        dec = CommandDecoder(response, self.VT_REQ_CODE, self.VT_MOVE)
        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
            self._curState = self.VT_IDLE_STATE
        else:
            self._curState = self.VT_MOVING_STATE

        return retVal

        
    ##
    # @brief
    #      Clear the error flag of VT module. When the previous command is failed, 
    #       this command should be sent befor any other commands.
    #
    # @details
    #      This function creates a sending buffer and sent it to the modbus.
    #
    # @return
    #      @arg    True: Sending successful.
    #      @arg    False: Sending failed.
    # ##
    def clear_error_flags(self):
        retVal = True
        builder = CommandBuilder()
        builder.add_1byte_uint(self.VT_REQ_CODE)
        builder.add_1byte_uint(self.VT_CLEAR_ERROR_FLAG)
        response = send_command(builder.build(), self.VT_COMMAND_TOUT)

        if not response:
            raise ValueError('VT clear_error_flags has no response')

        # Check response
        dec = CommandDecoder(response, self.VT_REQ_CODE, self.VT_CLEAR_ERROR_FLAG)
        # Get ACK value
        if dec.decode_1byte_uint() != self.ACK:
            retVal = False
        return retVal

    async def wait(self):
        retVal = await super().wait()
        logger.debug({'VT status':'{0}'}, self._VTStatus(self._state, self._flags, self._pos, self._current)) # RM2-2065
        if retVal != CommandRetCode.CMD_RETCODE_NOERR:
            #print("Wait() failed")
            #print(self._VTStatus(self._state, self._flags, self._pos, self._current))
            # logger.debug({'VT Wait() failed':'{0}'}, self._VTStatus(self._state, self._flags, self._pos, self._current))
            return retVal

        if self._flags & self.VT_MOVE_ERR_BIT or \
            self._flags & self.VT_MOTOR_ERR_BIT or \
            self._flags & self.VT_OVERLOAD_ERR_BIT or \
            self._flags & self.VT_FIND_DATUM_ERR_BIT:
            #print("Flags failed")
            #print(self._VTStatus(self._state, self._flags, self._pos, self._current))
            # logger.debug({'VT Flags failed':'{0}'}, self._VTStatus(self._state, self._flags, self._pos, self._current))
            retVal = CommandRetCode.CMD_RETCODE_EXEERR

        return retVal
