try:
    import asyncio as uasyncio
except ImportError:
    import uasyncio
from actions import map_actions

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
    ['act:dispenser(water)', 'act:heater_on(100)|post:heater_off()'],
    ['pre:is_temp(100)|act:dispenser(sugar)'],
    ['act:dispenser(coffee)'],
    ['act:heater_on(100)', 'act:stirrer_on(150)'],
    ['act:sleep(3)'], #180
    ['act:heater_off()'],
    ['act:sleep(4)'],#120
    ['act:stirrer_off()'],
    ['act:dispenser(milk)'],
    ['act:heater_on(45)|post:heater_off()'],
    ['act:stirrer_on(1500)'],
    ['act:sleep(5)'],#120
    ['act:stirrer_off()'],
    ['act:heater_on(70)|post:heater_off()'],
    ['act:alarm(on)'],
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
user_chosen = receipes[user_receipe][user_cups]


class Worker:
    def __init__(self, id):
        self.id = id
        self.queue = []
        self.done = uasyncio.Event()
        self.running = True
    
    def parse_func(self, element):
        s = element.index(':')
        e = element.index('(')
        func = element[s+1:e]
        p = element[e+1:len(element)-1]
        return func, p

    async def execute(self, cmd):
        cmd = cmd.split('|')
        for element in cmd:
            valid = False
            if element.startswith('pre'):
                valid = True
            elif element.startswith('act'):
                valid = True
            elif element.startswith('post'):
                valid = True
            if valid:
                func, p = self.parse_func(element)
                if func in map_actions:
                    invoke = map_actions[func]([user_chosen] + p.split(','))
                    await invoke
                else:
                    print('invalid function')
            else:
                print('invalid action')
            
    async def run(self):
        while self.running:
            if len(self.queue) > 0:
                command = self.queue.pop(0)
                print('w', self.id, command)
                await self.execute(command)
                self.iamdone()
            else:
                await uasyncio.sleep(0.5)

    def iamdone(self):
        self.done.set()

    async def wait_done(self):
        await self.done.wait()
        self.done.clear()

    def put(self, command):
        self.queue.append(command)


class app:
    def __init__(self):
        print('this is brimo app')

    async def makeit(self):
        w1 = Worker(1)
        w2 = Worker(2)

        tasks = [uasyncio.create_task(w1.run()),
                 uasyncio.create_task(w2.run())]
        ## just care main worker ##
        for command in filtered_coffee_receipe[0]:
            count = len(command)
            if count == 1:
                w1.put(command[0])
            else:
                w1.put(command[0])
                w2.put(command[1])
            await w1.wait_done()
            print('=>>>> next command =>>>>')

#app = app()
#uasyncio.run(app.makeit())
