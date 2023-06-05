# param.get_all_keys(namespace)

This function returns a tuple of all keys available in a non-volatile storage namespace. In case the function fails, __None__ shall be returned.

Example in MicroPython:

```python
import param
keys = param.get_all_keys('my_namespace')
for key in keys:
    print(key)
```

# param.erase_all(namespace)

This function erases all parameters in a non-volatile storage namespace. If the function succeeds, __True__ shall be returned. Otherwise, __False__ is returned.

Example in MicroPython:

```python
import param
param.erase_all('my_namespace')
keys = param.get_all_keys('my_namespace')   # "keys" should be an empty tuple
```
