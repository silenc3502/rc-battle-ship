/*
 * GPIO driver for AMD 8111 south bridges
 *
 * Copyright (c) 2012 Dmitry Eremin-Solenikov
 *
 * Based on the AMD RNG driver:
 * Copyright 2005 (c) MontaVista Software, Inc.
 * with the majority of the code coming from:
 *
 * Hardware driver for the Intel/AMD/VIA Random Number Generators (RNG)
 * (c) Copyright 2003 Red Hat Inc <jgarzik@redhat.com>
 *
 * derived from
 *
 * Hardware driver for the AMD 768 Random Number Generator (RNG)
 * (c) Copyright 2001 Red Hat Inc
 *
 * derived from
 *
 * Hardware driver for Intel i810 Random Number Generator (RNG)
 * Copyright 2000,2001 Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2000,2001 Philipp Rumpf <prumpf@mandrakesoft.com>
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
  * AMD 8111 사우스 브리지 용 GPIO 드라이버
  *
  * Copyright (c) 2012 Dmitry Eremin-Solenikov
  *
  * AMD RNG 드라이버 기반 :
  * Copyright 2005 (c) 대부분의 코드는 MontaVista Software, Inc.에서 제공합니다.
  *
  * Intel / AMD / VIA 난수 생성기 (RNG) 용 하드웨어 드라이버
  * (c) AMD 768 난수 생성기 (RNG) 용 하드웨어 드라이버에서 파생 된 Copyright 2003 Red Hat Inc <jgarzik@redhat.com>
  * (c) Intel i810 용 하드웨어 드라이버에서 파생 된 Copyright 2001 Red Hat Inc RNG (Random Number Generator)
  * Copyright 2000, 2001 Jeff Garzik <jgarzik@pobox.com>
  * Copyright 2000,2001 Philipp Rumpf <prumpf@mandrakesoft.com>
  *
  * 이 파일의 사용 허가는 GNU General Public License 버전 2에 의거합니다.이 프로그램은 명시 적이든 묵시적이든 어떠한 종류의 보증도없이 "있는 그대로"사용 허가되었습니다.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/pci.h>
#include <linux/spinlock.h>

#define PMBASE_OFFSET 0xb0
#define PMBASE_SIZE   0x30

#define AMD_REG_GPIO(i) (0x10 + (i))

#define AMD_GPIO_LTCH_STS	0x40 /* Latch status, w1 */
#define AMD_GPIO_RTIN		0x20 /* Real Time in, ro */
#define AMD_GPIO_DEBOUNCE	0x10 /* Debounce, rw */
#define AMD_GPIO_MODE_MASK	0x0c /* Pin Mode Select, rw */
#define AMD_GPIO_MODE_IN	0x00
#define AMD_GPIO_MODE_OUT	0x04
/* Enable alternative (e.g. clkout, IRQ, etc) function of the pin */
#define AMD_GPIO_MODE_ALTFN	0x08 /* Or 0x09 */
#define AMD_GPIO_X_MASK		0x03 /* In/Out specific, rw */
#define AMD_GPIO_X_IN_ACTIVEHI	0x01 /* Active High */
#define AMD_GPIO_X_IN_LATCH	0x02 /* Latched version is selected */
#define AMD_GPIO_X_OUT_LOW	0x00
#define AMD_GPIO_X_OUT_HI	0x01
#define AMD_GPIO_X_OUT_CLK0	0x02
#define AMD_GPIO_X_OUT_CLK1	0x03

/*
 * Data for PCI driver interface
 *
 * This data only exists for exporting the supported
 * PCI ids via MODULE_DEVICE_TABLE.  We do not actually
 * register a pci_driver, because someone else might one day
 * want to register another driver on the same PCI id.
   PCI 드라이버 인터페이스 용 데이터
  
   이 데이터는 MODULE_DEVICE_TABLE을 통해 지원되는 PCI ID를 내보낼 때만 존재합니다. 다른 사용자가 같은 PCI ID에 다른 드라이버를 등록하려고하기 때문에 실제로는 pci_driver를 등록하지 않습니다.
 */
static const struct pci_device_id pci_tbl[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_8111_SMBUS), 0 },
	{ 0, },	/* terminate list */
};
MODULE_DEVICE_TABLE(pci, pci_tbl);

struct amd_gpio {
	struct gpio_chip	chip;
	u32			pmbase;
	void __iomem		*pm;
	struct pci_dev		*pdev;
	spinlock_t		lock; /* guards hw registers and orig table
					 경호원 hw 레지스터 및 orig 테이블 */
	u8			orig[32];
};

#define to_agp(chip)	container_of(chip, struct amd_gpio, chip)

static int amd_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	struct amd_gpio *agp = to_agp(chip);

	agp->orig[offset] = ioread8(agp->pm + AMD_REG_GPIO(offset)) &
		(AMD_GPIO_DEBOUNCE | AMD_GPIO_MODE_MASK | AMD_GPIO_X_MASK);

	dev_dbg(&agp->pdev->dev, "Requested gpio %d, data %x\n", offset, agp->orig[offset]);

	return 0;
}

static void amd_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	struct amd_gpio *agp = to_agp(chip);

	dev_dbg(&agp->pdev->dev, "Freed gpio %d, data %x\n", offset, agp->orig[offset]);

	iowrite8(agp->orig[offset], agp->pm + AMD_REG_GPIO(offset));
}

static void amd_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct amd_gpio *agp = to_agp(chip);
	u8 temp;
	unsigned long flags;

	spin_lock_irqsave(&agp->lock, flags);
	temp = ioread8(agp->pm + AMD_REG_GPIO(offset));
	temp = (temp & AMD_GPIO_DEBOUNCE) | AMD_GPIO_MODE_OUT | (value ? AMD_GPIO_X_OUT_HI : AMD_GPIO_X_OUT_LOW);
	iowrite8(temp, agp->pm + AMD_REG_GPIO(offset));
	spin_unlock_irqrestore(&agp->lock, flags);

	dev_dbg(&agp->pdev->dev, "Setting gpio %d, value %d, reg=%02x\n", offset, !!value, temp);
}

static int amd_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct amd_gpio *agp = to_agp(chip);
	u8 temp;

	temp = ioread8(agp->pm + AMD_REG_GPIO(offset));

	dev_dbg(&agp->pdev->dev, "Getting gpio %d, reg=%02x\n", offset, temp);

	return (temp & AMD_GPIO_RTIN) ? 1 : 0;
}

static int amd_gpio_dirout(struct gpio_chip *chip, unsigned offset, int value)
{
	struct amd_gpio *agp = to_agp(chip);
	u8 temp;
	unsigned long flags;

	spin_lock_irqsave(&agp->lock, flags);
	temp = ioread8(agp->pm + AMD_REG_GPIO(offset));
	temp = (temp & AMD_GPIO_DEBOUNCE) | AMD_GPIO_MODE_OUT | (value ? AMD_GPIO_X_OUT_HI : AMD_GPIO_X_OUT_LOW);
	iowrite8(temp, agp->pm + AMD_REG_GPIO(offset));
	spin_unlock_irqrestore(&agp->lock, flags);

	dev_dbg(&agp->pdev->dev, "Dirout gpio %d, value %d, reg=%02x\n", offset, !!value, temp);

	return 0;
}

static int amd_gpio_dirin(struct gpio_chip *chip, unsigned offset)
{
	struct amd_gpio *agp = to_agp(chip);
	u8 temp;
	unsigned long flags;

	spin_lock_irqsave(&agp->lock, flags);
	temp = ioread8(agp->pm + AMD_REG_GPIO(offset));
	temp = (temp & AMD_GPIO_DEBOUNCE) | AMD_GPIO_MODE_IN;
	iowrite8(temp, agp->pm + AMD_REG_GPIO(offset));
	spin_unlock_irqrestore(&agp->lock, flags);

	dev_dbg(&agp->pdev->dev, "Dirin gpio %d, reg=%02x\n", offset, temp);

	return 0;
}

static struct amd_gpio gp = {
	.chip = {
		.label		= "AMD GPIO",
		.owner		= THIS_MODULE,
		.base		= -1,
		.ngpio		= 32,
		.request	= amd_gpio_request,
		.free		= amd_gpio_free,
		.set		= amd_gpio_set,
		.get		= amd_gpio_get,
		.direction_output = amd_gpio_dirout,
		.direction_input = amd_gpio_dirin,
	},
};

static int __init amd_gpio_init(void)
{
	int err = -ENODEV;
	struct pci_dev *pdev = NULL;
	const struct pci_device_id *ent;


	/* We look for our device - AMD South Bridge
	 * I don't know about a system with two such bridges,
	 * so we can assume that there is max. one device.
	 *
	 * We can't use plain pci_driver mechanism,
	 * as the device is really a multiple function device,
	 * main driver that binds to the pci_device is an smbus
	 * driver and have to find & bind to the device this way.
	   우리는 우리 장치를 찾습니다. - AMD South Bridge 나는 두 개의 그러한 다리가있는 시스템에 대해 모릅니다. 
	   따라서 우리는 최대 값이 있다고 가정 할 수 있습니다. 하나의 장치.

	   장치가 실제로는 여러 기능 장치이므로 pci_driver 메커니즘을 사용할 수 없습니다. 
	   pci_device에 바인딩하는 기본 드라이버는 smbus 드라이버이며이 방법으로 장치에 바인드해야합니다.
	 */
	for_each_pci_dev(pdev) {
		ent = pci_match_id(pci_tbl, pdev);
		if (ent)
			goto found;
	}
	/* Device not found. */
	goto out;

found:
	err = pci_read_config_dword(pdev, 0x58, &gp.pmbase);
	if (err)
		goto out;
	err = -EIO;
	gp.pmbase &= 0x0000FF00;
	if (gp.pmbase == 0)
		goto out;
	if (!request_region(gp.pmbase + PMBASE_OFFSET, PMBASE_SIZE, "AMD GPIO")) {
		dev_err(&pdev->dev, "AMD GPIO region 0x%x already in use!\n",
			gp.pmbase + PMBASE_OFFSET);
		err = -EBUSY;
		goto out;
	}
	gp.pm = ioport_map(gp.pmbase + PMBASE_OFFSET, PMBASE_SIZE);
	if (!gp.pm) {
		dev_err(&pdev->dev, "Couldn't map io port into io memory\n");
		release_region(gp.pmbase + PMBASE_OFFSET, PMBASE_SIZE);
		err = -ENOMEM;
		goto out;
	}
	gp.pdev = pdev;
	gp.chip.dev = &pdev->dev;

	spin_lock_init(&gp.lock);

	printk(KERN_INFO "AMD-8111 GPIO detected\n");
	err = gpiochip_add(&gp.chip);
	if (err) {
		printk(KERN_ERR "GPIO registering failed (%d)\n",
		       err);
		ioport_unmap(gp.pm);
		release_region(gp.pmbase + PMBASE_OFFSET, PMBASE_SIZE);
		goto out;
	}
out:
	return err;
}

static void __exit amd_gpio_exit(void)
{
	gpiochip_remove(&gp.chip);
	ioport_unmap(gp.pm);
	release_region(gp.pmbase + PMBASE_OFFSET, PMBASE_SIZE);
}

module_init(amd_gpio_init);
module_exit(amd_gpio_exit);

MODULE_AUTHOR("The Linux Kernel team");
MODULE_DESCRIPTION("GPIO driver for AMD chipsets");
MODULE_LICENSE("GPL");
