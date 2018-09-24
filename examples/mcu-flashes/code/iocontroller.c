/**

  @file    iocontroller.c
  @brief   IO controller example 1.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    7.8.2018

  iocontroller1 example controls an IO board through TCP socket.

  This example assumes one memory block for inputs and the other for outputs. It uses both
  supports dynamic memory allocation and multithreading, thus this example cannot be used in
  most microcontrollers.

  Copyright 2018 Pekka Lehtikoski. This file is part of the iocom project and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "iocontroller.h"

typedef struct
{
    os_long
        count;

    int
        start_addr,
        end_addr;
}
ioControllerCallbackData;

ioControllerCallbackData callbackdata;


/* Forward referred static functions.
 */
static void iocontroller_callback(
    struct iocMemoryBlock *mblk,
    int start_addr,
    int end_addr,
    os_ushort flags,
    void *context);


/**
****************************************************************************************************

  @brief IO controller example 1.

  The iocontroller() function connects two memory blocks, inputs and
  outputs, to IO board.

  @return  None.

****************************************************************************************************
*/
void iocontroller(void)
{
    iocRoot *root;
    iocMemoryBlock *inputs, *outputs;
    iocConnection *con;
    ioControllerCallbackData cd;

    const int
        input_block_sz = 1000,
        output_block_sz = 1000;

    os_char
        text[80],
        nbuf[32];

    int i;
    os_uchar u;

    root = ioc_initialize_root(OS_NULL);
    inputs = ioc_initialize_memory_block(OS_NULL, root, IOC_INPUT_MBLK,
        OS_NULL, input_block_sz, IOC_TARGET|IOC_AUTO_RECEIVE);
    outputs = ioc_initialize_memory_block(OS_NULL, root, IOC_OUTPUT_MBLK,
        OS_NULL, output_block_sz, IOC_SOURCE|IOC_AUTO_SEND);
    ioc_add_callback(inputs, iocontroller_callback, OS_NULL);

    con  = ioc_initialize_connection(OS_NULL, root);
    ioc_initialize_target_buffer(OS_NULL, con, inputs, 0, IOC_SAME_MBLK, 0, 0);
    ioc_initialize_source_buffer(OS_NULL, con, outputs, 0, IOC_SAME_MBLK, 0, 0);

//    ioc_connect(con, "192.168.1.229" ":" IOC_DEFAULT_SOCKET_PORT_STR,
    ioc_connect(con, "127.0.0.1" ":" IOC_DEFAULT_SOCKET_PORT_STR,
        OS_NULL, OS_NULL, 0, OS_NULL, 0, IOC_SOCKET|IOC_CREATE_THREAD);

    while (!osal_console_read())
    {
        /* Get callback data, copy it to local stack
         */
        ioc_lock(root);
        cd = callbackdata;
        callbackdata.count = 0;
        ioc_unlock(root);

        if (cd.count)
        {
            os_strncpy(text, "callback ", sizeof(text));
            osal_int_to_string(nbuf, sizeof(nbuf), cd.count);
            os_strncat(text, nbuf, sizeof(text));
            os_strncat(text, ": ", sizeof(text));
            osal_int_to_string(nbuf, sizeof(nbuf), cd.start_addr);
            os_strncat(text, nbuf, sizeof(text));
            os_strncat(text, " - ", sizeof(text));
            osal_int_to_string(nbuf, sizeof(nbuf), cd.end_addr);
            os_strncat(text, nbuf, sizeof(text));
            os_strncat(text, ": ", sizeof(text));

             for (i = cd.start_addr; i <= cd.end_addr; i++)
            {
                if (i > cd.start_addr) os_strncat(text, ", ", sizeof(text));
                ioc_read(inputs, i, &u, 1);
                osal_int_to_string(nbuf, sizeof(nbuf), (os_long)((os_uint)u));
                os_strncat(text, nbuf, sizeof(text));
            }

            osal_trace(text);
        }
        os_sleep(1000);
    }

    ioc_release_root(root);
}


/**
****************************************************************************************************

  @brief Callback function.

  The iocontroller_callback() function is called when changed data is received from connection.

  No heavy processing or printing data should be placed in callback. The callback should return
  quickly. Readon is that the communication must be albe to process all data it receives,
  and delays here will cause connection buffers to fill up, which at wors could cause time shift
  like deltay in communication.

  @param   mblk Pointer to the memory block object.
  @param   start_addr Address of first changed byte.
  @param   end_addr Address of the last changed byte.
  @param   flags Reserved  for future.
  @param   context Application specific pointer passed to this callback function.

  @return  None.

****************************************************************************************************
*/
static void iocontroller_callback(
    struct iocMemoryBlock *mblk,
    int start_addr,
    int end_addr,
    os_ushort flags,
    void *context)
{
    static os_long count;

    callbackdata.count = ++count;
    callbackdata.start_addr = start_addr;
    callbackdata.end_addr = end_addr;
}
