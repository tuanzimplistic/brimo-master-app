try:
    import asyncio as uasyncio
except ImportError:
    import uasyncio
    from zimplistic_pyapp import *
    #from zimplistic_pyapp.app.log import *
    # logger.setLevel(logging.DEBUG)


class DummyDispenser:
    def __init__(self):
        self.current_wt = 0

    def get_current_loadcell(self):
        print('wt:', self.current_wt)
        return self.current_wt

    def dispense_ingredient_by_weight(self, id, weight, timeout):
        self.current_wt = 0
        delta = 1
        if weight > 90:
            delta = 30
        while self.get_current_loadcell() < weight:
            self.current_wt += delta


class DummyKneader:
    def __init__(self):
        self.current_rpm = 0

    def stirrer_off(self):
        self.current_rpm = 0

    def get_status(self):
        print('rpm:', self.current_rpm)
        return ('stirrer_status', 0, 0, 0, self.current_rpm, 0)

    def stirrer_on(self, rpm):
        while self.get_status()[4] < rpm:
            self.current_rpm += 100


class DummyHeater:
    def __init__(self):
        self.current_temp = 0

    def heater_off(self):
        self.current_temp = 0

    def get_status(self, sensor_id):
        self.current_temp += 5
        return ('sensor_id', sensor_id, 'temp', self.current_temp)

    def heater_on(self, temp):
        print('set heater on', temp)


#kn = DummyKneader()
#ht = DummyHeater()
#ds = DummyDispenser()
kn = KN()
ht = HT()
ds = DS()

flavour1 = ('flavour1', 0)
flavour2 = ('flavour2', 1)
flavour3 = ('flavour2', 2)
water = ('water', 3)
milk = ('milk', 4)

map_flavour = {}
map_flavour['sugar'] = flavour1
map_flavour['coffee'] = flavour2
map_flavour['water'] = water
map_flavour['milk'] = milk


async def dispenser(args):
    # ingredient, expected_weight
    ing_id = map_flavour[args[1]][1]
    print('dispense ingredient:', args[1], ing_id,
          ' expected_weight:', args[0][args[1]])
    ds.dispense_ingredient_by_weight(ing_id, int(args[0][args[1]]), 30)
    while True:
        w = ds.get_current_loadcell()
        print('current weight', w)
        if w >= int(args[0][args[1]]):
            break
        await uasyncio.sleep(1)


async def wait_temp(args):
    # expected_temp
    print('wait_temp expected_temp:', args[2])
    while True:
        temp = ht.get_status(int(args[1]))[3]
        if temp >= int(args[2]):
            break
        print('waiting temp to:', args[2], temp)
        await uasyncio.sleep(1)


async def heater_on(args):
    # expected_temp
    print('heater_on expected_temp:', args[1])

    ht.heater_on(int(args[1]))
    while True:
        temp = ht.get_status(0)[3]
        if temp >= int(args[1]):
            break
        print('heating temp ', temp)
        await uasyncio.sleep(1)


async def heater_off(args):
    print('heater_off')
    await uasyncio.sleep(3)
    ht.heater_off()


async def stirrer_on(args):
    # expected_rpm
    print('stirrer_on expected_rpm:', args[1])
    kn.stirrer_on(int(args[1]))
    await uasyncio.sleep(3)


async def stirrer_off(args):
    print('stirrer_off')
    kn.stirrer_off()
    await uasyncio.sleep(1)


async def sleep(args):
    # seconds
    print('sleep seconds:', args[1])
    await uasyncio.sleep(int(args[1]))


async def alarm(args):
    # seconds
    print('alarm:', args[1])
    await uasyncio.sleep(1)

map_actions = {
    'dispenser': dispenser,
    'heater_on': heater_on,
    'heater_off': heater_off,
    'wait_temp': wait_temp,
    'stirrer_on': stirrer_on,
    'stirrer_off': stirrer_off,
    'sleep': sleep,
    'alarm': alarm,
}
