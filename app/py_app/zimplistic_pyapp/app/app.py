from zimplistic_pyapp.app.log import *
from zimplistic_pyapp.app.app import *
logger.setLevel(logging.DEBUG)

from zimplistic_pyapp.master.gpio import *
import uasyncio
import time

io = GPIO()

io.set_dout_state(15, 1)
