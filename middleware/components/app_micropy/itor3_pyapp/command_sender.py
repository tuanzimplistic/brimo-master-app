"""
command_sender
"""
import cmp_queue

##
# @brief
#      Send buffer to modbus.
#
#
# @details
#      This function sends the buffer to the C modbus stack on C layer and waits for the response
#       in "tout" miliseconds. After the time is out, if there is no response message, the function
#       will resend the buffer 3 times. Note: This resend mechanism is a work around. It will be 
#       deleted after the bug on modbus is fixed.
#
# @param [in]
#       buf: buffer to be sent to the modbus.
# @param [in]
#       tout: waiting time in miliseconds for the response from slave board.
#
# @return
#      @arg    response: responded buffer from the slave.
# @return
#      @arg    None: if there's any error.
# ##
def send_command(buf, tout):
    cnt = 0
    response = cmp_queue.exchange_bytes(buf, tout)
    while (not response):
        # try to resend the command
        if cnt >= 3:
            break
        response = cmp_queue.exchange_bytes(buf, tout)
        cnt += 1
    return response