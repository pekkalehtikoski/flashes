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

/* Select either boot loader mode or dual bank mode
 */
#define FLASHES_BOOT_LOADER_MODE 0
#define FLASHES_DUAL_BANK_MODE 1


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

/* In boot loaded mode
 */
#define APPLICATION_BASE_ADDR ADDR_FLASH_SECTOR_5

static os_uint flashes_get_sector(
    os_uint addr);

/**
****************************************************************************************************

  @brief Write program binary to flash memory.
  @anchor flashes_write

  The flashes_write() function writes nbytes data from buffer to flash memory. When writing a
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
osalStatus flashes_write(
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

#if FLASHES_DUAL_BANK_MODE
    /* Move to the beginning of STM32 flash banks.
     * Programming address is always bank 2.
     * Erase sector handled handled by address within bank beging written.
     */
    progaddr = addr + ADDR_BANK_2_START;
    addr += bank2 ? ADDR_BANK_2_START : ADDR_BANK_1_START;
#else
    /* Boot loader mode. We always write bank 1.
     */
    bank2 = OS_FALSE;
    addr += APPLICATION_BASE_ADDR;
    progaddr = addr;
#endif

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
    first_sector = flashes_get_sector(addr);
    last_sector = flashes_get_sector(addr + nbytes - 1);

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
  @anchor flashes_is_bank2_selected

  The flashes_is_bank2_selected() function checks if we are currently running from flash bank 2.

  Option bit FLASH_OPTCR_BFB2 bit is "boot from bank 2".

  @return  OS_TRUE if running from bank 2, OS_FALSE if running from bank 1.

****************************************************************************************************
*/
os_boolean flashes_is_bank2_selected(void)
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

#if FLASHES_BOOT_LOADER_MODE
    /* In boot loader mode: If bank to is active, switch to bank 1.
     */
    if (bank2)
    {
        AdvOBInit.BootConfig = OB_DUAL_BOOT_DISABLE;

        /* Start the option bytes programming process */
        if (HAL_FLASH_OB_Launch() != HAL_OK)
        {
            osal_debug_error("HAL_FLASH_OB_Launch failed");
        }
    }
#endif

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
        flashes_select_bank(OS_FALSE);
    }

    return bank2 && (bankmode ==  LL_SYSCFG_BANKMODE_BANK2);
}


/**
****************************************************************************************************

  @brief Set bank to boot from and reboot.
  @anchor flashes_select_bank

  The flashes_is_bank2_selected() function checks if we are currently running from flash bank 2.

  @param   bank2 OS_TRUE to select flash bank 2, or OS_FALSE to select bank 1.
  @return  OSAL_SUCCESS (0) if all is fine. Other values indicate an error.

****************************************************************************************************
*/
osalStatus flashes_select_bank(
    os_boolean bank2)
{
    osalStatus rval = OSAL_SUCCESS;

#if FLASHES_DUAL_BANK_MODE
    FLASH_AdvOBProgramInitTypeDef AdvOBInit;
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
static os_uint flashes_get_sector(
    os_uint addr)
{
    os_uint sector;

    if (addr >= ADDR_BANK_2_START)
    {
        return FIRST_BANK_2_SECTOR + flashes_get_sector(addr - ADDR_BANK_2_START + ADDR_BANK_1_START);
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


#if 0
void nvicDisableInterrupts() {
    NVIC_TypeDef *rNVIC = (NVIC_TypeDef *) NVIC_BASE;
    rNVIC->ICER[0] = 0xFFFFFFFF;
    rNVIC->ICER[1] = 0xFFFFFFFF;
    rNVIC->ICPR[0] = 0xFFFFFFFF;
    rNVIC->ICPR[1] = 0xFFFFFFFF;

    SET_REG(STK_CTRL, 0x04); /* disable the systick, which operates separately from nvic */
}

void systemReset(void) {
    SET_REG(RCC_CR, GET_REG(RCC_CR)     | 0x00000001);
    SET_REG(RCC_CFGR, GET_REG(RCC_CFGR) & 0xF8FF0000);
    SET_REG(RCC_CR, GET_REG(RCC_CR)     & 0xFEF6FFFF);
    SET_REG(RCC_CR, GET_REG(RCC_CR)     & 0xFFFBFFFF);
    SET_REG(RCC_CFGR, GET_REG(RCC_CFGR) & 0xFF80FFFF);

    SET_REG(RCC_CIR, 0x00000000);  /* disable all RCC interrupts */
}
#endif

void setMspAndJump(os_uint usrAddr) {
    // Dedicated function with no call to any function (appart the last call)
    // This way, there is no manipulation of the stack here, ensuring that GGC
    // didn't insert any pop from the SP after having set the MSP.
    typedef void (*funcPtr)(void);
    os_uint jumpAddr = *(volatile os_uint *)(usrAddr + 0x04); /* reset ptr in vector table */

    funcPtr usrMain = (funcPtr) jumpAddr;

//    SET_REG(SCB_VTOR, (vu32) (usrAddr));
    SCB->VTOR = usrAddr;

    asm volatile("msr msp, %0"::"g"(*(volatile os_uint *)usrAddr));

    usrMain();                                /* go! */
}

#define EnablePrivilegedMode() __asm("SVC #0")

void flashes_jump_to_application(void)
{
#if FLASHES_BOOT_LOADER_MODE

// os_uint aaa = APPLICATION_BASE_ADDR;
os_uint *Address = (os_uint *)0x00020000;
// os_uint *Address = (os_uint *)APPLICATION_BASE_ADDR;

 if( CONTROL_nPRIV_Msk & __get_CONTROL( ) )
  {  /* not in privileged mode */
    EnablePrivilegedMode( ) ;
  }

NVIC->ICER[ 0 ] = 0xFFFFFFFF ;
NVIC->ICER[ 1 ] = 0xFFFFFFFF ;
NVIC->ICER[ 2 ] = 0xFFFFFFFF ;
NVIC->ICER[ 3 ] = 0xFFFFFFFF ;
NVIC->ICER[ 4 ] = 0xFFFFFFFF ;
NVIC->ICER[ 5 ] = 0xFFFFFFFF ;
NVIC->ICER[ 6 ] = 0xFFFFFFFF ;
NVIC->ICER[ 7 ] = 0xFFFFFFFF ;

NVIC->ICPR[ 0 ] = 0xFFFFFFFF ;
NVIC->ICPR[ 1 ] = 0xFFFFFFFF ;
NVIC->ICPR[ 2 ] = 0xFFFFFFFF ;
NVIC->ICPR[ 3 ] = 0xFFFFFFFF ;
NVIC->ICPR[ 4 ] = 0xFFFFFFFF ;
NVIC->ICPR[ 5 ] = 0xFFFFFFFF ;
NVIC->ICPR[ 6 ] = 0xFFFFFFFF ;
NVIC->ICPR[ 7 ] = 0xFFFFFFFF ;

SysTick->CTRL = 0 ;
SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk ;

SCB->SHCSR &= ~( SCB_SHCSR_USGFAULTENA_Msk | \
                 SCB_SHCSR_BUSFAULTENA_Msk | \
                 SCB_SHCSR_MEMFAULTENA_Msk ) ;

// Activate the MSP, if the core is found to currently run with the PSP.
if( CONTROL_SPSEL_Msk & __get_CONTROL( ) )
{  /* MSP is not active */
  __set_CONTROL( __get_CONTROL( ) & ~CONTROL_SPSEL_Msk ) ;
}

// Load the vector table address of the user application into SCB->VTOR register. Make sure the address meets the alignment requirements.
SCB->VTOR = ( uint32_t )Address;

// A few device families, like the NXP 4300 series, will also have a "shadow pointer" to the VTOR, which also needs to be updated with the new address. Review the device datasheet to see if one exists.
//Set the MSP to the value found in the user application vector table.
__set_MSP( Address[ 0 ] ) ;

// Set the PC to the reset vector value of the user application via a function call.
( ( void ( * )( void ) )Address[ 1 ] )( ) ;

#if 0
    /* tear down all the dfu related setup */
    // disable usb interrupts, clear them, turn off usb, set the disc pin
    // todo pick exactly what we want to do here, now its just a conservative
//    flashLock();
//    usbDsbISR();
//    nvicDisableInterrupts();
    HAL_NVIC_DisableIRQ(0);
    HAL_NVIC_DisableIRQ(1);
    HAL_NVIC_DisableIRQ(2);
    HAL_NVIC_DisableIRQ(3);
    HAL_NVIC_DisableIRQ(4);




// Does nothing, as PC12 is not connected on teh Maple mini according to the schemmatic     setPin(GPIOC, 12); // disconnect usb from host. todo, macroize pin
    // systemReset(); // resets clocks and periphs, not core regs

    // HAL_NVIC_SystemReset();

    setMspAndJump(APPLICATION_BASE_ADDR);
#endif
#endif
}
