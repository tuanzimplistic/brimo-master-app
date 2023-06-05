# This module should be imported from REPL, not run from command line.
import socket
import uos
import uwebsocket
import websocket_helper
import _webrepl
import ssl
import binascii

listen_s = None
client_s = None
websslrepl = False

def setup_conn(port, accept_handler):
    global listen_s, websslrepl
    listen_s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listen_s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    ai = socket.getaddrinfo("0.0.0.0", port)
    addr = ai[0][4]

    listen_s.bind(addr)
    listen_s.listen(1)
    if accept_handler:
        listen_s.setsockopt(socket.SOL_SOCKET, 20, accept_handler)
    return listen_s


def accept_conn(listen_sock):
    global client_s, websslrepl

    # Private key (SSL_key.der) and certificate (SSL_certificate.der) for WSS server are generated as below:
    #   openssl ecparam -out SSL_key.pem -name secp256r1 -genkey
    #   openssl req -new -key SSL_key.pem -x509 -nodes -days 36500 -out SSL_certificate.pem
    #   openssl x509 -in SSL_certificate.pem -out SSL_certificate.der -outform DER
    #   openssl ec -in SSL_key.pem -out SSL_key.der -outform DER
    key = binascii.unhexlify(
        b'30770201010420CB5CDC00C4B1EFA4B48B80047282F05AAAD877F56AEA2F32F33E50AC7A7C7839A00A0608'
        b'2A8648CE3D030107A14403420004ADDF07B8E24B83A77A5350C9B0C29E2B5F05A45110FBE57E64DEAF941A'
        b'FE2E4A63906ED2F097E5AA75837B48511F35784C6C2E2E0963C81050D3A48A5885DF33')
    cert = binascii.unhexlify(
        b'3082028B30820231A00302010202145E709D59B96F32BA8966AF5C9D9DFC1171828A11300A06082A8648CE'
        b'3D040302308199310B30090603550406130253473112301006035504080C0953696E6761706F7265311230'
        b'1006035504070C0953696E6761706F726531133011060355040A0C0A5A696D706C69737469633113301106'
        b'0355040B0C0A5A696D706C69737469633110300E06035504030C075765625245504C3126302406092A8648'
        b'86F70D01090116176E676F6374756E672E6468626B40676D61696C2E636F6D3020170D3231303832313130'
        b'343130375A180F32313231303732383130343130375A308199310B30090603550406130253473112301006'
        b'035504080C0953696E6761706F72653112301006035504070C0953696E6761706F72653113301106035504'
        b'0A0C0A5A696D706C697374696331133011060355040B0C0A5A696D706C69737469633110300E0603550403'
        b'0C075765625245504C3126302406092A864886F70D01090116176E676F6374756E672E6468626B40676D61'
        b'696C2E636F6D3059301306072A8648CE3D020106082A8648CE3D03010703420004ADDF07B8E24B83A77A53'
        b'50C9B0C29E2B5F05A45110FBE57E64DEAF941AFE2E4A63906ED2F097E5AA75837B48511F35784C6C2E2E09'
        b'63C81050D3A48A5885DF33A3533051301D0603551D0E0416041449317B7A486BA925D95AFF12E86BCA3CE6'
        b'D5CC7C301F0603551D2304183016801449317B7A486BA925D95AFF12E86BCA3CE6D5CC7C300F0603551D13'
        b'0101FF040530030101FF300A06082A8648CE3D0403020348003045022100EF9D4156E0713354566C32D8FC'
        b'C0C91769919F86EC77A7260AC5EF4A713123BB02205D47A6124C7DF3479DB80B362A91A27D4AC2D8E5B8B3'
        b'77FEC4F0F128A8213E1B')

    cl, remote_addr = listen_sock.accept()
    prev = uos.dupterm(None)
    uos.dupterm(prev)
    if prev:
        print("\nConcurrent WebREPL connection from", remote_addr, "rejected")
        cl.close()
        return
    print("\nWebREPL connection from:", remote_addr)
    client_s = cl
    if websslrepl:
        if hasattr(uos, 'dupterm_notify'):
            cl.setsockopt(socket.SOL_SOCKET, 20, uos.dupterm_notify)
        cl = ssl.wrap_socket(cl, server_side=True, key=key, cert=cert)
        websocket_helper.server_handshake(cl, ssl=True)
    else:
        websocket_helper.server_handshake(cl)
    ws = uwebsocket.websocket(cl, True)
    ws = _webrepl._webrepl(ws)
    cl.setblocking(False)
    # notify REPL on socket incoming data (ESP32/ESP8266-only)
    if not websslrepl:
        if hasattr(uos, "dupterm_notify"):
            cl.setsockopt(socket.SOL_SOCKET, 20, uos.dupterm_notify)
    uos.dupterm(ws)


def stop():
    global listen_s, client_s
    uos.dupterm(None)
    if client_s:
        client_s.close()
    if listen_s:
        listen_s.close()


def start(port=8266, password=None, ssl=False):
    global websslrepl
    if ssl:
        websslrepl = True
        port = 8833
    else:
        websslrepl = False
    stop()
    if password is None:
        try:
            import webrepl_cfg

            _webrepl.password(webrepl_cfg.PASS)
            setup_conn(port, accept_conn)
            print("Started webrepl in normal mode")
        except:
            print("WebREPL is not configured, run 'import webrepl_setup'")
    else:
        _webrepl.password(password)
        setup_conn(port, accept_conn)
        print("Started webrepl in manual override mode")


def start_foreground(port=8266):
    stop()
    s = setup_conn(port, None)
    accept_conn(s)
