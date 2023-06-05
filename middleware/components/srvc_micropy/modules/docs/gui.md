# GUI binding data

**Example in MicroPython:**

```python
import gui

# This changes recipe name in Roti making screen
gui.set_data (gui.GUI_DATA_RECIPE_NAME, 'My recipe')

# This get roast level selected in Roti making screen
roast_level = gui.get_data (gui.GUI_DATA_ROAST_LEVEL)
```

# GUI notify message box:

**Syntax:**
```python
gui.notify(detail, type=gui.INFO, brief='', timer=0)
```
- detail: detail description about the notify
- type: type of the notify, which is one of the followings: `gui.INFO`, `gui.WARNING`, `gui.ERROR`. Default value is `gui.INFO`
- brief: brief description about the notify. Default value is empty string.
- timer: Timeout (ms) that the notify will be automatically closed. If this is 0, the notify will only be closed when user acknowledges it. Default value is 0.

**Example in MicroPython:**

```python
import gui

gui.notify('IP address obtained')

gui.notify('Connection lost', type=gui.WARNING, timer=4000)

gui.notify('Temperature exceeds threshold', type=gui.ERROR, brief='High temperature', timer=10000)
```

# GUI query message box:

**Syntax:**
```python
gui.query(detail, options, type=gui.INFO, brief='', timer=0, default=0)
```
- detail: detail description about the query
- options: list or tuple of user options
- type: type of the query, which is one of the followings: `gui.INFO`, `gui.WARNING`, `gui.ERROR`. Default value is `gui.INFO`
- brief: brief description about the query. Default value is empty string.
- timer: timeout (ms) that the query will be automatically closed. If this is 0, the query will only be closed when user selects an option. Default value is 0.
- default: index of the option to be selected by default if `timer` argument is not 0 and expires

**Example in MicroPython:**

```python
import gui

select = gui.query('Which cake do you want to make?', ('Roti one', 'Roti two', 'Roti three'))

select = gui.query('Which cake do you want to make?', ('Roti one', 'Roti two2', 'Roti three'), default=2, timer=10000)

select = gui.query('Failed to make cake. Do you want to retry?', ['Retry', 'Ignore'], brief='Failure', type=gui.WARNING)

select = gui.query('Oops, an unknown error occurred.', ['Retry', 'Reset', 'Cancel'], default=1, brief='Critical error', type=gui.ERROR, timer=3000)
```

# GUI activity timing
If user does not interact with GUI (touch it) for a predefined period of time (10 minutes), the LCD backlight will be turned off. GUI module provides the following functions to work with GUI idle time:

```python
# Get the elapsed time in millisecond since the last user activity on the GUI
elapsed_time = gui.get_idle_time()
```

```python
# Trigger a pseudo user activity to keep the LCD active or wake it up
gui.keep_active()
```

**Example in MicroPython:**

```python
import gui

# Wait until user is not using GUI
while gui.get_idle_time() < 60000:
    pass

# Switch the machine into power down mode
# ...

# Wait until user touches the LCD to wake the machine up
while gui.get_idle_time() >= 1000:
    pass

# Switch the machine into normal power mode
#...

# Start working
working = True
while (working):
    # Working...
    # Keep the LCD on while the machine is working
    if gui.get_idle_time() >= 30000:
        gui.keep_active()
```
