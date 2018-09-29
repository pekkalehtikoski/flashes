/**

  @file    flash_write.h
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
#include "flashes.h"


/**
****************************************************************************************************

  @brief Write program binary to flash memory.
  @anchor flash_write

  The flash_write() function writes nbytes data from buffer to flash memory. When writing a
  bigger block, this function is called repeatedly from smallest address to large
  one. This allows erashing flash as needed.

  @param   addr Flash address. Address 0 is the beginning of the flags. This is bank 1
           address. To write to bank 2 use bank 1 address, but set bank2 flag. This address
           needs to be divisible by by 4 (minimum write size is dword).
  @param   buf Pointer to data to write.
  @param   nbytes This needs to be divisible by 4 (minimum write size is dword).
  @param   bank2 OS_FALSE to write to bank1, OS_TRUE to write to bank 2.
  @param   next_sector_to_erase Pointer to erase tracking variable. Set to value of
           next_sector_to_erase to zero before the first flash_write() call. For following
           calls, pass the same pointer. This function modifies the variable as needed.

  @return  OSAL_SUCCESS if all is fine. Other values indicate an error.

****************************************************************************************************
*/
osalStatus flash_write(
    os_uint addr,
    os_uchar* buf,
    os_uint nbytes,
    os_boolean bank2,
    os_uint *next_sector_to_erase)
{
    os_char strbuf[64];

    osal_console_write("writing ");
    osal_int_to_string(strbuf, sizeof(strbuf), nbytes);
    osal_console_write(strbuf);
    osal_console_write(" bytes at bank ");
    osal_console_write(bank2 ? "2" : "1");
    osal_console_write(" address ");
    osal_int_to_string(strbuf, sizeof(strbuf), addr);
    osal_console_write(strbuf);
    osal_console_write("\n");

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Check which bank is currently selected?
  @anchor flash_is_bank2_selected

  The flash_is_bank2_selected() function checks if we are currently running from flash bank 2.

  Option bit FLASH_OPTCR_BFB2 bit is "boot from bank 2".

  @return  OS_TRUE if running from bank 2, OS_FALSE if running from bank 1.

****************************************************************************************************
*/
os_boolean flash_is_bank2_selected(void)
{
    osal_console_write("check for selected bank, bank 1 returned\n");
    return OS_FALSE;
}


/**
****************************************************************************************************

  @brief Set bank to boot from and reboot.
  @anchor flash_select_bank

  The flash_is_bank2_selected() function checks if we are currently running from flash bank 2.

  @param   bank2 OS_TRUE to select flash bank 2, or OS_FALSE to select bank 1.
  @return  OSAL_SUCCESS (0) if all is fine. Other values indicate an error.

****************************************************************************************************
*/
osalStatus flash_select_bank(
    os_boolean bank2)
{
    osal_console_write(bank2 ? "bank 2 selected\n" : "bank 1 selected\n");
    return OSAL_SUCCESS;
}
