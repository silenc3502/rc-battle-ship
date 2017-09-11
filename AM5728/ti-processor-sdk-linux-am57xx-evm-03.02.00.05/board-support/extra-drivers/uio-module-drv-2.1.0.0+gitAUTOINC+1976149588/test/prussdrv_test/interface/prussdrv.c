/*
 * prussdrv.c
 *
 * User space driver for PRUSS
 *
 * Copyright (C) 2012-2016 Texas Instruments Incorporated - http://www.ti.com/ 
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

//#define DEBUG

#include <prussdrv.h>
#include "__prussdrv.h"
#include <pthread.h>
#include <dirent.h>

#define PRUSS_UIO_PRAM_PATH_LEN 128
#define PRUSS_UIO_PARAM_VAL_LEN 20
#define HEXA_DECIMAL_BASE 16
#define MAX_NAME_LENGTH	64

int uioutil_get_string (char *filename, char *str, int str_len)
{
    FILE *fpr = 0;
    int ret_val = -1;

    if (!filename || !str || !str_len) {
        goto close_n_exit;
    }

    fpr = fopen(filename, "r");
    if (!fpr) {
        goto close_n_exit;
    }
    if (!fgets(str, str_len, fpr)) {
        goto close_n_exit;
    }

    /* Terminate string at new line or carriage return if any */
    str[strcspn(str, "\n\r")] = '\0';

    ret_val = 0;

close_n_exit:
    if (fpr) fclose(fpr);
    return ret_val;
}

int uioutil_get_device (char *uio_name, char *class_name, int class_name_length)
{
    struct dirent *entry = 0;
    char filename[PRUSS_UIO_PRAM_PATH_LEN];
    char name[MAX_NAME_LENGTH];
    int ret_val = -1;
    DIR *dir = 0;

    if (!uio_name || !class_name || !strlen(uio_name)) {
        goto close_n_exit;
    }

    dir = opendir("/sys/class/uio");
    if (!dir) {
        perror("readdir /sys/class/uio");
        goto close_n_exit;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strstr (entry->d_name, "uio") == NULL) {
            continue;
        }

        snprintf(filename, PRUSS_UIO_PRAM_PATH_LEN, "/sys/class/uio/%s/name", entry->d_name);

        if (uioutil_get_string(filename, name, MAX_NAME_LENGTH) < 0)
            goto close_n_exit;

        if (!strncmp (uio_name, name, strlen(uio_name))) {
            ret_val = 0;
            strncpy (class_name, entry->d_name, class_name_length);
            goto close_n_exit;
        }
    }

close_n_exit:
    if (dir) closedir(dir);

    return ret_val;
}

int prussdrv_get_mem_section_by_id(char *class_name, int section_id, int mmap_fd,
    unsigned int *phys_base_p, unsigned int *size_p, void **va_base_p)
{
    int fd;
    char hexstring[PRUSS_UIO_PARAM_VAL_LEN];
    char filename[PRUSS_UIO_PRAM_PATH_LEN];
    int ret=0;
    uint32_t page_size = getpagesize();

    sprintf(filename, "/sys/class/misc/%s/maps/map%d/addr", class_name, section_id);

    fd = open(filename, O_RDONLY);
    if (fd >= 0) {
     ret = read(fd, hexstring, PRUSS_UIO_PARAM_VAL_LEN);
     if ( ret == 0) {
         /* Read failed */
         ret = -1;
         goto err_exit;
     }
     *phys_base_p =
         strtoul(hexstring, NULL, HEXA_DECIMAL_BASE);
    } else
        return -1;
    close(fd);

    sprintf(filename, "/sys/class/misc/%s/maps/map%d/size", class_name, section_id);
    fd = open(filename, O_RDONLY);
    if (fd >= 0) {
        ret = read(fd, hexstring, PRUSS_UIO_PARAM_VAL_LEN);
        if ( ret == 0) {
            /* Read failed */
            ret = -1;
            goto err_exit;
        }

        *size_p =
            strtoul(hexstring, NULL, HEXA_DECIMAL_BASE);
    } else
        return -1;
    *va_base_p =  
        mmap(0, *size_p, PROT_READ | PROT_WRITE,
             MAP_SHARED, mmap_fd, section_id*page_size );
    if(*va_base_p == MAP_FAILED) {
        ret =-1;
        goto err_exit;
    }
    /* If the page start is not aligned, it needs correction here */
    *va_base_p += (*phys_base_p & (page_size-1));
err_exit:
    close(fd);
    return ret;
}

int prussdrv_get_mem_section_by_name(char *class_name, char *map_name,  int mmap_fd,
    unsigned int *phys_base_p, unsigned int *size_p, void **va_base_p)
{
    int fd;
    char hexstring[PRUSS_UIO_PARAM_VAL_LEN];
    char filename[PRUSS_UIO_PRAM_PATH_LEN];
    int ret=0;
    int index;

    index = 0;
    do {
        sprintf(filename, "/sys/class/misc/%s/maps/map%d/name", class_name, index);

        fd = open(filename, O_RDONLY);
        if (fd >= 0) {
            ret = read(fd, hexstring, PRUSS_UIO_PARAM_VAL_LEN);
            if ( ret == 0) {
                ret = -1;
                /* Read failed */
                goto err_exit;
            }
            hexstring[ret-1] = '\0';
        } else {
            return -1;
        }

        close(fd);
        index++;
    } while(strcmp(map_name, hexstring));
    ret =  prussdrv_get_mem_section_by_id(class_name, index-1, mmap_fd, phys_base_p, size_p, va_base_p);
    if (ret < 0) {
        ret = -1;
        goto err_exit1;
    }
#ifdef DEBUG
    printf("\nDEBUG:Got Memory section %s:%s physbase 0x%x, virtp %p, size 0x%x\n",
        class_name, map_name, *phys_base_p, *va_base_p,
        *size_p);
    printf("DEBUG:Memory section %s: First location 0x%x\n",
	map_name, (((uint32_t *)(*va_base_p))[0]));
#endif
    return 0;
err_exit:
    close(fd);
err_exit1:
    return ret;
}

int __prussdrv_memmap_init(void *prussdrv_handle, char *pruss_device_name)
{
    char name[PRUSS_UIO_PRAM_PATH_LEN];
    char class_name[PRUSS_UIO_PRAM_PATH_LEN];
    char device_name[PRUSS_UIO_PRAM_PATH_LEN];
    int ret_val;
    tprussdrv *prussdrv_p = (tprussdrv *)prussdrv_handle;

    if (prussdrv_p->mmap_fd == 0) {
        sprintf(name, "uio_%s_mem", pruss_device_name);
        sprintf(device_name, "/dev/%s", name);
        prussdrv_p->mmap_fd = open(device_name, O_RDWR | O_SYNC);
	if(prussdrv_p->mmap_fd < 0) {
            return(PRUSSDRV_ERROR_OPEN_PRUSS_MEM);
	}
    }
    if (prussdrv_p->mmap_fd2 == 0) {
        sprintf(name, "uio_%s_mem2", pruss_device_name);
        sprintf(device_name, "/dev/%s", name);
        prussdrv_p->mmap_fd2 = open(device_name, O_RDWR | O_SYNC);
	if(prussdrv_p->mmap_fd2 < 0) {
            return(PRUSSDRV_ERROR_OPEN_PRUSS_MEM2);
	}
    }
    if (prussdrv_p->pru0_mmap_fd == 0) {
        sprintf(name, "uio_%s_0_mem", pruss_device_name);
        sprintf(device_name, "/dev/%s", name);
        prussdrv_p->pru0_mmap_fd = open(device_name, O_RDWR | O_SYNC);
	if(prussdrv_p->pru0_mmap_fd < 0) {
            return(PRUSSDRV_ERROR_OPEN_PRU0_MEM);
	}
    }
    if (prussdrv_p->pru1_mmap_fd == 0) {
        sprintf(name, "uio_%s_1_mem", pruss_device_name);
        sprintf(device_name, "/dev/%s", name);
        prussdrv_p->pru1_mmap_fd = open(device_name, O_RDWR | O_SYNC);
	if(prussdrv_p->pru1_mmap_fd < 0) {
            return(PRUSSDRV_ERROR_OPEN_PRU1_MEM);
	}
    }

    sprintf(class_name, "uio_%s_mem", pruss_device_name);
    ret_val = prussdrv_get_mem_section_by_name(class_name, "dram0", prussdrv_p->mmap_fd,
        &prussdrv_p->pru0_dataram_phy_base, &prussdrv_p->pru0_dataram_size, 
	&prussdrv_p->pru0_dataram_base);
    if (ret_val <  0) {
	return(PRUSSDRV_ERROR_DRAM0_MEM_SECTION);
    }
    ret_val = prussdrv_get_mem_section_by_name(class_name, "intc", prussdrv_p->mmap_fd,
               &prussdrv_p->intc_phy_base, &prussdrv_p->intc_size, 
               &prussdrv_p->intc_base);
    if(ret_val <  0) {
        return(PRUSSDRV_ERROR_INTC_MEM_SECTION);
    }
    prussdrv_p->version =
        __pruss_detect_hw_version(prussdrv_p->intc_base);
#ifdef DEBUG
    printf("DEBUG:Got hw version %d\n", prussdrv_p->version);
#endif

    ret_val = prussdrv_get_mem_section_by_name(class_name, "dram1" , prussdrv_p->mmap_fd,
        &prussdrv_p->pru1_dataram_phy_base, &prussdrv_p->pru1_dataram_size, 
	&prussdrv_p->pru1_dataram_base);
    if (ret_val <  0) {
        return(PRUSSDRV_ERROR_DRAM1_MEM_SECTION);
    }

    switch (prussdrv_p->version) {
    case PRUSS_V1:
        break;

    case PRUSS_V2:
#ifdef DEBUG
            printf("PRUSS V2 \n");
#endif
	    ret_val = prussdrv_get_mem_section_by_name(class_name, "shrdram2" , prussdrv_p->mmap_fd,
                &prussdrv_p->pruss_sharedram_phy_base, &prussdrv_p->pruss_sharedram_size, 
	        &prussdrv_p->pruss_sharedram_base);
            if (ret_val <  0) {
                return(PRUSSDRV_ERROR_SHRDRAM2_MEM_SECTION);
            }

	    ret_val = prussdrv_get_mem_section_by_name(class_name, "cfg", prussdrv_p->mmap_fd,
                &prussdrv_p->pruss_cfg_phy_base, &prussdrv_p->pruss_cfg_size, 
	        &prussdrv_p->pruss_cfg_base);
            if (ret_val <  0) {
                return(PRUSSDRV_ERROR_CFG_MEM_SECTION);
            }

	    ret_val = prussdrv_get_mem_section_by_name(class_name, "iep", prussdrv_p->mmap_fd,
                &prussdrv_p->pruss_iep_phy_base, &prussdrv_p->pruss_iep_size, 
	        &prussdrv_p->pruss_iep_base);
            if (ret_val <  0) {
                return(PRUSSDRV_ERROR_IEP_MEM_SECTION);
            }

	    ret_val = prussdrv_get_mem_section_by_name(class_name, "mii_rt", prussdrv_p->mmap_fd,
                &prussdrv_p->pruss_miirt_phy_base, &prussdrv_p->pruss_miirt_size, 
	        &prussdrv_p->pruss_miirt_base);
            if (ret_val <  0) {
                return(PRUSSDRV_ERROR_MIIRT_MEM_SECTION);
            }

            sprintf(class_name, "uio_%s_mem2", pruss_device_name);

	    ret_val = prussdrv_get_mem_section_by_name(class_name, "uart", prussdrv_p->mmap_fd2,
                &prussdrv_p->pruss_uart_phy_base, &prussdrv_p->pruss_uart_size, 
	        &prussdrv_p->pruss_uart_base);
            if (ret_val <  0) {
                return(PRUSSDRV_ERROR_UART_MEM_SECTION);
            }
	    ret_val = prussdrv_get_mem_section_by_name(class_name, "ecap", prussdrv_p->mmap_fd2,
                &prussdrv_p->pruss_ecap_phy_base, &prussdrv_p->pruss_ecap_size, 
	        &prussdrv_p->pruss_ecap_base);
            if (ret_val <  0) {
                return(PRUSSDRV_ERROR_ECAP_MEM_SECTION);
            }

	    ret_val = prussdrv_get_mem_section_by_name(class_name, "mdio", prussdrv_p->mmap_fd2,
                &prussdrv_p->pruss_mdio_phy_base, &prussdrv_p->pruss_ecap_size, 
	        &prussdrv_p->pruss_mdio_base);
            if (ret_val <  0) {
                return(PRUSSDRV_ERROR_MDIO_MEM_SECTION);
            }
        break;
    default:
#ifdef DEBUG
        printf("UNKNOWN pruss version\n");
#endif
	return(PRUSSDRV_ERROR_UNSUPPORTED_VERSION);
    }

    sprintf(class_name, "uio_%s_0_mem", pruss_device_name);
    ret_val = prussdrv_get_mem_section_by_name(class_name, "iram", prussdrv_p->pru0_mmap_fd,
        &prussdrv_p->pru0_iram_phy_base, &prussdrv_p->pru0_iram_size, 
        &prussdrv_p->pru0_iram_base);
   if (ret_val <  0) {
       return(PRUSSDRV_ERROR_IRAM_MEM_SECTION);
   }
    ret_val = prussdrv_get_mem_section_by_name(class_name, "control", prussdrv_p->pru0_mmap_fd,
        &prussdrv_p->pru0_control_phy_base, &prussdrv_p->pru0_control_size, 
        &prussdrv_p->pru0_control_base);
   if (ret_val <  0) {
       return(PRUSSDRV_ERROR_CONTROL_MEM_SECTION);
   }
    ret_val = prussdrv_get_mem_section_by_name(class_name, "debug", prussdrv_p->pru0_mmap_fd,
        &prussdrv_p->pru0_debug_phy_base, &prussdrv_p->pru0_debug_size, 
        &prussdrv_p->pru0_debug_base);
   if (ret_val <  0) {
       return(PRUSSDRV_ERROR_DEBUG_MEM_SECTION);
   }
    sprintf(class_name, "uio_%s_1_mem", pruss_device_name);
    ret_val = prussdrv_get_mem_section_by_name(class_name, "iram", prussdrv_p->pru1_mmap_fd,
        &prussdrv_p->pru1_iram_phy_base, &prussdrv_p->pru1_iram_size, 
        &prussdrv_p->pru1_iram_base);
   if (ret_val <  0) {
       return(PRUSSDRV_ERROR_IRAM_MEM_SECTION);
   }
    ret_val = prussdrv_get_mem_section_by_name(class_name, "control", prussdrv_p->pru1_mmap_fd,
        &prussdrv_p->pru1_control_phy_base, &prussdrv_p->pru1_control_size, 
        &prussdrv_p->pru1_control_base);
   if (ret_val <  0) {
       return(PRUSSDRV_ERROR_CONTROL_MEM_SECTION);
   }
    ret_val = prussdrv_get_mem_section_by_name(class_name, "debug", prussdrv_p->pru1_mmap_fd,
        &prussdrv_p->pru1_debug_phy_base, &prussdrv_p->pru1_debug_size, 
        &prussdrv_p->pru1_debug_base);
   if (ret_val <  0) {
       return(PRUSSDRV_ERROR_DEBUG_MEM_SECTION);
   }

    return 0;

}

int prussdrv_init(void **handle, char *pruss_name)
{
    tprussdrv *prussdrv_p;
    int ret_val;

    if (strlen(pruss_name) >= MAX_DEVICE_NAME_LENGTH)
	return PRUSSDRV_ERROR_INVALID_NAME;

    prussdrv_p = malloc(sizeof(tprussdrv));
    memset(prussdrv_p, 0, sizeof(tprussdrv));

    ret_val = __prussdrv_memmap_init((void *)prussdrv_p, pruss_name);
    if (!ret_val) {
        *handle = (void *)prussdrv_p;
        strcpy(prussdrv_p->device_name, pruss_name);
    }
    return ret_val;

}

int prussdrv_open(void *prussdrv_handle, unsigned int pru_evtout_num)
{

    char name[PRUSS_UIO_PRAM_PATH_LEN];
    char class_name[PRUSS_UIO_PRAM_PATH_LEN];
    char device_name[PRUSS_UIO_PRAM_PATH_LEN];
    uint32_t value;
    int ret;
    tprussdrv *prussdrv_p = (tprussdrv *)prussdrv_handle;

    if (!prussdrv_p->fd[pru_evtout_num]) {
        sprintf(name, "uio_%s_evt%d", prussdrv_p->device_name, pru_evtout_num);
        if (uioutil_get_device(name, class_name, PRUSS_UIO_PRAM_PATH_LEN) < 0) {
#ifdef DEBUG
           printf("\nError getting device %s\n",name);fflush(stdout);
#endif
           return PRUSSDRV_ERROR_OPEN_GET_UIO_DEVICE;
        }
        sprintf(device_name, "/dev/%s", class_name);
        prussdrv_p->fd[pru_evtout_num] = open(device_name, O_RDWR | O_SYNC);

        /* Disable interrupt */
        value = 0;
        ret = write(prussdrv_p->fd[pru_evtout_num], &value, sizeof(uint32_t));
        if (ret != sizeof(uint32_t)) {
#ifdef DEBUG
            printf("\nunable to disable interrupt \n");
#endif
            return PRUSSDRV_ERROR_OPEN_DISABLE_INTRPT;
    }

        /* Enable interrupt */
        value = 1;
        ret = write(prussdrv_p->fd[pru_evtout_num], &value, sizeof(uint32_t));
        if (ret != sizeof(uint32_t)) {
#ifdef DEBUG
        printf("\nunable to enable interrupt \n");
#endif
        return PRUSSDRV_ERROR_OPEN_ENABLE_INTRPT;
    }

        return 0;
    } else {
        return PRUSSDRV_ERROR_OPEN_UIOFD_NONZERO;
    }
}




int prussdrv_pru_reset(void *prussdrv_handle, unsigned int prunum)
{
    unsigned int *prucontrolregs;
    tprussdrv *prussdrv_p = (tprussdrv *)prussdrv_handle;

    if (prunum == 0)
        prucontrolregs = (unsigned int *) prussdrv_p->pru0_control_base;
    else if (prunum == 1)
        prucontrolregs = (unsigned int *) prussdrv_p->pru1_control_base;
    else
        return -1;
    *prucontrolregs = 0;
    return 0;
}

int prussdrv_pru_enable(void *prussdrv_handle, unsigned int prunum)
{
    unsigned int *prucontrolregs;
    tprussdrv *prussdrv_p = (tprussdrv *)prussdrv_handle;

    if (prunum == 0)
        prucontrolregs = (unsigned int *) prussdrv_p->pru0_control_base;
    else if (prunum == 1)
        prucontrolregs = (unsigned int *) prussdrv_p->pru1_control_base;
    else
        return -1;

    *prucontrolregs = 2;
    return 0;

}

int prussdrv_pru_disable(void *prussdrv_handle, unsigned int prunum)
{
    unsigned int *prucontrolregs;
    tprussdrv *prussdrv_p = (tprussdrv *)prussdrv_handle;

    if (prunum == 0)
        prucontrolregs = (unsigned int *) prussdrv_p->pru0_control_base;
    else if (prunum == 1)
        prucontrolregs = (unsigned int *) prussdrv_p->pru1_control_base;
    else
        return -1;
    *prucontrolregs = 1;
    return 0;

}

int prussdrv_pru_write_memory(void *prussdrv_handle,
                              unsigned int pru_ram_id,
                              unsigned int wordoffset,
                              unsigned int *memarea,
                              unsigned int bytelength)
{
    unsigned int *pruramarea, i, wordlength;
    tprussdrv *prussdrv_p = (tprussdrv *)prussdrv_handle;

    switch (pru_ram_id) {
    case PRUSS_PRU0_IRAM:
        pruramarea = (unsigned int *) prussdrv_p->pru0_iram_base;
        break;
    case PRUSS_PRU1_IRAM:
        pruramarea = (unsigned int *) prussdrv_p->pru1_iram_base;
        break;
    case PRUSS_PRU0_DATARAM:
        pruramarea = (unsigned int *) prussdrv_p->pru0_dataram_base;
        break;
    case PRUSS_PRU1_DATARAM:
        pruramarea = (unsigned int *) prussdrv_p->pru1_dataram_base;
        break;
    case PRUSS_SHARED_DATARAM:
        if (prussdrv_p->version != PRUSS_V2)
            return -1;
        pruramarea = (unsigned int *) prussdrv_p->pruss_sharedram_base;
        break;
    default:
        return -1;
    }

    wordlength = (bytelength + 3) >> 2; //Adjust length as multiple of 4 bytes
    for (i = 0; i < wordlength; i++) {
        *(pruramarea + i + wordoffset) = *(memarea + i);
    }
    return wordlength;

}


int prussdrv_pruintc_init(void *prussdrv_handle, tpruss_intc_initdata * prussintc_init_data)
{
    tprussdrv *prussdrv_p = (tprussdrv *)prussdrv_handle;
    volatile unsigned int *pruintc_io = (unsigned int *) prussdrv_p->intc_base;
    unsigned int i, mask1, mask2;

    pruintc_io[PRU_INTC_SIPR1_REG >> 2] = 0xFFFFFFFF;
    pruintc_io[PRU_INTC_SIPR2_REG >> 2] = 0xFFFFFFFF;

    for (i = 0; i < (NUM_PRU_SYS_EVTS + 3) >> 2; i++)
        pruintc_io[(PRU_INTC_CMR1_REG >> 2) + i] = 0;
    for (i = 0;
         ((prussintc_init_data->sysevt_to_channel_map[i].sysevt != -1)
          && (prussintc_init_data->sysevt_to_channel_map[i].channel !=
              -1)); i++) {
        __prussintc_set_cmr(pruintc_io,
                            prussintc_init_data->sysevt_to_channel_map[i].
                            sysevt,
                            prussintc_init_data->sysevt_to_channel_map[i].
                            channel);
    }
    for (i = 0; i < (NUM_PRU_HOSTS + 3) >> 2; i++)
        pruintc_io[(PRU_INTC_HMR1_REG >> 2) + i] = 0;
    for (i = 0;
         ((prussintc_init_data->channel_to_host_map[i].channel != -1)
          && (prussintc_init_data->channel_to_host_map[i].host != -1));
         i++) {

        __prussintc_set_hmr(pruintc_io,
                            prussintc_init_data->channel_to_host_map[i].
                            channel,
                            prussintc_init_data->channel_to_host_map[i].
                            host);
    }

    pruintc_io[PRU_INTC_SITR1_REG >> 2] = 0x0;
    pruintc_io[PRU_INTC_SITR2_REG >> 2] = 0x0;


    mask1 = mask2 = 0;
    for (i = 0; prussintc_init_data->sysevts_enabled[i] != 255; i++) {
        if (prussintc_init_data->sysevts_enabled[i] < 32) {
            mask1 =
                mask1 + (1 << (prussintc_init_data->sysevts_enabled[i]));
        } else if (prussintc_init_data->sysevts_enabled[i] < 64) {
            mask2 =
                mask2 +
                (1 << (prussintc_init_data->sysevts_enabled[i] - 32));
        } else {
#ifdef DEBUG
            printf("Error: SYS_EVT%d out of range\n",
                   prussintc_init_data->sysevts_enabled[i]);
#endif
            return -1;
        }
    }
    pruintc_io[PRU_INTC_ESR1_REG >> 2] = mask1;
    pruintc_io[PRU_INTC_SECR1_REG >> 2] = mask1;
    pruintc_io[PRU_INTC_ESR2_REG >> 2] = mask2;
    pruintc_io[PRU_INTC_SECR2_REG >> 2] = mask2;

    for (i = 0; i < MAX_HOSTS_SUPPORTED; i++)
        if (prussintc_init_data->host_enable_bitmask & (1 << i)) {
            pruintc_io[PRU_INTC_HIEISR_REG >> 2] = i;
        }

    pruintc_io[PRU_INTC_GER_REG >> 2] = 0x1;

    return 0;
}


int prussdrv_pru_send_event(void *prussdrv_handle, unsigned int eventnum)
{
    tprussdrv *prussdrv_p = (tprussdrv *)prussdrv_handle;
    unsigned int *pruintc_io = (unsigned int *) prussdrv_p->intc_base;

    if (eventnum < 32)
        pruintc_io[PRU_INTC_SRSR1_REG >> 2] = 1 << eventnum;
    else
        pruintc_io[PRU_INTC_SRSR2_REG >> 2] = 1 << (eventnum - 32);
    return 0;
}

int prussdrv_pru_wait_event(void *prussdrv_handle, unsigned int pru_evtout_num)
{
    int event_count;
    tprussdrv *prussdrv_p = (tprussdrv *)prussdrv_handle;
    unsigned int *pruintc_io = (unsigned int *) prussdrv_p->intc_base;
    int ret;

#if 0
    /* This is needed only if the interrupt-mode is configured for one-shot mode */
    uint32_t value;
 
    value = 1;
    ret = write(prussdrv_p->fd[pru_evtout_num], &value, sizeof(uint32_t));
    if (ret != sizeof(uint32_t)) {
#ifdef DEBUG
        printf("\nunable to enable interrupt \n");
#endif
        return -1;
    }
#endif
    ret = read(prussdrv_p->fd[pru_evtout_num], &event_count, sizeof(int));
    if ( ret == 0) {
        /* Read failed */
        return -1;
    }
    pruintc_io[PRU_INTC_HIEISR_REG >> 2] = pru_evtout_num+2; 
    return 0;

}

int prussdrv_pru_clear_event(void *prussdrv_handle, unsigned int eventnum)
{
    tprussdrv *prussdrv_p = (tprussdrv *)prussdrv_handle;
    volatile unsigned int *pruintc_io = (volatile unsigned int *) prussdrv_p->intc_base;
#ifdef DEBUG
/*  if (eventnum < 32)
        printf("\n event seen 0x%x\n", pruintc_io[PRU_INTC_SRSR1_REG >> 2]);
    else
        printf("\n event seen 0x%x\n", pruintc_io[PRU_INTC_SRSR2_REG >> 2]);*/
#endif
    if (eventnum < 32)
        pruintc_io[PRU_INTC_SECR1_REG >> 2] = 1 << eventnum;
    else
        pruintc_io[PRU_INTC_SECR2_REG >> 2] = 1 << (eventnum - 32);
    return 0;
}

int prussdrv_pru_send_wait_clear_event(void *prussdrv_handle,
                                       unsigned int send_eventnum,
                                       unsigned int pru_evtout_num,
                                       unsigned int ack_eventnum)
{
    prussdrv_pru_send_event(prussdrv_handle, send_eventnum);
    prussdrv_pru_wait_event(prussdrv_handle, pru_evtout_num);
    prussdrv_pru_clear_event(prussdrv_handle, ack_eventnum);
    return 0;

}

int prussdrv_map_prumem(void *prussdrv_handle, unsigned int pru_ram_id,
    void **address)
{
    tprussdrv *prussdrv_p = (tprussdrv *)prussdrv_handle;
 
    switch (pru_ram_id) {
    case PRUSS_PRU0_DATARAM:
        *address = prussdrv_p->pru0_dataram_base;
        break;
    case PRUSS_PRU1_DATARAM:
        *address = prussdrv_p->pru1_dataram_base;
        break;
    case PRUSS_SHARED_DATARAM:
        if (prussdrv_p->version != PRUSS_V2)
            return -1;
        *address = prussdrv_p->pruss_sharedram_base;
        break;
    default:
        *address = 0;
        return -1;
    }
    return 0;
}

int prussdrv_map_peripheral_io(void *prussdrv_handle, unsigned int per_id,
    void **address)
{
    tprussdrv *prussdrv_p = (tprussdrv *)prussdrv_handle;
 
    if (prussdrv_p->version != PRUSS_V2)
        return -1;

    switch (per_id) {
    case PRUSS_CFG:
        *address = prussdrv_p->pruss_cfg_base;
        break;

    case PRUSS_UART:
        *address = prussdrv_p->pruss_uart_base;
        break;

    case PRUSS_IEP:
        *address = prussdrv_p->pruss_iep_base;
        break;

    case PRUSS_ECAP:
        *address = prussdrv_p->pruss_ecap_base;
        break;

    case PRUSS_MII_RT:
        *address = prussdrv_p->pruss_miirt_base;
        break;

    case PRUSS_MDIO:
        *address = prussdrv_p->pruss_mdio_base;
        break;

    default:
        *address = 0;
        return -1;
    }
    return 0;
}

unsigned int prussdrv_get_phys_addr(void *prussdrv_handle, void *address)
{
    unsigned int retaddr = 0;
    tprussdrv *prussdrv_p = (tprussdrv *)prussdrv_handle;
 
    if ((address >= prussdrv_p->pru0_dataram_base)
        && (address <
            prussdrv_p->pru0_dataram_base + prussdrv_p->pru0_dataram_size)) {
        retaddr =
            ((unsigned int) (address - prussdrv_p->pru0_dataram_base) +
             prussdrv_p->pru0_dataram_phy_base);
    } else if ((address >= prussdrv_p->pru1_dataram_base)
        && (address <
            prussdrv_p->pru1_dataram_base + prussdrv_p->pru1_dataram_size)) {
        retaddr =
            ((unsigned int) (address - prussdrv_p->pru1_dataram_base) +
             prussdrv_p->pru1_dataram_phy_base);
    } else if ((address >= prussdrv_p->intc_base)
        && (address <
            prussdrv_p->intc_base + prussdrv_p->intc_size)) {
        retaddr =
            ((unsigned int) (address - prussdrv_p->intc_base) +
             prussdrv_p->intc_phy_base);
    } else if ((address >= prussdrv_p->pruss_sharedram_base)
        && (address <
            prussdrv_p->pruss_sharedram_base + prussdrv_p->pruss_sharedram_size)) {
        retaddr =
            ((unsigned int) (address - prussdrv_p->pruss_sharedram_base) +
             prussdrv_p->pruss_sharedram_phy_base);
    } else if ((address >= prussdrv_p->pruss_cfg_base)
        && (address <
            prussdrv_p->pruss_cfg_base + prussdrv_p->pruss_cfg_size)) {
        retaddr =
            ((unsigned int) (address - prussdrv_p->pruss_cfg_base) +
             prussdrv_p->pruss_cfg_phy_base);
    } else if ((address >= prussdrv_p->pruss_iep_base)
        && (address <
            prussdrv_p->pruss_iep_base + prussdrv_p->pruss_iep_size)) {
        retaddr =
            ((unsigned int) (address - prussdrv_p->pruss_iep_base) +
             prussdrv_p->pruss_iep_phy_base);
    } else if ((address >= prussdrv_p->pruss_miirt_base)
        && (address <
            prussdrv_p->pruss_miirt_base + prussdrv_p->pruss_miirt_size)) {
        retaddr =
            ((unsigned int) (address - prussdrv_p->pruss_miirt_base) +
             prussdrv_p->pruss_miirt_phy_base);
    } else if ((address >= prussdrv_p->pruss_ecap_base)
        && (address <
            prussdrv_p->pruss_ecap_base + prussdrv_p->pruss_ecap_size)) {
        retaddr =
            ((unsigned int) (address - prussdrv_p->pruss_ecap_base) +
             prussdrv_p->pruss_ecap_phy_base);
    } else if ((address >= prussdrv_p->pruss_uart_base)
        && (address <
            prussdrv_p->pruss_uart_base + prussdrv_p->pruss_uart_size)) {
        retaddr =
            ((unsigned int) (address - prussdrv_p->pruss_uart_base) +
             prussdrv_p->pruss_uart_phy_base);
    } else if ((address >= prussdrv_p->pruss_mdio_base)
        && (address <
            prussdrv_p->pruss_mdio_base + prussdrv_p->pruss_mdio_size)) {
        retaddr =
            ((unsigned int) (address - prussdrv_p->pruss_mdio_base) +
             prussdrv_p->pruss_mdio_phy_base);
    }

    return retaddr;

}

void *prussdrv_get_virt_addr(void *prussdrv_handle, unsigned int phyaddr)
{
    tprussdrv *prussdrv_p = (tprussdrv *)prussdrv_handle;
 
    void *address = 0;
    if ((phyaddr >= prussdrv_p->pru0_dataram_phy_base)
        && (phyaddr <
            prussdrv_p->pru0_dataram_phy_base + prussdrv_p->pru0_dataram_size)) {
        address =
            (void *) ((char *) prussdrv_p->pru0_dataram_base +
                      (phyaddr - prussdrv_p->pru0_dataram_phy_base));
    } else if ((phyaddr >= prussdrv_p->pru1_dataram_phy_base)
        && (phyaddr <
            prussdrv_p->pru1_dataram_phy_base + prussdrv_p->pru1_dataram_size)) {
        address =
            (void *) ((char *) prussdrv_p->pru1_dataram_base +
                      (phyaddr - prussdrv_p->pru1_dataram_phy_base));
    } else if ((phyaddr >= prussdrv_p->intc_phy_base)
        && (phyaddr <
            prussdrv_p->intc_phy_base + prussdrv_p->intc_size)) {
        address =
            (void *) ((char *) prussdrv_p->intc_base +
                      (phyaddr - prussdrv_p->intc_phy_base));
    } else if ((phyaddr >= prussdrv_p->pruss_sharedram_phy_base)
        && (phyaddr <
            prussdrv_p->pruss_sharedram_phy_base + prussdrv_p->pruss_sharedram_size)) {
        address =
            (void *) ((char *) prussdrv_p->pruss_sharedram_base +
                      (phyaddr - prussdrv_p->pruss_sharedram_phy_base));
    } else if ((phyaddr >= prussdrv_p->pruss_cfg_phy_base)
        && (phyaddr <
            prussdrv_p->pruss_cfg_phy_base + prussdrv_p->pruss_cfg_size)) {
        address =
            (void *) ((char *) prussdrv_p->pruss_cfg_base +
                      (phyaddr - prussdrv_p->pruss_cfg_phy_base));
    } else if ((phyaddr >= prussdrv_p->pruss_iep_phy_base)
        && (phyaddr <
            prussdrv_p->pruss_iep_phy_base + prussdrv_p->pruss_iep_size)) {
        address =
            (void *) ((char *) prussdrv_p->pruss_iep_base +
                      (phyaddr - prussdrv_p->pruss_iep_phy_base));
    } else if ((phyaddr >= prussdrv_p->pruss_miirt_phy_base)
        && (phyaddr <
            prussdrv_p->pruss_miirt_phy_base + prussdrv_p->pruss_miirt_size)) {
        address =
            (void *) ((char *) prussdrv_p->pruss_miirt_base +
                      (phyaddr - prussdrv_p->pruss_miirt_phy_base));
    } else if ((phyaddr >= prussdrv_p->pruss_ecap_phy_base)
        && (phyaddr <
            prussdrv_p->pruss_ecap_phy_base + prussdrv_p->pruss_ecap_size)) {
        address =
            (void *) ((char *) prussdrv_p->pruss_ecap_base +
                      (phyaddr - prussdrv_p->pruss_ecap_phy_base));
    } else if ((phyaddr >= prussdrv_p->pruss_uart_phy_base)
        && (phyaddr <
            prussdrv_p->pruss_uart_phy_base + prussdrv_p->pruss_uart_size)) {
        address =
            (void *) ((char *) prussdrv_p->pruss_uart_base +
                      (phyaddr - prussdrv_p->pruss_uart_phy_base));
    } else if ((phyaddr >= prussdrv_p->pruss_mdio_phy_base)
        && (phyaddr <
            prussdrv_p->pruss_mdio_phy_base + prussdrv_p->pruss_mdio_size)) {
        address =
            (void *) ((char *) prussdrv_p->pruss_mdio_base +
                      (phyaddr - prussdrv_p->pruss_mdio_phy_base));
    }
    return address;

}

int prussdrv_exit(void *prussdrv_handle)
{
    int i;
    tprussdrv *prussdrv_p = (tprussdrv *)prussdrv_handle;
 
    munmap(prussdrv_p->pru0_dataram_base, prussdrv_p->pru0_dataram_size);
    munmap(prussdrv_p->pru1_dataram_base, prussdrv_p->pru1_dataram_size);
    munmap(prussdrv_p->intc_base, prussdrv_p->intc_size);
    munmap(prussdrv_p->pruss_sharedram_base, prussdrv_p->pruss_sharedram_size);
    munmap(prussdrv_p->pruss_cfg_base, prussdrv_p->pruss_cfg_size);
    munmap(prussdrv_p->pruss_iep_base, prussdrv_p->pruss_iep_size);
    munmap(prussdrv_p->pruss_miirt_base, prussdrv_p->pruss_miirt_size);
    munmap(prussdrv_p->pruss_ecap_base, prussdrv_p->pruss_ecap_size);
    munmap(prussdrv_p->pruss_uart_base, prussdrv_p->pruss_uart_size);
    munmap(prussdrv_p->pruss_mdio_base, prussdrv_p->pruss_mdio_size);
    munmap(prussdrv_p->pru0_iram_base, prussdrv_p->pru0_iram_size);
    munmap(prussdrv_p->pru0_control_base, prussdrv_p->pru0_control_size);
    munmap(prussdrv_p->pru0_debug_base, prussdrv_p->pru0_debug_size);
    munmap(prussdrv_p->pru1_iram_base, prussdrv_p->pru1_iram_size);
    munmap(prussdrv_p->pru1_control_base, prussdrv_p->pru1_control_size);
    munmap(prussdrv_p->pru1_debug_base, prussdrv_p->pru1_debug_size);

    for (i = 0; i < NUM_PRU_HOSTIRQS; i++) {
        if (prussdrv_p->fd[i])
            close(prussdrv_p->fd[i]);
        if (prussdrv_p->irq_thread[i])
            pthread_join(prussdrv_p->irq_thread[i], NULL);
    }
    free(prussdrv_handle);
    return 0;
}

int prussdrv_exec_program(void *prussdrv_handle, int prunum, char *filename)
{
    FILE *fPtr;
    unsigned char fileDataArray[PRUSS_MAX_IRAM_SIZE];
    int fileSize = 0;
    unsigned int pru_ram_id;

    if (prunum == 0)
        pru_ram_id = PRUSS_PRU0_IRAM;
    else if (prunum == 1)
        pru_ram_id = PRUSS_PRU1_IRAM;
    else
        return -1;

    /* Open an File from the hard drive */
    fPtr = fopen(filename, "rb");
    if (fPtr == NULL) {
#ifdef DEBUG
        printf("File %s open failed\n", filename);
#endif
        return -1;
    }
    /* Read file size*/
    fseek(fPtr, 0, SEEK_END);
    fileSize = ftell(fPtr);

    if (fileSize == 0) {
#ifdef DEBUG
        printf("File read failed.. Closing program\n");
#endif
        fclose(fPtr);
        return -1;
    }

    fseek(fPtr, 0, SEEK_SET);

    if (fileSize !=
        fread((unsigned char *) fileDataArray, 1, fileSize, fPtr)) {
#ifdef DEBUG
        printf("WARNING: File Size mismatch\n");
#endif
    }

    fclose(fPtr);

    /* Make sure PRU sub system is first disabled/reset  */
    prussdrv_pru_disable(prussdrv_handle, prunum);
    prussdrv_pru_write_memory(prussdrv_handle, pru_ram_id, 0,
                              (unsigned int *) fileDataArray, fileSize);
    prussdrv_pru_enable(prussdrv_handle, prunum);

    return 0;
}

int prussdrv_start_irqthread(void *prussdrv_handle, unsigned int pru_evtout_num,
                             int priority,
                             prussdrv_function_handler irqhandler)
{
    pthread_attr_t pthread_attr;
    struct sched_param sched_param;
    tprussdrv *prussdrv_p = (tprussdrv *)prussdrv_handle;
    int ret_value;
#ifdef DEBUG
    printf("\n starting irq thread for event %d priority %d\n", pru_evtout_num, priority);
#endif
    pthread_attr_init(&pthread_attr);
    if (priority != 0) {
        ret_value = pthread_attr_setinheritsched(&pthread_attr,
                                     PTHREAD_EXPLICIT_SCHED);
        if(ret_value) {
            printf("\npthread set inheritsched failed %d\n", ret_value);
            return -1;
        }

        ret_value = pthread_attr_setschedpolicy(&pthread_attr, SCHED_FIFO);
        if(ret_value) {
            printf("\npthread set schedpolicy failed %d\n", ret_value);
            return -1;
        }

        ret_value = pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_JOINABLE);
        if(ret_value) {
            printf("\npthread set detachstate failed %d\n", ret_value);
            return -1;
        }

        sched_param.sched_priority = priority;
        ret_value = pthread_attr_setschedparam(&pthread_attr, &sched_param);
         if(ret_value) {
            printf("\npthread set schedparam failed %d\n", ret_value);
            return -1;
        }

    }

    ret_value = pthread_create(&prussdrv_p->irq_thread[pru_evtout_num], &pthread_attr,
                   irqhandler, NULL);
    if(ret_value) {
#ifdef DEBUG
        printf("\npthread create failed %d\n", ret_value);
#endif
        return -1;
    }
#ifdef DEBUG
    printf("\npthread create success \n");
#endif
    pthread_attr_destroy(&pthread_attr);

    return 0;

}
