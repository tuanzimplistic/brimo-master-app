"""
Contain test scripts for testing micropython submodule
"""
import sys
sys.path.insert('', "itor3_pyapp")

from master.verticaltray import *
from master.kneader import *
from master.dispenser import *
from master.kicker import *
from master.wedgepress import *

from master.itor3command import CommandRetCode
from master.kneader import KNConstants

import time

##
# @brief
#      This function test the VT.move() function.
#
# @details
#      This function moves the VT up and down continuosly and print out the VT moduls status.
#       The number of repeating, start position and end position are indicated by the input parameters.
#       Condition of this test is that the VT.find_datum() has been done successfully before.
#
# @code
#   # Command to run the test
#   vt_move_test(10, 3, 40, 10, 20)
# @endcode
#
# @param [in]
#       times: number of repeating times
# @param [in]
#       start_pos: Starting Position (mm)
# @param [in]
#       end_pos: Ending Position (mm)
# @param [in]
#       start2end_spd: Speed to move the VT from start positon to end positio
# @param [in]
#       end2start_spd: Speed to move the VT from end positon to start position
#
# @return
#      @arg    True: Successful.
#      @arg    False: Failed.
# ##
def vt_move_test(times, start_pos, end_pos, start2end_spd, end2start_spd):
    cnt = 0
    vt_obj = VT()
    retVal = False

    while cnt < times:
        print("VT move test no.: %d" % (cnt))
        if cnt % 2 == 0:
            print("VT move from %d to %d with speed: %d" % (start_pos, end_pos, start2end_spd))
            retVal = vt_obj.move(0, end_pos, start2end_spd)
        else:
            print("VT move from %d to %d with speed: %d" % (end_pos, start_pos, end2start_spd))
            retVal = vt_obj.move(0, start_pos, end2start_spd)
        if retVal != True:
            print("move command sending failed")
            return False
        # print the VT status
        status = vt_obj.get_status()
        
        while status.state == vt_obj.VT_MOVING_STATE:
            print(status)
            status = vt_obj.get_status()
            time.sleep_ms(100)
        if status.state != vt_obj.VT_IDLE_STATE:
            print("VT move failed")
            return False
        cnt += 1
    
    return True


##
# @brief
#      This function test the KR.move() function.
#
# @details
#      This function moves the KR backward and forward continuosly and print out the KR status.
#       The number of repeating, start position and end position are indicated by the input parameters.
#       Condition of this test is that the KR.find_home() has been done successfully before.
#
# @code
#   # Command to run the test
#   kr_move_test(10, 0, 310, 50, 100)
# @endcode
#
# @param [in]
#       times: number of repeating times
# @param [in]
#       start_pos: Starting Position (mm)
# @param [in]
#       end_pos: Ending Position (mm)
# @param [in]
#       start2end_spd: Speed to move the VT from start positon to end positio
# @param [in]
#       end2start_spd: Speed to move the VT from end positon to start position
#
# @return
#      @arg    True: Successful.
#      @arg    False: Failed.
# ##
def kr_move_test(times, start_pos, end_pos, start2end_spd, end2start_spd):
    cnt = 0
    kr = KR()
    retVal = False

    while cnt < times:
        print("KR move test no.: %d" % (cnt))
        if cnt % 2 == 0:
            print("KR move from %d to %d with speed: %d" % (start_pos, end_pos, start2end_spd))
            retVal = kr.move(0, end_pos, start2end_spd)
        else:
            print("KR move from %d to %d with speed: %d" % (end_pos, start_pos, end2start_spd))
            retVal = kr.move(0, start_pos, end2start_spd)
        if retVal != True:
            print("move command sending failed")
            return False
        # print the VT status
        status = kr.get_status()

        while status.state == kr.KR_STATE_MOVING:
            print(status)
            status = kr.get_status()
            time.sleep_ms(100)
        if status.state != kr.KR_STATE_IDLE:
            print("VT move failed, state: %d" % status.state)
            return False
        cnt += 1

    return True

##
# @brief
#      This function test the DS.dispense_by_time() function.
#
# @details
#      This function dispense FLOUR/WATER/OIL based on input parameter. The function will dispense in the period
#       of time set by user and displays the loadcell every input ms.
#
# @code
#   # Command to run the test, for example FLOUR dispensing in 5000 ms
#   # and display loadcell value every 100 ms. The speed of flour dispensing is 3rad/s
#   ds_dispense_by_time_test(ds.DS_ID_FLOUR, 3, 5000, 100)
#
#   # Command to run the test, for example WATER dispensing in 5000 ms
#   # and display loadcell value every 100 ms. The speed of WATER dispensing is dummy.
#   ds_dispense_by_time_test(ds.DS_ID_WATER, 0, 5000, 100)
# @endcode
#
# @param [in]
#       id: DS.DS_ID_FLOUR, DS.DS_ID_WATER or DS.DS_ID_OIL
# @param [in]
#       spd: flour motor speed (rad/s), only applicable for FLOUR dispensing
# @param [in]
#       totaltime: dispensing time (ms)
# @param [in]
#       timeslice: time slice to print the load cell (ms)
#
# @return
#      @arg    True: Successful.
#      @arg    False: Failed.
# ##
def ds_dispense_by_time_test(id, spd, totaltime, timeslice):
    ds = DS()
    retVal = ds.dispense_by_time(id, spd, totaltime)
    if not retVal:
        print("ds_dispense_by_time_test() failed")
        return False
    status = ds.get_status(id)
    cnt = totaltime / timeslice
    while cnt > 0:
        print(status)
        status = ds.get_status(id)
        cnt -= 1

    return True

##
# @brief
#      Coroutine to test the VT move function
#
# @details
#      This function test the function master.verticaltray.VT.move() function. This function will move the VT
#       based on the input parameter and wait until complete.
#
# @param [in]
#       pos: VT Position
# @param [in]
#       spd: Speed to move the VT
#
# @return
#      @arg    mater.itor3command.CommandRetCode
# ##
async def vt_wait_test(pos, spd):
    vt = VT()
    retCmdSend = vt.move(0, pos, spd)
    if not retCmdSend:
        print("VT move command send failed")
        return CommandRetCode.CMD_RETCODE_SENDERR
    retVal = await vt.wait()
    return retVal

##
# @brief
#      Coroutine to test the KR move function
#
# @details
#      This function test the function master.kicker.KR.move() function. This function will move the KR
#       based on the input parameter and wait until complete.
#
# @param [in]
#       pos: KR Position
# @param [in]
#       spd: Speed to move the KR
#
# @return
#      @arg    mater.itor3command.CommandRetCode
# ##
async def kr_wait_test(pos, spd):
    kr = KR()
    retCmdSend = kr.move(0, pos, spd)
    if not retCmdSend:
        print("KR move send failed")
        return CommandRetCode.CMD_RETCODE_SENDERR
    retVal = await kr.wait()
    return retVal

##
# @brief
#      Coroutine to test the WP move function
#
# @details
#      This function test the function master.wedgepress.WP.move_absoblute() function. This function will move the WP
#       based on the input parameter and wait until complete.
#
# @param [in]
#       prePos: Press Position
# @param [in]
#       wedPos: Pivot Position
# @param [in]
#       desire_time: Desized time
#
# @return
#      @arg    mater.itor3command.CommandRetCode
# ##
async def wp_wait_test(prePos, wedPos, desire_time):
    wp = WP()
    retCmdSend = wp.move_absolute(prePos, wedPos, desire_time)
    if not retCmdSend:
        print("WP move send failed")
        return CommandRetCode.CMD_RETCODE_SENDERR
    retVal = await wp.wait()
    return retVal

async def kn_set_dis_pos(id):
    kn = KN()
    retVal = CommandRetCode.CMD_RETCODE_NOERR
    knCmdSend = kn.set_dis_pos(id)
    if not knCmdSend:
        retVal = CommandRetCode.CMD_RETCODE_SENDERR
        print("KN set_dis_pos sending failed")
        return retVal
    retVal = await kn.wait()
    if retVal != CommandRetCode.CMD_RETCODE_NOERR:
        print("KN set_dis_pos failed")
        return retVal

async def kn_alt_test(no_test):
    kn = KN()
    cnt = 1
    retVal = CommandRetCode.CMD_RETCODE_NOERR
    while cnt <= no_test:
        print("Test case no. starts" % cnt)
        retVal = kn_set_dis_pos(kn.FLOUR_ID)
        if retVal != CommandRetCode.CMD_RETCODE_NOERR:
            return retVal
        retVal = kn_set_dis_pos(kn.WATER_ID)
        if retVal != CommandRetCode.CMD_RETCODE_NOERR:
            return retVal
        retVal = kn_set_dis_pos(kn.OIL_ID)
        if retVal != CommandRetCode.CMD_RETCODE_NOERR:
            return retVal

        kn.run(20)
        await uasyncio.sleep_ms(2000)
        kn.stop()
        print("Test case no. ends" % cnt)

    return retVal