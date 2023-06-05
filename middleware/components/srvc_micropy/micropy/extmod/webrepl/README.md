# WebREPL server

This WebREPL server supports both ws (WebSocket over TCP) and wss (WebSocket over SSL) protocols. The implementation is based on this pull request: https://github.com/micropython/micropython/pull/5611

To start the WebREPL server with WebSocket over TCP protocol, password '123456' and port 8266:

```python
import webrepl

# This will start listening on ws://x.x.x.x:8266
webrepl.start(password='123456')
```

To start the WebREPL server with WebSocket over SSL protocol, password '123456' and port 8833:

```python
import webrepl

# This will start listening on wss://x.x.x.x:8833
webrepl.start(password='123456', ssl='True')
```


# Key

Folder __Key__ contains the private key (SSL_key.der) and certificate (SSL_certificate.der) used for the SSL WebREPL server. The key and certificate were generated as below:

```
openssl ecparam -out SSL_key.pem -name secp256r1 -genkey
openssl req -new -key SSL_key.pem -x509 -nodes -days 36500 -out SSL_certificate.pem
openssl x509 -in SSL_certificate.pem -out SSL_certificate.der -outform DER
openssl ec -in SSL_key.pem -out SSL_key.der -outform DER
```


# SSL WebREPL client

In order to interact with an SSL WebREPL server, an SSL WebREPL client is required. Such a client can be found in sub-folder __SSLWebREPL_Client__.

This is an example running on a PC to communicate with an SSL WebREPL server:

```python
from websocket_client import BASE_WS_DEVICE

# Asumming that IP address of the WebREPL server is 192.168.1.42 and the password to access it is '123456'
wss_webrepl = BASE_WS_DEVICE('192.168.1.42', '123456', ssl=True, init=True)

# Execute some commands on the WebREPL server
wss_webrepl.wr_cmd("1+2")
wss_webrepl.wr_cmd("print('Hello')")

# Close connection with the WebREPL server
wss_webrepl.close_wconn()
```
