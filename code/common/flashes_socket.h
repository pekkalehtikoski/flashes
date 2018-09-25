/**

  @file    flashes_socket.h
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

/** Default flashes library socket port as string. This can be appended to IP address.
 */
#define FLASHES_SOCKET_PORT_STR ":6827"

/* API functions.
 */

/* Open listening socket port to wait for binary program.
 */
void flashes_socket_setup(void);

/* Close listening socket and interrupt any ongoing program transfer.
 */
void flashes_socket_cleanup(void);


void flashes_socket_loop(void);
