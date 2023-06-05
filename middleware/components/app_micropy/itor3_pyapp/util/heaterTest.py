import time

from itor3_pyapp.app.common import *
from itor3_pyapp.app.dak import *
from itor3_pyapp.app.upr import *

## TODO: remove these lines for production code
vt = VT()
kn = KN()
ds = DS()

wp = WP()
ht = HT()
kr = KR()

WAIT_TIM_IN_MS = 200

def heaterRamup(ht, id, temp, dutyMax, dutyThreshold, coolTemp):
    if id == HT.HT_ID_TOP:
        start_temp = ht.get_status().ht1_temp
    elif id == HT.HT_ID_BTM:
        start_temp = ht.get_status().ht2_temp

    cnt = 0
    cur_temp = start_temp
    ht.set_state(id, 1, temp, dutyMax, dutyThreshold)
    while cur_temp < temp - 10:
    #while cnt < 200:
        time.sleep_ms(1000)
        cnt += 1
        if id == HT.HT_ID_TOP:
            cur_temp = ht.get_status().ht1_temp
        elif id == HT.HT_ID_BTM:
            cur_temp = ht.get_status().ht2_temp
        print("Heating - Temp: {}, cnt: {}".format(cur_temp, cnt))

    print("Start Temp: {}, End Temp: {}, Time: {}, Ramp up: {}".format(start_temp,
            cur_temp,
            cnt,
            (cur_temp - start_temp) / (cnt)
        ))

    ## Cooling down
    start_temp = cur_temp
    ht.set_state(id, 0, temp, dutyMax, dutyThreshold)
    # ht.set_fan(0xff)
    ht.set_fan_pwm(0x00, 100)
    ht.set_fan_pwm(0x01, 100)
    ht.set_fan_pwm(0x02, 100)
    ht.set_fan_pwm(0x03, 100)
    cnt = 0
    while cur_temp > coolTemp:
        cnt += 1
        if id == HT.HT_ID_TOP:
            cur_temp = ht.get_status().ht1_temp
        elif id == HT.HT_ID_BTM:
            cur_temp = ht.get_status().ht2_temp
        print("Cooling - Temp: {}, cnt: {}".format(cur_temp, cnt))
        time.sleep_ms(1000)

    print("Start Temp: {}, End Temp: {}, Time: {}, Cooling rate: {}".format(start_temp,
            cur_temp,
            cnt,
            (start_temp - cur_temp) / (cnt)
        ))
       
    ht.set_fan_pwm(0x00, 0)
    ht.set_fan_pwm(0x01, 0)
    ht.set_fan_pwm(0x02, 0)
    ht.set_fan_pwm(0x03, 0)
    
def heaterRamupMiddle(ht, id, start_cnt_temp, end_cnt_temp, set_temp, dutyMax, dutyThreshold, coolTemp):
    # ht.set_fan(0x00)
    ht.set_fan_pwm(0x00, 0)
    ht.set_fan_pwm(0x01, 0)
    ht.set_fan_pwm(0x02, 0)
    ht.set_fan_pwm(0x03, 0)
    ht.set_state(id, 1, set_temp, dutyMax, dutyThreshold)
    
    if id == HT.HT_ID_TOP:
        start_temp = ht.get_status().ht1_temp
    elif id == HT.HT_ID_BTM:
        start_temp = ht.get_status().ht2_temp

    cnt = 0
    cur_temp = start_temp
    
    t1 = 0
    t2 = 0
    
    while cur_temp < set_temp - 10:
    #while cnt < 200:
        time.sleep_ms(WAIT_TIM_IN_MS)
        if id == HT.HT_ID_TOP:
            cur_temp = ht.get_status().ht1_temp
        elif id == HT.HT_ID_BTM:
            cur_temp = ht.get_status().ht2_temp
        
        if cur_temp < start_cnt_temp:
            print("Heating - Temp: {}".format(cur_temp))
            
        elif cur_temp > end_cnt_temp:
            t2 = time.time()
            cnt += 1
            print("Heating - Temp: {}, cnt: {}".format(cur_temp, cnt))
            break
            
        else:
            if cnt == 0:
                t1 = time.time()
                start_temp = cur_temp
            cnt += 1
            print("Heating - Temp: {}, cnt: {}".format(cur_temp, cnt))
        
    print("Start Temp: {}, End Temp: {}, Time: {}, Ramp up: {}".format(start_temp,
            cur_temp,
            cnt,
            ((cur_temp - start_temp) / (t2 - t1))
        ))

    ## Cooling down
    start_temp = cur_temp
    ht.set_state(id, 0, 0, 0, 0)
    # ht.set_fan(0xff)
    ht.set_fan_pwm(0x00, 100)
    ht.set_fan_pwm(0x01, 100)
    ht.set_fan_pwm(0x02, 100)
    ht.set_fan_pwm(0x03, 100)
    cnt = 0
    while cur_temp > coolTemp:
        cnt += 1
        if id == HT.HT_ID_TOP:
            cur_temp = ht.get_status().ht1_temp
        elif id == HT.HT_ID_BTM:
            cur_temp = ht.get_status().ht2_temp
        print("Cooling - Temp: {}, cnt: {}".format(cur_temp, cnt))
        time.sleep_ms(1000)

    print("Start Temp: {}, End Temp: {}, Time: {}, Cooling rate: {}".format(start_temp,
            cur_temp,
            cnt,
            (start_temp - cur_temp) / (cnt)
        ))
       
    # ht.set_fan(0x00)
    ht.set_fan_pwm(0x00, 0)
    ht.set_fan_pwm(0x01, 0)
    ht.set_fan_pwm(0x02, 0)
    ht.set_fan_pwm(0x03, 0)

# TOP w/o FAN: heaterRamupMiddleFanPWM(ht, 0, 100, 200, 290, 1.0, 290, 80, 2, 0, 0)
# BTM w/o FAN: heaterRamupMiddleFanPWM(ht, 1, 100, 200, 290, 1.0, 290, 80, 3, 0, 0) 
# TOP wth FAN: heaterRamupMiddleFanPWM(ht, 0, 100, 200, 290, 1.0, 290, 80, 2, 10, 1000)
# BTM wth FAN: heaterRamupMiddleFanPWM(ht, 1, 100, 200, 290, 1.0, 290, 80, 3, 10, 1000) 
def heaterRamupMiddleFanPWM(ht, id, start_cnt_temp, end_cnt_temp, set_temp, dutyMax, dutyThreshold, coolTemp, fan_id, fan_pwm, offTimeMs):
    ht.set_fan_pwm(fan_id, fan_pwm)
    ht.set_state(id, 1, set_temp, dutyMax, dutyThreshold)
    
    if id == HT.HT_ID_TOP:
        start_temp = ht.get_status().ht1_temp
    elif id == HT.HT_ID_BTM:
        start_temp = ht.get_status().ht2_temp

    cnt = 0
    cur_temp = start_temp
    off_cnt = round(offTimeMs / WAIT_TIM_IN_MS)
    off_cur_cnt = 0
    
    t1 = 0
    t2 = 0
    
    while cur_temp < set_temp - 10:
    #while cnt < 200:
        time.sleep_ms(WAIT_TIM_IN_MS)
        off_cur_cnt += 1
        if off_cur_cnt >= off_cnt:
            ht.set_fan_pwm(fan_id, fan_pwm)
            off_cur_cnt = 0
        else:
            ht.set_fan_pwm(fan_id, 0)

        if id == HT.HT_ID_TOP:
            cur_temp = ht.get_status().ht1_temp
        elif id == HT.HT_ID_BTM:
            cur_temp = ht.get_status().ht2_temp
        
        if cur_temp < start_cnt_temp:
            print("Heating - Temp: {}".format(cur_temp))
            
        elif cur_temp > end_cnt_temp:
            t2 = time.time()
            cnt += 1
            print("Heating - Temp: {}, cnt: {}".format(cur_temp, cnt))
            break
            
        else:
            if cnt == 0:
                t1 = time.time()
                start_temp = cur_temp
            cnt += 1
            print("Heating - Temp: {}, cnt: {}".format(cur_temp, cnt))
        
    print("Start Temp: {}, End Temp: {}, Time: {}, Ramp up: {}".format(start_temp,
            cur_temp,
            t2 - t1,
            ((cur_temp - start_temp) / (t2 - t1))
        ))

    ## Cooling down
    start_temp = cur_temp
    ht.set_state(id, 0, 0, 0, 0)
    ht.set_fan_pwm(fan_id, 100)
    cnt = 0
    while cur_temp > coolTemp:
        cnt += 1
        if id == HT.HT_ID_TOP:
            cur_temp = ht.get_status().ht1_temp
        elif id == HT.HT_ID_BTM:
            cur_temp = ht.get_status().ht2_temp
        print("Cooling - Temp: {}, cnt: {}".format(cur_temp, cnt))
        time.sleep_ms(1000)

    print("Start Temp: {}, End Temp: {}, Time: {}, Cooling rate: {}".format(start_temp,
            cur_temp,
            cnt,
            (start_temp - cur_temp) / (cnt)
        ))
       
    ht.set_fan_pwm(fan_id, 0)

def rampUpTestRun(no_test, ht, id, start_cnt_temp, end_cnt_temp, set_temp, dutyMax, dutyThreshold, coolTemp, fan_id, fan_pwm, offTimeMs):
    cnt = 1
    while cnt <= no_test:
        print("Test No.{}".format(cnt))
        heaterRamupMiddleFanPWM(ht, id, start_cnt_temp, end_cnt_temp, set_temp, dutyMax, dutyThreshold, coolTemp, fan_id, fan_pwm, offTimeMs)
        print("\n")
        cnt += 1

    print("End Test")
        
def heaterRamupTwoHeaters(ht, temp_top, dutyMax_top, dutyThreshold_top, temp_btm, dutyMax_btm, dutyThreshold_btm):
    start_temp_top = ht.get_status().ht1_temp
    start_temp_btm = ht.get_status().ht2_temp

    cnt = 0
    cur_temp_top = start_temp_top
    cur_temp_btm = start_temp_btm
    ht.set_state(HT.HT_ID_TOP, 1, temp_top, dutyMax_top, dutyThreshold_top)
    ht.set_state(HT.HT_ID_BTM, 1, temp_btm, dutyMax_btm, dutyThreshold_btm)
    # while cur_temp_top < temp_top or cur_temp_btm < temp_btm:
    while cnt < 200:
        cnt += 1
        cur_temp_top = ht.get_status().ht1_temp
        cur_temp_btm = ht.get_status().ht2_temp
        print("Heating - TOP: {}, BTM: {}, cnt: {}".format(cur_temp_top, cur_temp_btm, cnt))
        time.sleep_ms(1000)

    print("Start TOP: {}, End TOP: {}, Ramp up: {}\n \
            Sart BTM: {}, End BTM: {}, Ramp up: {}\n \
            Time: {}".format(start_temp_top, cur_temp_top, (cur_temp_top - start_temp_top) / (cnt),
            start_temp_btm, cur_temp_btm, (cur_temp_btm - start_temp_btm) / (cnt),
            cnt * 0.5
        ))

    ## Cooling down
    ht.set_state(HT.HT_ID_TOP, 0, 0, 0, 0)
    ht.set_state(HT.HT_ID_BTM, 0, 0, 0, 0)
    # ht.set_fan(0xff)
    ht.set_fan_pwm(0x00, 100)
    ht.set_fan_pwm(0x01, 100)
    ht.set_fan_pwm(0x02, 100)
    ht.set_fan_pwm(0x03, 100)
    cnt = 0
    while cur_temp_top > 110 and cur_temp_btm > 110:
        cnt += 1
        cur_temp_top = ht.get_status().ht1_temp
        cur_temp_btm = ht.get_status().ht2_temp
        print("Cooling - TOP: {}, BTM: {}, cnt: {}".format(cur_temp_top, cur_temp_btm, cnt))
        time.sleep_ms(1000)

    print("Start TOP: {}, End TOP: {}, Cooling Rate: {} \n \
            Start BTM: {}, End BTM: {}, Cooling Rate: {} \n \
            ".format(temp_top, 110, (temp_top - 110) / (cnt),
            temp_btm, 110, (temp_btm - 110) / (cnt)
        ))
       
    # ht.set_fan(0x00)
    ht.set_fan_pwm(0x00, 0)
    ht.set_fan_pwm(0x01, 0)
    ht.set_fan_pwm(0x02, 0)
    ht.set_fan_pwm(0x03, 0)

#heaterRamup(ht, HT.HT_ID_TOP, 130, 0.8, 10)
#heaterRamup(ht, HT.HT_ID_BTM, 130, 0.8, 10)
#heaterRamup(ht, HT.HT_ID_TOP, 250, 0.8, 10)
##heaterRamupTwoHeaters(ht, 275, 0.8, 10, 280, 0.8, 10)