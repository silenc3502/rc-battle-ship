/*
 * Debug Sub-System (DebugSS) module for Keystone and DRA7xx devices
 *
 * Copyright (C) 2014 Texas Instruments.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/err.h>
#include <asm/io.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include "debugss_kmodule.h"

/* un-comment the line below to enable printing for kernel debug messages */
//#define DEBUGSS_DRV_DEBUG

#ifdef DEBUGSS_DRV_DEBUG
#define __D(fmt, args...) dev_info(debugss_device, "debug_info: " fmt, ## args)
#else
#define __D(fmt, args...)
#endif

#define __E(fmt, args...) dev_err(debugss_device, "drv_error: " fmt, ## args)

#ifdef KEYSTONE_PLATFORM

/* static variable for storing the current device type */
static ekeystone_device_type target_device_type; 

/* Device driver static variables */
static struct device* debugss_device = NULL;
static struct clk		*mainpll_clk;
#ifndef NO_DEBUGSS_ARM_PLL
static struct clk		*armpll_clk;
#endif
static struct clk		*gemtrace_clk;
static struct clk		*debugss_clk;
static struct clk		*tetb_clk;

#endif

#ifdef DRA7xx_PLATFORM

/* static variable for storing the current device type */
static edra7xx_device_type target_device_type; 

/* Device driver static variables */
static struct device* debugss_device = NULL;
static struct clk		*sys_clk_in1;

#endif

/* Address map linked list node */
struct addr_map_node_t {
        struct vm_area_struct *vma;
        pid_t process_id;
        uint32_t phy_addr_start;
        uint32_t phy_addr_end;
        struct addr_map_node_t * next_node;
        struct addr_map_node_t * prev_node;
};

/* static head pointer for address map linked list */
static struct addr_map_node_t * head_ptr = NULL;

#ifdef KEYSTONE_PLATFORM
static ekeystone_device_type identify_keystone_device(uint32_t jtagid)
{
        __D("%s:\n",__FUNCTION__);    

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
static edra7xx_device_type identify_dra7xx_device(uint32_t die_id_2, uint32_t id_code)
{
	uint32_t base_pn;
	uint32_t ramp_system;

	__D("%s:\n",__FUNCTION__);

	base_pn = die_id_2 >> 24;
        ramp_system = (id_code & 0x0FFFF000) >> 12;

        __D("%s:DIE_ID_2 = 0x%x\n",__FUNCTION__,die_id_2);
        __D("%s:ID_CODE = 0x%x\n",__FUNCTION__,id_code);
        __D("%s:BASE_PN = %d\n",__FUNCTION__,base_pn);
        __D("%s:RAMP_SYSTEM = %d\n",__FUNCTION__,ramp_system);

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

/* This fuction returns 1 if the input address range is a valid debugss 
address range or else returns -1 for unvalid debugss address range */
static int check_valid_debugss_addr_range(uint32_t range_start_addr, 
                                          uint32_t range_end_addr)
{
        uint32_t idx;

        __D("%s:\n",__FUNCTION__);

#ifdef KEYSTONE_PLATFORM


		for(idx=0; idx<eK2H_DEBUGSS_NUM_RANGE_MAX; idx++) {
			if((range_start_addr >= k2h_debugss_addr_range[idx][0]) 
				&& (range_end_addr <= k2h_debugss_addr_range[idx][1])) {
					/* valid debugss address range */ 
					return(1);
            }
        }

        if(eK2H_DEBUGSS_NUM_RANGE_MAX == idx){
                return(-1); /* unvalid debugss address range */
        }

#endif

#ifdef DRA7xx_PLATFORM

                for(idx=0; idx<eDRA7xx_DEBUGSS_NUM_RANGE_MAX; idx++) {
                        if((range_start_addr >= dra7xx_debugss_addr_range[idx][0]) 
                       && (range_end_addr <= dra7xx_debugss_addr_range[idx][1])) {
                                /* valid debugss address range */                                
                                return(1);
                        }

                if(eDRA7xx_DEBUGSS_NUM_RANGE_MAX == idx){
                        return(-1); /* unvalid debugss address range */
                }
        }

#endif

        return(-1);
}

static int range_is_allowed(unsigned long pfn, unsigned long size)
{
        return 1;
}

int __weak phys_mem_access_prot_allowed(struct file *file,
        unsigned long pfn, unsigned long size, pgprot_t *vma_prot)
{
        return 1;
}

#ifndef __HAVE_PHYS_MEM_ACCESS_PROT

/*
 * Architectures vary in how they handle caching for addresses
 * outside of main memory.
 *
 */
#ifdef pgprot_noncached
static int uncached_access(struct file *file, phys_addr_t addr)
{
        /*
         * Accessing memory above the top the kernel knows about or through a
         * file pointer
         * that was marked O_DSYNC will be done non-cached.
         */
        if (file->f_flags & O_DSYNC)
                return 1;
        return addr >= __pa(high_memory);
}
#endif

static pgprot_t phys_mem_access_prot(struct file *file, unsigned long pfn,
                                     unsigned long size, pgprot_t vma_prot)
{
#ifdef pgprot_noncached
        phys_addr_t offset = pfn << PAGE_SHIFT;

        if (uncached_access(file, offset))
                return pgprot_noncached(vma_prot);
#endif
        return vma_prot;
}
#endif

static int private_mapping_ok(struct vm_area_struct *vma)
{
        return 1;
}

static int debugss_device_open(struct inode* inode, struct file* filp)
{
	__D("%s:\n",__FUNCTION__);

	/* Do nothing */

	return 0;
}

static int debugss_device_close(struct inode* inode, struct file* filp)
{
	__D("%s:\n",__FUNCTION__);

	/* Do nothing */

	return 0;
}

void debugss_vma_open(struct vm_area_struct *vma)
{   
	__D("%s:Phy address:0x%lx, Virt address:0x%lx, size:%d\n",__FUNCTION__,
		vma->vm_pgoff << PAGE_SHIFT, vma->vm_start, 
                (int)(vma->vm_end - vma->vm_start));

	return;
}
void debugss_vma_close(struct vm_area_struct *vma)
{
	struct addr_map_node_t * current_ptr = NULL;
	unsigned int phy_addr_start, phy_addr_end;
	phy_addr_start = vma->vm_pgoff << PAGE_SHIFT;
	phy_addr_end = (vma->vm_pgoff << PAGE_SHIFT) + (vma->vm_end - vma->vm_start) - sizeof(uint32_t);    
	__D("%s:Phy address:0x%x, Virt address:0x%lx, size:%d\n",__FUNCTION__, 
	    phy_addr_start, vma->vm_start, (int)(vma->vm_end - vma->vm_start));

	if(NULL != head_ptr) {
		current_ptr = head_ptr;

		while(1){
			if((current_ptr->vma == vma) && 
			(current_ptr->phy_addr_start == phy_addr_start) &&
			(current_ptr->phy_addr_end == phy_addr_end) &&
			(current_ptr->process_id == current->pid)) {
				/* Remove the current node from the list */
				/* check if this is the only node in the list */
				if((NULL == current_ptr->prev_node) && 
				   (NULL == current_ptr->next_node)) {
					head_ptr = NULL;
				} else if(NULL == current_ptr->prev_node) {
					/* first node of the list */
					current_ptr->next_node->prev_node = NULL;
				} else if(NULL == current_ptr->next_node) {
		                	/* last node of the list */
					current_ptr->prev_node->next_node = NULL;
				} else {
					/* any node in the middle */
					current_ptr->prev_node->next_node = current_ptr->next_node;
					current_ptr->next_node->prev_node = current_ptr->prev_node;
				}

				/* Free current node */
				kfree((const void *)current_ptr);

				__D("%s: Removed node successfully\n",__FUNCTION__);

				break;
			}

			if(NULL == current_ptr->next_node) {
				break;
			} else {
				current_ptr = current_ptr->next_node;
			}
		}
	}

	return;
}

static struct vm_operations_struct debugss_remap_vm_ops = {
        .open = debugss_vma_open,
        .close = debugss_vma_close,
};

static const struct vm_operations_struct mmap_mem_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
        .access = generic_access_phys
#endif
};

static int debugss_remap_mmap(struct file *filp, struct vm_area_struct *vma)
{
	size_t size;
	unsigned long phy_addr_start, phy_addr_end;
	struct addr_map_node_t * current_ptr = NULL;
	struct addr_map_node_t * new_ptr = NULL;
	__D("%s:\n",__FUNCTION__);

	size = vma->vm_end - vma->vm_start;

	if (!private_mapping_ok(vma))
	    return -ENOSYS;

	if (!range_is_allowed(vma->vm_pgoff, size))
	    return -EPERM;

	if (!phys_mem_access_prot_allowed(filp, vma->vm_pgoff, size,
		                            &vma->vm_page_prot))
	    return -EINVAL;


	vma->vm_page_prot = phys_mem_access_prot(filp, vma->vm_pgoff,
		                             size,
		                             vma->vm_page_prot);

	vma->vm_ops = &mmap_mem_ops;

	/* check if the requested PHY address range to be mapped is part
	of the device debugss memory map */
	phy_addr_start = vma->vm_pgoff << PAGE_SHIFT;
	phy_addr_end = (vma->vm_pgoff << PAGE_SHIFT) + size - sizeof(uint32_t);
	__D("%s:Phy address start:0x%lx, Phy address end:0x%lx, size:%d\n",__FUNCTION__,phy_addr_start,phy_addr_end, size);
	if(-1 == check_valid_debugss_addr_range(phy_addr_start, phy_addr_end)) {
		/* return error, invalid debugss address range */
		__E("%s: Invalid debugss physical address range is being provided\n",__FUNCTION__);
		return -EINVAL;
	}

	/* check if the requested PHY address range is already mapped by
	a different process */
	if(NULL != head_ptr) {
		current_ptr = head_ptr;

		while(1) {
			if(((phy_addr_start >= current_ptr->phy_addr_start) && 
			(phy_addr_start <= current_ptr->phy_addr_end)) || 
			((phy_addr_end >= current_ptr->phy_addr_start) && 
			(phy_addr_end <= current_ptr->phy_addr_end))) {
				if(current->pid != current_ptr->process_id) {
					/* return error, the requested debugss 
   				        address range is already mapped by 
					another process*/
					__E("%s: Requested debugss physical address range is already mapped by another process with pid:%d\n",__FUNCTION__,current_ptr->process_id);
					return -EINVAL;
				}
			}

			if(NULL == current_ptr->next_node) {
				break;
			} else {
				current_ptr = current_ptr->next_node;
			}
		}
	}

	/* Remap-pfn-range will mark the range VM_IO */
	if (remap_pfn_range(vma,
		        vma->vm_start,
		        vma->vm_pgoff,
		        size,
		        vma->vm_page_prot)) {
		return -EAGAIN;
	}

	vma->vm_ops = &debugss_remap_vm_ops;
	debugss_vma_open(vma);

	/* Add a new address map node to the end of the list */
	new_ptr = (struct addr_map_node_t *)kmalloc(sizeof(struct addr_map_node_t), GFP_KERNEL);
	if(NULL == new_ptr) {
		__E("%s: kmalloc returned NULL, Kernel memory allocation error\n",__FUNCTION__);
		return -ENOMEM;
	}

	//populate the new node
	new_ptr->vma = vma;
	new_ptr->process_id = current->pid;
	new_ptr->phy_addr_start = vma->vm_pgoff << PAGE_SHIFT;
	new_ptr->phy_addr_end = (vma->vm_pgoff << PAGE_SHIFT) + 
			        (vma->vm_end - vma->vm_start) - sizeof(uint32_t);

	/* Traverse to the end of the list */
	if(NULL != head_ptr) {
		current_ptr = head_ptr;

		while(NULL != current_ptr->next_node) {
			current_ptr = current_ptr->next_node;
		}
		//Add this node to end of the existing list
		new_ptr->prev_node = current_ptr;
		current_ptr->next_node = new_ptr;
		new_ptr->next_node = NULL;        
	} else {
		//Add this node as the first node in the list
		head_ptr = new_ptr;
		new_ptr->prev_node = NULL;
		new_ptr->next_node = NULL;
	}

	__D("%s: Added node successfully\n",__FUNCTION__);

	return 0;
}

static const struct file_operations debugss_fops = {
	.open = debugss_device_open,
	.mmap = debugss_remap_mmap,
	.release = debugss_device_close,
};

static struct miscdevice debugss_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "debugss",
	.fops = &debugss_fops,
};

static ssize_t show_device_name(struct device* dev, 
				struct device_attribute* attr, 
				char* buf)
{
	ssize_t byte_count;

	__D("%s:\n",__FUNCTION__);

	//Please Note: device name should be <= 15 characters
	//to be compatible with DSPTOP utility

#ifdef KEYSTONE_PLATFORM

	if(e66AK2Hxx_device == target_device_type) {
		byte_count = sprintf(buf,"66AK2Hxx\n");
	}
	else if(eTCI6614_device == target_device_type) {
		byte_count = sprintf(buf,"TCI6614\n");
	}
	else if(eTCI6630K2L_device == target_device_type) {
		byte_count = sprintf(buf,"TCI6630K2L\n");
	}
	else if(e66AK2Exx_device == target_device_type) {
		byte_count = sprintf(buf,"66AK2Exx\n");
	}
	else {
		byte_count = sprintf(buf,"Un-Supported Device\n");
	}

#endif

#ifdef DRA7xx_PLATFORM

	if(eDRA74xx_device == target_device_type) {
		byte_count = sprintf(buf,"DRA74xx\n");
	}
	else if(eDRA75xx_device == target_device_type) {
		byte_count = sprintf(buf,"DRA75xx\n");
	}
	else if(eAM572x_device == target_device_type) {
		byte_count = sprintf(buf,"AM572x\n");
	}
	else if(eAM571x_device == target_device_type) {
		byte_count = sprintf(buf,"AM571x\n");
	}
	else {
		byte_count = sprintf(buf,"Un-Supported Device\n");
	}

#endif

	return byte_count;
}

#ifdef KEYSTONE_PLATFORM

static ssize_t get_mainpllclk(struct device* dev, 
			      struct device_attribute* attr, 
                              char* buf)
{
	ssize_t byte_count;
	unsigned long clk_rate;

	__D("%s:\n",__FUNCTION__);

	/* Get main PLL clock rate*/
	clk_rate = clk_get_rate(mainpll_clk);

	byte_count = sprintf(buf,"%ld\n",clk_rate);

	return byte_count;
}

#ifndef NO_DEBUGSS_ARM_PLL
static ssize_t get_armpllclk(struct device* dev, 
				struct device_attribute* attr, 
				char* buf)
{
	ssize_t byte_count;
	unsigned long clk_rate;

	__D("%s:\n",__FUNCTION__);

	/* Get arm PLL clock rate*/
	clk_rate = clk_get_rate(armpll_clk);

	byte_count = sprintf(buf,"%ld\n",clk_rate);

	return byte_count;
}
#endif

static ssize_t get_debugssclk(struct device* dev, 
			      struct device_attribute* attr, 
   			      char* buf)
{
	ssize_t byte_count;
	unsigned long clk_rate;

	__D("%s:\n",__FUNCTION__);

	/* Get debugss clock rate*/
	clk_rate = clk_get_rate(debugss_clk);

	byte_count = sprintf(buf,"%ld\n",clk_rate);

	return byte_count;
}

static ssize_t get_gemtraceclk(struct device* dev, 
			       struct device_attribute* attr, 
			       char* buf)
{
	ssize_t byte_count;
	unsigned long clk_rate;

	__D("%s:\n",__FUNCTION__);

	/* Get gemtrace clock rate*/
	clk_rate = clk_get_rate(gemtrace_clk);

	byte_count = sprintf(buf,"%ld\n",clk_rate);

	return byte_count;
}

static ssize_t get_tetbclk(struct device* dev, 
		           struct device_attribute* attr, 
			   char* buf)
{
	ssize_t byte_count;
	unsigned long clk_rate;

	__D("%s:\n",__FUNCTION__);

	/* Get tetb clock rate*/
	clk_rate = clk_get_rate(tetb_clk);

	byte_count = sprintf(buf,"%ld\n",clk_rate);

	return byte_count;

}

/* Declare the sysfs entries. The macros create instances of 
dev_attr_device_name, dev_attr_mainpllclk, dev_attr_armpllclk 
dev_attr_debugssclk and dev_attr_gemtraceclk */
static DEVICE_ATTR(device_name, S_IRUGO, show_device_name, NULL);
static DEVICE_ATTR(mainpllclk, S_IRUGO, get_mainpllclk, NULL);
#ifndef NO_DEBUGSS_ARM_PLL
static DEVICE_ATTR(armpllclk, S_IRUGO, get_armpllclk, NULL);
#endif
static DEVICE_ATTR(debugssclk, S_IRUGO, get_debugssclk, NULL);
static DEVICE_ATTR(gemtraceclk, S_IRUGO, get_gemtraceclk, NULL);
static DEVICE_ATTR(tetbclk, S_IRUGO, get_tetbclk, NULL);

static int keystone_debugss_probe(struct platform_device *pdev)
{
	void * reg_map_ptr;
	unsigned int jtagid;
	unsigned int reg_value;
	int retval = 0;
	int timeout;

	__D("%s:\n",__FUNCTION__);

	debugss_device = &pdev->dev;

	/* Prepare mainpll clock */
	mainpll_clk = devm_clk_get(debugss_device, "mainpllclock");
	if (WARN_ON(IS_ERR(mainpll_clk)))
		return PTR_ERR(mainpll_clk);

	clk_prepare_enable(mainpll_clk);

#ifndef NO_DEBUGSS_ARM_PLL
	/* Prepare armpll clock */
	armpll_clk = devm_clk_get(debugss_device, "armpllclock");
	if (WARN_ON(IS_ERR(armpll_clk)))
		return PTR_ERR(armpll_clk);

	clk_prepare_enable(armpll_clk);
#endif

	/* Prepare gemtrace clock */
	gemtrace_clk = devm_clk_get(debugss_device, "gemtraceclock");
	if (WARN_ON(IS_ERR(gemtrace_clk)))
		return PTR_ERR(gemtrace_clk);

	clk_prepare_enable(gemtrace_clk);

	/* Prepare debugss clock */
	debugss_clk = devm_clk_get(debugss_device, "debugssclock");
	if (WARN_ON(IS_ERR(debugss_clk)))
		return PTR_ERR(debugss_clk);

	clk_prepare_enable(debugss_clk);

	/* Prepare tetb clock */
	tetb_clk = devm_clk_get(debugss_device, "tetbclock");
	if (WARN_ON(IS_ERR(tetb_clk)))
		return PTR_ERR(tetb_clk);

	clk_prepare_enable(tetb_clk);

	/* Create debugss misc device */
	retval = misc_register(&debugss_miscdev);
	if (retval < 0) {
		dev_err(debugss_device, "cannot register debugss misc device\n");
	}

	debugss_device = debugss_miscdev.this_device;

        //Power up DebugSS, if not already powered up

	/* Map first 4K of PSC memory to kernel space memory */
	reg_map_ptr = ioremap_nocache(PSC_BASE, PAGE_SIZE);

	reg_value = ioread32((void *)(((char *)reg_map_ptr) + 
					PSC_DEBUGSS_MDSTAT));

	if((reg_value & 0x1F) != PSC_ENABLE)
	{
		
		//wait PSC_GOSTAT to clear
		timeout = 100000;

		while(timeout > 0)
		{
			reg_value = ioread32((void *)(((char *)reg_map_ptr) + 
					PSC_PTSTAT));

			if((reg_value & (0x1 << PSC_DEBUGSS_PD)) == 0x0)
			{
				break;
			} else {
				timeout--;
			}
		}

		if(timeout <= 0)
		{
			__E("EMU DebugSS power up timeout\n");
		}

		// Set power domain control
		reg_value = ioread32((void *)(((char *)reg_map_ptr) + 
				PSC_DEBUGSS_PDCTL));
		reg_value = reg_value | 0x00000001;
		iowrite32(reg_value, (void *)(((char *)reg_map_ptr) + 
					PSC_DEBUGSS_PDCTL));

		// Set MDCTL NEXT to new state
		reg_value = ioread32((void *)(((char *)reg_map_ptr) + 
				PSC_DEBUGSS_MDCTL));
		reg_value = (reg_value & ~(0x1f)) | PSC_ENABLE;
		iowrite32(reg_value, (void *)(((char *)reg_map_ptr) + 
					PSC_DEBUGSS_MDCTL));

		// Start power transition by setting PTCMD GO to 1
		reg_value = ioread32((void *)(((char *)reg_map_ptr) + 
				PSC_PTCMD));
		reg_value = reg_value | (0x1<<PSC_DEBUGSS_PD);
		iowrite32(reg_value, (void *)(((char *)reg_map_ptr) + 
					PSC_PTCMD));

		//wait PSC_GOSTAT to clear
		timeout = 100000;

		while(timeout > 0)
		{
			reg_value = ioread32((void *)(((char *)reg_map_ptr) + 
					PSC_PTSTAT));

			if((reg_value & (0x1 << PSC_DEBUGSS_PD)) == 0x0)
			{
				break;
			} else {
				timeout--;
			}
		}

		if(timeout <= 0)
		{
			__E("EMU DebugSS power up timeout\n");
		}

		//Verify if DebugSS is powered up
		reg_value = ioread32((void *)(((char *)reg_map_ptr) + 
					PSC_DEBUGSS_MDSTAT));

		if((reg_value & 0x1F) != PSC_ENABLE)
		{
			__E("EMU DebugSS power up failed\n");
	        }

                __D("%s:EMU DebugSS power up done\n",__FUNCTION__);
		
	}else {
		__D("%s:EMU DebugSS already powered up\n",__FUNCTION__);
	}

	/* Identify target device type by reading the JTAG ID */
	/* Map first 4K of BOOTCFG register memory to kernel space memory */
	reg_map_ptr = ioremap_nocache(BOOTCFG_BASE_ADDR, PAGE_SIZE);

	/* Read JTAG ID */
	jtagid = ioread32((void *)(((char *)reg_map_ptr) + 
					BOOTCFG_JTAGID_REG));

	/* identify device type */
	target_device_type = identify_keystone_device(jtagid);

	/* Un-map BOOTCFG register memory from kernel space memory */
	iounmap(reg_map_ptr);

	/* Now we can create the sysfs endpoints */
	retval = device_create_file(debugss_device, &dev_attr_device_name);
	if (retval < 0) {
		__E("failed to create debugss device_name /sys endpoint\n");
	}
	retval = device_create_file(debugss_device, &dev_attr_mainpllclk);
	if (retval < 0) {
		__E("failed to create debugss mainpllclk /sys endpoint\n");
	}
#ifndef NO_DEBUGSS_ARM_PLL
	retval = device_create_file(debugss_device, &dev_attr_armpllclk);
	if (retval < 0) {
		__E("failed to create debugss armpllclk /sys endpoint\n");
	}
#endif
	retval = device_create_file(debugss_device, &dev_attr_debugssclk);
	if (retval < 0) {
	__E("failed to create debugss debugssclk /sys endpoint\n");
	}
	retval = device_create_file(debugss_device, &dev_attr_gemtraceclk);
	if (retval < 0) {
		__E("failed to create debugss gemtraceclk /sys endpoint\n");
	}
	retval = device_create_file(debugss_device, &dev_attr_tetbclk);
	if (retval < 0) {
		__E("failed to create debugss tetbclk /sys endpoint\n");
	}

	return retval;
}

static int keystone_debugss_remove(struct platform_device *pdev)
{
	__D("%s:\n",__FUNCTION__);

	device_remove_file(debugss_device, &dev_attr_device_name);
	device_remove_file(debugss_device, &dev_attr_mainpllclk);
#ifndef NO_DEBUGSS_ARM_PLL
	device_remove_file(debugss_device, &dev_attr_armpllclk);
#endif
	device_remove_file(debugss_device, &dev_attr_debugssclk);
	device_remove_file(debugss_device, &dev_attr_gemtraceclk);
	device_remove_file(debugss_device, &dev_attr_tetbclk);

	/* De-register debugss misc device */
	misc_deregister(&debugss_miscdev);

	/* Un-prepare mainpll clock */
	clk_disable_unprepare(mainpll_clk);

#ifndef NO_DEBUGSS_ARM_PLL
	/* Un-prepare armpll clock */
	//clk_disable_unprepare(armpll_clk);
#endif

	/* Un-prepare gemtrace clock */
	clk_disable_unprepare(gemtrace_clk);

	/* Un-prepare debugSS clock */
	clk_disable_unprepare(debugss_clk);

	/* Un-prepare TETB clock */
	clk_disable_unprepare(tetb_clk);

	return 0;
}

static const struct of_device_id keystone_debugss_of_match[] = {
	{ .compatible = "ti,keystone-debugss", },
	{},
};
MODULE_DEVICE_TABLE(of, keystone_debugss_of_match);

static struct platform_driver platform_debugss_driver = {
	.driver = {
		.name = "debugss",
		.owner	= THIS_MODULE,
		.of_match_table = keystone_debugss_of_match,
	},
	.probe = keystone_debugss_probe,
	.remove = keystone_debugss_remove,
};

#endif

#ifdef DRA7xx_PLATFORM

static ssize_t get_debugssclk(struct device* dev, 
			      struct device_attribute* attr, 
   			      char* buf)
{
	ssize_t byte_count;
	unsigned long clk_rate;
	void * reg_map_ptr;
	unsigned int reg_value,M,N,M5_CLKDIV;

	__D("%s:\n",__FUNCTION__);

	/* Get sysclockin1 rate*/
	clk_rate = clk_get_rate(sys_clk_in1);

	if(NULL != request_mem_region(DPLL_DEBUG_BASE_ADDR, PAGE_SIZE, DRIVER_NAME))
	{
		/* Map 4K of DebugSS DPLL register memory to kernel space memory */
		reg_map_ptr = ioremap_nocache(DPLL_DEBUG_BASE_ADDR, PAGE_SIZE);

		/* Read PLLCTRL_STATUS */
		reg_value = ioread32((void *)(((char *)reg_map_ptr) + 
						PLLCTRL_STATUS_REG));

		__D("%s:pllctrl_status = 0x%x\n",__FUNCTION__,reg_value);

		/* check if DebugSS DPLL is locked */
		if(0 == (reg_value & 0x2))
		{
			// DebugSS DPLL not locked 
			byte_count = sprintf(buf,"-NA-\n");

			return byte_count;
		}

		reg_value = ioread32((void *)(((char *)reg_map_ptr) + 
						PLLCTRL_CONFIG1_REG));

		__D("%s:pllctrl_config1 = 0x%x\n",__FUNCTION__,reg_value);

		/* calculate DCODCLK using EMU_SYSCLK, M and N values */
		N = ((reg_value & 0x1FE) >> 1); //bits[8:1]
		M = ((reg_value & 0x1FFE00) >> 9); //bits[20:9]
		M5_CLKDIV = ((reg_value & 0x7C000000) >> 26); //bits[30:26]

		__D("%s:EMU_SYSCLK = %lu\n",__FUNCTION__,clk_rate);
		__D("%s:N = %u\n",__FUNCTION__,N);
		__D("%s:M = %u\n",__FUNCTION__,M);
		__D("%s:M5_CLKDIV = %u\n",__FUNCTION__,M5_CLKDIV);

		/* Calculate STM functional clock rate */
		clk_rate = (clk_rate * 2 * (M/(N+1)))/(M5_CLKDIV + 1);

		byte_count = sprintf(buf,"%ld\n",clk_rate);

		/* Un-map DebugSS DPLL register memory from kernel space memory */
		iounmap(reg_map_ptr);

		/* Release mem region */
		release_mem_region(DPLL_DEBUG_BASE_ADDR, PAGE_SIZE);

		return byte_count;
	} else {
		__E("failed to get access to DebugSS DPLL address space: start_addr:0x%x, length:%lu\n",DPLL_DEBUG_BASE_ADDR,PAGE_SIZE);
		byte_count = sprintf(buf,"-NA-\n");
		return byte_count;
	}
}

/* Declare the sysfs entries. The macros create instances of 
dev_attr_device_name and dev_attr_debugssclk */
static DEVICE_ATTR(device_name, S_IRUGO, show_device_name, NULL);
static DEVICE_ATTR(debugssclk, S_IRUGO, get_debugssclk, NULL);

static int dra7xx_debugss_probe(struct platform_device *pdev)
{
	unsigned int die_id_2;
        unsigned int id_code;
	void * reg_map_ptr;
	unsigned int reg_value;
	int retval = 0;
	int timeout;

	__D("%s:\n",__FUNCTION__);

	debugss_device = &pdev->dev;

	/* Prepare sysclkin clock */
	sys_clk_in1 = devm_clk_get(debugss_device, "sysclockin1");
	if (WARN_ON(IS_ERR(sys_clk_in1)))
		return PTR_ERR(sys_clk_in1);

	clk_prepare_enable(sys_clk_in1);

	/* Create debugss misc device */
	retval = misc_register(&debugss_miscdev);
	if (retval < 0) {
		dev_err(debugss_device, "cannot register debugss misc device\n");
	}

	debugss_device = debugss_miscdev.this_device;

	/* Map first 4K of EMU CM register memory to kernel space memory */
	reg_map_ptr = ioremap_nocache(EMU_CM_BASE_ADDR, PAGE_SIZE);

	reg_value = ioread32((void *)(((char *)reg_map_ptr) + 
					CM_EMU_CLKSTCTRL));

	if((reg_value & 0x100) != 0x100)
	{
		iowrite32(((reg_value & 0xFFFFFFFC) | 0x2),(void *)(((char *)reg_map_ptr) + 
					CM_EMU_CLKSTCTRL));

		for(reg_value=0;reg_value<0xFFFF;reg_value++);

		timeout = 100000;

		while(timeout > 0)
		{
			/* Read CM_EMU_CLKSTCTRL */
			reg_value = ioread32((void *)(((char *)reg_map_ptr) + 
							CM_EMU_CLKSTCTRL));
			if((reg_value & 0x100) == 0x100)
			{
				__D("%s:EMU DebugSS power up done\n",__FUNCTION__);
				break;
			} else {
				timeout--;
			}
		}

		if(timeout <= 0)
		{
			__E("EMU DebugSS power up timeout\n");
		}

	} else {
		__D("%s:EMU DebugSS already powered up\n",__FUNCTION__);
	}

	iounmap(reg_map_ptr);

	//Enable L3 instrumentation clocks, if not already enabled
	if(NULL != request_mem_region(CM_L3INSTR_L3_3_BASE_ADDR, PAGE_SIZE, DRIVER_NAME))
	{
		/* Map first 4K of CM L3INSTR register memory to kernel space memory */
		reg_map_ptr = ioremap_nocache(CM_L3INSTR_L3_3_BASE_ADDR, PAGE_SIZE);

		/* Read CM_L3INSTR_L3_3_CLKCTRL */
		reg_value = ioread32((void *)(((char *)reg_map_ptr) + 
						CM_L3INSTR_L3_3_CLKCTRL));

		if(0x1 != reg_value)
		{
			iowrite32(0x1, (void *)(((char *)reg_map_ptr) + 
			CM_L3INSTR_L3_3_CLKCTRL));
		}

		/* Read CM_L3INSTR_L3_INSTR_CLKCTRL */
		reg_value = ioread32((void *)(((char *)reg_map_ptr) + 
						CM_L3INSTR_L3_INSTR_CLKCTRL));

		if(0x1 != reg_value)
		{
			iowrite32(0x1, (void *)(((char *)reg_map_ptr) + 
			CM_L3INSTR_L3_INSTR_CLKCTRL));
		}

		/* Un-map CM L3INSTR register memory from kernel space memory */
		iounmap(reg_map_ptr);

		/* Release mem region */
		release_mem_region(CM_L3INSTR_L3_3_BASE_ADDR, PAGE_SIZE);

		__D("%s:L3 instrumentation clocks are enabled\n",__FUNCTION__);

	} else {
		__E("failed to get access to CM L3INSTR address space: start_addr:0x%x, length:%lu\n",CM_L3INSTR_L3_3_BASE_ADDR,PAGE_SIZE);
	}

	//Check if DebugSS PWR/CLK is configured, if not configure the same
	if(NULL != request_mem_region(DPLL_DEBUG_BASE_ADDR, PAGE_SIZE, DRIVER_NAME))
	{
		/* Map 4K of DebugSS DPLL register memory to kernel space memory */
		reg_map_ptr = ioremap_nocache(DPLL_DEBUG_BASE_ADDR, PAGE_SIZE);

		/* Read PLLCTRL_STATUS */
		reg_value = ioread32((void *)(((char *)reg_map_ptr) + 
						PLLCTRL_STATUS_REG));

		/* check if DebugSS DPLL is locked */
		if(0 == (reg_value & 0x2))
		{
	 
			__D("%s:DebugSS DPLL not locked\n",__FUNCTION__);
			__D("%s:Configuring DebugSS DPLL\n",__FUNCTION__);
			__D("%s:Setup DebugSS DPLL output to 1.9GHz in progress...\n",__FUNCTION__);

			// PLL CONFIGURATION1
			// M5 [30:26] = 0x9; M4 [25:21] = 0x4;
			// M  [20:9] = 0x1DB (475); N  [8:1] = 0x9
			iowrite32(0x2483B612, (void *)(((char *)reg_map_ptr) + 
						PLLCTRL_CONFIG1_REG));
			// PLL CONFIGURATION2
			iowrite32(0xE52008, (void *)(((char *)reg_map_ptr) + 
						PLLCTRL_CONFIG2_REG));

			// Write 0x1 to PLLCTRL_GO
			iowrite32(0x1, (void *)(((char *)reg_map_ptr) + 
						PLLCTRL_GO_REG));
			//check if PLL locked or timeout after waiting for a predefined period
                        timeout = 100000;
			while(timeout > 0)
			{
				/* Read PLLCTRL_STATUS */
				reg_value = ioread32((void *)(((char *)reg_map_ptr) + 
						PLLCTRL_STATUS_REG));
				if(0x2 == (reg_value & 0x2))
				{
					__D("%s:DebugSS DPLL locked\n",__FUNCTION__);
					__D("%s:Done with Setup DebugSS PLL Clocking 1.9GHz\n",__FUNCTION__);
					__D("%s:Done with Setup DebugSS ATB Clocking 380MHz\n",__FUNCTION__);
					break;
				} else {
					timeout--;
				}
			}

			if(timeout <= 0)
			{
				__E("DebugSS DPLL lock time out\n");
			}	
	
		} else {
			__D("%s:DEBUGSS DPLL is already locked\n",__FUNCTION__);
		}

		/* Un-map DebugSS DPLL register memory from kernel space memory */
		iounmap(reg_map_ptr);

		/* Release mem region */
		release_mem_region(DPLL_DEBUG_BASE_ADDR, PAGE_SIZE);

	} else {
		__E("failed to get access to DebugSS DPLL address space: start_addr:0x%x, length:%lu\n",DPLL_DEBUG_BASE_ADDR,PAGE_SIZE);;
	}

	/* Identify target device type by reading the DIE ID 2 */
	/* Map first 4K of Die indentification register memory to kernel space memory */
	reg_map_ptr = ioremap_nocache(DIE_ID_BASE_ADDR, PAGE_SIZE);

	/* Read DIE_ID_2 */
	die_id_2 = ioread32((void *)(((char *)reg_map_ptr) + 
					DIE_ID_2_REG));

	/* Read ID_CODE */
	id_code = ioread32((void *)(((char *)reg_map_ptr) + 
					ID_CODE_REG));

	/* identify device type */
	target_device_type = identify_dra7xx_device(die_id_2, id_code);

	/* Un-map DIEID register memory from kernel space memory */
	iounmap(reg_map_ptr);

	/* Now we can create the sysfs endpoints */
	retval = device_create_file(debugss_device, &dev_attr_device_name);
	if (retval < 0) {
		__E("failed to create debugss device_name /sys endpoint\n");
	}
	retval = device_create_file(debugss_device, &dev_attr_debugssclk);
	if (retval < 0) {
		__E("failed to create debugss debugssclk /sys endpoint\n");
	}

	return retval;
}

static int dra7xx_debugss_remove(struct platform_device *pdev)
{
	__D("%s:\n",__FUNCTION__);

	device_remove_file(debugss_device, &dev_attr_device_name);
	device_remove_file(debugss_device, &dev_attr_debugssclk);

	/* De-register debugss misc device */
	misc_deregister(&debugss_miscdev);

	/* Un-prepare debugSS clock */
	clk_disable_unprepare(sys_clk_in1);

	return 0;
}

static const struct of_device_id dra7xx_debugss_of_match[] = {
	{ .compatible = "ti,dra7xx-debugss", },
	{},
};
MODULE_DEVICE_TABLE(of, dra7xx_debugss_of_match);

static struct platform_driver platform_debugss_driver = {
	.driver = {
		.name = "debugss",
		.owner	= THIS_MODULE,
		.of_match_table = dra7xx_debugss_of_match,
	},
	.probe = dra7xx_debugss_probe,
	.remove = dra7xx_debugss_remove,
};

#endif

module_platform_driver(platform_debugss_driver);

/* Module information */
MODULE_AUTHOR("Texas Instruments Incorporated");

#ifdef KEYSTONE_PLATFORM
MODULE_DESCRIPTION("Debug Sub-System (DebugSS) module for Keystone devices");
#endif

#ifdef DRA7xx_PLATFORM
MODULE_DESCRIPTION("Debug Sub-System (DebugSS) module for DRA7xx devices");
#endif

MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRIVER_NAME);
