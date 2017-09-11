/*
 * AMD CS5535/CS5536 GPIO driver
 * Copyright (C) 2006  Advanced Micro Devices, Inc.
 * Copyright (C) 2007-2009  Andres Salomon <dilinger@collabora.co.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/cs5535.h>
#include <asm/msr.h>

#define DRV_NAME "cs5535-gpio"

/*
 * Some GPIO pins
 *  31-29,23 : reserved (always mask out)
 *  28       : Power Button
 *  26       : PME#
 *  22-16    : LPC
 *  14,15    : SMBus
 *  9,8      : UART1
 *  7        : PCI INTB
 *  3,4      : UART2/DDC
 *  2        : IDE_IRQ0
 *  1        : AC_BEEP
 *  0        : PCI INTA
 *
 * If a mask was not specified, allow all except
 * reserved and Power Button
   마스크가 지정되지 않은 경우 예약 및 전원 버튼을 제외한 모두 허용
 */
#define GPIO_DEFAULT_MASK 0x0F7FFFFF

static ulong mask = GPIO_DEFAULT_MASK;
module_param_named(mask, mask, ulong, 0444);
MODULE_PARM_DESC(mask, "GPIO channel mask.");

static struct cs5535_gpio_chip {
	struct gpio_chip chip;
	resource_size_t base;

	struct platform_device *pdev;
	spinlock_t lock;
} cs5535_gpio_chip;

/*
 * The CS5535/CS5536 GPIOs support a number of extra features not defined
 * by the gpio_chip API, so these are exported.  For a full list of the
 * registers, see include/linux/cs5535.h.
CS5535 / CS5536 GPIO는 gpio_chip API에 정의되지 않은 추가 기능을 지원하므로 익스포트됩니다. 레지스터의 전체 목록을 보려면 include / linux / cs5535.h를 참조하십시오.
 */

static void errata_outl(struct cs5535_gpio_chip *chip, u32 val,
		unsigned int reg)
{
	unsigned long addr = chip->base + 0x80 + reg;

	/*
	 * According to the CS5536 errata (#36), after suspend
	 * a write to the high bank GPIO register will clear all
	 * non-selected bits; the recommended workaround is a
	 * read-modify-write operation.
	 *
	 * Don't apply this errata to the edge status GPIOs, as writing
	 * to their lower bits will clear them.
	 * CS5536 정오표 (# 36)에 따르면, 일시 중지 후 상위 뱅크 GPIO 레지스터에 대한 쓰기는 선택되지 않은 모든 비트를 지 웁니다. 
	   권장되는 임시 해결책은 읽기 - 수정 - 쓰기 작업입니다.

	   이 에라타를 에지 상태 GPIO에 적용하지 마십시오. 하위 비트를 쓰면 클리어됩니다.
	 */
	if (reg != GPIO_POSITIVE_EDGE_STS && reg != GPIO_NEGATIVE_EDGE_STS) {
		if (val & 0xffff)
			val |= (inl(addr) & 0xffff); /* ignore the high bits */
		else
			val |= (inl(addr) ^ (val >> 16));
	}
	outl(val, addr);
}

static void __cs5535_gpio_set(struct cs5535_gpio_chip *chip, unsigned offset,
		unsigned int reg)
{
	if (offset < 16)
		/* low bank register */
		outl(1 << offset, chip->base + reg);
	else
		/* high bank register */
		errata_outl(chip, 1 << (offset - 16), reg);
}

void cs5535_gpio_set(unsigned offset, unsigned int reg)
{
	struct cs5535_gpio_chip *chip = &cs5535_gpio_chip;
	unsigned long flags;

	spin_lock_irqsave(&chip->lock, flags);
	__cs5535_gpio_set(chip, offset, reg);
	spin_unlock_irqrestore(&chip->lock, flags);
}
EXPORT_SYMBOL_GPL(cs5535_gpio_set);

static void __cs5535_gpio_clear(struct cs5535_gpio_chip *chip, unsigned offset,
		unsigned int reg)
{
	if (offset < 16)
		/* low bank register */
		outl(1 << (offset + 16), chip->base + reg);
	else
		/* high bank register */
		errata_outl(chip, 1 << offset, reg);
}

void cs5535_gpio_clear(unsigned offset, unsigned int reg)
{
	struct cs5535_gpio_chip *chip = &cs5535_gpio_chip;
	unsigned long flags;

	spin_lock_irqsave(&chip->lock, flags);
	__cs5535_gpio_clear(chip, offset, reg);
	spin_unlock_irqrestore(&chip->lock, flags);
}
EXPORT_SYMBOL_GPL(cs5535_gpio_clear);

int cs5535_gpio_isset(unsigned offset, unsigned int reg)
{
	struct cs5535_gpio_chip *chip = &cs5535_gpio_chip;
	unsigned long flags;
	long val;

	spin_lock_irqsave(&chip->lock, flags);
	if (offset < 16)
		/* low bank register */
		val = inl(chip->base + reg);
	else {
		/* high bank register */
		val = inl(chip->base + 0x80 + reg);
		offset -= 16;
	}
	spin_unlock_irqrestore(&chip->lock, flags);

	return (val & (1 << offset)) ? 1 : 0;
}
EXPORT_SYMBOL_GPL(cs5535_gpio_isset);

int cs5535_gpio_set_irq(unsigned group, unsigned irq)
{
	uint32_t lo, hi;

	if (group > 7 || irq > 15)
		return -EINVAL;

	rdmsr(MSR_PIC_ZSEL_HIGH, lo, hi);

	lo &= ~(0xF << (group * 4));
	lo |= (irq & 0xF) << (group * 4);

	wrmsr(MSR_PIC_ZSEL_HIGH, lo, hi);
	return 0;
}
EXPORT_SYMBOL_GPL(cs5535_gpio_set_irq);

void cs5535_gpio_setup_event(unsigned offset, int pair, int pme)
{
	struct cs5535_gpio_chip *chip = &cs5535_gpio_chip;
	uint32_t shift = (offset % 8) * 4;
	unsigned long flags;
	uint32_t val;

	if (offset >= 24)
		offset = GPIO_MAP_W;
	else if (offset >= 16)
		offset = GPIO_MAP_Z;
	else if (offset >= 8)
		offset = GPIO_MAP_Y;
	else
		offset = GPIO_MAP_X;

	spin_lock_irqsave(&chip->lock, flags);
	val = inl(chip->base + offset);

	/* Clear whatever was there before 
	    이전에 있던 것이 무엇이든 지우기 */
	 
	val &= ~(0xF << shift);

	/* Set the new value 
	    새 값 설정 		*/
	val |= ((pair & 7) << shift);

	/* Set the PME bit if this is a PME event 
	    이것이 PME 이벤트 인 경우 PME 비트를 설정	*/
	if (pme)
		val |= (1 << (shift + 3));

	outl(val, chip->base + offset);
	spin_unlock_irqrestore(&chip->lock, flags);
}
EXPORT_SYMBOL_GPL(cs5535_gpio_setup_event);

/*
 * Generic gpio_chip API support. 일반적인 gpio_chip API 지원.
 */

static int chip_gpio_request(struct gpio_chip *c, unsigned offset)
{
	struct cs5535_gpio_chip *chip =
		container_of(c, struct cs5535_gpio_chip, chip);
	unsigned long flags;

	spin_lock_irqsave(&chip->lock, flags);

	/* check if this pin is available
	   이 핀이 사용 가능한지 확인한다. 	 */
	if ((mask & (1 << offset)) == 0) {
		dev_info(&chip->pdev->dev,
			"pin %u is not available (check mask)\n", offset);
		spin_unlock_irqrestore(&chip->lock, flags);
		return -EINVAL;
	}

	/* disable output aux 1 & 2 on this pin */
	__cs5535_gpio_clear(chip, offset, GPIO_OUTPUT_AUX1);
	__cs5535_gpio_clear(chip, offset, GPIO_OUTPUT_AUX2);

	/* disable input aux 1 on this pin */
	__cs5535_gpio_clear(chip, offset, GPIO_INPUT_AUX1);

	spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

static int chip_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	return cs5535_gpio_isset(offset, GPIO_READ_BACK);
}

static void chip_gpio_set(struct gpio_chip *chip, unsigned offset, int val)
{
	if (val)
		cs5535_gpio_set(offset, GPIO_OUTPUT_VAL);
	else
		cs5535_gpio_clear(offset, GPIO_OUTPUT_VAL);
}

static int chip_direction_input(struct gpio_chip *c, unsigned offset)
{
	struct cs5535_gpio_chip *chip =
		container_of(c, struct cs5535_gpio_chip, chip);
	unsigned long flags;

	spin_lock_irqsave(&chip->lock, flags);
	__cs5535_gpio_set(chip, offset, GPIO_INPUT_ENABLE);
	__cs5535_gpio_clear(chip, offset, GPIO_OUTPUT_ENABLE);
	spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

static int chip_direction_output(struct gpio_chip *c, unsigned offset, int val)
{
	struct cs5535_gpio_chip *chip =
		container_of(c, struct cs5535_gpio_chip, chip);
	unsigned long flags;

	spin_lock_irqsave(&chip->lock, flags);

	__cs5535_gpio_set(chip, offset, GPIO_INPUT_ENABLE);
	__cs5535_gpio_set(chip, offset, GPIO_OUTPUT_ENABLE);
	if (val)
		__cs5535_gpio_set(chip, offset, GPIO_OUTPUT_VAL);
	else
		__cs5535_gpio_clear(chip, offset, GPIO_OUTPUT_VAL);

	spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

static const char * const cs5535_gpio_names[] = {
	"GPIO0", "GPIO1", "GPIO2", "GPIO3",
	"GPIO4", "GPIO5", "GPIO6", "GPIO7",
	"GPIO8", "GPIO9", "GPIO10", "GPIO11",
	"GPIO12", "GPIO13", "GPIO14", "GPIO15",
	"GPIO16", "GPIO17", "GPIO18", "GPIO19",
	"GPIO20", "GPIO21", "GPIO22", NULL,
	"GPIO24", "GPIO25", "GPIO26", "GPIO27",
	"GPIO28", NULL, NULL, NULL,
};

static struct cs5535_gpio_chip cs5535_gpio_chip = {
	.chip = {
		.owner = THIS_MODULE,
		.label = DRV_NAME,

		.base = 0,
		.ngpio = 32,
		.names = cs5535_gpio_names,
		.request = chip_gpio_request,

		.get = chip_gpio_get,
		.set = chip_gpio_set,

		.direction_input = chip_direction_input,
		.direction_output = chip_direction_output,
	},
};

static int cs5535_gpio_probe(struct platform_device *pdev)
{
	struct resource *res;
	int err = -EIO;
	ulong mask_orig = mask;

	/* There are two ways to get the GPIO base address; one is by
	 * fetching it from MSR_LBAR_GPIO, the other is by reading the
	 * PCI BAR info.  The latter method is easier (especially across
	 * different architectures), so we'll stick with that for now.  If
	 * it turns out to be unreliable in the face of crappy BIOSes, we
	 * can always go back to using MSRs..
	 GPIO 기본 주소를 얻는 방법에는 두 가지가 있습니다.
	 하나는 MSR_LBAR_GPIO에서 가져 오는 것이고, 다른 하나는 PCI BAR 정보를 읽는 것입니다. 
	 후자의 방법은 (특히 다른 아키텍처에서) 더 쉽기 때문에 지금 당장이를 적용 할 것입니다. 
	 진절머리 나는 BIOS에도 불구하고 신뢰할 수 없다는 것이 밝혀지면 항상 MSR을 사용합니다. */

	res = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (!res) {
		dev_err(&pdev->dev, "can't fetch device resource info\n");
		goto done;
	}

	if (!devm_request_region(&pdev->dev, res->start, resource_size(res),
				 pdev->name)) {
		dev_err(&pdev->dev, "can't request region\n");
		goto done;
	}

	/* set up the driver-specific struct 
	    드라이버 관련 구조체를 설정	 	*/
	cs5535_gpio_chip.base = res->start;
	cs5535_gpio_chip.pdev = pdev;
	spin_lock_init(&cs5535_gpio_chip.lock);

	dev_info(&pdev->dev, "reserved resource region %pR\n", res);

	/* mask out reserved pins
	    예약 된 핀을 마스크 아웃한다.  */
	mask &= 0x1F7FFFFF;

	/* do not allow pin 28, Power Button, as there's special handling
	 * in the PMC needed. (note 12, p. 48) 
	   특수 처리가 있으므로 핀 28, 전원 버튼을 허용하지 않습니다.
	   PMC에 필요합니다. (주 12, 48면) */
	mask &= ~(1 << 28);

	if (mask_orig != mask)
		dev_info(&pdev->dev, "mask changed from 0x%08lX to 0x%08lX\n",
				mask_orig, mask);

	/* finally, register with the generic GPIO API
	    마지막으로 일반 GPIO API로 등록  */
	err = gpiochip_add(&cs5535_gpio_chip.chip);
	if (err)
		goto done;

	return 0;

done:
	return err;
}

static int cs5535_gpio_remove(struct platform_device *pdev)
{
	gpiochip_remove(&cs5535_gpio_chip.chip);

	return 0;
}

static struct platform_driver cs5535_gpio_driver = {
	.driver = {
		.name = DRV_NAME,
	},
	.probe = cs5535_gpio_probe,
	.remove = cs5535_gpio_remove,
};

module_platform_driver(cs5535_gpio_driver);

MODULE_AUTHOR("Andres Salomon <dilinger@queued.net>");
MODULE_DESCRIPTION("AMD CS5535/CS5536 GPIO driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);
