/*
 * Debug Sub-System (DebugSS) driver for Keystone and DRA7xx devices
 *
 * Copyright (C) 2014 Texas Instruments.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define DRIVER_NAME "debugss_module_drv"

#ifdef K2E_PLATFORM
#define KEYSTONE_PLATFORM
#define NO_DEBUGSS_ARM_PLL
#endif

#ifdef KEYSTONE_PLATFORM

#define DEBUGSS_KM_MAJOR_VERSION 1
#define DEBUGSS_KM_MINOR_VERSION 3
#define DEBUGSS_KM_PATCH_VERSION 0

/* enum defining indexes for all the valid debugss address ranges */
/* For K2H platform */
typedef enum {
    eK2H_DEBUGSS_ADDR_RANGE_IDX0 = 0,
    eK2H_DEBUGSS_ADDR_RANGE_IDX1,
    eK2H_DEBUGSS_ADDR_RANGE_IDX2,
    eK2H_DEBUGSS_ADDR_RANGE_IDX3,
    eK2H_DEBUGSS_ADDR_RANGE_IDX4,
    eK2H_DEBUGSS_ADDR_RANGE_IDX5,
    eK2H_DEBUGSS_NUM_RANGE_MAX
}ek2h_debugss_addr_range_idx;

/* Note: A new platform specific enum needs to be added here, whenever support
   for a new platform needs to be added */

/* Static data structure for storing all the valid debugss address ranges */
/* For K2H platform */
static unsigned int k2h_debugss_addr_range[eK2H_DEBUGSS_NUM_RANGE_MAX][2] = {
    {0x01100000, 0x0110FFFC}, /* ARM STM stimulus port address range */      
    {0x01D00000, 0x01DE807C}, /* CP tracers address range */
    {0x02440000, 0x024B3FFC}, /* C66x CorePAC ADTF address range */
    {0x027D0000, 0x02857FFC}, /* Embedded Trace Buffer (ETB) address range */
    {0x03000000, 0x030FFFFC}, /* DebugSS configuration address range */
    {0x20000000, 0x200FFFFC}  /* STM configuration address range */
};

//JTAGID-part numbers based identifiers
#define TCI6614_JTAGID_PARTNUMBER       (0xB962)
#define C66AK2Hxx_JTAGID_PARTNUMBER      (0xB981)
#define C66AK2Exx_JTAGID_PARTNUMBER      (0xB9A6)
#define TCI6630K2L_JTAGID_PARTNUMBER    (0xB9A7)

/* enum for supported keystone device/platform types */
typedef enum {
    eNotSupported_device = -1, /* device is Not supported */
    e66AK2Hxx_device = 0, /* device is 66AK2Hxx, TCI6636K2H, TCI6634K2K, TCI6638K2K and 6636AK2Hxx */
    eTCI6614_device, /* device is TCI6614 */
    e66AK2Exx_device, /* device is 66AK2Exx */
    eTCI6630K2L_device, /* device is TCI6630 */
    eMAX_DEVICES
}ekeystone_device_type;

/* Macros for BOOTCFG JTAGID register */
#define BOOTCFG_BASE_ADDR            (0x02620000)
#define BOOTCFG_JTAGID_REG           (0x18)

#define PSC_BASE                     (0x02350000)
#define PSC_PTCMD                    (0x120)
#define PSC_PTSTAT                   (0x128)
#define PSC_DEBUGSS_PD               (1)
#define PSC_DEBUGSS_MD               (5)
#define PSC_DEBUGSS_MDCTL            (0xA00 + (4 * PSC_DEBUGSS_MD))
#define PSC_DEBUGSS_MDSTAT           (0x800 + (4 * PSC_DEBUGSS_MD))
#define PSC_DEBUGSS_PDCTL            (0x300 + (4 * PSC_DEBUGSS_PD))

#define PSC_ENABLE                   (0x3)

#endif

#ifdef DRA7xx_PLATFORM

#define DEBUGSS_KM_MAJOR_VERSION 1
#define DEBUGSS_KM_MINOR_VERSION 0
#define DEBUGSS_KM_PATCH_VERSION 0

/* enum defining indexes for all the valid debugss address ranges */
/* For DRA7xx platform */
typedef enum {
    eDRA7xx_DEBUGSS_ADDR_RANGE_IDX0 = 0,
    eDRA7xx_DEBUGSS_ADDR_RANGE_IDX1,
    eDRA7xx_DEBUGSS_ADDR_RANGE_IDX2,
    eDRA7xx_DEBUGSS_ADDR_RANGE_IDX3,
    eDRA7xx_DEBUGSS_ADDR_RANGE_IDX4,
    eDRA7xx_DEBUGSS_ADDR_RANGE_IDX5,
    eDRA7xx_DEBUGSS_NUM_RANGE_MAX
}edra7xx_debugss_addr_range_idx;

/* Note: A new platform specific enum needs to be added here, whenever support
   for a new platform needs to be added */

/* Static data structure for storing all the valid debugss address ranges */
/* For DRA7xx platform */
static unsigned int dra7xx_debugss_addr_range[eDRA7xx_DEBUGSS_NUM_RANGE_MAX][2] = {
    {0x45000000, 0x4500AFFC}, /* Statistics collectors address range */      
    {0x47000000, 0x47FFFFFC}, /* MPU_CS_STM config address range */
    {0x4A005000, 0x4A005FFC}, /* CMI (L4_CFG module target port) address range */
    {0x4AE06000, 0x4AE07FFC}, /* PMI (L4_WKUP module target port) address range */
    {0x54000000, 0x541FFFFC}, /* L3_INSTR address range */
    {0x54800000, 0x54FFFFFC} /* CT_TBR address range */
};

//DRA7xx device base PN
#define DRA742_BASE_PN		(3)
#define DRA744_BASE_PN		(5)
#define DRA745_BASE_PN		(6)
#define DRA746_BASE_PN		(7)
#define DRA750_BASE_PN		(9)
#define DRA751_BASE_PN		(10)
#define DRA752_BASE_PN		(11)
#define DRA754_BASE_PN		(13)
#define DRA755_BASE_PN		(14)
#define DRA756_BASE_PN		(15)
#define AM5726_BASE_PN		(57)
#define AM5726E_BASE_PN		(58)
#define AM5728_BASE_PN		(59)
#define AM5728E_BASE_PN		(60)
#define AM5728_3E_BASE_PN	(62)
#define OMAP572x_BASE_PN    (63)

//AM571x ID_CODE-RAMP_SYSTEM value
#define AM571x_RAMP_SYSTEM      (0xB9BC)

/* enum for supported DRA7xx device/platform types */
typedef enum {
    eNotSupported_device = -1, /* device is Not supported */
    eDRA74xx_device = 0, /* device is DRA74xx */
    eDRA75xx_device, /* device is DRA75xx */
    eAM572x_device, /* device is AM572x */
    eAM571x_device, /* device is AM571x */
    eMAX_DEVICES
}edra7xx_device_type;

/* Macros for device identification */
#define DIE_ID_BASE_ADDR            (0x4AE0C000)
#define DIE_ID_2_REG                (0x20C)
#define ID_CODE_REG                  (0x204)

/* Macros for DebugSS PLLCTRL registers */
#define DPLL_DEBUG_BASE_ADDR         (0x54171000)
#define PLLCTRL_STATUS_REG           (0x804)
#define PLLCTRL_GO_REG               (0x808)
#define PLLCTRL_CONFIG1_REG          (0x80C)
#define PLLCTRL_CONFIG2_REG          (0x810)
#define PLLCTRL_CONFIG3_REG          (0x814)

//L3 INSTR clock registers
#define CM_L3INSTR_L3_3_BASE_ADDR    (0x4A008000)
#define CM_L3INSTR_L3_3_CLKCTRL      (0xE20)
#define CM_L3INSTR_L3_INSTR_CLKCTRL  (0xE28)

//EMU PRCM registers
#define EMU_CM_BASE_ADDR             (0x4AE07000)
#define CM_EMU_CLKSTCTRL             (0xA00)

#endif 
  
