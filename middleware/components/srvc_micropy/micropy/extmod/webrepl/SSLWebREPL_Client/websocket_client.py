import ast
import socket
import ws_client_handshake as wsclient

class BASE_WS_DEVICE:
    def __init__(self, target, password, init=False, ssl=False, auth=False, capath='.'):
        self.ws = None
        self.ip = target
        self.pswd = password
        self.port = 8266
        self.bytes_sent = 0
        self.buff = b''
        self.raw_buff = b''
        self.prompt = b'>>> '
        self.response = ''
        self._kbi = '\x03'
        self._banner = '\x02'
        self._reset = '\x04'
        self._traceback = b'Traceback (most recent call last):'
        self._flush = b''
        self.output = None
        self.platform = None
        self.connected = False
        if init:
            if not ssl:
                self.ws = wsclient.connect('ws://{}:{}'.format(self.ip, self.port), self.pswd)
            else:
                self.port = 8833
                self.ws = wsclient.connect('wss://{}:{}'.format(self.ip, self.port), self.pswd, auth=auth, capath=capath)
            self.connected = True

    def open_wconn(self, ssl=False, auth=False, capath='.'):
        if not ssl:
            self.ws = wsclient.connect('ws://{}:{}'.format(self.ip, self.port), self.pswd)
        else:
            self.port = 8833
            self.ws = wsclient.connect('wss://{}:{}'.format(self.ip, self.port), self.pswd, auth=auth, capath=capath)
        self.connected = True

    def close_wconn(self):
        self.ws.close()
        self.connected = False

    def write(self, cmd):
        n_bytes = len(bytes(cmd, 'utf-8'))
        self.ws.send(cmd)
        return n_bytes

    def read_all(self):
        self.ws.sock.settimeout(None)
        try:
            self.raw_buff = b''
            while self.prompt not in self.raw_buff:
                fin, opcode, data = self.ws.read_frame()
                self.raw_buff += data

            return self.raw_buff
        except socket.timeout as e:
            return self.raw_buff

    def flush(self):
        self.ws.sock.settimeout(0.01)
        self._flush = b''
        while True:
            try:
                fin, opcode, data = self.ws.read_frame()
                self._flush += data
            except socket.timeout as e:
                break
            except protocol.NoDataException as e:
                break

    def wr_cmd(self, cmd, silent=False, rtn=True, rtn_resp=False, long_string=False):
        self.output = None
        self.response = ''
        self.buff = b''
        self.flush()
        self.bytes_sent = self.write(cmd+'\r')
        # time.sleep(0.1)
        # self.buff = self.read_all()[self.bytes_sent:]
        self.buff = self.read_all()
        if self.buff == b'':
            # time.sleep(0.1)
            self.buff = self.read_all()
        # print(self.buff)
        # filter command
        cmd_filt = bytes(cmd + '\r\n', 'utf-8')
        self.buff = self.buff.replace(cmd_filt, b'', 1)
        if self._traceback in self.buff:
            long_string = True
        if long_string:
            self.response = self.buff.replace(b'\r', b'').replace(b'\r\n>>> ', b'').replace(b'>>> ', b'').decode()
        else:
            self.response = self.buff.replace(b'\r\n', b'').replace(b'\r\n>>> ', b'').replace(b'>>> ', b'').decode()
        if not silent:
            if self.response != '\n' and self.response != '':
                print(self.response)
            else:
                self.response = ''
        if rtn:
            self.get_output()
            if self.output == '\n' and self.output == '':
                self.output = None
            if self.output is None:
                if self.response != '' and self.response != '\n':
                    self.output = self.response
        if rtn_resp:
            return self.output

    def cmd(self, cmd, silent=False, rtn=False, ssl=False):
        self.open_wconn(ssl=ssl, auth=True)
        self.wr_cmd(cmd, silent=True)
        self.close_wconn()
        self.get_output()
        if not silent:
            print(self.response)
        if rtn:
            return self.output

    def reset(self, silent=False, ssl=False):
        if not silent:
            print('Rebooting device...')
        if self.connected:
            self.bytes_sent = self.write(self._reset)
            self.close_wconn()
            time.sleep(1)
            while True:
                try:
                    self.open_wconn()
                    self.wr_cmd(self._banner, silent=True)
                    break
                except Exception as e:
                    time.sleep(0.5)
            if not silent:
                print('Done!')
        else:
            self.open_wconn(ssl=ssl, auth=True)
            self.bytes_sent = self.write(self._reset)
            self.close_wconn()
            if not silent:
                print('Done!')

    def kbi(self, silent=True):
        if self.connected:
            self.wr_cmd(self._kbi, silent=silent)
        else:
            self.cmd(self._kbi, silent=silent)

    def banner(self):
        self.wr_cmd(self._banner, silent=True, long_string=True)
        print(self.response.replace('\n\n', '\n'))

    def get_output(self):
        try:
            self.output = ast.literal_eval(self.response)
        except Exception as e:
            if 'bytearray' in self.response:
                try:
                    self.output = bytearray(ast.literal_eval(
                        self.response.strip().split('bytearray')[1]))
                except Exception as e:
                    pass
            else:
                if 'array' in self.response:
                    try:
                        arr = ast.literal_eval(
                            self.response.strip().split('array')[1])
                        self.output = array(arr[0], arr[1])
                    except Exception as e:
                        pass
            pass
