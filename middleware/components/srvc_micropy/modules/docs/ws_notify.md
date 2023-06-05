# Broadcast slave board's status over Websocket connection

Master board has a built-in Websocket server at the URL __ws://<master_board_ip>/slave/status__. The server is automatically started upon device startup. All Websocket clients that connects to the server will receive status messages sent by MicroPython scripts.

**Syntax:**
```python
ws_notify.notify_slave_status(message)
```
- _message_: status of slave board, data type of _message_ could be string, tuple, or list

**Example in MicroPython:**

```python
import ws_notify

# Send a string message
ws_notify.notify_slave_status('Bottom temperature = 102 Celsius degrees')

# Send a raw message in tuple format
ws_notify.notify_slave_status((0x11, 0x22, 0x33, 0x44))

# Send a raw message in list format
ws_notify.notify_slave_status([0x11, 0x22, 0x33, 0x44])
```
