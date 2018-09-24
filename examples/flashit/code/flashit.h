/**

  @file    ioboard.h
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
#include "iocom/iocom.h"

/* Define "inputs" and "outputs" memory block sizes in bytes.
 */
#define IOBOARD_INPUT_BLOCK_SZ 432
#define IOBOARD_OUTPUT_BLOCK_SZ 64

/* Use code template for basic network IO board.
 */
#include "iocom/codetemplates/ioboard_basic_static_network_io.h"

/* Example IO board functionality.
 */
void ioboard(void);
