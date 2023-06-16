import logging
from zimplistic_pyapp import *

logg = logging.getLogger()
logg.setLevel(logging.DEBUG)
#websocket & webrpl
#logg.addHandler(logging.WsNotifyHandler())
logg.addHandler(logging.StreamHandler())
logger.debug = logg.debug