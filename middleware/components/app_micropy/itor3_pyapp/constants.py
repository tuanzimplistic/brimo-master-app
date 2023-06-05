"""
Constant Class
"""

## Broadcast address
BROADCAST_ADDRESS = 0

## Slave address
SLAVE_ADDRESS = 1

class Endian():
    """
    Endian Class
    """
    ## Auto
    AUTO   = '@'

    ## Big endian
    BIG    = '>'

    ## Little endian
    LITTLE = '<'

class FormatCharacters():
    """
    FormatCharacters Class
    """
    ## Boolean type
    BOOL = '?'

    ## UINT1 type
    UINT1 = 'B'

    ## UINT4 type
    UINT4 = 'I'

    ## INT4 type
    INT4  = 'i'