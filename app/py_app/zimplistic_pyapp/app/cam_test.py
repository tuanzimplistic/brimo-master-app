import cam
import os
import time
from itor3_pyapp.master.gpio import *

cam.cv_init()
io = GPIO()

class Cam:
    def __init__(self, num):
        self.num = num

    def take(self, exposure):
        io.set_dout_state(15, 1)
        for i in range(self.num):
            s = "test{}.jpg".format(i)
            logger.debug({'take picture': s})
            cam.cv_take_picture_exposure(s, exposure)
        io.set_dout_state(15, 0)
        
    def clean(self):
        for i in range(self.num):
            s = "test{}.jpg".format(i)
            logger.debug({'delete picture': s})
            os.remove(s)


