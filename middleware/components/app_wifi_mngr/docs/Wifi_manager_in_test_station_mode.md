# Wifi Manager in test station mode

When Master firmware runs in test station mode, Wifi Manager behaves differently from normal operation mode. The following configs in menuconfig specify the behavior of Wifi Manager in test station mode:

## _Enable test station build_

This config enables or disables test station mode of Wifi Manager. By default, test station mode is disabled.

+ When test station mode is disabled, after bootup, Wifi Manager will first try to connect with user configured access point whose information is stored inside the non-volatile memory. If that fails, Wifi Manager will try with the access points in its hard-coded list, one by one. If that still fails, Wifi Manager will restart the attempt with user configured access point again, and so on. In this mode, IP address of Master board is assigned via DHCP server of the connected access point.

+ When test station mode is enabled, Wifi Manager will first try to connect to a predefined test access point and assign itself with static IP address (see the configs below). Wifi Manager only does that once. If the connection with the test access point drops or Wifi Manager fails to establish a connection with the test access point, it will continue with the normal routine (i.e, try to connect with user configured access point and hard-coded access point list with dynamically assigned IP address) and never revisit the test access point until the firmware restarts.

## WiFi access point for test station

### _WiFi SSID_

This config specifies SSID (network name) of the test access point. By default, SSID of the test access point is "OQCZPLAP" (without quote).

### _WiFi password_

This config specifies password of the test access point. By default, password of the test access point is "Zpl@1234!" (without quote).

### _Number of retries connecting to the WiFi access point_

This config specifies number of attemps that Wifi Manager does when it fails to connect to the given test access point. After this many attempts and Wifi Manager is still not able to connect to the test access point, it will give up and start the normal routine.

Normally, one connecting attempt takes approximately 2.5 seconds. Value of this config is 100 by default.

## Static IP address of test station

### _IP address_

This config specifies the static IP address that Master board uses while connecting with the test access point. By default, value of this config is "192.168.1.2".

### _Netmask_

This config specifies the subnet mask that Master board uses while connecting with the test access point. By default, value of this config is "255.255.255.0".

### _Default gateway_

This config specifies address of the subnet gateway that Master board uses while connecting with the test access point. By default, value of this config is "192.168.1.1".

### _DNS server_

This config specifies IP address of the DNS server that Master board uses while connecting with the test access point. By default, value of this config is "192.168.1.1".
