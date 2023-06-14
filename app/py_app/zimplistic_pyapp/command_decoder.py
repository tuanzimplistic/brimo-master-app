"""
Contains functions to decode the buffer.
"""

from ustruct import unpack

from zimplistic_pyapp.constants import Endian, FormatCharacters
##
# @brief 
# This class is used to decode the memory buffer sent from C.
# ##
class CommandDecoder(object):
    def __init__(self, adu, reqcode, subcode, byteorder = Endian.LITTLE):
        """
        Constructor.
        """
        self._byteorder = byteorder
        self._adu = adu
        self._reqcode   = reqcode
        self._subcode   = subcode
        self._pointer = 0
        
        # read moduleid and sub command from adu
        # assert that the expected moduleid and sub command
        if  parsed_code := self.decode_1byte_uint() != self._reqcode:
            raise ValueError('Module ID mismatch! Expected {}, Found {}', self._reqcode, parsed_code)
        if  parsed_code := self.decode_1byte_uint() != self._subcode:
            raise ValueError('Sub Commandd mismatch! Expected {}, Found {}', self._subcode, parsed_code)

    def _decode(self, value, character):
        fstring = self._byteorder + character
        return unpack(fstring, value)[0]

    def _get_next(self, size):
        self._pointer += size
        if(self._pointer > len(self._adu)):
            raise LookupError('No more things to decode')
        return self._adu[self._pointer - size:self._pointer]

    def decode_1byte_uint(self):
        """
        Decode 1byte to integer.
        """
        handle = self._get_next(1)
        return self._decode(handle, FormatCharacters.UINT1)

    def decode_4bytes_uint_time(self):
        """
        Decode 4byte to integer.
        """
        handle = self._get_next(4)
        return self._decode(handle, FormatCharacters.UINT4)


    def decode_4bytes_uint(self):
        """
        Decode 4byte to integer.
        """
        handle = self._get_next(4)
        return self._decode(handle, FormatCharacters.UINT4) / 65536

    def decode_4bytes_int(self):
        """
        Decode 4byte to integer.
        """
        handle = self._get_next(4)
        return self._decode(handle, FormatCharacters.INT4) / 65536

    def decode_4bytes_q16(self):
        """
        Decode 4byte to float.
        """
        handle = self._get_next(4)
        return round(float(self._decode(handle, FormatCharacters.INT4) / 65536.0), 2)