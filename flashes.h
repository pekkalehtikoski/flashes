/**

  @file    flashes.h
  @brief   Main library header file.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    24.9.2018

  The flashes library main header file. If further includes rest of the library headers.

  Copyright 2018 Pekka Lehtikoski. This file is part of the iocom project and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef FLASHES_INCLUDED
#define FLASHES_INCLUDED

/* turn trace on for nor... */
#define OSAL_TRACE 1

/* Include operating system abstraction layer with extension headers.
 */
#include "eosalx.h"

/* If C++ compilation, all functions, etc. from this point on in included headers are
   plain C and must be left undecorated.
 */
OSAL_C_HEADER_BEGINS

/* Include all flashes library headers.
 */
// #include "iocom/code/ioc_root.h"

/* If C++ compilation, end the undecorated code.
 */
OSAL_C_HEADER_ENDS

#endif
