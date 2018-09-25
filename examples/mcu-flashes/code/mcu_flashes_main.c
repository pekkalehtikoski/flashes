/**

  @file    eosal/examples/simple_socket_server/code/simple_socket_server_main.c
  @brief   Socket server example.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    7.12.2017

  Echo back server. Extermely simple: No dynamic memory alloccation, multithreading, stream
  interface, socket select, etc. Just bare bones.

  Copyright 2012 Pekka Lehtikoski. This file is part of the eobjects project and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "flashes.h"


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
    flashes_socket_setup();

    while (OS_TRUE)
    {
        flashes_socket_loop();
        os_timeslice();
    }

    flashes_socket_cleanup();
}
