# OTA firmware update

The following items of Rotimatic can be updated over-the-air:
+ Main application firmware of master board
+ An arbitrary file in filesystem of master board
+ Main application firmware of slave board
+ Bootloader firmware of slave board

On the cloud, files of those items must be stored at HTTPs servers of Amazon S3 service (s3.amazonaws.com). The master board firmware is preflashed with certificate of the server so that it can authenticate the server and download the firmware. For the files to be accessed and downloaded by the master board, the following conditions must be met:
+ The Amazon S3 bucket(s) storing the files must allow public access to its content.
+ The files must be made public.

A typical OTA update process on Rotimatic consists of 4 steps:

## 1) Trigger

To start an OTA update, a MQTT request message must be sent to __itor3/esp32/request__ topic at broker.hivemq.com. JSON structure of the message is as below:
```json
{
    "command":"ota_update",
    "target":"<master_fw|slave_fw|master_file>",
    "checkNewer":<true|false>,
    "path":"<file_path>",
    "url":"<download_url>"
}
```
+ __"command"__ : value of this attribute is always _"ota_update"_
+ __"target"__ :
    + _"master_fw"_ : update firmware of master board
    + _"slave_fw"_ : update firmware of slave board
    + _"master_file"_: update an arbitrary file in master board's filesystem
+ __"checkNewer"__ : this attribute is optional and applied only for firmware update. In case it is not available, its value is _false_ by default
    + _true_ : version of the firmware must be checked before updating. OTA update is only performed if the to-be-updated firmware is newer than the running firmware; otherwise, it must be ignored. The to-be-updated firmware is newer if its version is greater than the running firmware (for e.g, "0.10.0" > "0.9.9")
    + _false_ : the OTA update is always performed regardless of version of the to-be-updated firmware.
+ __"path"__ : Path on the filesystem where the downloaded file to be stored. This attribute is applicable and mandatory only for file update. For firmware update, it could be omitted.
+ __"url"__ : AWS S3 link where the firmware or files are stored in the cloud.

Upon receiving the request, the master board shall respond with the following MQTT message through __itor3/esp32/response__ topic at broker.hivemq.com:
```json
{
    "command":"ota_update",
    "status":"<ok|error>",
}
```
+ __"command"__ : value of this attribute is always _"ota_update"_
+ __"status"__ :
    + _"ok"_: OTA update process has been started successfully and is running in the background
    + _"error"_: some error has occurred while starting OTA update process (e.g, the master board is busy)

### Examples:
+ Trigger an OTA firmware update for master board with __mosquitto_pub__:
```
mosquitto_pub -h broker.hivemq.com -t itor3/esp32/request -m "{ \"command\":\"ota_update\", \"target\":\"master_fw\", \"checkNewer\":true, \"url\":\"https://itor3otabucket.s3.ap-southeast-1.amazonaws.com/Itor3.bin\" }"
```
+ Trigger an OTA firmware update for slave board with __mosquitto_pub__:
```
mosquitto_pub -h broker.hivemq.com -t itor3/esp32/request -m "{ \"command\":\"ota_update\", \"target\":\"slave_fw\", \"checkNewer\":false, \"url\":\"https://itor3otabucket.s3.ap-southeast-1.amazonaws.com/Itor3_Slave_App_Test.bin\" }"
```
+ Trigger an OTA update for a file in master board with __mosquitto_pub__:
```
mosquitto_pub -h broker.hivemq.com -t itor3/esp32/request -m "{ \"command\":\"ota_update\", \"target\":\"master_file\", \"path\":\"/docs/README.md\", \"url\":\"https://itor3otabucket.s3.ap-southeast-1.amazonaws.com/README.md\" }"
```

## 2) Validate

Upon receiving OTA update trigger, the master board shall perform some validation on the item to be updated. If the validation fails, the OTA update request shall be rejected and an error message shall be displayed on the GUI.

### 2.1) Validate the master board firmare to be updated
The firmware file of master board contains an embedded structure called _firmware descriptor_ which describes some important information about the firmware itself. Before downloading the whole firmware, the master board shall download its _firmware descriptor_ first and validate the followings:
+ __Project name__: for firmware of master board, its project name must be "Itor3". For firmware of slave board, its project name must be "Itor3_slave" (t.b.d).
+ __Firmware version__: if _"checkNewer"_ attribute of the request message in the first step is _true_, version of the to-be-updated firmware must be greater than that of the running firmware.
+ __Firmware size__: size of the to-be-updated firmware must be fit inside the target flash partition.

### 2.2) Validate the master board file to be updated
Name of the file to be updated shall be first extracted from the download URL. For example, if download URL is _"https://itor3otabucket.s3.ap-southeast-1.amazonaws.com/test.mpy"_ then name of the file to be downloaded shall be _"test.mpy"_. File size shall also be obtained from the HTTPs server containing the file. The master board shall validate the followings:
+ File name must have less than 64 characters. Itor3 firmware only supports files whose name contains less than 64 characters.
+ The remaining space of the filesystem storage must be greater than size of the file to download.

## 3) Download

While the firmware is being downloaded, downloading progress shall be displayed on GUI and user is not allowed to interract with Rotimatic via the GUI. If the downloading fails, an error message shall be displayed to user on the GUI.

### 3.1) Download the firmware to be updated
The whole firmware shall be downloaded and stored onto an OTA partition in internal flash of the master board's MCU. The OTA partition has the same size as that of the main partition of the master board firmware. If the downloading is discrupted in between, the current running firmware is intact.

### 3.2) Download the file to be updated
A temporary file named _"~temp.tmp"_ shall be created in root directory of the master board's filesystem to store data of the downloading file. That temporary file shall be deleted when the OTA update is done. If a file with the same name already exists, its data shall be truncated.

### 3.3) Cancel downloading
To cancel an OTA firmware update process, one can send an MQTT request message to __itor3/esp32/post__ topic at broker.hivemq.com. Content of the request is as below:

```json
{
    "command":"ota_cancel"
}
```

On receiving the cancellation request, the master board shall stop the OTA update and display a notification message on the GUI.

This example uses __mosquitto_pub__ to cancel an OTA update process:
```
mosquitto_pub -h broker.hivemq.com -t itor3/esp32/post -m "{ \"command\":\"ota_cancel\" }"
```

## 4) Install

### 4.1) Install master board firmware
The newly downloaded firmware shall be installed with the following steps:
+ _Firmware descriptor_ structure of the master board firmware contains SHA-256 checksum of the whole firmware. This checksum is validated to ensure that the firmware is not corrupt while being downloaded.
+ The OTA partition which contains the newly downloaded firmware is marked as temporary running partition. The previous running partition is marked as OTA partition.
+ The ESP32 MCU of the master board is restarted to run the new firmware.
+ If the new firmware runs successfully, its partition shall be marked as official running partition and a message shall be displayed on the GUI. Otherwise (the new firmware fails to run), the master board's MCU shall be reset by watchdog timer and the previous working partition is marked as running partition again (firmware rollback).

### 4.2) Install master board file
+ All necessary folders in the path where the file will be saved are created (if they are not available).
+ The temporary file _"~temp.tmp"_ shall be moved to the destination folder and renamed to the actual name of the file. If a file with the same name already exists, it shall be overwritten.

### 4.2) Install slave board firmware
T.b.d.
