/**

  @file    ioboard.c
  @brief   IO board example 2.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    7.8.2018

  ioboard2 example demonstrates basic IO board with network communication.

  This example assumes one memory block for inputs and the other for outputs. Implementation
  doesn't need dynamic memory allocation or multithreading, thus it should run on any platform
  including microcontrollers. Template file is used for easy setup.

  Copyright 2018 Pekka Lehtikoski. This file is part of the iocom project and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"


/**
****************************************************************************************************

  @brief Process entry point.

  The osal_main() function is OS independent entry point.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
os_int osal_main(
    os_int argc,
    os_char *argv[])
{
    osalStream socket;
    os_uchar buf[64], *pos, testdata[] = "?-testdata*", c = 'a';
    os_memsz n_read, n_written;
    os_int n;
    os_timer timer;

    socket = osal_socket_open("127.0.0.1:6827" , OS_NULL, OS_NULL,
        OSAL_STREAM_CONNECT|OSAL_STREAM_NO_SELECT);
    if (socket == OS_NULL)
    {
        osal_debug_error("osal_stream_open failed");
        return 0;
    }
    osal_trace("socket connected");

    os_get_timer(&timer);
    while (OS_TRUE)
    {
        osal_socket_maintain();

        if (os_elapsed(&timer, 200))
        {
            testdata[0] = c;
            if (c++ == 'z') c = 'a';
            pos = testdata;
            n = os_strlen((os_char*)testdata) - 1;

            while (n > 0)
            {
                if (osal_socket_write(socket, pos, n, &n_written, OSAL_STREAM_DEFAULT))
                {
                    osal_debug_error("socket connection broken");
                    goto getout;
                }

                n -= n_written;
                pos += n_written;
                if (n == 0) break;
                os_timeslice();
            }
            os_get_timer(&timer);
        }

        os_memclear(buf, sizeof(buf));
        if (osal_socket_read(socket, buf, sizeof(buf)-1, &n_read, OSAL_STREAM_DEFAULT))
        {
            osal_debug_error("socket connection broken");
            goto getout;
        }

        if (n_read > 0)
        {
            osal_console_write((os_char *)buf);
        }

        os_timeslice();
    }

getout:
    osal_stream_close(socket);
    socket = OS_NULL;
    return 0;
}
