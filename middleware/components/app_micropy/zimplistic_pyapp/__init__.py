#############################
# Script information        #
#############################

version = 'DV2.5'
description = 'Released: 2023 Mar 21'

import gui

def display_info(version = '', description = ''):
    """
    Display info on splash screen and About screen
    """
    info = f"+ Version: {version}\n+ {description}"
    try:
        gui.set_data(gui.GUI_DATA_SCRIPT_BRIEF_INFO, version)
        gui.set_data(gui.GUI_DATA_SCRIPT_DETAIL_INFO, info)
    except:
        return False

    return True

# Send script information to GUI
# This must be called very first so that the information can be displayed on splash screen.
display_info(version, description)


#############################
# PyApp initialization code #
#############################

from itor3_pyapp.command_builder import CommandBuilder
from itor3_pyapp.command_decoder import CommandDecoder
from itor3_pyapp.command_sender import send_command

from itor3_pyapp.master.verticaltray import *
from itor3_pyapp.master.kicker import *
from itor3_pyapp.master.wedgepress import *
from itor3_pyapp.master.kneader import *
from itor3_pyapp.master.heater import *

from itor3_pyapp.util.Addons import *

"""
To be run at start up
"""

# Run calibrate
calibrate_vt_kr()
#
