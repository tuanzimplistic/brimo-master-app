"""
Switch Context (SW) class
"""

from collections import namedtuple
import sys

from itor3_pyapp import *
from itor3_pyapp.master.itor3command import *

class SWConstants():
    ## Switch context code
    SW_REQ_CODE    = 0x02

    ## Command Time out
    SW_COMMAND_TOUT = 500

    ## ACK
    ACK = 0x00

    ## NACK
    NACK = 0x01

    ## Switch context command timeout
    SW_CMD_EXE_TOUT     = 180000

    ## DS Command check period
    SW_CMD_CHECK_TIME = 500

class SW(SWConstants, Itor3Command):
    ##
    # @brief
    #      Constructor
    # ##
    def __init__(self):
        Itor3Command.__init__(self, self.SW_CMD_EXE_TOUT, self.SW_CMD_CHECK_TIME)
        
    ##
    # @brief
    #      Switch context of the slave board
    #
    #
    # @details
    #      This function sends command to slave by calling the function command_sender.send_command() to switch the context of the slave board.
    #
    # @param [in]
    #       id: 0x01 to switch to Bootloader; 0x02 to switch to Application
    #
    # @return
    #      @arg    True: if the command sent successfully.
    # @return
    #      @arg    False: if the command sent failed.
    # ##
    def switch_context(self, id):
        builder = CommandBuilder()
        builder.add_1byte_uint(self.SW_REQ_CODE)
        
        builder.add_1byte_uint(id)
        response = send_command(builder.build(), self.SW_COMMAND_TOUT)
        
        # print(response) # for debugging
        if not response:
            raise ValueError('SW switch_context has no response')

        return True