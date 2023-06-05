"""
upr Class
"""
import uasyncio
import json # for debugging
# import aioconsole
# import uihelper

from itor3_pyapp.master import *
from itor3_pyapp.app.common import *
from itor3_pyapp.app.log import *

class upr(DakUprCommon):
    ##
    # @brief
    #      Constructor
    # ##
    def __init__(self):
        DakUprCommon.__init__(self)
        
        ## Event to start the UPR
        self.beginUPREvent = uasyncio.Event()

        ## Event to end the UPR
        self.endUPREvent  = uasyncio.Event()

        ## UPR Task
        self.uprTask      = uasyncio.create_task(self._upr())
        # self.uihelper = uihelper.uihelper(4)

        ## Configs
        self._pressingCfgArray = []
        self._roastingCfgArray = []
        self._coolingCfgArray = []
        self._transferCfgArray = []

        ## Timing
        self._pressTime = 0
        self._roastTime = 0
        self._coolTime = 0

    ##
    # @brief
    #      Disable modules of Roti machine.
    #
    #
    # @details
    #      This function is used to disable modules of the machine. If one module is disabled, any action of this module
    #       is not performed when running the cycle.
    #
    # @param [in]
    #       disable_modules: List of disabled modules
    #
    # @code
    #       # To disable any WP movement during the roti cycle
    #       myUpr = upr()
    #       myUpr.DisableModules({DakUprStrs.wp})
    # @endcode
    #
    # @return
    #      @arg    None
    # ##
    def DisableModules(self, disable_modules):
        if DakUprStrs.wp in disable_modules : self._isWPAvailbale = False
        if DakUprStrs.ht in disable_modules : self._isHTAvailable = False
        if DakUprStrs.kr in disable_modules : self._isKRAvailable = False
        if DakUprStrs.sl in disable_modules : self._isSleepEnable = False
        if DakUprStrs.fa in disable_modules : self._isFaAvailbale = False

    async def _upr(self):
        retVal = CommandRetCode.CMD_RETCODE_NOERR
        while True:
            await self.beginUPREvent.wait()
            self.beginUPREvent.clear()
            
            self._pressTime = 0
            self._roastTime = 0
            self._coolTime = 0

            # self.debugPrint("Start Pressing")
            # retVal, self._pressTime = await self._performPressing()
            # self.debugPrint("Finish Pressing")
            
            if retVal == CommandRetCode.CMD_RETCODE_NOERR:
                self.debugPrint("Start roasting")
                retVal, self._roastTime = await self._performRoasting()
                self.debugPrint("Finish roasting")

            # if retVal == CommandRetCode.CMD_RETCODE_NOERR:
            #     self.debugPrint("Start cooling")
            #     retVal, self._coolTime = await self._performCooling()
            #     self.debugPrint("Finish cooling")

            if retVal != CommandRetCode.CMD_RETCODE_NOERR:    
                ## The commands below ensure the safety before fully error hanling is available
                ## Turn OFF all heaters
                self.ht.set_state(HT.HT_ID_TOP, 0, 0, 0, 0)
                self.ht.set_state(HT.HT_ID_BTM, 0, 0, 0, 0)
                self.ht.set_state(HT.HT_ID_CHARTOP, 0, 0, 0, 0)
                self.ht.set_state(HT.HT_ID_CHARBTM, 0, 0, 0, 0)

            self.endUPREvent.set()
    
    def GetTimeReport(self):
        return self._pressTime, self._roastTime, self._coolTime

    ##
    # @brief
    #      Start the UPR process
    #
    #
    # @details
    #      This function starts the UPR process.
    #
    # @return
    #      @arg    None
    # ##
    def start_upr(self):
        self.endUPREvent.clear()
        self.beginUPREvent.set()
    
    ##
    # @brief
    #      Wait for UPR completion
    #
    #
    # @details
    #      This coroutine waits for the UPR completion
    #
    # @return
    #      @arg    None
    # ##
    async def wait_for_upr_to_finish(self):
        await self.endUPREvent.wait()
        
    ##
    # @brief
    #      Error Handler
    #
    #
    # @details
    #      TBD
    #
    # @return
    #      @arg    TBD
    # ##
    async def handlePressError(self):
        # await self.uihelper.showErrorDialog("Please remove obstruction and continue")
        pass
    
    ##
    # @brief
    #      Print debug information for UPR
    #
    #
    # @details
    #      Print debug information for the UPR class
    #
    # @param [in]
    #       message: printed message
    #
    # @return
    #      @arg    None
    # ##
    def debugPrint(self, message):
        # self.uihelper.debugPrint(message)
        #print("upr_thread " + message)
        logger.debug({'upr_thread ':'{0}'}, message)

    def _commandIsValid(self, command):
        isValid = True
        
        if command[DakUprStrs.module] == DakUprStrs.wp:
            # Wedge command: [pressPos wedgePos desiredTime]
            if not command["cmd"]:
                isValid = False
            elif len(command["cmd"]) != 3:
                isValid = False

        elif command[DakUprStrs.module] == DakUprStrs.kr:
            # Kicker command: [krPos krSpd]
            if not command["cmd"]:
                isValid = False
            else:
                if len(command["cmd"]) != 2:
                    isValid = False

        elif command[DakUprStrs.module] == DakUprStrs.ht: 
            if not command["cmd"]:
                isValid = False
            else:
                htCmd = command["cmd"]
                # Heater command ON: [ON htID setTemperature maxDuty duty]
                if htCmd[0] == DakUprStrs.htCmdON:
                    if len(htCmd) != 5:
                        isValid = False
                ## Heater command OFF: [OFF htID]
                elif htCmd[0] == DakUprStrs.htCmdOFF:
                    if len(htCmd) != 2:
                        isValid = False
                ## Heater command LE: [LE htID tempearature timeOut]
                ## Heater command GE: [GE htID tempearature timeOut]
                elif htCmd[0] == DakUprStrs.htCmdLE or \
                    htCmd[0] == DakUprStrs.htCmdGE:
                    if len(htCmd) != 4:
                        isValid = False
                else:
                    isValid = False
        elif command[DakUprStrs.module] == DakUprStrs.sl:
            # Sleep command: [SleepingTime]
            if len(command["cmd"]) != 1:
                isValid = False
        elif command[DakUprStrs.module] == DakUprStrs.fa:
            ## Fan command: [fanMask]
            if len(command["cmd"]) != 1:
                isValid = False
        else:
            isValid = False
        
        return isValid

    ##
    # @brief
    #      Pressing Configure
    #
    #
    # @details
    #      This function is used to add commands into the pressing configuration. The format of commands 
    #       for every modules is explained at the example below.
    # @code
    #   myUpr = dak()
    #   # Format of WP command: ["Wedge Position", "Pivot Position", "Desired Time"]
    #   myUpr.PressingConfigAddCmd({DakUprStrs.module : DakUprStrs.wp, "cmd" : [56, 50, 500]})
    #   # Format of KR command: ["Kicker Position", "Moving Speed"]
    #   myUpr.PressingConfigAddCmd({DakUprStrs.module : DakUprStrs.kr, "cmd" : [150, 50]})
    #   # Format of HT ON command: [DakUprStrs.htCmdON, "HT ID", "Temperature", "Maximum duty cycle", "Duty cycle"]
    #   myUpr.PressingConfigAddCmd({DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdON, HT.HT_ID_TOP, 250, 75, 45]})
    #   # Format of HT OFF command: [DakUprStrs.htCmdOFF, "HT ID"]
    #   myUpr.CoolingConfigAddCmd({DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdOFF, HT.HT_ID_TOP]})
    #   # Format of HT LE (less then or equal) command: [DakUprStrs.htCmdLE, "HT ID", "Temperature", "Time out"]
    #   myUpr.CoolingConfigAddCmd({DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdLE, HT.HT_ID_TOP, 100, 50000]})
    #   # Format of HT GE (greater or equal) command: [DakUprStrs.htCmdGE, "HT ID", "Temperature", "Time out"]
    #   myUpr.PressingConfigAddCmd({DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdGE, HT.HT_ID_TOP, 100, 50000]})
    # @endcode
    #
    # @param [in]
    #       command: as explained as the above example.
    #
    #
    # @return
    #      @arg    None
    # ##
    def PressingConfig(self, command_array):
        self._pressingCfgArray = command_array

    ##
    # @brief
    #      Roasting Configure
    #
    #
    # @details
    #      This function is used to add commands into the Roasting configuration. The format of commands 
    #       for every modules is explained at the example in function PressingConfigAddCmd()
    #
    # @param [in]
    #       command: as explained as the example in function PressingConfigAddCmd()
    #
    #
    # @return
    #      @arg    None
    # ##
    def RoastingConfig(self, command_array):
        self._roastingCfgArray = command_array

    def TransferringConfig(self, command_array):
        self._transferCfgArray = command_array
    
    ##
    # @brief
    #      Cooling Configure
    #
    #
    # @details
    #      This function is used to add commands into the Cooling configuration. The format of commands 
    #       for every modules is explained at the example in function PressingConfigAddCmd()
    #
    # @param [in]
    #       command: as explained as the example in function PressingConfigAddCmd()
    #
    #
    # @return
    #      @arg    None
    # ##
    def CoolingConfig(self, command_array):
        self._coolingCfgArray = command_array
    
    async def _performPressing(self):
        return await self._exe_conf(self._pressingCfgArray)

    async def _performRoasting(self):
        return await self._exe_conf(self._roastingCfgArray)

    async def _performCooling(self):
        return await self._exe_conf(self._coolingCfgArray)

    async def PerformTransfer(self):
        return await self._exe_conf(self._transferCfgArray)