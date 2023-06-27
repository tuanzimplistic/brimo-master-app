import uasyncio
from zimplistic_pyapp.app.log import *
from zimplistic_pyapp.app.app import *
logger.setLevel(logging.DEBUG)

filtered_coffee_receipe = {}
filtered_coffee_receipe[1] = {'water': 100,
                              'coffee': 13, 'sugar': 15, 'milk': 100}
filtered_coffee_receipe[2] = {'water': 200,
                              'coffee': 26, 'sugar': 30, 'milk': 200}
filtered_coffee_receipe[3] = {'water': 300,
                              'coffee': 39, 'sugar': 45, 'milk': 300}
filtered_coffee_receipe[4] = {'water': 400,
                              'coffee': 52, 'sugar': 60, 'milk': 400}
filtered_coffee_receipe[5] = {'water': 500,
                              'coffee': 65, 'sugar': 75, 'milk': 500}
filtered_coffee_formula = [
    ['dispenser => water', 'heater => 100 => off'],
    ['dispenser => sugar'],
    ['dispenser => coffee'],
    ['heater => 100 => keep', 'stirrer => 150'],
    ['sleep => 180'],
    ['heater => off'],
    ['sleep => 120'],
    ['stirrer => off'],
    ['dispenser => milk'],
    ['heater => 45 => off'],
    ['stirrer => 1500'],
    ['sleep => 120'],
    ['stirrer => off'],
    ['heater => 70 => off'],
    ['alarm => on'],
]

filtered_coffee_receipe[0] = filtered_coffee_formula

chai_receipe = {}
chai_receipe[1] = {'water': 100, 'black_tea': 6,
                   'sugar': 10, 'ginger_powder': 1.6, 'milk': 100}
chai_receipe[2] = {'water': 200, 'black_tea': 12,
                   'sugar': 20, 'ginger_powder': 3.2, 'milk': 200}
chai_receipe[3] = {'water': 300, 'black_tea': 18,
                   'sugar': 30, 'ginger_powder': 4.8, 'milk': 300}
chai_receipe[4] = {'water': 400, 'black_tea': 24,
                   'sugar': 40, 'ginger_powder': 6.4, 'milk': 400}
chai_receipe[5] = {'water': 500, 'black_tea': 30,
                   'sugar': 50, 'ginger_powder': 8, 'milk': 500}

receipes = {
    'filtered coffee': filtered_coffee_receipe,
    'chai': chai_receipe,
}

user_receipe = 'filtered coffee'
user_cups = 1

# print(receipes[user_receipe][user_cups])


class Wait:
    def __init__(self):
        self.count = 0
        self.ev = uasyncio.Event()
        self.lock = uasyncio.Lock()

    def set_ref(self, count):
        self.count = count

    async def notify(self):
        async with self.lock:
            self.count -= 1
            if self.count == 0:
                self.ev.set()

    async def done(self):
        await self.ev.wait()
        self.ev.clear()


class Worker:
    def __init__(self, id, notify):
        self.notify = notify
        self.id = id
        self.queue = []
        self.ev = uasyncio.Event()
        self.run = True

    async def execute(self):
        while self.run:
            await self.ev.wait()
            self.ev.clear()
            command = self.queue.pop(0)
            await uasyncio.sleep(3)
            print('w', self.id, command)
            await self.notify()

    def trigger(self):
        self.ev.set()

    def put(self, command):
        self.queue.append(command)


class app:
    def __init__(self):
        print('this is brimo app')

    async def make(self):
        wait = Wait()
        w1 = Worker(1, wait.notify)
        w2 = Worker(2, wait.notify)

        tasks = [uasyncio.create_task(w1.execute()),
                 uasyncio.create_task(w2.execute())]

        for command in filtered_coffee_receipe[0]:
            count = len(command)
            wait.set_ref(count)
            if count == 1:
                w1.put(command[0])
                w1.trigger()
            else:
                w1.put(command[0])
                w2.put(command[1])
                w1.trigger()
                w2.trigger()
            await wait.done()
            print('next command =>')

# app = app()
# uasyncio.run(app.make())
