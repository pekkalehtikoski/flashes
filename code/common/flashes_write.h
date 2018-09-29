/**

  @file    flashes_write.h
  @brief   Write program to flash.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    24.9.2018

  X...

  Copyright 2018 Pekka Lehtikoski. This file is part of the iocom project and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#ifndef FLASHES_WRITE_INCLUDED
#define FLASHES_WRITE_INCLUDED



/**
****************************************************************************************************

  @name X...

  The X..

****************************************************************************************************
 */
/*@{*/


/* Write program binary to flash memory.
 */
osalStatus flash_write(
    os_uint addr,
    os_uchar* buf,
    os_uint nbytes,
    os_boolean bank2,
    os_uint *next_sector_to_erase);

/* Check which bank is currently selected?
 */
os_boolean flash_is_bank2_selected(void);

/* Set bank to boot from and reboot.
 */
osalStatus flash_select_bank(
    os_boolean bank2);

/*@}*/

#endif
