import logging

logger = logging.getLogger()
logger.setLevel(logging.DEBUG)
#websocket & webrpl
logger.addHandler(logging.WsNotifyHandler())
logger.addHandler(logging.StreamHandler())