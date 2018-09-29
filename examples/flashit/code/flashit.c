/**

  @file    flashit.c
  @brief   Command line utility for Windows/Linux to transfer program to MCU over Ethernet.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    20.9.2018

  Copyright 2018 Pekka Lehtikoski. This file is part of the iocom project and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"

/* TCP port for transferring the program.
 */
#ifndef FLASHES_SOCKET_PORT_STR
// #define FLASHES_SOCKET_PORT_STR ":6827"
#define FLASHES_SOCKET_PORT_STR ":6001"
#endif

/* Block size for the transfer. Selected to be small enough to fit easily to MCU RAM and
   within an Ethernet frame, but large enough not to be a bottlenect in program transfer speed.
   This must be also divisible by minimum transfer size. Changing block size from 1024 may
   cause problems.
 */
#ifndef FLASHES_TRANSFER_BLOCK_SIZE
#define FLASHES_TRANSFER_BLOCK_SIZE 1024
#endif

/* Time out if transfer connection becomes silent.
 */
#define FLASHES_TRANSFER_TIMEOUT_MS 20000


/**
****************************************************************************************************

  @brief Flashit example program main function.

  The osal_main() function is OS independent entry point. This contains the flashit example
  code. The flashit is simple command line utility to transfer binary program to MCU
  over Ethernet.

  This implementation uses non blocking sockets, but would be simpler using blocking sockets.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
os_int osal_main(
    os_int argc,
    os_char *argv[])
{
    osalStream f = OS_NULL, socket = OS_NULL;
    osalStatus s;
    os_char ipaddr[OSAL_HOST_BUF_SZ], *binfile, nbuf[64];
    os_uchar buf[FLASHES_TRANSFER_BLOCK_SIZE], *pos = OS_NULL, reply[4], block_sz[2];
    os_memsz n_read, n_written, buf_n;
    os_int i, n, block_count;
    os_timer timer;
    os_boolean whole_file_read, waiting_for_reply, terminating_zero_packet_sent;

    /* Get IP address/port and path to binary file.
     */
    ipaddr[0] = '\0';
    binfile = OS_NULL;
    for (i = 1; i<argc; i++)
    {
        if (argv[i][0] == '-') continue;
        if (ipaddr[0] == '\0')
        {
            os_strncpy(ipaddr, argv[i], sizeof(ipaddr));
        }
        else if (binfile == OS_NULL)
        {
            binfile = argv[i];
        }
    }
    if (binfile == OS_NULL) goto showhelp;
    os_strncat(ipaddr, FLASHES_SOCKET_PORT_STR, sizeof(ipaddr));

    /* Open source file.
     */
    f = osal_file_open(binfile, OS_NULL, OS_NULL, OSAL_STREAM_READ);
    if (f == OS_NULL)
    {
        osal_console_write("opening binary file failed\n");
        goto getout;
    }
    osal_trace("binary file opened");

    /* Connect socket.
     */
    socket = osal_stream_open(OSAL_SOCKET_IFACE, ipaddr, OS_NULL, OS_NULL,
        OSAL_STREAM_CONNECT|OSAL_STREAM_NO_SELECT);
    if (socket == OS_NULL)
    {
        osal_console_write("socket connection failed\n");
        goto getout;
    }
    socket->write_timeout_ms = FLASHES_TRANSFER_TIMEOUT_MS;
    osal_trace("socket connection initiated");

    /* Transfer the program
     */
    buf_n = 0;
    whole_file_read = OS_FALSE;
    waiting_for_reply = OS_FALSE;
    terminating_zero_packet_sent = OS_FALSE;
    block_count = 0;
    while (OS_TRUE)
    {
        osal_socket_maintain();

        /* Sending data.
         */
        if (!waiting_for_reply)
        {
            /* If we need to read more data.
             */
            if (buf_n == 0)
            {
                if (whole_file_read)
                {
                    buf_n = 0;
                }
                else
                {
                    s = osal_file_read(f, buf, sizeof(buf), &buf_n, OSAL_STREAM_DEFAULT);
                    if (s)
                    {
                        osal_console_write("reading file failed\n");
                        goto getout;
                    }
                }
                pos = buf;

                /* If all done, break.
                 */
                whole_file_read = (os_boolean)(buf_n < sizeof(buf));

                /* Write block size with two bytes. Less significant byte first.
                   We always write zero length block in the end to indicate end
                   of the program.
                 */
                block_sz[0] = (os_uchar)buf_n;
                block_sz[1] = (os_uchar)(buf_n >> 8);
                n = 2;
                s = osal_stream_write(socket, block_sz, n, &n_written, OSAL_STREAM_WAIT);
                if (s || n_written != n)
                {
                    osal_console_write("socket connection failed\n");
                    goto getout;
                }
                if (buf_n == 0)
                {
                    waiting_for_reply = terminating_zero_packet_sent = OS_TRUE;
                    os_get_timer(&timer);
                }
                else
                {
                    osal_console_write("transferring block ");
                    osal_int_to_string(nbuf, sizeof(nbuf), ++block_count);
                    osal_console_write(nbuf);
                    osal_console_write("... ");
                }
            }

            /* Write data to socket.
             */
            if (buf_n)
            {
                s = osal_stream_write(socket, pos, buf_n, &n_written, OSAL_STREAM_DEFAULT);
                if (s)
                {
                    osal_console_write("socket connection failed\n");
                    goto getout;
                }
                buf_n -= n_written;
                pos += n_written;
                waiting_for_reply = (os_boolean)(buf_n == 0);
                os_get_timer(&timer);
            }
        }

        /* Waiting for reply
         */
        else
        {
            /* Try to get MCU reply.
             */
            os_memclear(reply, sizeof(reply));
            if (osal_stream_read(socket, reply, sizeof(reply), &n_read, OSAL_STREAM_DEFAULT))
            {
                osal_console_write("socket connection broken\n");
                goto getout;
            }

            /* If we got the replay.
             */
            if (n_read > 0)
            {
                /* If reply is OK (small 'o' letter), then all is fine. Other replies
                   indicate error. Interrupt the transfer.
                 */
                if (reply[0] == 'o')
                {
                    osal_console_write("ok\n");
                }
                else
                {
                    osal_console_write("error\n");
                    osal_console_write("program transfer failed\n");
                    goto getout;
                }

                /* No longer waiting for replay, we can move on to next packet.
                   If this is reply to terminating zero package, all is done.
                   Skip timeout checking by "continue".
                 */
                waiting_for_reply = OS_FALSE;
                if (terminating_zero_packet_sent) break;
                continue;
            }

            /* Check for time out.
             */
            if (os_elapsed(&timer, FLASHES_TRANSFER_TIMEOUT_MS))
            {
                osal_console_write("waiting MCU reply timed out\n");
                goto getout;
            }
        }

        /* Do not eat up all time of a processor core.
         */
        os_timeslice();
    }

    osal_console_write("Program succesfully transferred\n");

getout:
    osal_file_close(f);
    osal_stream_close(socket);
    return 0;

showhelp:
    osal_console_write("flashit 192.168.1.177 program.bin\n");
    return 0;
}
