/*
 * Intel Medfield MSIC GPIO driver>
 * Copyright (c) 2011, Intel Corporation.
 *
 * Author: Mathias Nyman <mathias.nyman@linux.intel.com>
 * Based on intel_pmic_gpio.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/mfd/intel_msic.h>

/* the offset for the mapping of global gpio pin to irq 
   글로벌 gpio 핀을 irq에 매핑하기위한 오프셋		*/
#define MSIC_GPIO_IRQ_OFFSET	0x100

#define MSIC_GPIO_DIR_IN	0
#define MSIC_GPIO_DIR_OUT	BIT(5)
#define MSIC_GPIO_TRIG_FALL	BIT(1)
#define MSIC_GPIO_TRIG_RISE	BIT(2)

/* masks for msic gpio output GPIOxxxxCTLO registers 
   msic gpio 출력을위한 마스크 GPIOxxxxCTLO 레지스터	*/
#define MSIC_GPIO_DIR_MASK	BIT(5)
#define MSIC_GPIO_DRV_MASK	BIT(4)
#define MSIC_GPIO_REN_MASK	BIT(3)
#define MSIC_GPIO_RVAL_MASK	(BIT(2) | BIT(1))
#define MSIC_GPIO_DOUT_MASK	BIT(0)

/* masks for msic gpio input GPIOxxxxCTLI registers 
   msic gpio 입력을위한 마스크 GPIOxxxxCTLI 레지스터	*/
#define MSIC_GPIO_GLBYP_MASK	BIT(5)
#define MSIC_GPIO_DBNC_MASK	(BIT(4) | BIT(3))
#define MSIC_GPIO_INTCNT_MASK	(BIT(2) | BIT(1))
#define MSIC_GPIO_DIN_MASK	BIT(0)

#define MSIC_NUM_GPIO		24

struct msic_gpio {
	struct platform_device	*pdev;
	struct mutex		buslock;
	struct gpio_chip	chip;
	int			irq;
	unsigned		irq_base;
	unsigned long		trig_change_mask;
	unsigned		trig_type;
};

/*
 * MSIC has 24 gpios, 16 low voltage (1.2-1.8v) and 8 high voltage (3v).
 * Both the high and low voltage gpios are divided in two banks.
 * GPIOs are numbered with GPIO0LV0 as gpio_base in the following order:
 * GPIO0LV0..GPIO0LV7: low voltage, bank 0, gpio_base
 * GPIO1LV0..GPIO1LV7: low voltage, bank 1,  gpio_base + 8
 * GPIO0HV0..GPIO0HV3: high voltage, bank 0, gpio_base + 16
 * GPIO1HV0..GPIO1HV3: high voltage, bank 1, gpio_base + 20
    MSIC는 24 gpios, 16 개의 저전압 (1.2-1.8v) 및 8 개의 고전압 (3v)을 갖추고 있습니다.
  * 높은 전압과 낮은 전압의 gpios는 두 개의 뱅크로 나누어집니다.
  * GPIO는 GPIO0LV0으로 gpio_base로 다음 순서대로 번호가 매겨집니다.
  * GPIO0LV0..GPIO0LV7 : 저전압, 뱅크 0, gpio_base
  * GPIO1LV0..GPIO1LV7 : 저전압, 뱅크 1, gpio_base + 8
  * GPIO0HV0..GPIO0HV3 : 고전압, 뱅크 0, gpio_base + 16
  * GPIO1HV0..GPIO1HV3 : 고전압, 뱅크 1, gpio_base + 20
 */

static int msic_gpio_to_ireg(unsigned offset)
{
	if (offset >= MSIC_NUM_GPIO)
		return -EINVAL;

	if (offset < 8)
		return INTEL_MSIC_GPIO0LV0CTLI - offset;
	if (offset < 16)
		return INTEL_MSIC_GPIO1LV0CTLI - offset + 8;
	if (offset < 20)
		return INTEL_MSIC_GPIO0HV0CTLI - offset + 16;

	return INTEL_MSIC_GPIO1HV0CTLI - offset + 20;
}

static int msic_gpio_to_oreg(unsigned offset)
{
	if (offset >= MSIC_NUM_GPIO)
		return -EINVAL;

	if (offset < 8)
		return INTEL_MSIC_GPIO0LV0CTLO - offset;
	if (offset < 16)
		return INTEL_MSIC_GPIO1LV0CTLO - offset + 8;
	if (offset < 20)
		return INTEL_MSIC_GPIO0HV0CTLO - offset + 16;

	return INTEL_MSIC_GPIO1HV0CTLO - offset + 20;
}

static int msic_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	int reg;

	reg = msic_gpio_to_oreg(offset);
	if (reg < 0)
		return reg;

	return intel_msic_reg_update(reg, MSIC_GPIO_DIR_IN, MSIC_GPIO_DIR_MASK);
}

static int msic_gpio_direction_output(struct gpio_chip *chip,
			unsigned offset, int value)
{
	int reg;
	unsigned mask;

	value = (!!value) | MSIC_GPIO_DIR_OUT;
	mask = MSIC_GPIO_DIR_MASK | MSIC_GPIO_DOUT_MASK;

	reg = msic_gpio_to_oreg(offset);
	if (reg < 0)
		return reg;

	return intel_msic_reg_update(reg, value, mask);
}

static int msic_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	u8 r;
	int ret;
	int reg;

	reg = msic_gpio_to_ireg(offset);
	if (reg < 0)
		return reg;

	ret = intel_msic_reg_read(reg, &r);
	if (ret < 0)
		return ret;

	return r & MSIC_GPIO_DIN_MASK;
}

static void msic_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	int reg;

	reg = msic_gpio_to_oreg(offset);
	if (reg < 0)
		return;

	intel_msic_reg_update(reg, !!value , MSIC_GPIO_DOUT_MASK);
}

/*
 * This is called from genirq with mg->buslock locked and
 * irq_desc->lock held. We can not access the scu bus here, so we
 * store the change and update in the bus_sync_unlock() function below
이것은 mg-> buslock locked와 irq_desc-> lock을 가지고있는 genirq에서 호출된다. 여기서 scu 버스에 액세스 할 수 없으므로 아래의 bus_sync_unlock () 함수에 변경 사항을 저장하고 업데이트합니다
 */
static int msic_irq_type(struct irq_data *data, unsigned type)
{
	struct msic_gpio *mg = irq_data_get_irq_chip_data(data);
	u32 gpio = data->irq - mg->irq_base;

	if (gpio >= mg->chip.ngpio)
		return -EINVAL;

	/* mark for which gpio the trigger changed, protected by buslock
	   gpio가 트리거를 변경하고 buslock으로 보호 된 표시		 */
	mg->trig_change_mask |= (1 << gpio);
	mg->trig_type = type;

	return 0;
}

static int msic_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	struct msic_gpio *mg = container_of(chip, struct msic_gpio, chip);
	return mg->irq_base + offset;
}

static void msic_bus_lock(struct irq_data *data)
{
	struct msic_gpio *mg = irq_data_get_irq_chip_data(data);
	mutex_lock(&mg->buslock);
}

static void msic_bus_sync_unlock(struct irq_data *data)
{
	struct msic_gpio *mg = irq_data_get_irq_chip_data(data);
	int offset;
	int reg;
	u8 trig = 0;

	/* We can only get one change at a time as the buslock covers the
	   entire transaction. The irq_desc->lock is dropped before we are
	   called but that is fine
	   buslock이 전체 트랜잭션을 다루기 때문에 한 번에 하나의 변경 만 얻을 수 있습니다. 우리가 호출되기 전에 irq_desc-> lock이 삭제되었지만 괜찮습니다. */
	if (mg->trig_change_mask) {
		offset = __ffs(mg->trig_change_mask);

		reg = msic_gpio_to_ireg(offset);
		if (reg < 0)
			goto out;

		if (mg->trig_type & IRQ_TYPE_EDGE_RISING)
			trig |= MSIC_GPIO_TRIG_RISE;
		if (mg->trig_type & IRQ_TYPE_EDGE_FALLING)
			trig |= MSIC_GPIO_TRIG_FALL;

		intel_msic_reg_update(reg, trig, MSIC_GPIO_INTCNT_MASK);
		mg->trig_change_mask = 0;
	}
out:
	mutex_unlock(&mg->buslock);
}

/* Firmware does all the masking and unmasking for us, no masking here.
   펌웨어는 모든 마스킹 및 마스킹 해제를 수행하며 마스킹은 수행하지 않습니다. */
static void msic_irq_unmask(struct irq_data *data) { }

static void msic_irq_mask(struct irq_data *data) { }

static struct irq_chip msic_irqchip = {
	.name			= "MSIC-GPIO",
	.irq_mask		= msic_irq_mask,
	.irq_unmask		= msic_irq_unmask,
	.irq_set_type		= msic_irq_type,
	.irq_bus_lock		= msic_bus_lock,
	.irq_bus_sync_unlock	= msic_bus_sync_unlock,
};

static void msic_gpio_irq_handler(struct irq_desc *desc)
{
	struct irq_data *data = irq_desc_get_irq_data(desc);
	struct msic_gpio *mg = irq_data_get_irq_handler_data(data);
	struct irq_chip *chip = irq_data_get_irq_chip(data);
	struct intel_msic *msic = pdev_to_intel_msic(mg->pdev);
	int i;
	int bitnr;
	u8 pin;
	unsigned long pending = 0;

	for (i = 0; i < (mg->chip.ngpio / BITS_PER_BYTE); i++) {
		intel_msic_irq_read(msic, INTEL_MSIC_GPIO0LVIRQ + i, &pin);
		pending = pin;

		if (pending) {
			for_each_set_bit(bitnr, &pending, BITS_PER_BYTE)
				generic_handle_irq(mg->irq_base +
						   (i * BITS_PER_BYTE) + bitnr);
		}
	}
	chip->irq_eoi(data);
}

static int platform_msic_gpio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct intel_msic_gpio_pdata *pdata = dev_get_platdata(dev);
	struct msic_gpio *mg;
	int irq = platform_get_irq(pdev, 0);
	int retval;
	int i;

	if (irq < 0) {
		dev_err(dev, "no IRQ line\n");
		return -EINVAL;
	}

	if (!pdata || !pdata->gpio_base) {
		dev_err(dev, "incorrect or missing platform data\n");
		return -EINVAL;
	}

	mg = kzalloc(sizeof(*mg), GFP_KERNEL);
	if (!mg)
		return -ENOMEM;

	dev_set_drvdata(dev, mg);

	mg->pdev = pdev;
	mg->irq = irq;
	mg->irq_base = pdata->gpio_base + MSIC_GPIO_IRQ_OFFSET;
	mg->chip.label = "msic_gpio";
	mg->chip.direction_input = msic_gpio_direction_input;
	mg->chip.direction_output = msic_gpio_direction_output;
	mg->chip.get = msic_gpio_get;
	mg->chip.set = msic_gpio_set;
	mg->chip.to_irq = msic_gpio_to_irq;
	mg->chip.base = pdata->gpio_base;
	mg->chip.ngpio = MSIC_NUM_GPIO;
	mg->chip.can_sleep = true;
	mg->chip.dev = dev;

	mutex_init(&mg->buslock);

	retval = gpiochip_add(&mg->chip);
	if (retval) {
		dev_err(dev, "Adding MSIC gpio chip failed\n");
		goto err;
	}

	for (i = 0; i < mg->chip.ngpio; i++) {
		irq_set_chip_data(i + mg->irq_base, mg);
		irq_set_chip_and_handler(i + mg->irq_base,
					 &msic_irqchip,
					 handle_simple_irq);
	}
	irq_set_chained_handler_and_data(mg->irq, msic_gpio_irq_handler, mg);

	return 0;
err:
	kfree(mg);
	return retval;
}

static struct platform_driver platform_msic_gpio_driver = {
	.driver = {
		.name		= "msic_gpio",
	},
	.probe		= platform_msic_gpio_probe,
};

static int __init platform_msic_gpio_init(void)
{
	return platform_driver_register(&platform_msic_gpio_driver);
}

subsys_initcall(platform_msic_gpio_init);

MODULE_AUTHOR("Mathias Nyman <mathias.nyman@linux.intel.com>");
MODULE_DESCRIPTION("Intel Medfield MSIC GPIO driver");
MODULE_LICENSE("GPL v2");
