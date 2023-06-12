"""
Eject and Fish Class
"""

import uasyncio
import sys

sys.path.insert('', "itor3_pyapp")

from master.itor3command import CommandRetCode
from app.common import *

class Utils(DakUprCommon):
    ##
    # @brief
    #      Constructor
    # ##
    def __init__(self):
        DakUprCommon.__init__(self)

        self._vt_top_limit = 47         ## VT Limit Position
        self._vt_cup_weight = 63        ## VT cup's weight in gram
    
    def SetVtLimit(self, limit):
        self._vt_top_limit = round(limit)

    def SetVTCupWeight(self, weight):
        self._vt_cup_weight = weight

    async def SetVTLimit(self):
        vtTopLimit = 0
        retCmdVal = self.vt.find_top_limit()
        if not retCmdVal:
            self.debugPrint("VT Top Limit sending error")
            return

        retVal = await self.vt.wait()
        if retVal == CommandRetCode.CMD_RETCODE_NOERR:
            vtTopLimit = self.vt.get_status().pos
            self.SetVtLimit(vtTopLimit)
            self.debugPrint("VT Top Limit:{}".format(vtTopLimit))
        else:
            self.debugPrint("VT Top Limit execution error: {}".format(retVal))

        return retVal, vtTopLimit

    async def Ejecting(self, vtPos, openTime, knSpeed, weightTime, attemps, tolWt = 10):
        wtBefore = 0.0
        wtAfter = 0.0
        
        exeTime = 0
        totalExeTime = 0
        cnt = 0

        isEjected = False
        
        
        retVal, exeTime = await self._exe_conf([
            {DakUprStrs.module : DakUprStrs.vt, "cmd" : ["move_top", vtPos, 35]},
            {DakUprStrs.module : DakUprStrs.sl, "cmd" : [weightTime]}
        ])
        totalExeTime += exeTime

        ## Get weight before
        if retVal == CommandRetCode.CMD_RETCODE_NOERR:
            wtBefore = self.ds.get_status(0).weight
            self.debugPrint("Before Wt: {:.2f}".format(wtBefore))
        else:
            return retVal, totalExeTime

        ejectCommands = ([
                {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["eject", 0]},
                {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["run", knSpeed]},
                {DakUprStrs.module : DakUprStrs.sl, "cmd" : [openTime]},
                {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["stop"]},
                {DakUprStrs.module : DakUprStrs.sl, "cmd" : [500]},
                {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["eject", 1]},
            ])
        
        while isEjected != True:
            retVal, exeTime = await self._exe_conf(ejectCommands)
            totalExeTime += exeTime

            ## Get weight after, only retry when weight check fails
            if retVal == CommandRetCode.CMD_RETCODE_NOERR:
                wtAfter = self.ds.get_status(0).weight
                self.debugPrint("After Wt: {:.2f}".format(wtAfter))
            
            if wtAfter > (wtBefore + self._vt_cup_weight) - tolWt:
                isEjected = True
            
            cnt += 1
            if cnt == attemps:
                retVal = CommandRetCode.CMD_RETCODE_EXEERR
                break
        
        # Move VT to 0 position to let user take the KN cup out
        _, exeTime = await self._exe_conf([
            {DakUprStrs.module : DakUprStrs.vt, "cmd" : ["move_block", 0, 35]},
            {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["stop"]},
        ])
        totalExeTime += exeTime

        if retVal != CommandRetCode.CMD_RETCODE_NOERR:
            self.debugPrint("Ejecting error")

        return retVal, totalExeTime

    async def Fishing(self, vtPosWt, vtPosHigh, vtSpeed, knSpeed, waitTime, attemps, tolWt = 10):
        wtBefore = 0.0 
        wtAfter = 0.0
        totalExeTime = 0
        isFished = False
        cnt = 0

        retVal, exeTime = await self._exe_conf([
            {DakUprStrs.module : DakUprStrs.vt, "cmd" : ["move_top", vtPosWt, 35]},
            {DakUprStrs.module : DakUprStrs.sl, "cmd" : [500]},
        ])
        totalExeTime += exeTime

        if retVal == CommandRetCode.CMD_RETCODE_NOERR:
            wtBefore = self.ds.get_status(0).weight
            self.debugPrint("Before Wt: {:.2f}".format(wtBefore))
        else:
            return retVal, totalExeTime
    
        fishingLoop = [
            {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["run", knSpeed]},
            {DakUprStrs.module : DakUprStrs.vt, "cmd" : ["move_top", vtPosHigh, vtSpeed]},
            {DakUprStrs.module : DakUprStrs.sl, "cmd" : [waitTime]},
            {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["stop"]},
            {DakUprStrs.module : DakUprStrs.vt, "cmd" : ["move_top", vtPosWt, 35]},
            {DakUprStrs.module : DakUprStrs.sl, "cmd" : [500]},
        ]

        while isFished != True:
            retVal, exeTime = await self._exe_conf(fishingLoop)
            totalExeTime += exeTime
            if retVal != CommandRetCode.CMD_RETCODE_NOERR:
                break
            wtAfter = self.ds.get_status(0).weight
            if wtAfter < (wtBefore - self._vt_cup_weight) + tolWt:
                isFished = True
            
            cnt += 1
            if cnt == attemps:
                retVal = CommandRetCode.CMD_RETCODE_EXEERR
                break
        
        _, exeTime = await self._exe_conf([
            {DakUprStrs.module : DakUprStrs.vt, "cmd" : ["move_block", 0, 35]},
            {DakUprStrs.module : DakUprStrs.kn, "cmd" : ["stop"]},
        ])
        totalExeTime += exeTime
        
        if retVal != CommandRetCode.CMD_RETCODE_NOERR:
            self.debugPrint("Fishing fails")

        return retVal, totalExeTime

    async def FishEjectALT(self, noTest, breakTime):
        cnt = 1
        
        retVal = CommandRetCode.CMD_RETCODE_NOERR

        while cnt <= noTest:
            totalTime = 0
            retVal, exeTime = await self.Fishing(26, 2, 10, 10, 1000, 3)
            totalTime += exeTime

            if retVal != CommandRetCode.CMD_RETCODE_NOERR:
                self.debugPrint("Fishing fails, cycle: {} / {}".format(cnt, noTest))
                break

            retVal, exeTime = await self.Ejecting(26, 1000, 5, 500, 3)
            totalTime += exeTime
            if retVal != CommandRetCode.CMD_RETCODE_NOERR:
                self.debugPrint("Ejecting fails, cycle: {} / {}".format(cnt, noTest))
                break

            self.debugPrint("FishEjectALT Passed, cycle: {} / {}, Exe Time: {}".format(cnt, noTest, totalTime))

            await self._exe_conf([
                {DakUprStrs.module : DakUprStrs.sl, "cmd" : [breakTime]},
            ])

            cnt += 1

        return retVal

    def debugPrint(self, message):
        # self.uihelper.debugPrint(message)
        print("EjectFish " + message)