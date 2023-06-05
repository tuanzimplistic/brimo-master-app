import time
import gui
from itor3_pyapp.master.verticaltray import *
from itor3_pyapp.master.kicker import *
from itor3_pyapp.master.wedgepress import *
from itor3_pyapp.master.kneader import *
from itor3_pyapp.master.heater import *
from itor3_pyapp.master.dispenser import *


def assembly_wait(assembly, display_name: str, idle_code, ping_delay = 10):
    """
    Wait until an assembly return a idle state. Delay between ping in ms.

    Return: last status
    """
    try:
        status = assembly.get_status()
        while status.state != idle_code:
            print(f'{display_name} status: {status}')
            status = assembly.get_status()
            time.sleep_ms(ping_delay)
        return status
    except:
        return None

def assembly_run(assembly, display_name: str, method_name: str, *args):
    print(f'{display_name} {method_name}')
    method = getattr(assembly, method_name)
    output = method(*args)
    print(f'{display_name} {method_name} {args} -> {output}')

def KR_check_position(kr: KR, lower, upper):
    """
    To check whether the KR postion is within specification.
    """
    try:
        status = kr.get_status()
        output = lower <= status.pos <= upper
        if output:
            print(f'KR position is {status.pos}, within {lower} and {upper}')
        else:
            print(f'KR position is {status.pos}, not within {lower} and {upper}')
        return output
    except:
        print(f'Failed to check KR position to be within {lower} and {upper}')
        return False

def KR_wait_find_home(kr: KR):
    """
    To wait until KR successfully find_home.
    """
    status = assembly_wait(kr, 'KR', kr.KR_STATE_IDLE)

    success = not status.flags & kr.KR_BIT_FIND_DATUM_ERR
    success = success and (not status.flags & kr.KR_BIT_MOTOR_ERR)
    success = success and (status.flags & kr.KR_BIT_DATUM_KNOWN)
    return success

def KR_wait_move(kr: KR, lower, upper):
    """
    To wait for KR to move to a position
    """
    status = assembly_wait(kr, 'KR', kr.KR_STATE_IDLE)

    success = KR_check_position(kr, lower, upper)
    success = success and (not status.flags & kr.KR_BIT_MOTOR_ERR)
    return success

def VT_wait_find_datum(vt: VT):
    """
    To wait until VT successfully find_datum.
    """
    status = assembly_wait(vt, 'VT', vt.VT_IDLE_STATE)

    success = not status.flags & vt.VT_FIND_DATUM_ERR_BIT
    success = success and (not status.flags & vt.VT_MOTOR_ERR_BIT)
    success = success and (status.flags & vt.VT_DATUM_KNOWN)
    return success

def VT_wait_position(vt: VT, lower: int, upper: int):
    """
    To wait until VT get into a range of potision.
    """
    status = assembly_wait(vt, 'VT', vt.VT_IDLE_STATE)
    if status is not None:
        success = (lower <= status.pos <= upper)
        return success
    else:
        return False

def WP_wait_find_datum(wp: WP):
    """
    To wait until WP successfully find_datum.
    """
    status = assembly_wait(wp, 'WP', wp.WP_STATE_IDLE)
    
    success = not status.flags & wp.WP_BIT_MOVE_ERR
    success = success and (not status.flags & wp.WP_BIT_FIND_DATUM_ERR)
    success = success and (not status.flags & wp.WP_BIT_PRESS_LINK_ERR)
    success = success and (not status.flags & wp.WP_BIT_PIVOT_LINK_ERR)
    success = success and (status.flags & wp.WP_BIT_PRESS_DATUM_KNOWN)
    success = success and (status.flags & wp.WP_BIT_PIVOT_DATUM_KNOWN)
    return success

def WP_wait_move_absolute(wp: WP, press_pos_lower, press_pos_upper, pivot_pos_lower, pivot_pos_upper):
    status = assembly_wait(wp, 'WP', wp.WP_STATE_IDLE)
    
    success = (press_pos_lower <= status.press_gap <= press_pos_upper) and (pivot_pos_lower <= status.pivot_gap <= pivot_pos_upper)
    success = success and (not status.flags & wp.WP_BIT_MOVE_ERR)
    success = success and (not status.flags & wp.WP_BIT_PRESS_LINK_ERR)
    success = success and (not status.flags & wp.WP_BIT_PIVOT_LINK_ERR)
    return success

def calibrate_vt_kr():
    """
    Executed when starting the Rotimatic or starting to make a new roti
    """

    vt = VT()
    vt.clear_error_flags()

    kr = KR()
    kr.clear_error_flags()

    wp = WP()
    wp.clear_error_flags()

    try:
        # Failed to find_datum could either raise a ValueError (no response from VT)
        # or a False value (failed to find_datum)
        # vt.find_datum()
        
        # KR must do find_home at boot to know its position
        # print('VT find_datum')
        success = vt.find_datum_and_limit()
        if not success: 
            return False

        # wp.find_datum(3)
        # print('WP find_datum')
        assembly_run(wp, 'WP', 'find_datum', 3)
        success = WP_wait_find_datum(wp)
        if not success: 
            return False
        
        # args = (55,55,5000)
        # wp.move_absolute(*args)
        # print(f'WP mote_absolute {args}')
        assembly_run(wp, 'WP', 'move_absolute', 55,55,5000)
        success = WP_wait_move_absolute(wp, 54, 56, 54, 56)
        if not success: 
            return False
        
        # kr.find_home()
        # print('KR find_home')
        assembly_run(kr, 'KR', 'find_home')
        success = KR_wait_find_home(kr)
        if not success:
            return False

    except ValueError as e:
        return False

########################################
# Check for dough residue              #
########################################
def check_dough_residue(weight_change = 2, kr = None, vt = None, wp = None, ds = None, kn = None, callback = gui):
    """
    Check for dough residue before making a roti.

    Params:
     - weight_increase: percentage of starting weight
    """
    if kr is None:
        kr = KR()
        kr.clear_error_flags()

    if vt is None:
        vt = VT()
        vt.clear_error_flags()
    
    if wp is None:
        wp = WP()
        wp.clear_error_flags()

    if ds is None:
        ds = DS()
        ds.clear_error_flags(ds.DS_ID_FLOUR)
        ds.clear_error_flags(ds.DS_ID_WATER)
        ds.clear_error_flags(ds.DS_ID_OIL)

    if kn is None:
        kn = KN()
        kn.clear_error_flags()
    
    # Move VT to dispensing location
    assembly_run(vt, 'VT', 'move', 0, 26, 20)   # is_relative, position, speed
    success = VT_wait_position(vt, 25, 27)
    if not success:
        return False

    # Get weight of the cup
    weight_start = ds.get_status(ds.DS_ID_FLOUR).weight
    weight_upper = weight_start + weight_change
    weight_lower = weight_start - weight_change
    weight_max = weight_start
    print(f'Initial weight {weight_start}')

    # Rotate stirrer 1 at speed 15 for 2s
    assembly_run(kn, 'KN', 'run', 15)           # at speed 15
    time_start = time.time()
    have_residue = False

    while time.time() - time_start < 2:
        status = ds.get_status(ds.DS_ID_FLOUR)
        print(f'DS {ds.DS_ID_FLOUR} {status}')
        if status.weight < weight_lower or status.weight > weight_upper:
            have_residue = True
            break
        time.sleep_ms(10)

    assembly_run(kn, 'KN', 'stop')

    if have_residue:
        # Wait for the UPR to finish the roti cooking
        assembly_wait(vt, 'VT', vt.VT_IDLE_STATE)

        # Make sure the KR is in a safe position, 330 front
        assembly_run(kr, 'KR', 'move', 0, 330, 20)          # new is 330, old is 146 is_relative, pos, speed
        success = KR_wait_move(kr, 329, 331)
        if not success:
            return False

        # Move WP to 30, 30
        assembly_run(wp, 'WP', 'move_absolute', 30, 30, 5000)
        success = WP_wait_move_absolute(wp, 29, 31, 29, 31)
        if not success:
            return False

        # Move VT to eject position
        assembly_run(vt, 'VT', 'move', 0, 30, 20)
        success = VT_wait_position(vt, 29, 31)
        if not success:
            return False

        # Move DS to water pos
        time.sleep_ms(500)
        assembly_run(ds, 'DS', 'dispense_by_time', ds.DS_ID_WATER, 500, 0) # for 500ms, speed not applicable
        time.sleep_ms(500)

        # Dislodge stirrer
        # The ejection sequence is from Utils.py
        retry = 0
        while True:
            assembly_run(kn, 'KN', 'set_stirrer_state', 0)      # eject stirrer
            assembly_run(kn, 'KN', 'run', 5)
            time.sleep_ms(1000)
            assembly_run(kn, 'KN', 'stop')
            time.sleep_ms(500)
            assembly_run(kn, 'KN', 'set_stirrer_state', 1)

            # Check if stirrer ejected successfully
            # https://github.com/Zimplistic/itor3-master-pyapp/blob/bbe6ffdfe2f18cee0881fee23b2bcb9d1548d8fc/util/Utils.py#L89
            # if kn.get_status().flags == kn.KN_BIT_DISLODGE_POS_ERR:
            #     return False
            weight_current = ds.get_status(ds.DS_ID_FLOUR).weight
            success = weight_current < weight_start
            if success:
                break
            else:
                if retry < 3:
                    retry += 1
                    continue
                else:
                    return False
            
        # Move VT to 8
        assembly_run(vt, 'VT', 'move', 0, 8, 20)
        success = VT_wait_position(vt, 7, 9)
        if not success:
            return False
        
        # Prompt user to clean up stirrer
        if callback is not None:
            callback.notify('Please turn of the Rotimatic and clean up the cup before making another roti.')
        else:
            print('Dough residue detected.')

        return True

#############################
## Kicker parking/cleaning ##
#############################
def after_cooking(kr = None, vt = None, wp = None, kn = None, ht = None, sleep = 420, callback = None):
    """
    Called after roti making
    """
    if kr is None:
        kr = KR()
        kr.clear_error_flags()
    
    if vt is None:
        vt = VT()
        vt.clear_error_flags()
    
    if wp is None:
        wp = WP()
        wp.clear_error_flags()

    if kn is None:
        kn = KN()
        kn.clear_error_flags()
    
    if ht is None:
        ht = HT()
        ht.clear_error_flags(ht.HT_ID_TOP)
        ht.clear_error_flags(ht.HT_ID_BTM)
        ht.clear_error_flags(ht.HT_ID_CHARTOP)
        ht.clear_error_flags(ht.HT_ID_CHARBTM)

    # VT must be in a safe position
    assembly_run(vt, 'VT', 'move', 0, 45, 20)
    success = VT_wait_position(vt, 44, 46)
    if not success:
        print('VT failed to reach the designated position.')
        if callback is not None:
            callback()      # Notify user
        return

    # WP must be in a safe position
    assembly_run(wp, 'WP', 'move_absolute', 55, 55, 5000)
    success = WP_wait_move_absolute(wp, 54, 56, 54, 56)
    if not success:
        print(f'WP is not in a safe position.')
        if callback is not None:
            callback()      # Notify user
        return

    # Move KR to home
    assembly_run(kr, 'KR', 'find_home')
    success = KR_wait_find_home(kr)
    if not success:
        print('KR failed to find_home.')
        if callback is not None:
            callback()      # Notify user
        return False

    # Move WP
    assembly_run(wp, 'WP', 'move_absolute', 20, 20, 5000)
    success = WP_wait_move_absolute(wp, 19, 21, 19, 21)
    if not success:
        print('WP failed to mote to the designated position.')
        if callback is not None:
            callback()      # Notify user
        return False

    # Turn on all fans
    for fan_id in range(4):
        assembly_run(ht, 'HT', 'set_fan_pwm', fan_id, 100)
        time.sleep(1)
    
    # Sleep 7 mins
    time.sleep(sleep)
    
    # Turn off all fans
    for fan_id in range(4):
        assembly_run(ht, 'HT', 'set_fan_pwm', fan_id, 0)
        time.sleep(1)

    # KN eject stirrer
    assembly_run(kn, 'KN', 'set_stirrer_state', 0)      # eject stirrer
    assembly_run(kn, 'KN', 'run', 5)
    time.sleep_ms(1000)
    assembly_run(kn, 'KN', 'stop')
    time.sleep_ms(500)
    assembly_run(kn, 'KN', 'set_stirrer_state', 1)

    # Move WP
    assembly_run(wp, 'WP', 'move_absolute', 55, 55, 5000)
    success = WP_wait_move_absolute(wp, 54, 56, 54, 56)
    if not success:
        print('WP failed to mote to thee designated position.')
        if callback is not None:
            callback()      # Notify user
        return False

    # Move KR to front
    assembly_run(kr, 'KR', 'move', 0, 330, 100)
    success = KR_wait_move(kr, 329, 331)
    if not success:
        print('KR failed to move to the front position.')
        if callback is not None:
            callback()      # Notify user
        return False

    # Move WP
    assembly_run(wp, 'WP', 'move_absolute', 20, 20, 5000)
    success = WP_wait_move_absolute(wp, 19, 21, 19, 21)
    if not success:
        print('WP failed to mote to designated position.')
        if callback is not None:
            callback()      # Notify user
        return False

    # Move VT
    assembly_run(vt, 'VT', 'move', 0, 8, 20)
    success = VT_wait_position(7, 9)
    if not success:
        print('VT failed to reach designated position.')
        if callback is not None:
            callback()      # Notify user
        return False
    
     # Move WP
    assembly_run(wp, 'WP', 'move_absolute', 40, 20, 5000)
    success = WP_wait_move_absolute(wp, 39, 41, 19, 21)
    if not success:
        print('WP failed to mote to designated position.')
        if callback is not None:
            callback()      # Notify user
        return False
    
    # Move KR to front
    assembly_run(kr, 'KR', 'move', 0, 300, 50)
    success = KR_wait_move(kr, 299, 301)
    if not success:
        print('KR failed to move to the dsignated position.')
        if callback is not None:
            callback()      # Notify user
        return False
