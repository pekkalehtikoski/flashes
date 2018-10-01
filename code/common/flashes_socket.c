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

static osalStream listening_socket;

typedef struct
{
    osalStream socket;

    os_uint addr;

    os_uint next_sector_to_erase;

    /* OS_TRUE if we are programming flash bank 2.
     */
    os_boolean bank2;
}
flashesProgrammingState;

static flashesProgrammingState flsock_state;

static os_timer boot_timer;


static void flashes_socket_program(
    flashesProgrammingState *state);


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
    listening_socket = osal_stream_open(OSAL_SOCKET_IFACE, FLASHES_SOCKET_PORT_STR,
        OS_NULL, OS_NULL, OSAL_STREAM_LISTEN|OSAL_STREAM_NO_SELECT);
    if (listening_socket == OS_NULL)
    {
        osal_debug_error("osal_stream_open failed");
    }
    os_memclear(&flsock_state, sizeof(flsock_state));
    osal_trace("listening for socket connections");

    os_get_timer(&boot_timer);

    /* Only to display the trace
    flashes_is_bank2_selected(); */

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
    osalStream accepted_socket;

    osal_socket_maintain();

    accepted_socket = osal_stream_accept(listening_socket, OS_NULL, OSAL_STREAM_DEFAULT);
    if (accepted_socket)
    {
        if (flsock_state.socket == OS_NULL)
        {
            osal_trace("socket connection accepted");
            os_memclear(&flsock_state, sizeof(flsock_state));
            flsock_state.socket = accepted_socket;
            flsock_state.socket->read_timeout_ms = 10000;
            flsock_state.socket->write_timeout_ms = 10000;
        }
        else
        {
            osal_debug_error("socket already open. This loader allows only one connected socket");
            osal_stream_close(accepted_socket);
        }
    }

    if (flsock_state.socket)
    {
        flashes_socket_program(&flsock_state);
        os_get_timer(&boot_timer);
    }
    else if (os_elapsed(&boot_timer, 5000))
    {
        flashes_jump_to_application();
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
    osal_stream_close(flsock_state.socket);
    osal_stream_close(listening_socket);
}


/**
****************************************************************************************************

  @brief Read program from socket and transfer it.
  @anchor flashes_socket_program

  The flashes_socket_program() function reads a program from socket and writes it to flash,
  block by block.

  @return  None.

****************************************************************************************************
*/
static void flashes_socket_program(
    flashesProgrammingState *state)
{
    os_uchar bytecount[2];
    os_uchar buf[FLASHES_TRANSFER_BLOCK_SIZE];
    os_memsz n_read, n_written;
    os_uint nbytes;
    osalStatus s;

    /* Read number of bytes
     */
    s = osal_stream_read(state->socket, bytecount, sizeof(bytecount), &n_read, OSAL_STREAM_WAIT);
    if (s || n_read != sizeof(bytecount)) goto broken;

    nbytes = (os_uint)bytecount[0] | (((os_uint)bytecount[1]) << 8);
    if (nbytes > sizeof(buf)) goto broken;

    s = osal_stream_read(state->socket, buf, nbytes, &n_read, OSAL_STREAM_WAIT);
    if (s || n_read != nbytes) goto broken;

    /* If this is terminating zero length block
     */
    if (nbytes == 0)
    {
        /* Set bank to boot from and reboot.
        */
        s = flashes_select_bank(state->bank2);
        if (s) goto broken;

        /* Write recipt that block was succesfully written
         */
        s = osal_stream_write(state->socket, (const os_uchar*)"o", 1, &n_written, OSAL_STREAM_WAIT);
        if (s || n_written != 1) goto broken;

        /* Close the socket, we are finished with it.
         */
        osal_stream_close(state->socket);
        state->socket = OS_NULL;

        /* Reboot the computer.
         */
os_sleep(1000);
        osal_reboot(0);
        return;
    }

    /* Otherwise data block
     */
    else
    {
        /* Check from which flash bank we are currently running on, and setup to
           load the software to the another bank.
         */
        if (state->addr == 0)
        {
            state->bank2 = !flashes_is_bank2_selected();
        }

        /* Write program binary to flash memory.
         */
        s = flashes_write(state->addr, buf, nbytes, state->bank2, &state->next_sector_to_erase);
        if (s) goto broken;
        state->addr += nbytes;
    }

    /* Write recipt that block was succesfully written
     */
    s = osal_stream_write(state->socket, (const os_uchar*)"o", 1, &n_written, OSAL_STREAM_WAIT);
    if (s || n_written != 1) goto broken;

   return;

broken:
    osal_debug_error("socket connection broken");
    osal_stream_close(state->socket);
    state->socket = OS_NULL;
}
