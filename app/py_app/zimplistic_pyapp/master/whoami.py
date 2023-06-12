"""
Who Am I (WAI) class
"""

from collections import namedtuple
from itor3_pyapp import *
from itor3_pyapp.master.itor3command import *

class WAIConstants():
    ## Switch context code
    WAI_REQ_CODE    = 0x01

    WAI_SUB_CODE    = 0x02 #SLAVE_APPL_CONTEXT

    ## Command Time out
    WAI_COMMAND_TOUT = 500

    ## ACK
    ACK = 0x00

    ## NACK
    NACK = 0x01

    ## Switch context command timeout
    WAI_CMD_EXE_TOUT     = 180000

    ## DS Command check period
    WAI_CMD_CHECK_TIME = 500

class WAI(WAIConstants, Itor3Command):
    ##
    # @brief
    #      Constructor
    # ##
    def __init__(self):
        ## namedtyple to store the Module State
        self._slaveversion = namedtuple("SlaveVersion", "major minor patch dirty")
        Itor3Command.__init__(self, self.WAI_COMMAND_TOUT, self.WAI_CMD_CHECK_TIME)
        
    ##
    # @brief
    #      Get slave board version
    #
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() to get the version of the slave board.
    #
    # @param [in]
    #       None
    #
    # @return
    #      @arg    True: if the command sent successfully.
    # @return
    #      @arg    False: if the command sent failed.
    # ##
    def get_version(self):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.WAI_REQ_CODE)
        
        response = send_command(builder.build(), self.WAI_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('WAI get_version has no response')

        dec = CommandDecoder(response, self.WAI_REQ_CODE, self.WAI_SUB_CODE)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('SW get_status has no response')

        _major = dec.decode_1byte_uint()
        _minor = dec.decode_1byte_uint()
        _patch = dec.decode_1byte_uint()
        _dirty = dec.decode_1byte_uint()

        return self._slaveversion(_major, _minor, _patch, _dirty)