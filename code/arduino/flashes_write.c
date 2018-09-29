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

#include "Arduino.h"
#include "stm32f4xx_hal_flash.h"


/* Base address of the flash sectors, bank 1
 */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */


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
    os_uint first_sector, last_sector;
    uint32_t secerror = 0;
    const os_uint dword_sz = sizeof(uint32_t);
    osalStatus err_rval = OSAL_STATUS_FAILED;

    /* Move to befinning of STM32 flash.
     */
    addr += 0x08000000;

    /* Unlock the flash.
     */
    HAL_FLASH_Unlock();

    /* The first and last sector to write
     */
    first_sector = flash_get_sector(addr);
    last_sector = flash_get_sector(addr - nbytes + 1);

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
        eraseprm.Banks = bank2 ? FLASH_BANK_2 : FLASH_BANK_1;

        /* Erase the flags as we go.
         */
        if (HAL_FLASHEx_Erase(&eraseprm, &secerror) != HAL_OK)
        {
            //  FLASH_ErrorTypeDef errorcode = HAL_FLASH_GetError();
            // ?????????????????????????????????????????????????????????????????????????????????????????????????
            goto failed;
        }

        /* Maintain next unerased sector.
         */
        *next_sector_to_erase = last_sector + 1;
    }

    /* If we are writing bank 2, switch to bank 2 address.
     */
    if (bank2) addr += 0x100000;

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
            //  FLASH_ErrorTypeDef errorcode = HAL_FLASH_GetError();
            // ?????????????????????????????????????????????????????????????????????????????????????????????????
            goto failed;
        }

        buf += fast_block_sz;
        addr += fast_block_sz;
        nbytes -= fast_block_sz;
    }
#endif

    /* Write rest as dwords
     */
    while (nbytes > 0)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, *(uint32_t*)buf) == HAL_OK)
        {
            buf += dword_sz;
            addr += dword_sz;
            nbytes -= dword_sz;
        }
        else
        {
            //  FLASH_ErrorTypeDef errorcode = HAL_FLASH_GetError();
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

    /* Put lock back to place.
     */
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();

    return bank2;
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
    // FLASH_OBProgramInitTypeDef    OBInit;
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
Serial.println("Set next boot to bank 2");
    }
    else {
        AdvOBInit.BootConfig = OB_DUAL_BOOT_DISABLE;
        HAL_FLASHEx_AdvOBProgram (&AdvOBInit);
Serial.println("Set next boot to bank 1");
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

    return rval;
}



/**
****************************************************************************************************

  @brief Get sector number by address.
  @anchor flash_get_sector

  The flash_get_sector() function converts bank 1 address to sector number. Flash is erased
  by sectors.


  @param   addr Flash address. Address 0x08000000 is the beginning of the flags. This is bank 1
           address.
  @return  Sector number for the given address.

****************************************************************************************************
*/
static os_uint flash_get_sector(
    os_uint addr)
{
  os_uint sector;

  if((addr < ADDR_FLASH_SECTOR_1) && (addr >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;
  }
  else if((addr < ADDR_FLASH_SECTOR_2) && (addr >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;
  }
  else if((addr < ADDR_FLASH_SECTOR_3) && (addr >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;
  }
  else if((addr < ADDR_FLASH_SECTOR_4) && (addr >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;
  }
  else if((addr < ADDR_FLASH_SECTOR_5) && (addr >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;
  }
  else
  {
    sector = FLASH_SECTOR_5;
  }

  return sector;
}
