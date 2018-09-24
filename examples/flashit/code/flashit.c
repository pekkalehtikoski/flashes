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
#include "ioboard.h"
#include "iocom/codetemplates/ioboard_basic_static_network_io.c"


/**
****************************************************************************************************

  @brief IO board example 2.

  The ioboard() function sets up an IO board with connects two memory blocks, inputs and
  outputs.

  @return  None.

****************************************************************************************************
*/
void ioboard(void)
{
    os_uchar u;
    os_uint uu = 100;

    /* Start IO board communication.
     */
    ioboard_initialize();

    while (!osal_console_read())
    {
        /* Keep the communication alive (This is single threaded approach).
         */
        ioc_run(&ioboard_root);

        /* SLEEP IS NOT THE RIGHT WAY DO THIS, normally ioc_run() would be
           called from IO board's main loop.
         */
        //os_sleep(10);

        u = ++uu; //  / 100;
        ioc_write(&ioboard_inputs, 2, &u, 1);
        ioc_write(&ioboard_inputs, 400, &u, 1);
    }

    /* End IO board communication.
     */
    ioboard_shutdown();
}
