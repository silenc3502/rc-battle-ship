/*
 * User-space  module driver
 *
 * Copyright (C) 2013-2015 Texas Instruments Incorporated - http://www.ti.com/
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
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/virtio.h>
#include <linux/uio_driver.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/miscdevice.h>
#include <linux/pm_runtime.h>
#include "uio_module_drv.h"

#define DRIVER_NAME		"uio_module_drv"
#define DRIVER_VERSION		"2.0"

/**
 * struct uio_module_drv_info - local information for uio module driver
 * @lock: Lock to protect shared resources with interrupt handler
 * @dev: device pointer
 * @misc: Misc device pointer
 * @clk: Clock pointer
 * @flags: flags to keep track of interrupt occurrence
 * @lock: Lock to protect shared resources with interrupt handler
 * @interrupt_mode: No disable on interrupt(0) or One shot mode(1)
 * @workqueue: Workqueue for interrupt triggered tasks
 * @mem: uio memory regions pointer
 * @num_maps: Number of uio memory regions
 */
struct uio_module_drv_info {
	struct uio_info		uio;
	struct device		*dev;
	struct miscdevice	misc;
	struct clk		*clk;
	unsigned long		flags;
	spinlock_t		lock;
	int			interrupt_mode;
	struct work_struct	workqueue;
	struct kobject		*map_dir;
	struct uio_mem *mem;
	int num_maps;
};

/*
 * attributes
 */
struct uio_map {
	struct kobject kobj;
	struct uio_mem *mem;
};
/**
 * uio_module_drv_open - uio module driver open routine
 */
static int uio_module_drv_open(struct uio_info *uio, struct inode *inode)
{
	struct uio_module_drv_info *uio_module_drv = uio->priv;

	if (uio_module_drv->clk)
		return clk_prepare_enable(uio_module_drv->clk);
	else
		return 0;
}

/**
 * uio_module_drv_release - uio module driver release routine
 */
static int uio_module_drv_release(struct uio_info *uio, struct inode *inode)
{
	struct uio_module_drv_info *uio_module_drv = uio->priv;

	if (uio_module_drv->clk)
		clk_disable_unprepare(uio_module_drv->clk);
	return 0;
}

/**
 * uio_module_drv_handler - uio module driver interrupt handler
 */
static irqreturn_t uio_module_drv_handler(int irq, struct uio_info *uio)
{
	struct uio_module_drv_info *uio_module_drv = uio->priv;

	if (uio_module_drv->interrupt_mode == 1) {
		spin_lock(&uio_module_drv->lock);
		if (!__test_and_set_bit(0, &uio_module_drv->flags))
			disable_irq_nosync(irq);
		spin_unlock(&uio_module_drv->lock);
	}
	return IRQ_HANDLED;
}

/**
 * uio_module_drv_irqcontrol - uio module driver interrupt control
 */
static int uio_module_drv_irqcontrol(struct uio_info *uio, s32 irq_on)
{
	struct uio_module_drv_info *uio_module_drv = uio->priv;
	unsigned long flags;

	spin_lock_irqsave(&uio_module_drv->lock, flags);
	if (irq_on) {
		if (__test_and_clear_bit(0, &uio_module_drv->flags))
			enable_irq(uio->irq);
	} else {
		if (!__test_and_set_bit(0, &uio_module_drv->flags))
			disable_irq_nosync(uio->irq);
	}
	spin_unlock_irqrestore(&uio_module_drv->lock, flags);

	return 0;
}

/**
 * uio_module_drv_dev_fop_open() : open for the uio module driver
 */
static int uio_module_drv_dev_fop_open(struct inode *inode, struct file *file)
{
	/* Need an empty open so that file->private_data gets populated */
	return 0;
}

/**
 * uio_module_drv_dev_fop_mmap() - provided mmap support for
 * device memory. This checks if user request is in valid range before providing
 * mmap access. The valid range can be configured using device tree or platform
 * data.
 */
static int uio_module_drv_dev_fop_mmap(
	struct file *file,
	struct vm_area_struct *vma
)
{
	size_t size = vma->vm_end - vma->vm_start;
	struct miscdevice *misc = file->private_data;
	struct uio_module_drv_info *uio_module_drv =
		container_of(misc, struct uio_module_drv_info, misc);

	int index;
	size_t req_offset;

	index = vma->vm_pgoff & UIO_MODULE_DRV_MAP_INDEX_MASK;
	/* Check if index is valid */
	if (index >= uio_module_drv->num_maps) {
		dev_err(uio_module_drv->dev,
			"mmap index %d is outside the allowed range\n",
			index);
		return -EINVAL;
	}

	req_offset = (vma->vm_pgoff - index)  << PAGE_SHIFT;

	/* Check if it fits within the page of configured size */
	/* NOTE: This allows exposing the whole page to user space */
	if ((req_offset + size)
	    > ((uio_module_drv->mem[index].size + PAGE_SIZE-1) & (~(PAGE_SIZE-1)))) {
		dev_err(uio_module_drv->dev,
			"mmap index %d: mmap offset(0x%zx) and size (0x%zx) is outside the allowed range\n",
			index, req_offset, size);
		return -EINVAL;
	}

	vma->vm_page_prot = phys_mem_access_prot(
		file, (uio_module_drv->mem[index].addr >> PAGE_SHIFT)
		+ (vma->vm_pgoff - index),
		size, vma->vm_page_prot
	);

	if (remap_pfn_range(vma, vma->vm_start,
			    (uio_module_drv->mem[index].addr >> PAGE_SHIFT)
			     + (vma->vm_pgoff - index),
			    size, vma->vm_page_prot)) {
		return -EAGAIN;
	}
	return 0;
}

/**
 * uio_module_drv_transfer() - provides ability to write to memory areas
 */

static ssize_t  uio_module_drv_transfer(struct file *file, char __user *buf,
				     size_t count, loff_t *ppos, int flag)
{
	struct miscdevice *misc = file->private_data;
	struct uio_module_drv_info *uio_module_drv =
		container_of(misc, struct uio_module_drv_info, misc);
	uint32_t phys_addr;
	void __iomem *virt_addr_p = NULL;
	int index;

	phys_addr = *ppos;

	for (index = 0; index < uio_module_drv->num_maps; index++) {
		if ( phys_addr >= uio_module_drv->mem[index].addr 
		     && phys_addr < (uio_module_drv->mem[index].addr
		     + uio_module_drv->mem[index].size)) {
			break;
		}
	}
	if (index == uio_module_drv->num_maps) {
		dev_err(uio_module_drv->dev,
			"request address 0x%x is not in the allowed ranges\n",
			phys_addr);
		return -EINVAL;
	}

	virt_addr_p = ioremap_nocache(phys_addr, count);
	if (!virt_addr_p) {
		dev_err(uio_module_drv->dev, "Mapping of virtual memory failed\n");
		return(-ENOMEM);
	}
	if (flag == 1) {
		if (copy_from_user((__force void *) virt_addr_p, buf, count)) {
			dev_err(uio_module_drv->dev, "copy_from_user failed\n");
			return -EFAULT;
		}
	} else {
		if (copy_to_user(buf, (__force void *)virt_addr_p, count)) {
			dev_err(uio_module_drv->dev, "copy_to_user failed\n");
			return -EFAULT;
		}
	}
	iounmap(virt_addr_p);
	return count;
}

/**
 * uio_module_drv_dev_fop_write() - provides ability to write to memory areas
 */

static ssize_t  uio_module_drv_dev_fop_write(struct file *file,
					  const char __user *buf, size_t count,
					  loff_t *ppos)
{
	return
		uio_module_drv_transfer(
			file, (char __user *)buf,
			count, ppos, 1
		);
}

/**
 * uio_module_drv_dev_fop_read() - provides ability to read from memory areas
 */

static ssize_t  uio_module_drv_dev_fop_read(struct file *file, char __user *buf,
					 size_t count, loff_t *ppos)
{
	return uio_module_drv_transfer(file, buf, count, ppos, 0);
}

static const struct file_operations uio_module_drv_dev_fops = {
	.owner		= THIS_MODULE,
	.open		= uio_module_drv_dev_fop_open,
	.mmap		= uio_module_drv_dev_fop_mmap,
	.read		= uio_module_drv_dev_fop_read,
	.write		= uio_module_drv_dev_fop_write,
	.llseek		= generic_file_llseek,
};

/**
 * uio_module_drv_populate_segments() - scan the configuration for "mem" and
 * populate in uio_module_drv local structure. This information will be used to
 * process user mmap requests.
 */
static inline int
uio_module_drv_populate_segments(struct device_node *np,
			      struct uio_module_drv_info *uio_module_drv)
{
	int len, i;
	u32 dt_value;
	int num_maps = 0;
	char *map_name;

	if (of_get_property(np, "mem", &len)) {
		/*
		 * check if length even multiple of sizeof(u32), i.e.,
		 *   the dt bindings need to be of the form <addr length>
		 */
		len = len / sizeof(u32);
		if ((len % 2) != 0) {
			dev_err(uio_module_drv->dev, "invalid address map in dt binding\n");
			return -EINVAL;
		}
		num_maps = len / 2;

		uio_module_drv->mem = devm_kzalloc(uio_module_drv->dev,
				sizeof(struct uio_mem) * num_maps,
				GFP_KERNEL);
		if (!uio_module_drv->mem) {
			dev_err(uio_module_drv->dev, "devm_kzalloc mapping failed\n");
			return -ENOMEM;
		}

		/* populate the uio_module_drv structure for policing */
		for (i = 0; i < num_maps; i++) {
			uio_module_drv->mem[i].memtype = UIO_MEM_PHYS;
			if (of_property_read_u32_index(np, "mem", 2 * i,
						       &dt_value)) {
				dev_err(uio_module_drv->dev,
					"Error reading dt bindings: addr\n");
				return -ENODEV;
			}
			uio_module_drv->mem[i].addr = dt_value;
			if (of_property_read_u32_index(np, "mem", (2 * i) + 1,
						       &dt_value)) {
				dev_err(uio_module_drv->dev,
					"Error reading dt bindings: size\n");
				return -ENODEV;
			}
			uio_module_drv->mem[i].size = dt_value;
	 		map_name = devm_kzalloc(uio_module_drv->dev, 16*sizeof(char), GFP_KERNEL);
			if (!of_property_read_string_index(np, "mem-names", i,
						       (const char **)&map_name)) {
				uio_module_drv->mem[i].name = map_name;
			}
		}
		uio_module_drv->num_maps = num_maps;
	}
	return 0;
}

#define to_map(map) container_of(map, struct uio_map, kobj)

static ssize_t map_name_show(struct uio_mem *mem, char *buf)
{
	if (unlikely(!mem->name))
		mem->name = "";

	return sprintf(buf, "%s\n", mem->name);
}

static ssize_t map_addr_show(struct uio_mem *mem, char *buf)
{
	return sprintf(buf, "%pa\n", &mem->addr);
}

static ssize_t map_size_show(struct uio_mem *mem, char *buf)
{
	return sprintf(buf, "%pa\n", &mem->size);
}

static ssize_t map_offset_show(struct uio_mem *mem, char *buf)
{
	return sprintf(buf, "0x%llx\n", (unsigned long long)mem->addr & ~PAGE_MASK);
}

struct map_sysfs_entry {
	struct attribute attr;
	ssize_t (*show)(struct uio_mem *, char *);
	ssize_t (*store)(struct uio_mem *, const char *, size_t);
};

static struct map_sysfs_entry name_attribute =
	__ATTR(name, S_IRUGO, map_name_show, NULL);
static struct map_sysfs_entry addr_attribute =
	__ATTR(addr, S_IRUGO, map_addr_show, NULL);
static struct map_sysfs_entry size_attribute =
	__ATTR(size, S_IRUGO, map_size_show, NULL);
static struct map_sysfs_entry offset_attribute =
	__ATTR(offset, S_IRUGO, map_offset_show, NULL);

static struct attribute *attrs[] = {
	&name_attribute.attr,
	&addr_attribute.attr,
	&size_attribute.attr,
	&offset_attribute.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

static void map_release(struct kobject *kobj)
{
	struct uio_map *map = to_map(kobj);
	kfree(map);
}

static ssize_t map_type_show(struct kobject *kobj, struct attribute *attr,
			     char *buf)
{
	struct uio_map *map = to_map(kobj);
	struct uio_mem *mem = map->mem;
	struct map_sysfs_entry *entry;

	entry = container_of(attr, struct map_sysfs_entry, attr);

	if (!entry->show)
		return -EIO;

	return entry->show(mem, buf);
}

static const struct sysfs_ops map_sysfs_ops = {
	.show = map_type_show,
};

static struct kobj_type map_attr_type = {
	.release	= map_release,
	.sysfs_ops	= &map_sysfs_ops,
	.default_attrs	= attrs,
};

static int misc_dev_add_attributes(struct uio_module_drv_info *uio_module_drv)
{
	int ret;
	int mi;
	int map_found = 0;
	struct uio_mem *mem;
	struct uio_map *map;

	for (mi = 0; mi < uio_module_drv->num_maps; mi++) {
		mem = &uio_module_drv->mem[mi];
		if (mem->size == 0)
			break;
		if (!map_found) {
			map_found = 1;
			uio_module_drv->map_dir = kobject_create_and_add("maps",
							&uio_module_drv->misc.this_device->kobj);

			if (!uio_module_drv->map_dir) {
				ret = -1;
				goto err_map;
			}
		}
		map = kzalloc(sizeof(*map), GFP_KERNEL);
		if (!map)
			goto err_map_kobj;
		kobject_init(&map->kobj, &map_attr_type);
		map->mem = mem;
		mem->map = map;
		ret = kobject_add(&map->kobj, uio_module_drv->map_dir, "map%d", mi);
		if (ret)
			goto err_map_kobj;
		ret = kobject_uevent(&map->kobj, KOBJ_ADD);
		if (ret)
			goto err_map;
	}
	return 0;

err_map:
	mi--;
err_map_kobj:
	for (; mi >= 0; mi--) {
		mem = &uio_module_drv->mem[mi];
		map = mem->map;
		kobject_put(&map->kobj);
	}
	kobject_put(uio_module_drv->map_dir);
	dev_err(uio_module_drv->dev, "error creating sysfs files (%d)\n", ret);
	return ret;

}

static void misc_dev_del_attributes(struct uio_module_drv_info *uio_module_drv)
{
	int mi;
	struct uio_mem *mem;

	for (mi = 0; mi < uio_module_drv->num_maps; mi++) {
		mem = &uio_module_drv->mem[mi];
		if (mem->size == 0)
			break;
		kobject_put(&mem->map->kobj);
	}
	kobject_put(uio_module_drv->map_dir);
}

/**
 * uio_module_drv_driver_probe() probe routine for the uio module driver
 */
static int uio_module_drv_driver_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct uio_module_drv_info *uio_module_drv;
	struct miscdevice *misc;
	struct uio_info *uio;
	struct resource *r;
	int error = 0;
	int i;
	int irq;
	char *name, *tmp_name;

	if (!np) {
		dev_err(dev, "Non dt case not supported\n");
		return -EINVAL;
	}

	uio_module_drv = kzalloc(sizeof(struct  uio_module_drv_info), GFP_KERNEL);

	if (!uio_module_drv) {
		error = -ENOMEM;
		goto fail;
	}

	uio_module_drv->dev = dev;
	spin_lock_init(&uio_module_drv->lock);
	uio_module_drv->flags = 0; /* interrupt is enabled to begin with */

	uio = &uio_module_drv->uio;

	name = strchr(dev_name(dev), '.');
	if (name) {
		name = name +1;
		tmp_name = strchr(name, ':');
		if (tmp_name)
			uio->name	= tmp_name + 1;
		else
			uio->name = name;
	} else {
		name = strchr(dev_name(dev), ':');
		if (name)
			uio->name	= name + 1;
		else
			uio->name = dev_name(dev);
	}

	uio->version	= DRIVER_VERSION;
	uio->priv	= uio_module_drv;
	uio->handler	= uio_module_drv_handler;
	uio->irqcontrol	= uio_module_drv_irqcontrol;
	uio->open	= uio_module_drv_open;
	uio->release	= uio_module_drv_release;

	irq = platform_get_irq(pdev, 0);
	if (!(irq < 0))
		/* pass up control irq to user-space */
		uio->irq	= irq;

	if (
		of_property_read_u32(
			np, "interrupt-mode",
			&uio_module_drv->interrupt_mode
		) < 0
	)
		uio_module_drv->interrupt_mode = 0;

	for (i = 0; i < MAX_UIO_MAPS; ++i) {
		r = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!r)
			break;
		uio->mem[i].memtype = UIO_MEM_PHYS;
		uio->mem[i].addr    = r->start & PAGE_MASK;
		uio->mem[i].size    = PAGE_ALIGN(resource_size(r));
		uio->mem[i].name    = r->name;
	}

	error = uio_module_drv_populate_segments(np, uio_module_drv);
	if (error) {
		dev_err(dev, "failed populating memory segments\n");
		goto fail_uio;
	}

	if (uio->irq || uio->mem[0].memtype != UIO_MEM_NONE) {
		error = uio_register_device(dev, uio);
		if (error) {
			dev_err(dev, "failed to register uio device\n");
			goto fail_uio;
		}
	}
	platform_set_drvdata(pdev, uio_module_drv);

	if (uio_module_drv->num_maps) {
		misc = &uio_module_drv->misc;
		misc->minor	= MISC_DYNAMIC_MINOR;
		misc->name	= uio->name;
		misc->fops	= &uio_module_drv_dev_fops;
		misc->parent	= dev;

		if (misc_register(misc)) {
			dev_err(dev, "could not register misc device\n");
			goto fail_misc_module;
		}
		dev_info(dev, "registered misc device %s\n", misc->name);
		misc_dev_add_attributes(uio_module_drv);
	}

	/* If hw mod is present power up module */
	pm_runtime_enable(dev);

	/* tell PRCM to de-assert IDLE request */
	error = pm_runtime_get_sync(dev);
	if (error < 0) {
		pm_runtime_put_noidle(dev);
		goto fail_pm;
	}

	/* Enable clock for the module */
	uio_module_drv->clk = clk_get(dev, NULL);
	if (IS_ERR(uio_module_drv->clk))
		uio_module_drv->clk = NULL;

	return 0;
fail_pm:
	pm_runtime_disable(dev);
	misc_deregister(&uio_module_drv->misc);
	misc_dev_del_attributes(uio_module_drv);
fail_misc_module:
	uio_unregister_device(uio);
fail_uio:
	devm_kfree(dev, uio_module_drv);
fail:
	return error;
}

/**
 * uio_module_drv_driver_remove() remove routine for the uio module driver
 */

static int uio_module_drv_driver_remove(struct platform_device *pdev)
{
	struct uio_module_drv_info *uio_module_drv = platform_get_drvdata(pdev);
	struct uio_info *uio = &uio_module_drv->uio;

	if (uio_module_drv) {
		if (uio_module_drv->clk)
			clk_put(uio_module_drv->clk);
		/* tell PRCM to initiate IDLE request */
		pm_runtime_put_sync(uio_module_drv->dev);
		pm_runtime_disable(uio_module_drv->dev);
		if (uio_module_drv->num_maps) {
			misc_deregister(&uio_module_drv->misc);
			misc_dev_del_attributes(uio_module_drv);
		}
		if (uio->irq || uio->mem[0].memtype != UIO_MEM_NONE)
			uio_unregister_device(&uio_module_drv->uio);
	}
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static const struct of_device_id uio_module_drv_of_match[] = {
	{ .compatible = "ti,uio-module-drv", },
	{},
};
MODULE_DEVICE_TABLE(of, uio_module_drv_of_match);

static struct platform_driver uio_module_drv_driver = {
	.driver	= {
		.name	= DRIVER_NAME,
		.of_match_table = uio_module_drv_of_match,
	},
	.probe	= uio_module_drv_driver_probe,
	.remove	= uio_module_drv_driver_remove,
};

module_platform_driver(uio_module_drv_driver);
MODULE_AUTHOR("Sam Nelson");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("User-space driver for a generic module ");
MODULE_ALIAS("platform:" DRIVER_NAME);
