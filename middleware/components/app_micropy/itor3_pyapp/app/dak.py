"""
dak Class
"""

import uasyncio
import json # for debugging
# import aioconsole
# import uihelper

from itor3_pyapp.app.common import *
from itor3_pyapp.master import *
from itor3_pyapp.app.log import *

class dak(DakUprCommon):
    ##
    # @brief
    #      Constructor
    # ##
    def __init__(self):
        DakUprCommon.__init__(self)

        ## Event to start the Kneading
        self.beginKneadingEvent = uasyncio.Event()
        
        ## Event to end the Kneading
        self.endKneadingEvent = uasyncio.Event()

        ## Kneading Task
        self.kneadingTask = uasyncio.create_task(self._dispenseAndKnead())

        ## Configs
        self._dispenseConfigs = []
        self._kneadingConfigs = []
        self._dispense = None
        # self.uihelper = uihelper.uihelper(0)

        ## Timing
        self._dispensingTime = 0
        self._kneadingTime = 0

        ## Doughball state, 0: no doughball created, 1: douball created
        self._db_state = 0

        ## Weight information
        self._flour_weight = 0
        self._water_weight = 0
        self._oil_weight = 0

        ## VT Limit Position
        self._vt_top_limit = 47
    
    def SetVtLimit(self, limit):
        self._vt_top_limit = round(limit)

    def GetDoughballState(self):
        return self._db_state

    def GetTimeReport(self):
        return self._dispensingTime, self._kneadingTime

    def GetWeightReport(self):
        return self._flour_weight, self._water_weight, self._oil_weight

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
    #       # To disable any VT movement during the roti cycle
    #       myDak = dak()
    #       myDak.DisableModules({DakUprStrs.vt})
    # @endcode
    #
    # @return
    #      @arg    None
    # ##
    def DisableModules(self, disable_modules):
        if DakUprStrs.vt in disable_modules : self._isVTAvailable = False
        if DakUprStrs.kn in disable_modules : self._isKNAvailable = False
        if DakUprStrs.ds in disable_modules : self._isDSAvailable = False
        if DakUprStrs.sl in disable_modules : self._isSleepEnable = False
        if DakUprStrs.kr in disable_modules : self._isKRAvailable = False

    ##
    # @brief
    #      Set Dispensing Function
    #
    #
    # @details
    #      This function is used to configure how the DAK does the dipensing. Becaue how to do dispensing
    #       is different from recipe to recipe, this API let user defines how to do dispensing.
    #
    # @param [in]
    #       dispense_func: Function to do dispensing
    #
    #
    # @return
    #      @arg    None
    # ##
    def SetDispenseFunction(self, dispense_func):
        self._dispense = dispense_func

    
    ##
    # @brief
    #      Kneading add command
    #
    #
    # @details
    #      This function is used to add commands into the Kneading array. The Kneading array 
    #       stores all command for knead. Each command follows the format as the example below.
    #
    # @code
    #   myDak = dak()
    #   myDak.KneadingConfig([
    #       {KnStrs.vtPos : 56,  KnStrs.vtSpd : 20, KnStrs.knSpd : 5,  KnStrs.knTime : 1000},
    #       {KnStrs.vtPos : 55,  KnStrs.vtSpd : 20, KnStrs.knSpd : 6,  KnStrs.knTime : 1500}
    #   ])
    # @endcode
    #
    # @param [in]
    #       command: the list of command format {KnStrs.vtPos :   KnStrs.vtSpd : , KnStrs.knSpd : ,  KnStrs.knTime : }
    #
    #
    # @return
    #      @arg    None
    # ##
    def KneadingConfig(self, command_array, toStop = True, str_pos = 0.0, str_spd = 0.0):
        ## Convert from user format to DAK commands format
        for config in command_array:
            if not config[KnStrs.vtPos] or \
                not config[KnStrs.vtSpd] or \
                not config[KnStrs.knSpd] or \
                not config[KnStrs.knTime]:
                raise ValueError('Command is invalid')
            ## Adding commands to config array
            self._kneadingConfigs.append({DakUprStrs.module : DakUprStrs.kn, "cmd" : ["run", config[KnStrs.knSpd]]})
            self._kneadingConfigs.append({DakUprStrs.module : DakUprStrs.vt, "cmd" : ["move_top", config[KnStrs.vtPos], config[KnStrs.vtSpd]]})
            self._kneadingConfigs.append({DakUprStrs.module : DakUprStrs.sl, "cmd" : [config[KnStrs.knTime]]})

        ## Stop the Kneader after Kneading
        if toStop == True:
            self._kneadingConfigs.append({DakUprStrs.module : DakUprStrs.kn, "cmd" : ["stop"]})
        else:
            self._kneadingConfigs.append({DakUprStrs.module : DakUprStrs.kn, "cmd" : ["set_pos", str_pos, str_spd]})

    ##
    # @brief
    #      Dispensing by time
    #
    #
    # @details
    #      This coroutine object dispenses the element by time.
    #
    # @param [in]
    #       id: master.dispenser.DSConstants.DS_ID_FLOUR, master.dispenser.DSConstants.DS_ID_WATER or master.dispenser.DSConstants.DS_ID_OIL
    # @param [in]
    #       dis_time: dispensing time in millisecond
    # @param [in]
    #       dis_spd: dispensing speed, only applicable for dispensing FLOUR
    # @param [in]
    #       vtPos: VT Position to do the dispensing
    # @param [in]
    #       vtSpd: VT Speed when moving the VT to the dispensing position
    #
    #
    # @return
    #      @arg    master.itor3command.CommandRetCode
    # ##
    async def DispenseElementByTime(self, id, dis_time, dis_spd, vtPos, vtSpd):
        config_array = [
            {DakUprStrs.module : DakUprStrs.vt, "cmd" : ["move_topnonblock", vtPos, vtSpd]},
            {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["set_dis_pos", id]},
            {DakUprStrs.module : DakUprStrs.vt, "cmd" : ["wait"]},
            {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["wait"]},
            
            {DakUprStrs.module : DakUprStrs.ds, "cmd" : ["tare"]},
            {DakUprStrs.module : DakUprStrs.ds, "cmd" : ["time", id, dis_time, dis_spd]}
        ]
        return await self._exe_conf(config_array)

    ##
    # @brief
    #      Dispensing by weight
    #
    #
    # @details
    #      This coroutine object dispenses the element by weight.
    #
    # @param [in]
    #       id: master.dispenser.DSConstants.DS_ID_FLOUR, master.dispenser.DSConstants.DS_ID_WATER or master.dispenser.DSConstants.DS_ID_OIL
    # @param [in]
    #       dis_weight: weight to dispense
    # @param [in]
    #       dis_spd: dispensing speed, only applicable for dispensing FLOUR
    # @param [in]
    #       vtPos: VT Position to do the dispensing
    # @param [in]
    #       vtSpd: VT Speed when moving the VT to the dispensing position
    #
    #
    # @return
    #      @arg    master.itor3command.CommandRetCode
    # ##
    async def DispenseElementByWeight(self, id, dis_weight, dis_spd, vtPos, vtSpd):
        config_array = [
            {DakUprStrs.module : DakUprStrs.vt, "cmd" : ["move_topnonblock", vtPos, vtSpd]},
            {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["set_dis_pos", id]},
            {DakUprStrs.module : DakUprStrs.vt, "cmd" : ["wait"]},
            {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["wait"]},
            
            {DakUprStrs.module : DakUprStrs.sl, "cmd" : [500]},
            {DakUprStrs.module : DakUprStrs.ds, "cmd" : ["tare"]},
            {DakUprStrs.module : DakUprStrs.ds, "cmd" : ["weight", id, dis_weight, dis_spd]}
        ]
        return await self._exe_conf(config_array)

    async def DispenseByWeight(self, id, dis_weight, dis_spd, vtPos, vtSpd):
        retVal = CommandRetCode.CMD_RETCODE_NOERR
        totalTime = 0
        dispensedWt = 0.0

        retVal, exeTime = await self._exe_conf([
            {DakUprStrs.module : DakUprStrs.vt, "cmd" : ["move_topnonblock", vtPos, vtSpd]},
            {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["set_dis_pos", id]},
            {DakUprStrs.module : DakUprStrs.vt, "cmd" : ["wait"]},
            {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["wait"]},
            {DakUprStrs.module : DakUprStrs.sl, "cmd" : [500]},
            {DakUprStrs.module : DakUprStrs.ds, "cmd" : ["tare"]}
        ])
        #totalTime += exeTime

        ## Get weight before dispensing
        lcB4Dispense = self.ds.get_status(id).weight
        if retVal == CommandRetCode.CMD_RETCODE_NOERR:
            retVal, exeTime = await self._exe_conf([
                {DakUprStrs.module : DakUprStrs.ds, "cmd" : ["weight", id, dis_weight, dis_spd]}
            ])
        totalTime += exeTime
        
        dispensedWt = self.ds.get_status(id).weight - lcB4Dispense
        if id == DS.DS_ID_FLOUR:
            self._flour_weight = dispensedWt
        elif id == DS.DS_ID_WATER:
            self._water_weight = dispensedWt
        elif id == DS.DS_ID_OIL:
            self._oil_weight = dispensedWt
        
        return retVal, totalTime

    ##
    # @brief
    #      Flour Dispensing with offset adaption
    #
    #
    # @details
    #      This function dispense FLOUR and set the offset back to the Dispenser for the second dispensing.
    #
    # @param [in]
    #       vtPos: VT Postion to mix the flour
    # @param [in]
    #       vtSpd: Speed to move the VT to the mixing position
    # @param [in]
    #       knSpd: KN speed (rad/s) when mixing
    # @param [in]
    #       mixingTime: Mixing Time
    #
    # @return
    #      @arg    master.itor3command.CommandRetCode
    # ##
    async def DispenseFlourByWeight(self, dis_weight, weight_each_dispense, dis_spd, vtPos, vtSpd, mix_pos, mix_spd, mix_time):
        retVal = CommandRetCode.CMD_RETCODE_NOERR
        self._flour_weight = 0

        total_time = 0
        is_last_dispens = False
        is_1st_dispense = True
        to_dis_wt = 0
        
        # Set stop tollerance to 0
        self.ds.set_param(DS.DS_ID_FLOUR, DS.DS_PARAM_STOP_TOLERANCE, 0)

        # FLour dispensing maximum time is 3
        for i in range(3):
            if self._flour_weight >= dis_weight:
                break

            if dis_weight - self._flour_weight <= weight_each_dispense:
                if dis_weight - self._flour_weight < 1: # Dont do dispensing
                    break
                else:
                    to_dis_wt = dis_weight - self._flour_weight
                    retVal, exe_time = await self.DispenseElementByWeight(DS.DS_ID_FLOUR, to_dis_wt, 5, vtPos, vtSpd)
                    is_last_dispens = True
            else:
                to_dis_wt = weight_each_dispense
                retVal, exe_time = await self.DispenseElementByWeight(DS.DS_ID_FLOUR, to_dis_wt, dis_spd, vtPos, vtSpd)

            total_time += exe_time
            dispensed_weight = self.ds.get_status(DS.DS_ID_FLOUR).weight
            self._flour_weight += dispensed_weight
            # Set stop tollerance for the next dispensing, only set after the 1st dispensing
            if is_1st_dispense:
                self.ds.set_param(DS.DS_ID_FLOUR, DS.DS_PARAM_STOP_TOLERANCE, dispensed_weight - to_dis_wt)
                is_1st_dispense = False

            # ## For debugging:
            self.debugPrint("Flour Dispense: {}, Offset: {}, Total Flour: {}".format(dispensed_weight, dispensed_weight - to_dis_wt, 
                self._flour_weight))

            if retVal != CommandRetCode.CMD_RETCODE_NOERR:
                self.debugPrint("Flour dispensing error")
                break

            ## Mixing
            if not is_last_dispens:
                retVal, exeTime = await self.FlourMixing(mix_pos, vtSpd, mix_spd, mix_time)
                total_time += exeTime
                if retVal != CommandRetCode.CMD_RETCODE_NOERR:
                    self.debugPrint("Flour mixing error")
                    break

        return retVal, total_time

    async def DispenseWaterByWeight(self, dis_weight, vtPos, vtSpd):
        self.ds.set_param(DS.DS_ID_WATER, DS.DS_PARAM_STOP_TOLERANCE, 0)
        self._water_weight = 0

        total_time = 0

        expect_water = round(dis_weight / 2.0, 2)
        retVal, exeTime = await self.DispenseElementByWeight(DS.DS_ID_WATER, expect_water, 0, vtPos, vtSpd)
        total_time += exeTime
        dispensed_weight = self.ds.get_status(DS.DS_ID_WATER).weight
        self._water_weight += dispensed_weight
        offset = round(dispensed_weight - expect_water, 2)
        self.debugPrint("Expected Water: {}, Dispensed Water: {}, Water Offset: {}".format(expect_water, dispensed_weight, offset))
        self.ds.set_param(DS.DS_ID_WATER, DS.DS_PARAM_STOP_TOLERANCE, offset)

        if retVal == CommandRetCode.CMD_RETCODE_NOERR:
            # Do the second dispensing
            remain_water = round(dis_weight - self._water_weight, 2)
            retVal, exeTime = await self._exe_conf([
                    {DakUprStrs.module : DakUprStrs.ds, "cmd" : ["tare"]},
                    {DakUprStrs.module : DakUprStrs.sl, "cmd" : [500]},
                    {DakUprStrs.module : DakUprStrs.ds, "cmd" : ["weight", DS.DS_ID_WATER, remain_water, 0]}
                ])
            total_time += exeTime
            dispensed_weight = self.ds.get_status(DS.DS_ID_WATER).weight
            self._water_weight += dispensed_weight
            offset = round(dispensed_weight - remain_water, 2)
            self.debugPrint("Expected Water: {}, Dispensed Water: {}, Water Offset: {}".format(remain_water, dispensed_weight, offset))

        return retVal, total_time

    # retVal, exeTime = await self.myDak.DispenseOilByWeight(self.DAK_OIL_WT, self.DAK_DIS_VT_POS, self.DAK_DIS_VT_SPD)
    async def DispenseOilByWeight(self, oil_offset, dis_weight, vtPos, vtSpd):
        self.ds.set_param(DS.DS_ID_OIL, DS.DS_PARAM_STOP_TOLERANCE, oil_offset)
        
        retVal, exeTime = await self.DispenseElementByWeight(DS.DS_ID_OIL, dis_weight, 0, vtPos, vtSpd)
        self._oil_weight = self.ds.get_status(DS.DS_ID_OIL).weight
        
        offset = (self._oil_weight - dis_weight) + oil_offset
        self.debugPrint("Expected Oil: {}, Dispensed Oil: {}, Oil Offset: {}".format(dis_weight, self._oil_weight, offset))

        return retVal, exeTime, offset
        
    ##
    # @brief
    #      Flour Mixing
    #
    #
    # @details
    #      This coroutine mixes the flour with the input VT Posion, VT Speed, KN Speed and 
    #       mixing time.
    #
    # @param [in]
    #       vtPos: VT Postion to mix the flour
    # @param [in]
    #       vtSpd: Speed to move the VT to the mixing position
    # @param [in]
    #       knSpd: KN speed (rad/s) when mixing
    # @param [in]
    #       mixingTime: Mixing Time
    #
    # @return
    #      @arg    master.itor3command.CommandRetCode
    # ##
    async def FlourMixing(self, vtPos, vtSpd, knSpd, mixingTime):
        config_array = [
            {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["run", knSpd]},
            {DakUprStrs.module : DakUprStrs.vt, "cmd" : ["move_top", vtPos, vtSpd]},
            {DakUprStrs.module : DakUprStrs.sl, "cmd" : [mixingTime]},
            {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["stop"]},
            {DakUprStrs.module : DakUprStrs.sl, "cmd" : [500]}
        ]
        return await self._exe_conf(config_array)

    async def DoughballDrop(self, vtPosHigh, vtPosLow, vtSpd, knTime, knSpd, holdTime):
        config_array = [
            {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["run", knSpd]},
            {DakUprStrs.module : DakUprStrs.vt, "cmd" : ["move_nonblock", vtPosLow, vtSpd]},
            {DakUprStrs.module : DakUprStrs.sl, "cmd" : [holdTime]},
            {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["stop"]},
            {DakUprStrs.module : DakUprStrs.vt, "cmd" : ["wait"]},
            {DakUprStrs.module : DakUprStrs.sl, "cmd" : [holdTime]},
            {DakUprStrs.module : DakUprStrs.vt, "cmd" : ["move_block", vtPosHigh, vtSpd]}
        ]
        retVal, exeTime = await self._exe_conf(config_array)
        if retVal != CommandRetCode.CMD_RETCODE_NOERR:
            self.kn.stop()

        return retVal, exeTime

    async def _knead(self):
        self.debugPrint("Start Kneading")
        retVal, exeTime = await self._exe_conf(self._kneadingConfigs)
        # Don't stop the KN to save cycle time
        #self.kn.stop() # Stop the Kneader
        self.debugPrint("End Kneading")
        return retVal, exeTime

    async def _dispenseAndKnead(self):
        while True:
            await self.beginKneadingEvent.wait()
            self.beginKneadingEvent.clear()
            self._db_state = 0 # set dougball state to 0 when starting dak
            self._dispensingTime = 0
            self._kneadingTime = 0

            retVal, self._dispensingTime = await self._dispense()
            if retVal == CommandRetCode.CMD_RETCODE_NOERR:
                retVal, self._kneadingTime = await self._knead()
            if retVal == CommandRetCode.CMD_RETCODE_NOERR:
                self._db_state = 1

            self.endKneadingEvent.set()

    ##
    # @brief
    #      Start the DAK
    #
    #
    # @details
    #      This function starts the DAK processing
    #
    # @return
    #      @arg    None
    # ## 
    def start_dak(self):
        self.endKneadingEvent.clear()
        self.beginKneadingEvent.set()
        
    ##
    # @brief
    #      Coroutine Object to wait for DAK completion
    #
    #
    # @details
    #      Wait until the DAK processing finishes
    #
    # @return
    #      @arg    None
    # ## 
    async def wait_for_dak_to_finish(self):
        t1 = uasyncio.ticks()
        await self.endKneadingEvent.wait()
        return (uasyncio.ticks() - t1)

    ##
    # @brief
    #      Hanle Water Dispensing Error (TBD)
    #
    #
    # @details
    #      TBD
    #
    # @return
    #      @arg    TBD
    # ##
    async def handleWaterDispensingError(self):
        pass
    
    ##
    # @brief
    #      Print debug information for DAK
    #
    #
    # @details
    #      Print debug information for the DAK class
    #
    # @param [in]
    #       message: printed message
    #
    # @return
    #      @arg    None
    # ##
    def debugPrint(self, message):
        #  self.uihelper.debugPrint(message)
        #print("dak_thread " + message)
        logger.debug({'dak_thread ':'{0}'}, message)