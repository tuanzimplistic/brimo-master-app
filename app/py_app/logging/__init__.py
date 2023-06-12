""" Logging module for PyApp

Logging module for PyApp is a light weight version of the built-in logging module of CPython.
It is heavily customized from the source code of the original module so that it can work well
in MicroPython environment of Rotimatic application while still keeping the basic features and
leveraging some advanced functions of the original code

Written: Nguyen Ngoc Tung
Date: 2022 Sep 11
"""

import utime
import sys
import uio
import ws_notify

# Predefined logging levels
CRITICAL = 50
ERROR    = 40
WARNING  = 30
INFO     = 20
DEBUG    = 10

# Singleton object of Logger class
_logger = None

# Helper variable to convert numeric logging level to str
_level_dict = {
    CRITICAL: "CRITICAL",
    ERROR: "ERROR",
    WARNING: "WARNING",
    INFO: "INFO",
    DEBUG: "DEBUG",
}


class Logger:
    """
    Logger class provides the methods for application code to use the logging module directly.
    Note that Logger objects should NEVER be instantiated directly, but always through the
    module-level function logging.getLogger()
    """

    # If logging level of this logger is not explicitly set by setLevel(), it will be WARNING by default
    _level = WARNING

    def __init__(self):
        self.handlers = []


    def setLevel(self, level):
        """ Sets the threshold for this logger to the given level

        Parameters
        ----------
        level : int
            Severe level to set for this logger, which could be DEBUG, INFO, WARNING, ERROR, or CRITICAL
        """

        self._level = level


    def isEnabledFor(self, level):
        """ Indicates if a message of severity 'level' would be processed by this logger

        Parameters
        ----------
        level : int
            Severe level to check, which could be DEBUG, INFO, WARNING, ERROR, or CRITICAL
        """

        return level >= self._level


    def _log(self, level, info, *args):
        """ Constructs a _LogRecord object and sends to associated handlers.

        Logging messages which are less severe than level of this logger will be ignored;
        logging messages which have severity same or higher that level will be emitted by
        whichever handler or handlers service this logger.

        Parameters
        ----------
        level : int
            Severe level of the logging message, which could be DEBUG, INFO, WARNING, ERROR, or CRITICAL

        info : dict or str
            Information about the logging event. If 'info' is a dict with multiple key – value pairs,
            the output logging message shall be a JSON-like message where single quotes ' are used
            in places of double quotes ". If 'info' is a str, the output logging message shall be simply
            a merge of 'info' and 'args'

        args: argument list
            The arguments which are merged into 'info' using the string formatting operator 'info.format()'.
            No formatting operation is performed on 'info' when no 'args' are supplied.
        """

        if level >= self._level:
            # Construct the log message to emit and encapsulate it as a log record
            record = _LogRecord(level, info, args)

            # If there is no handler when the first log is emitted, create a stream handler
            if len(self.handlers) == 0:
                self.handlers.append(StreamHandler())

            # Emit the log through the associated handler(s)
            for hdlr in self.handlers:
                hdlr.emit(record)


    def debug(self, info, *args):
        """ Logs a message with level DEBUG on this logger

        The 'info' is the base logging message, and the 'args' are the arguments which are merged
        into 'info' using the string formatting operator 'info.format(*args)'. No formatting operation
        is performed on msg when no 'args' are supplied.

        Parameters
        ----------
        info : dict or str
            Information about the logging event. If 'info' is a dict with multiple key – value pairs,
            the output logging message shall be a JSON-like message. If 'info' is a str, the output
            logging message shall be simply a merge of 'info' and 'args'

        args: argument list
            The arguments which are merged into 'info' using the string formatting operator 'info.format()'.
            No formatting operation is performed on 'info' when no 'args' are supplied.
        """

        self._log(DEBUG, info, *args)


    def info(self, info, *args):
        """ Logs a message with level INFO on this logger

        The 'info' is the base logging message, and the 'args' are the arguments which are merged
        into 'info' using the string formatting operator 'info.format(*args)'. No formatting operation
        is performed on msg when no 'args' are supplied.

        Parameters
        ----------
        info : dict or str
            Information about the logging event. If 'info' is a dict with multiple key – value pairs,
            the output logging message shall be a JSON-like message. If 'info' is a str, the output
            logging message shall be simply a merge of 'info' and 'args'

        args: argument list
            The arguments which are merged into 'info' using the string formatting operator 'info.format()'.
            No formatting operation is performed on 'info' when no 'args' are supplied.
        """

        self._log(INFO, info, *args)


    def warning(self, info, *args):
        """ Logs a message with level WARNING on this logger

        The 'info' is the base logging message, and the 'args' are the arguments which are merged
        into 'info' using the string formatting operator 'info.format(*args)'. No formatting operation
        is performed on msg when no 'args' are supplied.

        Parameters
        ----------
        info : dict or str
            Information about the logging event. If 'info' is a dict with multiple key – value pairs,
            the output logging message shall be a JSON-like message. If 'info' is a str, the output
            logging message shall be simply a merge of 'info' and 'args'

        args: argument list
            The arguments which are merged into 'info' using the string formatting operator 'info.format()'.
            No formatting operation is performed on 'info' when no 'args' are supplied.
        """

        self._log(WARNING, info, *args)


    def error(self, info, *args):
        """ Logs a message with level ERROR on this logger

        The 'info' is the base logging message, and the 'args' are the arguments which are merged
        into 'info' using the string formatting operator 'info.format(*args)'. No formatting operation
        is performed on msg when no 'args' are supplied.

        Parameters
        ----------
        info : dict or str
            Information about the logging event. If 'info' is a dict with multiple key – value pairs,
            the output logging message shall be a JSON-like message. If 'info' is a str, the output
            logging message shall be simply a merge of 'info' and 'args'

        args: argument list
            The arguments which are merged into 'info' using the string formatting operator 'info.format()'.
            No formatting operation is performed on 'info' when no 'args' are supplied.
        """

        self._log(ERROR, info, *args)


    def critical(self, info, *args):
        """ Logs a message with level CRITICAL on this logger

        The 'info' is the base logging message, and the 'args' are the arguments which are merged
        into 'info' using the string formatting operator 'info.format(*args)'. No formatting operation
        is performed on msg when no 'args' are supplied.

        Parameters
        ----------
        info : dict or str
            Information about the logging event. If 'info' is a dict with multiple key – value pairs,
            the output logging message shall be a JSON-like message. If 'info' is a str, the output
            logging message shall be simply a merge of 'info' and 'args'

        args: argument list
            The arguments which are merged into 'info' using the string formatting operator 'info.format()'.
            No formatting operation is performed on 'info' when no 'args' are supplied.
        """

        self._log(CRITICAL, info, *args)


    def addHandler(self, hdlr):
        """ Adds the specified handler 'hdlr' to this logger

        If this function is not called on a logger, a StreamHandler shall be automatically created and
        added for that logger when the first logging message is emitted. A logger may have multiple
        handlers by calling this method multiple times. In that case, a logging message sent to this 4
        logger shall be processed by multiple handlers in the same order as they were added to the logger.

        Parameters
        ----------
        hdlr : StreamHandler, FileHandler, or WsNotifyHandler
            An instance of the corresponding handler class to assign to this logger
        """

        self.handlers.append(hdlr)


def getLogger():
    """ Module function obtaining the single instance of Logger class

    Logger class should not be instantiated directly. Instead, this function should be used to get the
    instance of Logger class
    """

    global _logger
    if _logger is None:
        _logger = Logger()
    return _logger


class StreamHandler():
    """
    StreamHandler objects send logging output to streams such as sys.stdout, sys.stderr or any file-like
    object (or, more precisely, any object which supports write() and flush() methods). The default
    stream of StreamHandler object sends logging messages to sys.stderr, which is ended up sending
    to MicroPython WebREPL
    """

    def __init__(self, stream=None):
        """ Returns a new instance of the StreamHandler class

        Initialize the StreamHandler instance by associating it with a stream. If 'stream' is NONE or empty,
        the stream to WebREPL shall be used by default.

        Parameters
        ----------
        stream : standard stream or file-like object
            The stream to associate with this StreamHandler object
        """

        # If stream is not provided, use sys.stderr instead
        self._stream = stream or sys.stderr
        self.terminator = "\n"


    def emit(self, record):
        """ Writes message of a log record to the associated stream

        Parameters
        ----------
        record : _LogRecord
            The log record containing the message to write to the stream
        """

        self._stream.write(record.message + self.terminator)


class FileHandler():
    """ FileHandler objects send logging output to a file in the local filesystem """

    def __init__(self, filename, mode="a", delay=False):
        """ Returns a new instance of the FileHandler class

        The specified file is opened and used as the stream for logging. The file grows indefinitely.

        Parameters
        ----------
        filename : str
            Path and name of the file to write logging messages to

        mode : str
            Mode openning the file
            + 'a': opens a file for appending at the end of the file without truncating it,
              creates a new file if it does not exist
            + 'w': creates a new file if it does not exist or truncates the file if it exists

        delay : bool
            If 'delay' is True, then file opening is deferred until the first log message is emitted
        """

        self._stream = None
        self.terminator = "\n"
        self.mode = mode
        self.delay = delay
        self.filename = filename
        if not delay:
            self._stream = open(self.filename, self.mode)


    def emit(self, record):
        """ Writes message of a log record to the associated file

        Parameters
        ----------
        record : _LogRecord
            The log record containing the message to write to the file
        """

        # If the associated is not openned yet, open it when the first log is emitted
        if self._stream is None:
            self._stream = open(self.filename, self.mode)

        # Write the logging message to the file
        self._stream.write(record.message + self.terminator)
        self._stream.flush()


    def close(self):
        """ Closes the associated log file and free up the resources that were tied with the file """

        if self._stream is not None:
            self._stream.close()


class WsNotifyHandler():
    """
    WsNotifyHandler objects send logging messages to the channel exposed by the Websocket server.

    Rotimatic Master firmware has a built-in Websocket server in the platform part of the firmware.
    The server can be accessed by the outside world via WiFi connection and the URI
    ws://<master_board_ip>/slave/status. Any Websocket clients connected to the server shall be able
    to receive messages sent by PyApp via the binding channel exposed by the Websocket server.
    """

    def __init__(self):
        # Send a dummy message to the Websocket server to intialize it
        ws_notify.notify_slave_status('Start Websocket server')


    def emit(self, record):
        """ Writes message of a log record to the Websocket channel

        Parameters
        ----------
        record : _LogRecord
            The log record containing the message to write to the channel
        """

        # Call the API exposed by the platform to send the log message through the Websocket channel
        ws_notify.notify_slave_status(record.message)


class _LogRecord:
    """ Encapsulates log message as an object to pass it through classes """

    def __init__(self, level, info, args):
        """ Returns a new instance of _LogRecord class

        Parameters
        ----------
        level : int
            Severe level of the logging message, which could be DEBUG, INFO, WARNING, ERROR, or CRITICAL

        info : dict or str
            Information about the logging event. If 'info' is a dict with multiple key – value pairs,
            the output logging message shall be a JSON-like message where single quotes ' are used
            in places of double quotes ". If 'info' is a str, the output logging message shall be simply
            a merge of 'info' and 'args'

        args: argument list
            The arguments which are merged into 'info' using the string formatting operator 'info.format()'.
            No formatting operation is performed on 'info' when no 'args' are supplied.
        """

        # Log timestamp is number of seconds elapsed since the firmware started
        timestamp = utime.time()
        levelname = _level_dict.get(level, None)

        if type(info) is dict:
            self.message = "{'level': '" + levelname + "', 'timestamp': '" + str(timestamp) + "', " + \
                           str(info).strip('{}').format(*args) + '}'
        else:
            self.message = str(timestamp) + ":" + levelname + ": " + info.format(*args)
