/**
 * Interface for GDB to communicate with c66x DSP
 *
 * Copyright (C) 2014 Texas Instruments, Incorporated
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any kind,
 * whether express or implied; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <asm/io.h>
#include <asm/delay.h>

#define GDB_KM_MAJOR_VERSION 1
#define GDB_KM_MINOR_VERSION 1                        
#define GDB_KM_PATCH_VERSION 0                      
#define GDB_TTY_MAJOR        240                 
#define DRIVER_NAME      "gdbserverproxy_module_drv"
#define NAME             "gdbtty"

#define CTI_EMU0_TRIGGER     0x1
#define CTI_EMU1_TRIGGER     0x2

#define GDB_STR_SIZE         540

/**
 * The third word of the shared data block will be encoded as follows
 * {detach command (1 byte), count of block (2 bytes), ready flag (1 byte)}
 * where ready flag {MSN(ibble) signals DSP and LSN signals the ARM)}
 */
#define READY_MASK                 0x00000001
#define COUNT_MASK                 0x00FFFF00
#define STATUS_MASK                0x000000FE

#define READY_FLAG                 0x00000001
#define STATUS_NEW_PACKET          0x00000010
#define DETACH_COMM                0x10000000

#define COUNT_SHIFT                8
#define ACK                        0xFFFFFFFF

//Coresight lock/unlock macros
#define CS_LOCK_PATTERN      (0x1234ABCD)
#define CS_UNLOCK_PATTERN    (0xC5ACCE55)

#ifdef KEYSTONE_PLATFORM
#define JTAG_ID                    0x02620018
#endif

#ifdef DRA7xx_PLATFORM
#define ID_CODE                    0x4AE0C204
#define DIE_ID_2                   0x4AE0C20C
#endif

#define DEBUG                      0
#if DEBUG
     #define POLL_DELAY_MS         500
#else
     #define POLL_DELAY_MS         10
#endif


/**
 * Check the kernel version and don't allow build if it doesn't agree
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,10)
#error "Version of linux kernel must be greater than 3.10.10"
#endif

#ifdef KEYSTONE_PLATFORM

//DebugSS CTI registers
#define DEBUGSS_CTI_BASE_ADDR             (0x03003000)  
#define DEBUGSS_CTICNTL_REG_ADDR          (0x0)
#define DEBUGSS_CTIAPPSET_REG_ADDR        (0x14)
#define DEBUGSS_CTIAPPCLEAR_REG_ADDR      (0x18)
#define DEBUGSS_CTILAR_REG_ADDR           (0xFB0)

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

/**
 *   Structure used for identifying keystone II device
 */
struct dsp_type_t
{
     char name[9];
     ekeystone_device_type dev_enum;
     unsigned int num_dsps;
};

static struct dsp_type_t dsp_type_table[] = {
    {.name = "TCI6614", .dev_enum = eTCI6614_device, .num_dsps = 4},
    {.name = "TCI6638", .dev_enum = e66AK2Hxx_device, .num_dsps = 8},
    {.name = "TCI6630", .dev_enum = eTCI6630K2L_device, .num_dsps = 4},
    {.name = "66AK2Exx", .dev_enum = e66AK2Exx_device, .num_dsps = 1},
    {.name = "Unknown", .dev_enum = eNotSupported_device, .num_dsps = 8}  
};

static ekeystone_device_type identify_keystone_device(uint32_t jtagid)
{  
        jtagid = (jtagid & 0x0FFFF000) >> 12;

        if(jtagid == TCI6614_JTAGID_PARTNUMBER) {
                return(eTCI6614_device);
        } else if(jtagid == C66AK2Hxx_JTAGID_PARTNUMBER) {
                return(e66AK2Hxx_device);
        } else if(jtagid == C66AK2Exx_JTAGID_PARTNUMBER) {
                return(e66AK2Exx_device);
        } else if(jtagid == TCI6630K2L_JTAGID_PARTNUMBER) {
                return(eTCI6630K2L_device);
        }
    else{
                return(eNotSupported_device);
        }
}

#endif

#ifdef DRA7xx_PLATFORM

//DebugSS CTI registers
#define DEBUGSS_CTI_BASE_ADDR             (0x54169000)
#define DEBUGSS_CTICNTL_REG_ADDR          (0x0)
#define DEBUGSS_CTIAPPSET_REG_ADDR        (0x14)
#define DEBUGSS_CTIAPPCLEAR_REG_ADDR      (0x18)
#define DEBUGSS_CTILAR_REG_ADDR           (0xFB0)

//DRA7xx device base PN
#define DRA742_BASE_PN      (3)
#define DRA744_BASE_PN      (5)
#define DRA745_BASE_PN      (6)
#define DRA746_BASE_PN      (7)
#define DRA750_BASE_PN      (9)
#define DRA751_BASE_PN      (10)
#define DRA752_BASE_PN      (11)
#define DRA754_BASE_PN      (13)
#define DRA755_BASE_PN      (14)
#define DRA756_BASE_PN      (15)
#define AM5726_BASE_PN      (57)
#define AM5726E_BASE_PN     (58)
#define AM5728_BASE_PN      (59)
#define AM5728E_BASE_PN     (60)
#define AM5728_3E_BASE_PN   (62)
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

/**
 *   Structure used for identifying DRA7xx device
 */
struct dsp_type_t
{
     char name[9];
     edra7xx_device_type dev_enum;
     unsigned int num_dsps;
};

static struct dsp_type_t dsp_type_table[] = {
    {.name = "DRA74xx", .dev_enum = eDRA74xx_device, .num_dsps = 1},
    {.name = "DRA75xx", .dev_enum = eDRA75xx_device, .num_dsps = 2},
    {.name = "AM572x", .dev_enum = eAM572x_device, .num_dsps = 2},
    {.name = "AM571x", .dev_enum = eAM571x_device, .num_dsps = 1},
    {.name = "Unknown", .dev_enum = eNotSupported_device, .num_dsps = 8}  
};

static edra7xx_device_type identify_dra7xx_device(uint32_t die_id_2, uint32_t id_code)
{
    uint32_t base_pn;
    uint32_t ramp_system;

    base_pn = die_id_2 >> 24;
        ramp_system = (id_code & 0x0FFFF000) >> 12;

    if(AM571x_RAMP_SYSTEM == ramp_system)
    {
        return(eAM571x_device);
    }
    else
    {
        if((DRA742_BASE_PN == base_pn) || (DRA744_BASE_PN == base_pn) || (DRA745_BASE_PN == base_pn) || (DRA746_BASE_PN == base_pn)) {
                    return(eDRA74xx_device);
        } else if((DRA750_BASE_PN == base_pn) || (DRA751_BASE_PN == base_pn) || (DRA752_BASE_PN == base_pn) || (DRA754_BASE_PN == base_pn) 
                   || (DRA755_BASE_PN == base_pn) || (DRA756_BASE_PN == base_pn)) {
                return(eDRA75xx_device);
        } else if((AM5726_BASE_PN == base_pn) || (AM5726E_BASE_PN == base_pn) || (AM5728_BASE_PN == base_pn) || (AM5728E_BASE_PN == base_pn) || (AM5728_3E_BASE_PN == base_pn)) {
                return(eAM572x_device);
        }else if(OMAP572x_BASE_PN == base_pn) {
                return(eAM572x_device); //return OMAP572x as AM572x
        }
        else{
                return(eNotSupported_device);
        }
    }
}

#endif

/**
 * Handling resending of messages
 */ 
static char lastMsg[GDB_STR_SIZE];
static int  lastMsgCnt = 0;

struct dsp_session
{
     struct tty_port port;
     bool open;
     bool open_wronly;
     int openCount;
     struct mutex open_count_mutex;
     unsigned int sharedAddr;
     volatile int *handleSharedAddr;
};

/**
 * Structure used for storing session data
 */
struct tty_gdb_port 
{
     struct mutex port_write_mutex;
     //unsigned int edmaChannel;
     volatile int *handleInt;
     int numberOfDsps;
     int openDevices;
};

static struct tty_gdb_port gdb_port;
struct dsp_session *dsp_data;

/**
 * timer used for polling
 */
static struct timer_list timer;

static int initializeDSP(int numDSP, struct dsp_session *dsp, struct device *dev);
static void polling_register(unsigned long data);
static struct dsp_type_t *get_dspinfo(void);

/**
 * FUNCTION NAME: triggerInt()
 *
 * DESCRIPTION: This function is to trigger an interrupt to the DSP
 *
 * ARGUMENTS:
 *           int handleInt: The pointer to the DebugSS CTI APPSET
 *                          register
 *
 */
static void triggerInt(volatile int *handleInt, unsigned int status, volatile int *handleAddr)
{
     // also write the ready bit in the status word
     if (handleAddr != NULL)
          iowrite32(status | STATUS_NEW_PACKET, handleAddr);

     if (handleInt != NULL)
     {
        //clear EMU1 debugss cti trigger
        iowrite32(CTI_EMU1_TRIGGER, (void *)(((char *)handleInt) + DEBUGSS_CTIAPPCLEAR_REG_ADDR)); 

        //set EMU1 debugss cti trigger
        iowrite32(CTI_EMU1_TRIGGER, (void *)(((char *)handleInt) + DEBUGSS_CTIAPPSET_REG_ADDR)); //By default, GDB driver uses EMU1 trigger for ARM-DSP IPC
     }
}

/**
 * FUNCTION NAME: gdb_sendMessage()
 *
 * DESCRIPTION: This function sends a message to the DSP
 *
 * ARGUMENTS:
 *           const unsigned char *buf: The pointer to the message to be sent
 *           int                count: The count (in characters) of the 
 *                                     message to be sent
 *
 */
static int gdb_sendMessage(const unsigned char *buf, int count, struct dsp_session * dsp)
{
     if (buf != NULL && dsp->handleSharedAddr != NULL) {

          if (*buf == '-') {

               memcpy_toio(((char *)dsp->handleSharedAddr) + 8, lastMsg, lastMsgCnt);
               triggerInt(gdb_port.handleInt, 0, dsp->handleSharedAddr + 1);
               if (DEBUG) {

                    printk("gdbserverproxy: Message Sent= %.*s\n", lastMsgCnt, lastMsg);
                    printk("gdbserverproxy: Sent Count= %d\n", lastMsgCnt);
               }
          }
          else if ((*buf != '+') && (*buf != '-')) {

               memcpy(lastMsg, buf, count);
               lastMsgCnt = count;
               memcpy_toio(((char *)dsp->handleSharedAddr) + 8, buf, count);
               triggerInt(gdb_port.handleInt, 0, dsp->handleSharedAddr + 1);
               tty_insert_flip_char(&dsp->port, '+', 1);
               tty_flip_buffer_push(&dsp->port);
               if(DEBUG) {

                    printk("gdbserverproxy: Message Sent= %.*s\n", count, buf);
                    printk("gdbserverproxy: Sent Count= %d\n", count);
               }
          }
     }
     else
          count = 0;

    return count;
}

/**
 * FUNCTION NAME: gdb_open()
 *
 * DESCRIPTION: TTY operations open function
 *
 * ARGUMENTS:
 *           struct tty_struct *tty: The tty struct pointer
 *           struct file      *filp: File pointer
 * 
 *
 */
static int gdb_open(struct tty_struct *tty, struct file *filp)
{
     int ret = -ECONNREFUSED;
     int numDSP = tty->index;

     mutex_lock(&dsp_data[numDSP].open_count_mutex);
     dsp_data[numDSP].openCount++;
     mutex_unlock(&dsp_data[numDSP].open_count_mutex);

     // pass the specific dsp to read/write functions
     tty->driver_data = &dsp_data[numDSP];

     if (dsp_data[numDSP].open == true)
          return -EBUSY;

     // open with this flag to begin communication
     if (filp->f_flags & O_RDWR) {

          // only one dsp can get initialized at a time
          mutex_lock(&gdb_port.port_write_mutex);
          ret = initializeDSP(numDSP, &dsp_data[numDSP], tty->dev);
          if(ret == 0)
               gdb_port.openDevices++;
          mutex_unlock(&gdb_port.port_write_mutex);

          if (ret < 0) {

               if(DEBUG)
                    printk(KERN_ERR "Couldn't register gdbtty driver: Memory initialization failed\n");

               return ret;
          }

          if (gdb_port.openDevices == 1) {

               ret = mod_timer(&timer, jiffies + msecs_to_jiffies(POLL_DELAY_MS));
               if (ret < 0) {

                    if(DEBUG)
                         printk(KERN_ERR "Couldn't register gdbtty driver: Timer initialization failed\n");

                    return ret;
               }
          }

          return tty_port_open(&dsp_data[numDSP].port, tty, filp);
     }

     // open with this flag to write the initial address
     else if (filp->f_flags & O_WRONLY) {

          dsp_data[numDSP].open_wronly = true;
          dsp_data[numDSP].open = true;
          // pass the specific dsp to read/write functions
          tty->driver_data = &dsp_data[numDSP];
          return 0;
     }

     return ret;
     
}

/**
 * FUNCTION NAME: gdb_close()
 *
 * DESCRIPTION: TTY operations close function
 *
 * ARGUMENTS:
 *           struct tty_struct *tty: The tty struct pointer
 *           struct file      *filp: File pointer
 * 
 *
 */
static void gdb_close(struct tty_struct *tty, struct file *filp)
{
     struct dsp_session *dsp = tty->driver_data;

     if (NULL != dsp) {
          // trigger the interrupt, send release status, and release shared address
          if (dsp->open && (dsp->openCount == 1) && !dsp->open_wronly) {
 
               triggerInt(gdb_port.handleInt, DETACH_COMM, dsp->handleSharedAddr + 1);
               release_mem_region(dsp->sharedAddr, GDB_STR_SIZE);
               iounmap(dsp->handleSharedAddr);
               dsp->open = false;
               tty_port_close(&dsp->port, tty, filp);

               mutex_lock(&gdb_port.port_write_mutex);
               gdb_port.openDevices--;
               mutex_unlock(&gdb_port.port_write_mutex);

               // this signifies the last device closing
               if (gdb_port.openDevices == 0) {
                    // release the timer
                    del_timer(&timer);

                    // unmap the interrupt and set the address to NULL
                    if(gdb_port.handleInt != NULL) {

                         iounmap(gdb_port.handleInt);
             release_mem_region(DEBUGSS_CTI_BASE_ADDR, PAGE_SIZE);
                         gdb_port.handleInt = NULL;
                    }
               }
          }
     
          if (dsp->open && dsp->open_wronly) {
               dsp->open = false;
               dsp->open_wronly = false;
          }

          mutex_lock(&dsp->open_count_mutex);
          dsp->openCount--;
          mutex_unlock(&dsp->open_count_mutex);
     }   
}

/**
 * FUNCTION NAME: gdb_write()
 *
 * DESCRIPTION: TTY operations write function
 *
 * ARGUMENTS:
 *           struct tty_struct *tty: The tty struct pointer
 *           struct file      *filp: File pointer
 * 
 *
 */
static int gdb_write(struct tty_struct *tty,
        const unsigned char *buf, int count)
{
     struct dsp_session *dsp = tty->driver_data;
     int ret = 0;
     unsigned int sharedAddress = 0;
     //unsigned int test_value;

     if ((NULL != dsp) && !dsp->open_wronly) {

         mutex_lock(&gdb_port.port_write_mutex);
         ret = gdb_sendMessage(buf, count, dsp);
         mutex_unlock(&gdb_port.port_write_mutex);
     }
     
     if ((NULL != dsp) && dsp->open_wronly) {

          sscanf(buf, "%x", &sharedAddress);
          // the buffer is indexed by DSP number
          dsp->sharedAddr = sharedAddress + (tty->index * GDB_STR_SIZE);
          ret = count;

          if(DEBUG) {
               printk("gdbserverproxy: opened file /dev/gdbtty%d\n", tty->index);
               printk("gdbserverproxy: shared address = %x\n", dsp->sharedAddr);
          }
     }

     

    return ret;
}

/**
 * FUNCTION NAME: gdb_write_room()
 *
 * DESCRIPTION: TTY operations write_room function
 *
 * ARGUMENTS:
 *           struct tty_struct *tty: The tty struct pointer
 *           struct file      *filp: File pointer
 * 
 *
 */
static int gdb_write_room(struct tty_struct *tty)
{
     return GDB_STR_SIZE;
}

/**
 * FUNCTION NAME: gdb_ioctl()
 *
 * DESCRIPTION: TTY operations ioctl function
 *
 * ARGUMENTS:
 *           struct tty_struct *tty: The tty struct pointer
 *           struct file      *filp: File pointer
 * 
 *
 */
static int gdb_ioctl(struct tty_struct *tty,
            unsigned int cmd, unsigned long arg)
{
     struct dsp_session *dsp = tty->driver_data;

     if (NULL == dsp)
          return -EINVAL;

     return -ENOIOCTLCMD;
}

/**
 * Function pointer for tty operations
 */
static const struct tty_operations tty_gdb_ops = {
     .open          = gdb_open,
     .close         = gdb_close,
     .write         = gdb_write,
     .write_room    = gdb_write_room,
     .ioctl         = gdb_ioctl,
};

 
/**
 * FUNCTION NAME: initializeDSP()
 *
 * DESCRIPTION: Initialize the DSP to be debugged and 
 *              allocate the necessary memory regions
 *
 * ARGUMENTS:
 *           int              numDSP: The DSP number
 *           struct dsp_session *dsp: Pointer to the current dsp_session
 *           struct device      *dev: Pointer to the device
 *
 */
static int initializeDSP(int numDSP, struct dsp_session *dsp, struct device *dev)
{
     int retries = 50;
     int error = 0;

     if(dsp->sharedAddr != 0) {

          if(request_mem_region(dsp->sharedAddr, GDB_STR_SIZE, DRIVER_NAME) == NULL) {

               if (DEBUG)
                    printk("gdbserverproxy: Request for memory failed\n");
          }
          // map the shared memory
          dsp->handleSharedAddr = ioremap_nocache((volatile unsigned int)dsp->sharedAddr, GDB_STR_SIZE);
     }
     else {

          if(DEBUG)
               printk("gdbserverproxy: Shared address is uninitialized\n");
          return -ECONNREFUSED;
     }

     if (dsp->handleSharedAddr != NULL) {

          // if this isn't NULL, we aren't the first device to map the register, so
          // just use the current (valid) handle
          if(gdb_port.handleInt == NULL) {

        if(NULL != request_mem_region(DEBUGSS_CTI_BASE_ADDR, PAGE_SIZE, DRIVER_NAME))
        {
            gdb_port.handleInt = ioremap_nocache(DEBUGSS_CTI_BASE_ADDR, PAGE_SIZE);

            if (gdb_port.handleInt != NULL) {

                //unlock debugss cti registers
                iowrite32(CS_UNLOCK_PATTERN, (void *)(((char *)gdb_port.handleInt) + DEBUGSS_CTILAR_REG_ADDR));

                //enable debugss cti module
                iowrite32(0x1, (void *)(((char *)gdb_port.handleInt) + DEBUGSS_CTICNTL_REG_ADDR));

                //check whether debugss cti module is enabled
                if(1 != ioread32((void *)(((char *)gdb_port.handleInt) + DEBUGSS_CTICNTL_REG_ADDR)))
                {
                    if(DEBUG)
                         printk("gdbserverproxy: Enabling Debugss CTI module failed\n");
                    goto fail_1;
                }
            } else {

                   if (DEBUG)
                    printk("gdbserverproxy: Couldn't allocate debugss cti register\n");
                   goto fail_1;
            }

        } else {
            if(DEBUG)
                 printk("gdbserverproxy: failed to get access to DebugSS CTI address space: start_addr:0x%x, length:%lu\n",DEBUGSS_CTI_BASE_ADDR,PAGE_SIZE);
                        goto fail_1;
        }

          }

          // initialize the DSP
          if (gdb_port.handleInt != NULL) {

               // write the DSP num (+1) to initialize the specific DSP
               iowrite32(numDSP + 1, dsp->handleSharedAddr);

          if (DEBUG) {

               printk("gdbserverproxy: Physical address = %p\n", (void *)dsp->sharedAddr);
               printk("gdbserverproxy: Virtual address = %p\n", (void *)dsp->handleSharedAddr);
          }

               if (DEBUG)
                    printk("gdbserverproxy: dsp id at shared address: %d\n",ioread32(dsp->handleSharedAddr));

               triggerInt(gdb_port.handleInt, 0, NULL);
          }
          else {

               if (DEBUG)
                    printk("gdbserverproxy: Couldn't allocate debugss cti register\n");
               goto fail_1;
          }

          // wait for response
          while ((ioread32(dsp->handleSharedAddr) != ACK) && (retries > 0)) {
               udelay(2);
               retries--;
      }


          if (retries <= 0) {

               if(DEBUG)
                    printk("gdbserverproxy: The DSP did not respond to the request\n");
               goto fail_1;
          }

          dsp->open = true;
          if (DEBUG) {

               printk("gdbserverproxy: Physical address = %p\n", (void *)dsp->sharedAddr);
               printk("gdbserverproxy: Virtual address = %p\n", (void *)dsp->handleSharedAddr);
          }
          
          return error;
     }  

fail_1:
     if (gdb_port.handleInt != NULL) {

          iounmap(gdb_port.handleInt);
      release_mem_region(DEBUGSS_CTI_BASE_ADDR, PAGE_SIZE);
          gdb_port.handleInt = NULL;
     }

     if (dsp->handleSharedAddr != NULL) {

          release_mem_region(dsp->sharedAddr, GDB_STR_SIZE);
          iounmap(dsp->handleSharedAddr);
     }
     
     return -ECONNREFUSED;
}

/**
 * FUNCTION NAME: polling_register()
 *
 * DESCRIPTION: Continually polls a memory region in order
 *              to read data sent from the DSP
 *
 */
static void polling_register(unsigned long data)
{
     int value = 0;
     char *message = 0;
     int count = 0;
     int device = 0;

     // iterate through all DSPs to ensure all messages are consumed
     for (device = 0; device < gdb_port.numberOfDsps; device++) {

          if (dsp_data[device].open == true) {

               // read the status to find out if a new message is available
               value = ioread32(dsp_data[device].handleSharedAddr + 1);

               // process the data
               if (READY_FLAG == (READY_FLAG & value)) {

                    count = ((value & COUNT_MASK) >> COUNT_SHIFT);
                    if ((count > 0) && (count <= GDB_STR_SIZE)) {

                         message = (char *) kmalloc(count, GFP_KERNEL);
                         memcpy_fromio(message, ((char *)dsp_data[device].handleSharedAddr) + 8, count);
                         if(DEBUG) {

                              printk("gdbserverproxy: Message received = %.*s\n", count, message);
                              printk("gdbserverproxy: Received Count = %d\n", count);
                         }
                         tty_insert_flip_string(&dsp_data[device].port, message, count);
                         tty_flip_buffer_push(&dsp_data[device].port);
                         kfree(message);
                    }
                    else {
                         if (DEBUG)
                              printk("gdbserverproxy: Count check failed = %d\n", count);
                    }

                    // clear size and ready bit
                    iowrite32(0, dsp_data[device].handleSharedAddr + 1);
               }
          }        
     }
     // reset the timer
     mod_timer(&timer, jiffies + msecs_to_jiffies(POLL_DELAY_MS));
}

/**
 * FUNCTION NAME: get_dspinfo()
 *
 * DESCRIPTION: provides number of DSPs in a device
 *
 */
static struct dsp_type_t *get_dspinfo(void)
{
     int i = 0;
     // do not compare against "unknown" at the end of table
     int num_elements = sizeof(dsp_type_table)/sizeof(struct dsp_type_t) - 1;

#ifdef KEYSTONE_PLATFORM

        void * bootcfg_reg_map_ptr;
        unsigned int jtagid;
        ekeystone_device_type target_device_type;

        //if(NULL != request_mem_region(BOOTCFG_BASE_ADDR, PAGE_SIZE, DRIVER_NAME))
        //{
            /* Identify target device type by reading the JTAG ID */
            /* Map first 4K of BOOTCFG register memory to kernel space memory */
            bootcfg_reg_map_ptr = ioremap_nocache(BOOTCFG_BASE_ADDR, PAGE_SIZE);

            /* Read JTAG ID */
            jtagid = ioread32((void *)(((char *)bootcfg_reg_map_ptr) + 
                            BOOTCFG_JTAGID_REG));

            /* identify device type */
            target_device_type = identify_keystone_device(jtagid);

            /* Un-map BOOTCFG register memory from kernel space memory */
            iounmap(bootcfg_reg_map_ptr);

            /* Release mem region */
            //release_mem_region(BOOTCFG_BASE_ADDR, PAGE_SIZE);
        //} else {
        //  __E("failed to get access to BOOTCFG address space: start_addr:0x%x, length:%d\n",BOOTCFG_BASE_ADDR,PAGE_SIZE);
        //}

#endif

#ifdef DRA7xx_PLATFORM

    void * dieid_reg_map_ptr;
    unsigned int die_id_2;
        unsigned int id_code;
        edra7xx_device_type target_device_type;

    //if(NULL != request_mem_region(DIE_ID_BASE_ADDR, PAGE_SIZE, DRIVER_NAME))
    //{
        /* Identify target device type by reading the DIE ID 2 */
        /* Map first 4K of Die indentification register memory to kernel space memory */
        dieid_reg_map_ptr = ioremap_nocache(DIE_ID_BASE_ADDR, PAGE_SIZE);

        /* Read DIE_ID_2 */
        die_id_2 = ioread32((void *)(((char *)dieid_reg_map_ptr) + 
                        DIE_ID_2_REG));

        /* Read ID_CODE */
        id_code = ioread32((void *)(((char *)dieid_reg_map_ptr) + 
                        ID_CODE_REG));

        /* identify device type */
        target_device_type = identify_dra7xx_device(die_id_2, id_code);

        /* Un-map DIEID register memory from kernel space memory */
        iounmap(dieid_reg_map_ptr);

        /* Release mem region */
        //release_mem_region(DIE_ID_BASE_ADDR, PAGE_SIZE);
    //} else {
    //  __E("failed to get access to DIE_ID address space: start_addr:0x%x, length:%lu\n",DIE_ID_BASE_ADDR,PAGE_SIZE);
    //}

#endif 

     // compare to table of known IDs
     for (i = 0; i < num_elements; i++) {

          if (target_device_type == dsp_type_table[i].dev_enum)
               break;
     }
     
     return &dsp_type_table[i];
}

static struct tty_port_operations null_ops = { };

static struct tty_driver *tty_gdb_driver;

static int __init gdb_dev_init(void)
{
     int ret = -ENOMEM;
     int device = 0;

     // find number of DSPs in device
     struct dsp_type_t *dspType = get_dspinfo();
     
     if (DEBUG)
          printk("gdbserverproxy gdb_dev_init: number of DSPs = %d\n", dspType->num_dsps);
     
     // allocate space for number of DSPs
     dsp_data = (struct dsp_session *)kmalloc(dspType->num_dsps * sizeof(struct dsp_session), GFP_KERNEL);
     
     memset(dsp_data, 0, dspType->num_dsps * sizeof(struct dsp_session));

     mutex_init(&gdb_port.port_write_mutex);

     tty_gdb_driver = tty_alloc_driver(dspType->num_dsps,
               TTY_DRIVER_RESET_TERMIOS |
               TTY_DRIVER_REAL_RAW);
     if (IS_ERR(tty_gdb_driver))
          return PTR_ERR(tty_gdb_driver);

     // initialize the local data structure for all DSPs
     for (device = 0; device < dspType->num_dsps; device++) {

         tty_port_init(&dsp_data[device].port);
         dsp_data[device].port.ops = &null_ops;
          mutex_init(&dsp_data[device].open_count_mutex);
     }
     gdb_port.openDevices = 0;
     gdb_port.numberOfDsps = dspType->num_dsps;
     gdb_port.handleInt = NULL;

     tty_gdb_driver->driver_name = DRIVER_NAME;
     tty_gdb_driver->name = NAME;
     tty_gdb_driver->major = GDB_TTY_MAJOR;
     tty_gdb_driver->minor_start = 0;
     tty_gdb_driver->type = TTY_DRIVER_TYPE_CONSOLE;
     tty_gdb_driver->init_termios = tty_std_termios;
     tty_gdb_driver->init_termios.c_oflag = OPOST | OCRNL | ONOCR | ONLRET;
     tty_set_operations(tty_gdb_driver, &tty_gdb_ops);

     for (device = 0; device < gdb_port.numberOfDsps; device++)
          tty_port_link_device(&dsp_data[device].port, tty_gdb_driver, device);

     ret = tty_register_driver(tty_gdb_driver);

     if (ret < 0) {

          printk(KERN_ERR "Couldn't register gdbtty driver.\n");
          put_tty_driver(tty_gdb_driver);
          return ret;
     }

     // setup the timer for polling
     setup_timer(&timer, polling_register, 0);

     return 0;
}

static void __exit gdb_dev_exit(void)
{
     int device = 0;

     kfree(dsp_data);

     // unregister each device
     for(device = 0; device < gdb_port.numberOfDsps; device++)
          tty_unregister_device(tty_gdb_driver, device);

     // unregister driver
     tty_unregister_driver(tty_gdb_driver);
}

/* Module information */
MODULE_AUTHOR("Texas Instruments Incorporated");
MODULE_DESCRIPTION("Interface for GDB to communicate with c66x DSP");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS_CHARDEV_MAJOR(GDB_TTY_MAJOR);

module_exit(gdb_dev_exit);
module_init(gdb_dev_init);;









