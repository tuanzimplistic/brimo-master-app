"""
Based Class
"""
import uasyncio

class CommandRetCode():
    """
    Return code of submodules' commands.
    """
    ## No error
    CMD_RETCODE_NOERR = 0x00

    ## Command sending error
    CMD_RETCODE_SENDERR = 0x01

    ## Command execution error
    CMD_RETCODE_EXEERR = 0x02

    ## Timeout
    CMD_RETCODE_TOUTERR = 0x03

    ## General error
    CMD_RETCODE_GENERR = 0x04

class Itor3Command(CommandRetCode):
    ##
    # @brief
    #      Constructor
    # ##
    def __init__(self, cmd_exe_timeout, cmd_check_period):
        self._cmd_exe_tout = cmd_exe_timeout
        self._cmd_check_period = cmd_check_period
        self._idle_state = 0 # IDLE state value is 0 for every modules
        self._state = 0xFF
        self._flags = 0xFF
        self._pos = 0xFFFFFFFF
        self._current = 0xFFFFFFFF
    
    ##
    # @brief
    #      Derived class need to implement this function
    # ##
    def get_status(self):
        pass

    ##
    # @brief
    #      Coroutine to wait until complete
    #
    # @details
    #       This coroutine waits until the completion of the previous command.
    # @return
    #      @arg    CommandRetCode
    # ##
    async def wait(self):
        retVal = self.CMD_RETCODE_NOERR
        state = self._curState
        cnt = self._cmd_exe_tout / self._cmd_check_period
        while cnt > 0:
            # Update state every _cmd_check_period
            state = self.get_status().state
            if state != self._curState:
                break
            cnt -= 1
            # print("uasyncio.sleep_ms {}".format(self._cmd_check_period)) # for debugging
            await uasyncio.sleep_ms(self._cmd_check_period)
        
        if state == self._curState:
            retVal = self.CMD_RETCODE_TOUTERR
        elif state != self._idle_state: 
            retVal = self.CMD_RETCODE_EXEERR

        return retVal