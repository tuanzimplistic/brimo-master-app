# Roti making screen

Roti making screen is bound with the following data:

| GUI data                   | GUI access | Description                                                      |
|----------------------------|------------|------------------------------------------------------------------|
| GUI_DATA_ROTI_COUNT        | Read/Write | Number of Roti's to be made                                      |
| GUI_DATA_ROTI_MADE         | Read       | Number of Roti's that have been made                             |
| GUI_DATA_RECIPE_NAME       | Read       | Name of the recipe being used                                    |
| GUI_DATA_FLOUR_NAME        | Read       | Name of the flour being used                                     |
| GUI_DATA_ROAST_LEVEL       | Read/Write | Roast level                                                      |
| GUI_DATA_THICKNESS_LEVEL   | Read/Write | Thickness level                                                  |
| GUI_DATA_OIL_LEVEL         | Read/Write | Oil level                                                        |
| GUI_DATA_COOKING_STARTED   | Read/Write | Indicates if cooking has been started by user (1) or not (0)     |
| GUI_DATA_COOKING_STATE     | Read       | Instantaneous cooking state (0 = idle, 1 = cooking)              |

This is an example on how to interact with Roti making screen from MicroPython:

```python
import gui

# Initialize Roti making screen
gui.set_data (gui.GUI_DATA_ROTI_MADE, 0)
gui.set_data (gui.GUI_DATA_RECIPE_NAME, 'ROTI')
gui.set_data (gui.GUI_DATA_FLOUR_NAME, 'Pillsbury gold wholewheat atta')
gui.set_data (gui.GUI_DATA_COOKING_STATE, 0)

# Wait until user starts cooking
while gui.get_data (gui.GUI_DATA_COOKING_STARTED) == 0:
   pass

# Get cooking parameters
roti_count      = gui.get_data (gui.GUI_DATA_ROTI_COUNT)
roast_level     = gui.get_data (gui.GUI_DATA_ROAST_LEVEL)
thickness_level = gui.get_data (gui.GUI_DATA_THICKNESS_LEVEL)
oil_level       = gui.get_data (gui.GUI_DATA_OIL_LEVEL)

# Start cooking
gui.set_data (gui.GUI_DATA_COOKING_STATE, 1)
for i in range(0, roti_count):
    # Do the cooking
    # ...
    # This roti has been done
    gui.set_data (gui.GUI_DATA_ROTI_MADE, i + 1)

# Cooking is done
gui.set_data (gui.GUI_DATA_COOKING_STATE, 0)
```