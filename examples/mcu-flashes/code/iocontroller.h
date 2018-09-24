/**

  @file    iocontroller.h
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
#include "iocom/iocom.h"

void iocontroller(void);
