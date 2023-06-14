from zimplistic_pyapp.app.log import *
from zimplistic_pyapp.app.app import *
from zimplistic_pyapp.master.gpio import *
import uasyncio
import time
logger.setLevel(logging.DEBUG)

io = GPIO()

io.set_dout_state(15, 1)
