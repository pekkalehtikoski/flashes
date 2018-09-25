/**

  @file    flashes_socket.c
  @brief   Listen TCP socket for flash program binary.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    24.9.2018

  The flashes_socket_setup() opens listening socket port 6827. The flashes_socket_loop()
  function is intended to be called from IO board's main loop. It checks for incoming socket
  connections. If one is establised, the binary program is read from it and written to flash.
  The flashes_socket_cleanup() does the clean up.

  Copyright 2018 Pekka Lehtikoski. This file is part of the iocom project and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/


/* Force tracing on for this source file.
 */
#undef OSAL_TRACE
#define OSAL_TRACE 3

#include "flashes.h"

static osalStream listening_socket, accepted_socket, open_socket;


/**
****************************************************************************************************

  @brief Open listening socket port to wait for binary program.
  @anchor flashes_socket_setup

  The flashes_socket_setup() function...

  @return  None.

****************************************************************************************************
*/
void flashes_socket_setup(void)
{
    listening_socket = osal_socket_open(FLASHES_SOCKET_PORT_STR,
        OS_NULL, OS_NULL, OSAL_STREAM_LISTEN|OSAL_STREAM_NO_SELECT);
    if (listening_socket == OS_NULL)
    {
        osal_debug_error("osal_stream_open failed");
    }
    osal_trace("listening for socket connections");
    open_socket = OS_NULL;
}


/**
****************************************************************************************************

  @brief Open listening socket port to wait for binary program.
  @anchor flashes_socket_loop

  The flashes_socket_loop() function is intended to be called from IO board's main loop. It
  checks for incoming socket connections. If one is establised, the binary program is read
  from it and written to flash.

  Only one connection as accepted at a time.

  @return  None.

****************************************************************************************************
*/
void flashes_socket_loop(void)
{
    os_uchar buf[64], *pos;
    os_memsz n_read, n_written;

    osal_socket_maintain();

    accepted_socket = osal_socket_accept(listening_socket, OS_NULL, OSAL_STREAM_DEFAULT);
    if (accepted_socket)
    {
        if (open_socket == OS_NULL)
        {
            osal_trace("socket connection accepted");
            open_socket = accepted_socket;
        }
        else
        {
            osal_debug_error("socket already open. This example allows only one connected socket");
            osal_socket_close(accepted_socket);
        }
    }

    if (open_socket)
    {
        if (osal_socket_read(open_socket, buf, sizeof(buf), &n_read, OSAL_STREAM_DEFAULT))
        {
            osal_debug_error("socket connection broken");
            osal_socket_close(open_socket);
            open_socket = OS_NULL;
        }
        else
        {
            pos = buf;
            while (n_read > 0)
            {
                if (osal_socket_write(open_socket, pos, n_read, &n_written, OSAL_STREAM_DEFAULT))
                {
                    osal_socket_close(open_socket);
                    open_socket = OS_NULL;
                    break;
                }
                n_read -= n_written;
                pos += n_written;
                if (n_read == 0) break;
                os_timeslice();
            }
        }
    }
}


/**
****************************************************************************************************

  @brief Close listening socket and interrupt any ongoing program transfer.
  @anchor flashes_socket_cleanup

  The flashes_socket_cleanup() function closes socket currently used for program transfer, if any,
  and the socket listening for new incoming connections.

  @return  None.

****************************************************************************************************
*/
void flashes_socket_cleanup(void)
{
    osal_socket_close(open_socket);
    osal_socket_close(listening_socket);
}
