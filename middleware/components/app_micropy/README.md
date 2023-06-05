This folder contains all MicroPython scripts for Itor3 firmware. All files
ending with ".py" or ".mpy" extension with be recursively frozen and stored
in flash. A ".py" script will be compiled to a ".mpy" first then frozen, and
a ".mpy" file will be frozen directly.

When importing frozen modules, this folder is the base directory to search
for the modules. For example, if directory structure of this folder is as below:

    [app_micropy]
          |--- module1.py
          \--- [packageA]
                    |--- moduleA1.py
                    \--- [packageB]
                              |--- moduleB1.py

then for a MicroPython script to use these modules, it must import them as below:

```python
import module1
import packageA.moduleA1
import packageA.packageB.moduleB1
```
