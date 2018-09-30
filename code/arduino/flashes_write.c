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

#define OSAL_TRACE 2
#include "flashes.h"

#include "Arduino.h"
// #ifdef STM32F429xx

#ifdef STM32F4xx
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_ll_system.h"
#endif

/* Base address of the flash sectors, bank 1 THESE ARE VERY CHIP SPECIFIC
 */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */

#define ADDR_BANK_1_START       ((uint32_t)0x08000000)
#define ADDR_BANK_2_START       ((uint32_t)0x08100000)
#define FIRST_BANK_2_SECTOR     FLASH_SECTOR_12

static os_uint flash_get_sector(
    os_uint addr);

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
    static FLASH_EraseInitTypeDef eraseprm;
    os_uint first_sector, last_sector, progaddr;
    uint32_t secerror = 0;
    const os_uint dword_sz = sizeof(uint32_t);
    osalStatus err_rval = OSAL_STATUS_FAILED;

    os_char strbuf[64];

    /* Move to the beginning of STM32 flash banks.
     * Programming address is always bank 2. Erase sector handled differently.
     */
    progaddr = addr + ADDR_BANK_2_START;
    addr += bank2 ? ADDR_BANK_2_START : ADDR_BANK_1_START;

#if OSAL_TRACE >= 2
    osal_console_write("writing ");
    osal_int_to_string(strbuf, sizeof(strbuf), nbytes);
    osal_console_write(strbuf);
    osal_console_write(" bytes at bank ");
    osal_console_write(bank2 ? "2" : "1");
    osal_console_write(" address ");
    osal_int_to_string(strbuf, sizeof(strbuf), addr);
    osal_console_write(strbuf);
    osal_console_write("\n");
#endif

    /* Unlock the flash.
     */
    HAL_FLASH_Unlock();

    /* The first and last sector to write
     */
    first_sector = flash_get_sector(addr);
    last_sector = flash_get_sector(addr + nbytes - 1);

    /* Erase if not done already
     */
    if (last_sector >= *next_sector_to_erase)
    {
        if (first_sector < *next_sector_to_erase)
        {
            first_sector = *next_sector_to_erase;
        }

        /* Set erase parameter structure
         */
        os_memclear(&eraseprm, sizeof(eraseprm));
        eraseprm.TypeErase = TYPEERASE_SECTORS;
        eraseprm.VoltageRange = VOLTAGE_RANGE_3;
        eraseprm.Sector = first_sector;
        eraseprm.NbSectors = last_sector - first_sector + 1;
//        eraseprm.Banks = bank2 ? FLASH_BANK_2 : FLASH_BANK_1;

#if OSAL_TRACE >= 2
        osal_console_write("erasing ");
        osal_int_to_string(strbuf, sizeof(strbuf), eraseprm.NbSectors);
        osal_console_write(strbuf);
        osal_console_write(" sectors starting from  ");
        osal_int_to_string(strbuf, sizeof(strbuf), eraseprm.Sector);
        osal_console_write(strbuf);
        osal_console_write("\n");
#endif

        /* Erase the flags as we go.
         */
        if (HAL_FLASHEx_Erase(&eraseprm, &secerror) != HAL_OK)
        {
            osal_debug_error("HAL_FLASHEx_Erase failed");
            goto failed;
        }

        /* Maintain next unerased sector.
         */
        *next_sector_to_erase = last_sector + 1;
    }


#if 0
// fast_block_sz = 256
    /* FAST 8 x 32 bytes. Calculate number of 256 byte writes.
     */
    n = nbytes / fast_block_sz;
    for (i = 0; i<n; i++)
    {
        if (HAL_FLASH_Program(i == n-1 ? FLASH_TYPEPROGRAM_FAST_AND_LAST
            : FLASH_TYPEPROGRAM_FAST, addr, (uint32_t)buf) == HAL_OK)
        {
            osal_debug_error("HAL_FLASH_Program failed");
            goto failed;
        }

        buf += fast_block_sz;
        addr += fast_block_sz;
        nbytes -= fast_block_sz;
    }
#endif

    /* Write rest as dwords
     * Your flash start address when programming should always be 0x08100000 (2 MB flash).
     */
    while (nbytes > 0)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, progaddr, *(uint32_t*)buf) == HAL_OK)
        {
            buf += dword_sz;
            progaddr += dword_sz;
            nbytes -= dword_sz;
        }
        else
        {
            osal_debug_error("HAL_FLASH_Program failed");
            goto failed;
        }
    }

    /* Lock the flash.
     */
    HAL_FLASH_Lock();
    return OSAL_SUCCESS;

failed:
    HAL_FLASH_Lock();
    return err_rval;
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
    FLASH_AdvOBProgramInitTypeDef AdvOBInit;
    os_boolean bank2;
    uint32_t bankmode;

    os_memclear(&AdvOBInit, sizeof(AdvOBInit));

    /* Allow access to flash control registers and user falsh, and to option bytes sector.
     */
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();

    /* Get the dual boot configuration status.
     */
    AdvOBInit.OptionType = OBEX_BOOTCONFIG;
    HAL_FLASHEx_AdvOBGetConfig(&AdvOBInit);

    /* If we are dual boot is currently enabled?
     */
    bank2 = (((AdvOBInit.BootConfig) & (FLASH_OPTCR_BFB2)) == FLASH_OPTCR_BFB2) ? OS_TRUE : OS_FALSE;

    bankmode = LL_SYSCFG_GetFlashBankMode();

    /* Put lock back to place.
     */
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();

#if OSAL_TRACE >= 2
    osal_console_write("check for selected bank, ");
    osal_console_write(bank2 ? "bank 2 returned\n" : "bank 1 returned\n");
    osal_console_write("bank mode, ");
    osal_console_write(bankmode ==  LL_SYSCFG_BANKMODE_BANK2 ? "bank 2\n" : "bank 1\n");
#endif

    if (bank2 && bankmode !=  LL_SYSCFG_BANKMODE_BANK2)
    {
        flash_select_bank(OS_FALSE);
    }

    return bank2 && (bankmode ==  LL_SYSCFG_BANKMODE_BANK2);
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
    FLASH_AdvOBProgramInitTypeDef AdvOBInit;
    osalStatus rval = OSAL_SUCCESS;

    os_memclear(&AdvOBInit, sizeof(AdvOBInit));

    /* Allow access to flash control registers and user falsh, and to option bytes sector.
     */
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();

    /* Get the dual boot configuration status.
     */
    AdvOBInit.OptionType = OBEX_BOOTCONFIG;
    HAL_FLASHEx_AdvOBGetConfig(&AdvOBInit);

    /* Enable or Disable dual boot feature accorging to bank2 argument.
     */
    if (bank2)
    {
        AdvOBInit.BootConfig = OB_DUAL_BOOT_ENABLE;
        HAL_FLASHEx_AdvOBProgram (&AdvOBInit);
    }
    else {
        AdvOBInit.BootConfig = OB_DUAL_BOOT_DISABLE;
        HAL_FLASHEx_AdvOBProgram (&AdvOBInit);
    }

    /* Start the option bytes programming process */
    if (HAL_FLASH_OB_Launch() != HAL_OK)
    {
        rval = OSAL_STATUS_FAILED;
    }

    /* Put lock back to place.
     */
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();

#if OSAL_TRACE >= 2
    osal_console_write("setting boot bank boot: ");
    osal_console_write(bank2 ? "bank 2\n" : "bank 1\n");
#endif

    return rval;
}



/**
****************************************************************************************************

  @brief Get sector number by address.
  @anchor flash_get_sector

  The flash_get_sector() function converts bank 1 address to sector number. Flash is erased
  by sectors.

  @param   addr Flash address. Address 0x08000000 is the beginning of the flagh.
  @return  Sector number for the given address.

****************************************************************************************************
*/
static os_uint flash_get_sector(
    os_uint addr)
{
    os_uint sector;

    if (addr >= ADDR_BANK_2_START)
    {
        return FIRST_BANK_2_SECTOR + flash_get_sector(addr - ADDR_BANK_2_START + ADDR_BANK_1_START);
    }

    if (addr < ADDR_FLASH_SECTOR_1)
    {
        sector = FLASH_SECTOR_0;
    }
    else if (addr < ADDR_FLASH_SECTOR_2)
    {
        sector = FLASH_SECTOR_1;
    }
    else if (addr < ADDR_FLASH_SECTOR_3)
    {
        sector = FLASH_SECTOR_2;
    }
    else if (addr < ADDR_FLASH_SECTOR_4)
    {
        sector = FLASH_SECTOR_3;
    }
    else if (addr < ADDR_FLASH_SECTOR_5)
    {
        sector = FLASH_SECTOR_4;
    }
    else
    {
        sector = FLASH_SECTOR_5 + (addr - ADDR_FLASH_SECTOR_5) / 0x20000; // 128 Kbytes
    }

    return sector;
}
