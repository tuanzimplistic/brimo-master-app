"""
Application Constanst
"""

import uasyncio
import json # for debugging
# import aioconsole
# import uihelper
from itor3_pyapp.master import *
from itor3_pyapp.app.log import *

class KnStrs():
    ## VT Position constant string
    vtPos = "vtPos"

    ## VT Speed constant string
    vtSpd = "vtSpd"

    ## KN Speed constant string
    knSpd = "knSpd"

    ## KN Time constant string
    knTime = "knTime"

class DakUprStrs():
    ## DAK Module constant string
    module = "module"

    ## KN module constant string
    kn = "kn"

    ## DS module constant string
    ds = "ds"

    ## VT module constant string
    vt = "vt"

    ## Sleep module constant string
    sl = "sleep"

    ## WP Module constant string
    wp = "wp"

    ## KR Module constant string
    kr = "kr"
    
    ## HT Module constant string
    ht = "ht"
    
    ## HT Command UPDATE constant string
    htCmdUpdate = "UPDATE"

    ## HT Command ON constant string
    htCmdON = "ON"

    ## HT Command OFF constant string
    htCmdOFF = "OFF"

    ## HT Command LE (Less than or Equal) constant string
    htCmdLE = "LE" ## Less than or equal

    ## HT Command LE (Greater than or Equal) constant string
    htCmdGE = "GE" ## Greater than or equal

    ## HT Command EQ (equal)
    htCmdEQ = "EQ"

    chCmdOn = "CHON"    ## Charring command on
    chCmdOff = "CHOFF"  ## Charring command off
    
    ## FAN Module constant string
    fa = "fan"

class DakUprCommon():
    def __init__(self):
        ## VT
        self.vt = VT()

        ## KN
        self.kn = KN()

        ## DS
        self.ds = DS()

        ## WP Module
        self.wp = WP()

        ## HT Module
        self.ht = HT()

        ## KR Module
        self.kr = KR()

        ## Available Modules, default all are available
        self._isVTAvailable = True
        self._isKNAvailable = True
        self._isDSAvailable = True
        self._isHTAvailable = True
        self._isWPAvailbale = True
        self._isKRAvailable = True
        self._isFaAvailbale = True
        self._isSleepEnable = True

    async def _exe_conf(self, config_array):
        totalTime = 0
        retVal  = CommandRetCode.CMD_RETCODE_NOERR
        for cmd in config_array:
            retVal, exeTime = await self._worker(cmd)
            totalTime += exeTime
            if retVal != CommandRetCode.CMD_RETCODE_NOERR:
                break
        return retVal, totalTime
    
    async def _worker(self, command):
        isAvailable = False
        t1 = uasyncio.ticks()
        retVal = CommandRetCode.CMD_RETCODE_NOERR
        ## Kneader commands
        if command[DakUprStrs.module] == DakUprStrs.kn:
            if self._isKNAvailable:
                isAvailable = True
                retVal = await self._kn(command)
        ## VT commands
        elif command[DakUprStrs.module] == DakUprStrs.vt:
            if self._isVTAvailable:
                isAvailable = True
                retVal = await self._vt(command)
        ## DS commands
        elif command[DakUprStrs.module] == DakUprStrs.ds:
            if self._isDSAvailable:
                isAvailable = True
                retVal = await self._ds(command)
        ## Sleep commands
        elif command[DakUprStrs.module] == DakUprStrs.sl:
            if self._isSleepEnable:
                isAvailable = True
                await uasyncio.sleep_ms(command["cmd"][0])
        elif command[DakUprStrs.module] == DakUprStrs.wp:
            # Wedge command
            if self._isWPAvailbale:
                isAvailable = True
                retVal = await self._wp(command)
        elif command[DakUprStrs.module] == DakUprStrs.kr:
            # Kicker command
            if self._isKRAvailable:
                isAvailable = True
                retVal = await self._kr(command)
        elif command[DakUprStrs.module] == DakUprStrs.ht:
            # Heater command
            if self._isHTAvailable:
                isAvailable = True
                retVal = await self._ht(command)
                if command["cmd"][0] == DakUprStrs.htCmdON or command["cmd"][0] == DakUprStrs.htCmdOFF:
                    retVal = CommandRetCode.CMD_RETCODE_NOERR
        elif command[DakUprStrs.module] == DakUprStrs.sl:
            # Sleep command
            if self._isSleepEnable:
                isAvailable = True
                await uasyncio.sleep_ms(command["cmd"][0])
        elif command[DakUprStrs.module] == DakUprStrs.fa:
            if self._isFaAvailbale:
                isAvailable = True
                retVal = await self._fan(command)
        else:
            raise ValueError('Command is invalid')

        if isAvailable:
            exeTime = uasyncio.ticks() - t1
            if command[DakUprStrs.module] == DakUprStrs.vt:
                self.debugPrint(json.dumps(command) + " ,TOP Limit: {}, {} {}".format(self._vt_top_limit, retVal, exeTime))
            else:
                self.debugPrint(json.dumps(command) + " {} {}".format(retVal, exeTime))
        else:
            exeTime = 0

        return retVal, exeTime

    async def _kn(self, command):
        retVal = CommandRetCode.CMD_RETCODE_NOERR
        cmd = command["cmd"]
        if not cmd:
            raise ValueError('Command is invalid')

        if cmd[0] == "run":
            ## KN RUN command: ["run" speed]
            cmdSendRet = self.kn.run(cmd[1])
            if not cmdSendRet:
                self.debugPrint("KN run() sending failed")
                retVal = CommandRetCode.CMD_RETCODE_SENDERR
        elif cmd[0] == "set_dis_pos":
            ## KN set dispense position command: ["set_dis_pos" FLOUR/WATER/OIL]
            cmdSendRet = self.kn.set_dis_pos(cmd[1])
            if not cmdSendRet:
                self.debugPrint("KN set_dis_pos() sending failed")
                retVal = CommandRetCode.CMD_RETCODE_SENDERR
        elif cmd[0] == "stop":
            ## KN RUN command: ["stop"]
            cmdSendRet = self.kn.stop()
            if not cmdSendRet:
                self.debugPrint("KN stop() sending failed")
                retVal = CommandRetCode.CMD_RETCODE_SENDERR
        elif cmd[0] == "wait":
            ## KN wait command: ["wait"]
            retVal = await self.kn.wait()
        elif cmd[0] == "eject":
            ## KN RUN command: ["eject" state]
            cmdSendRet = self.kn.set_stirrer_state(cmd[1])
            if not cmdSendRet:
                self.debugPrint("KN set_stirrer_state() sending failed")
                retVal = CommandRetCode.CMD_RETCODE_SENDERR
        elif cmd[0] == "set_pos":
            ## KN wait command: ["set_pos"]
            cmdSendRet = self.kn.set_pos_spd(cmd[1], cmd[2])
            if not cmdSendRet:
                self.debugPrint("KN set_pos_spd() sending failed")
                retVal = CommandRetCode.CMD_RETCODE_SENDERR
        else:
            raise ValueError('Command is invalid')

        return retVal

    async def _vt(self, command):
        retVal = CommandRetCode.CMD_RETCODE_NOERR
        cmd = command["cmd"]
        if not cmd:
            raise ValueError('Command is invalid')

        if cmd[0] == "move_block" or cmd[0] == "move_nonblock":
            ## VT command: ["move_block"    position speed] or
            ##             ["move_nonblock" position speed]
            cmdSendRet = self.vt.move(0, cmd[1], cmd[2])
            if not cmdSendRet:
                self.debugPrint("VT move() sending failed")
                retVal = CommandRetCode.CMD_RETCODE_SENDERR
            else:
                # Only wait with move_block command
                if cmd[0] == "move_block":
                    retVal = await self.vt.wait()
        elif cmd[0] == "move_top" or cmd[0] == "move_topnonblock":
            cmdSendRet = self.vt.move(0, self._vt_top_limit - cmd[1], cmd[2])
            if not cmdSendRet:
                self.debugPrint("VT move() Top sending failed")
                retVal = CommandRetCode.CMD_RETCODE_SENDERR
            else:
                if cmd[0] == "move_top":
                    retVal = await self.vt.wait()

        elif cmd[0] == "wait":
            retVal = await self.vt.wait()
        else:
            raise ValueError('Command is invalid')

        return retVal

    async def _ds(self, command):
        retVal = CommandRetCode.CMD_RETCODE_NOERR
        cmd = command["cmd"]
        if not cmd:
            raise ValueError('Command is invalid')

        if cmd[0] == "weight":
            ## Command dispense by weight: ["weight" FLOUR/WATER/OIL weight speed]
            cmdSendRet = self.ds.dispense_by_weight(cmd[1], cmd[2], cmd[3])
        elif cmd[0] == "time":
            ## Command dispense by time: ["time" FLOUR/WATER/OIL time speed]
            cmdSendRet = self.ds.dispense_by_time(cmd[1], cmd[2], cmd[3])
        elif cmd[0] == "tare":
            self.ds.weight_tare()
            return retVal
        else:
            raise ValueError('Command is invalid')

        if not cmdSendRet:
            self.debugPrint("DS dispense sending failed")
            retVal = CommandRetCode.CMD_RETCODE_SENDERR
        else:
            retVal = await self.ds.wait(cmd[1])

        return retVal

    async def _wp(self, command):
        retVal = CommandRetCode.CMD_RETCODE_NOERR
        cmd = command["cmd"]
        if not cmd:
            return CommandRetCode.CMD_RETCODE_GENERR
        
        if cmd[0] == "asyn":
            retCmdVal = self.wp.move_absolute(cmd[1], cmd[2], cmd[3])
            if not retCmdVal:
                retVal = CommandRetCode.CMD_RETCODE_SENDERR
        elif cmd[0] == "wait":
            retVal = await self.wp.wait()
        elif cmd[0] == "press":
            retCmdVal = self.wp.move_press(cmd[1], cmd[2])
            if not retCmdVal:
                retVal = CommandRetCode.CMD_RETCODE_SENDERR
            else:
                retVal = await self.wp.wait()
        elif cmd[0] == "wedge":
            retCmdVal = self.wp.move_wedge(cmd[1], cmd[2])
            if not retCmdVal:
                retVal = CommandRetCode.CMD_RETCODE_SENDERR
            else:
                retVal = await self.wp.wait()
        else:
            retCmdVal = self.wp.move_absolute(cmd[0], cmd[1], cmd[2])
            if not retCmdVal:
                retVal = CommandRetCode.CMD_RETCODE_SENDERR
            else:
                retVal = await self.wp.wait()

        return retVal
    
    async def _kr(self, command):
        retVal = CommandRetCode.CMD_RETCODE_NOERR
        cmd = command["cmd"]
        if not cmd: 
            return CommandRetCode.CMD_RETCODE_GENERR
        
        ## absolute or relative move
        if cmd[0] == "abs": # absolute move
            retCmdVal = self.kr.move(0, cmd[1], cmd[2])
            if not retCmdVal:
                retVal = CommandRetCode.CMD_RETCODE_SENDERR
            else:
                retVal = await self.kr.wait()
        elif cmd[0] == "rel": # relative move
            krPos = self.kr.get_status().pos
            while krPos < cmd[1] - cmd[2]:
                retCmdVal = self.kr.move(1, cmd[2], cmd[3])
                if not retCmdVal:
                    retVal = CommandRetCode.CMD_RETCODE_SENDERR
                    break

                retVal = await self.kr.wait()
                if retVal != CommandRetCode.CMD_RETCODE_NOERR:
                    break

                ## Sleep
                if cmd[4] > 0:
                    await uasyncio.sleep_ms(cmd[4])

                krPos = self.kr.get_status().pos
            
            # Make the last move
            if krPos < cmd[1]:
                retCmdVal = self.kr.move(0, cmd[1], cmd[3])
                if not retCmdVal:
                    retVal = CommandRetCode.CMD_RETCODE_SENDERR
                else:
                    retVal = await self.kr.wait()
        else:
            return CommandRetCode.CMD_RETCODE_GENERR
        
        return retVal

    async def _ht(self, command):
        retVal = CommandRetCode.CMD_RETCODE_NOERR
        cmd = command["cmd"]
        if not cmd:
            return CommandRetCode.CMD_RETCODE_GENERR
        if cmd[0] == DakUprStrs.htCmdON:
            self.ht.set_state(cmd[1], 1, cmd[2], cmd[3], cmd[4])
        elif cmd[0] == DakUprStrs.htCmdOFF:
            self.ht.set_state(cmd[1], 0, 0, 0, 0)
        elif cmd[0] == DakUprStrs.htCmdLE:
            retVal = await self.ht.le(cmd[1], cmd[2], cmd[3])
        elif cmd[0] == DakUprStrs.htCmdGE:
            retVal = await self.ht.ge(cmd[1], cmd[2], cmd[3])
        elif cmd[0] == DakUprStrs.chCmdOn:
            self.ht.char_set_state_on(cmd[1], cmd[2])
        elif cmd[0] == DakUprStrs.chCmdOff:
            self.ht.char_set_state_off(cmd[1])
        elif cmd[0] == DakUprStrs.htCmdEQ:
            retVal = await self.ht.eq(cmd[1], cmd[2], cmd[3], cmd[4])
        return retVal

    async def _fan(self, command):
        cmd = command["cmd"]
        if not cmd:
            return CommandRetCode.CMD_RETCODE_GENERR
        if cmd[0] == "id":
            self.ht.set_fan_id(cmd[1], cmd[2])
        if cmd[0] == "pwm":
            self.ht.set_fan_pwm(cmd[1], cmd[2])
        else:
            self.ht.set_fan(cmd[0])
        return CommandRetCode.CMD_RETCODE_NOERR