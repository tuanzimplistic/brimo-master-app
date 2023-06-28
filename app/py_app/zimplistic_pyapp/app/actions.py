try:
    import asyncio as uasyncio
except ImportError:
    import uasyncio
    from zimplistic_pyapp.app.log import *
    from zimplistic_pyapp import *
    logger.setLevel(logging.DEBUG)

class DummyDispenser:
	def __init__(self):
		self.current_wt = 0

	def get_current_wt(self):
		print('wt:', self.current_wt)
		return self.current_wt

	async def dispense_by_weight(self, id, weight):
		self.current_wt = 0
		delta = 1
		if weight > 90:
			delta = 30
		while self.get_current_wt() < weight:
			self.current_wt += delta
			await uasyncio.sleep(1)

class DummyKneader:
	def __init__(self):
		self.current_rpm = 0

	def stirrer_off(self):
		self.current_rpm = 0

	def get_current_rpm(self):
		print('rpm:', self.current_rpm)
		return self.current_rpm

	async def stirrer_on(self, rpm):
		while self.get_current_rpm() < rpm:
			self.current_rpm += 100
			await uasyncio.sleep(1)

class DummyHeater:
	def __init__(self):
		self.current_temp = 0

	def heater_off(self):
		self.current_temp = 0

	def get_current_temp(self):
		print('temp:', self.current_temp)
		return self.current_temp

	async def heater_on(self, temp):
		while self.get_current_temp() < temp:
			self.current_temp += 5
			await uasyncio.sleep(1)

kn = DummyKneader()
ht = DummyHeater()
ds = DummyDispenser()

async def dispenser(args):
	#ingredient, expected_weight
	print('dispense ingredient:', args[1], ' expected_weight:', args[0][args[1]])
	await ds.dispense_by_weight(args[1], int(args[0][args[1]]))

async def heater_on(args):
	#expected_temp
	print('heater_on expected_temp:', args[1])
	await ht.heater_on(int(args[1]))

async def heater_off(args):
	print('heater_off')
	ht.heater_off()

async def is_temp(args):
	#expected_temp
	print('is_temp expected_temp:', args[1])
	while ht.get_current_temp() < int(args[1]):
		print('waiting temp to:', args[1])
		await uasyncio.sleep(1)

async def stirrer_on(args):
	#expected_rpm
	print('stirrer_on expected_rpm:', args[1])
	await kn.stirrer_on(int(args[1]))

async def stirrer_off(args):
	print('stirrer_off')
	kn.stirrer_off()

async def sleep(args):
	#seconds
	print('sleep seconds:', args[1])
	await uasyncio.sleep(int(args[1]))

async def alarm(args):
	#seconds
	print('alarm:', args[1])
	await uasyncio.sleep(1)

map_actions = {
	'dispenser': dispenser,
	'heater_on': heater_on,
	'heater_off': heater_off,
	'is_temp': is_temp,
	'stirrer_on': stirrer_on,
	'stirrer_off': stirrer_off,
	'sleep': sleep,
	'alarm':alarm,
}