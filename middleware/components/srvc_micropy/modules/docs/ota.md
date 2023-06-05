# Trigger an OTA update for master board firmware

**Syntax:**
```python
ota.update_master_firmware(url, checkNewer)
```
- _url_: the AWS S3 link to download the master board firmware for update
- _checkNewer_:
    - `True`: only perform the OTA update if the firmware on the cloud is newer than the running firmware
    - `False`: no firmware version check

**Example in MicroPython:**

```python
import ota
ota.update_master_firmware('https://itor3otabucket.s3.ap-southeast-1.amazonaws.com/Itor3.bin', False)
```

# Trigger an OTA update for a file in master board's filesystem

**Syntax:**
```python
ota.update_master_file(url, instDir)
```
- _url_: the AWS S3 link to download the file for update
- _instDir_: full path in the master board's filesystem to save the file. If any folders given in _instDir_ are not available in the target filesystem, they will be created. If a file is already available at _instDir_, it shall be overwritten. Examples of valid path are: '/new_file_1.txt', '/dir1/dir2/new_file_2.md'

**Example in MicroPython:**

```python
import ota
ota.update_master_file('https://itor3otabucket.s3.ap-southeast-1.amazonaws.com/README.md', '/docs/README.md')
```

# Cancel an OTA update

**Syntax:**
```python
ota.cancel()
```

**Example in MicroPython:**

```python
import ota
ota.update_master_firmware('https://itor3otabucket.s3.ap-southeast-1.amazonaws.com/Itor3.bin', False)
ota.cancel()
```
