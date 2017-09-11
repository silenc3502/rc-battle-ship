/*
 * prussdrv.h
 * 
 * Describes PRUSS userspace driver for Industrial Communications
 *
 * Copyright (C) 2010-2016 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

#ifndef _PRUSSDRV_H
#define _PRUSSDRV_H

#include <sys/types.h>
#include <pthread.h>

#if defined (__cplusplus)
extern "C" {
#endif

#define NUM_PRU_HOSTIRQS        8
#define NUM_PRU_HOSTS          10
#define NUM_PRU_CHANNELS       10
#define NUM_PRU_SYS_EVTS       64

#define PRUSS_PRU0_DATARAM     0
#define PRUSS_PRU1_DATARAM     1
#define PRUSS_PRU0_IRAM        2
#define PRUSS_PRU1_IRAM        3

#define PRUSS_SHARED_DATARAM   4
#define	PRUSS_CFG              5
#define	PRUSS_UART             6
#define	PRUSS_IEP              7
#define	PRUSS_ECAP             8
#define	PRUSS_MII_RT           9
#define	PRUSS_MDIO            10

#define PRU_EVTOUT_0            0
#define PRU_EVTOUT_1            1
#define PRU_EVTOUT_2            2
#define PRU_EVTOUT_3            3
#define PRU_EVTOUT_4            4
#define PRU_EVTOUT_5            5
#define PRU_EVTOUT_6            6
#define PRU_EVTOUT_7            7

#define PRUSSDRV_ERROR_INVALID_NAME           (-2)
#define PRUSSDRV_ERROR_OPEN_PRUSS_MEM         (-3)
#define PRUSSDRV_ERROR_OPEN_PRUSS_MEM2        (-4)
#define PRUSSDRV_ERROR_OPEN_PRU0_MEM          (-5)
#define PRUSSDRV_ERROR_OPEN_PRU1_MEM          (-6)
#define PRUSSDRV_ERROR_DRAM0_MEM_SECTION      (-7)
#define PRUSSDRV_ERROR_DRAM1_MEM_SECTION      (-8)
#define PRUSSDRV_ERROR_INTC_MEM_SECTION       (-9)
#define PRUSSDRV_ERROR_SHRDRAM2_MEM_SECTION  (-10)
#define PRUSSDRV_ERROR_CFG_MEM_SECTION       (-11)
#define PRUSSDRV_ERROR_IEP_MEM_SECTION       (-12)
#define PRUSSDRV_ERROR_UART_MEM_SECTION      (-13)
#define PRUSSDRV_ERROR_ECAP_MEM_SECTION      (-14)
#define PRUSSDRV_ERROR_MDIO_MEM_SECTION      (-15)
#define PRUSSDRV_ERROR_MIIRT_MEM_SECTION     (-16)
#define PRUSSDRV_ERROR_IRAM_MEM_SECTION      (-17)
#define PRUSSDRV_ERROR_CONTROL_MEM_SECTION   (-18)
#define PRUSSDRV_ERROR_DEBUG_MEM_SECTION     (-19)
#define PRUSSDRV_ERROR_UNSUPPORTED_VERSION   (-20)

#define PRUSSDRV_ERROR_OPEN_GET_UIO_DEVICE   (-1)
#define PRUSSDRV_ERROR_OPEN_DISABLE_INTRPT   (-2)
#define PRUSSDRV_ERROR_OPEN_ENABLE_INTRPT    (-3)
#define PRUSSDRV_ERROR_OPEN_UIOFD_NONZERO    (-4)

typedef void *(*prussdrv_function_handler) (void *);
typedef struct __sysevt_to_channel_map {
    short sysevt;
    short channel;
} tsysevt_to_channel_map;
typedef struct __channel_to_host_map {
    short channel;
    short host;
} tchannel_to_host_map;
typedef struct __pruss_intc_initdata {
    //Enabled SYSEVTs - Range:0..63
    //{-1} indicates end of list
    char sysevts_enabled[NUM_PRU_SYS_EVTS];
    //SysEvt to Channel map. SYSEVTs - Range:0..63 Channels -Range: 0..9
    //{-1, -1} indicates end of list
    tsysevt_to_channel_map sysevt_to_channel_map[NUM_PRU_SYS_EVTS];
    //Channel to Host map.Channels -Range: 0..9  HOSTs - Range:0..9
    //{-1, -1} indicates end of list
    tchannel_to_host_map channel_to_host_map[NUM_PRU_CHANNELS];
    //10-bit mask - Enable Host0-Host9 {Host0/1:PRU0/1, Host2..9 : PRUEVT_OUT0..7)
    unsigned int host_enable_bitmask;
} tpruss_intc_initdata;

int prussdrv_init(void **prussdrv_handle, char *pruss_name);

int prussdrv_open(void *prussdrv_handle, unsigned int pru_evtout_num);

int prussdrv_pru_reset(void *prussdrv_handle, unsigned int prunum);

int prussdrv_pru_disable(void *prussdrv_handle, unsigned int prunum);

int prussdrv_pru_enable(void *prussdrv_handle, unsigned int prunum);

int prussdrv_pru_write_memory(void *prussdrv_handle,
                              unsigned int pru_ram_id,
                              unsigned int wordoffset,
                              unsigned int *memarea,
                              unsigned int bytelength);

int prussdrv_pruintc_init(void *prussdrv_handle,
tpruss_intc_initdata * prussintc_init_data);

int prussdrv_map_l3mem(void *prussdrv_handle, void **address);

int prussdrv_map_extmem(void *prussdrv_handle, void **address);

int prussdrv_map_prumem(void *prussdrv_handle, unsigned int pru_ram_id,
                        void **address);

int prussdrv_map_peripheral_io(void *prussdrv_handle,
                               unsigned int per_id, void **address);

unsigned int prussdrv_get_phys_addr(void *prussdrv_handle, void *address);

void *prussdrv_get_virt_addr(void *prussdrv_handle, unsigned int phyaddr);

int prussdrv_pru_wait_event(void *prussdrv_handle, unsigned int pru_evtout_num);

int prussdrv_pru_send_event(void *prussdrv_handle, unsigned int eventnum);

int prussdrv_pru_clear_event(void *prussdrv_handle, unsigned int eventnum);

int prussdrv_pru_send_wait_clear_event(void *prussdrv_handle,
                                       unsigned int send_eventnum,
                                       unsigned int pru_evtout_num,
                                       unsigned int ack_eventnum);

int prussdrv_exit(void *prussdrv_handle);

int prussdrv_exec_program(void *prussdrv_handle, int prunum, char *filename);

int prussdrv_start_irqthread(void *prussdrv_handle,
                             unsigned int pru_evtout_num, int priority,
                             prussdrv_function_handler irqhandler);

#if defined (__cplusplus)
}
#endif
#endif
