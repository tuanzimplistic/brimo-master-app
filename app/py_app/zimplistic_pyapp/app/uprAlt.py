import time

from itor3_pyapp.app.common import *
from itor3_pyapp.app.dak import *
from itor3_pyapp.app.upr import *

vt = VT()
kn = KN()
ds = DS()

wp = WP()
ht = HT()
kr = KR()

async def PressingAlt(roti, no_test, break_time):
    cnt = 1
    while cnt <= no_test:
        retVal, pressTime = await roti.myUpr._performPressing()
        if retVal != CommandRetCode.CMD_RETCODE_NOERR:
            print("Pressing ALT Fail, cnt: {}".format(cnt))
            break
        await roti.myUpr._exe_conf([
                    {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdOFF, HT.HT_ID_TOP]},
                    {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdOFF, HT.HT_ID_BTM]}
                ])
        await uasyncio.sleep_ms(break_time)
        print("Pressing ALT PASS, cnt: {}, exeTime: {}".format(cnt, pressTime + break_time))
        cnt += 1

    await roti.myUpr._exe_conf([
                    {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdOFF, HT.HT_ID_TOP]},
                    {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdOFF, HT.HT_ID_BTM]}
                ])

async def RoastingAlt(roti, no_test, break_time):
    cnt = 1
    while cnt <= no_test:
        retVal, roastTime = await roti.myUpr._performRoasting()
        if retVal != CommandRetCode.CMD_RETCODE_NOERR:
            print("Roasting ALT Fail, cnt: {}".format(cnt))
            break
        
        retVal, coolTime = await roti.myUpr._performCooling()
        if retVal != CommandRetCode.CMD_RETCODE_NOERR:
            print("Roasting ALT Fail, cnt: {}".format(cnt))
            break

        await uasyncio.sleep_ms(break_time)

        print("Roasting ALT PASS, cnt: {}, exeTime: {}".format(cnt, roastTime + coolTime + break_time))
        cnt += 1
    
    await roti.myUpr._exe_conf([
                {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdOFF, HT.HT_ID_TOP]},
                {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdOFF, HT.HT_ID_BTM]}
            ])

async def UPRAlt(roti, no_test, break_time):
    cnt = 1
    while cnt <= no_test:
        retVal, pressTime = await roti.myUpr._performPressing()
        if retVal != CommandRetCode.CMD_RETCODE_NOERR:
            print("UPR ALT Fail, cnt: {}".format(cnt))
            break

        retVal, roastTime = await roti.myUpr._performRoasting()

        if retVal != CommandRetCode.CMD_RETCODE_NOERR:
            print("UPR ALT Fail, cnt: {}".format(cnt))
            break

        retVal, coolTime = await roti.myUpr._performCooling()
        if retVal != CommandRetCode.CMD_RETCODE_NOERR:
            print("UPR ALT Fail, cnt: {}".format(cnt))
            break

        await uasyncio.sleep_ms(break_time)

        print("UPR ALT PASS, cnt: {}, exeTime: {}".format(cnt, pressTime + roastTime + coolTime + break_time))
        cnt += 1

    await roti.myUpr._exe_conf([
                {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdOFF, HT.HT_ID_TOP]},
                {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdOFF, HT.HT_ID_BTM]}
            ])
            
async def heaterTopProfile(roti, no_test, break_time):
    cnt = 1
    while cnt <= no_test:
        await roti.myUpr._exe_conf([
            {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdON, HT.HT_ID_TOP, 130, 0.8, 120]},
            {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdGE, HT.HT_ID_TOP, 130, 30000]},
            {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdOFF, HT.HT_ID_TOP]},
            {DakUprStrs.module : DakUprStrs.sl, "cmd" : [1000]},
            {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdON, HT.HT_ID_TOP, 275, 1.0, 265]},
            # Added to simulate Charring Time
            {DakUprStrs.module : DakUprStrs.sl, "cmd" : [58000]},
            {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdOFF, HT.HT_ID_TOP]},
        ])

        await uasyncio.sleep_ms(break_time)

        cnt += 1

    await roti.myUpr._exe_conf([
                {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdOFF, HT.HT_ID_TOP]}
            ])
            
async def heaterBtmProfile(roti, no_test, break_time):
    cnt = 1
    while cnt <= no_test:
        await roti.myUpr._exe_conf([
            {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdON, HT.HT_ID_BTM, 110, 0.8, 100]},
            {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdGE, HT.HT_ID_BTM, 110, 30000]},
            {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdON, HT.HT_ID_BTM, 280, 0.6, 270]},
            # Added to simulate Charring Time
            {DakUprStrs.module : DakUprStrs.sl, "cmd" : [58000]},
            {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdOFF, HT.HT_ID_BTM]},
        ])

        await uasyncio.sleep_ms(break_time)

        cnt += 1

    await roti.myUpr._exe_conf([
                {DakUprStrs.module : DakUprStrs.ht, "cmd" : [DakUprStrs.htCmdOFF, HT.HT_ID_BTM]}
            ])