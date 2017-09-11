/*
 * STMicroelectronics ConneXt (STA2X11) GPIO driver
 *
 * Copyright 2012 ST Microelectronics (Alessandro Rubini)
 * Based on gpio-ml-ioh.c, Copyright 2010 OKI Semiconductors Ltd.
 * Also based on previous sta2x11 work, Copyright 2011 Wind River Systems, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/mfd/sta2x11-mfd.h>

struct gsta_regs {
	u32 dat;		/* 0x00 */
	u32 dats;
	u32 datc;
	u32 pdis;
	u32 dir;		/* 0x10 */
	u32 dirs;
	u32 dirc;
	u32 unused_1c;
	u32 afsela;		/* 0x20 */
	u32 unused_24[7];
	u32 rimsc;		/* 0x40 */
	u32 fimsc;
	u32 is;
	u32 ic;
};

struct gsta_gpio {
	spinlock_t			lock;
	struct device			*dev;
	void __iomem			*reg_base;
	struct gsta_regs __iomem	*regs[GSTA_NR_BLOCKS];
	struct gpio_chip		gpio;
	int				irq_base;
	/* FIXME: save the whole config here (AF, ...)
		  여기에 전체 설정 저장 (AF, ...)				 */
	unsigned			irq_type[GSTA_NR_GPIO];
};

static inline struct gsta_regs __iomem *__regs(struct gsta_gpio *chip, int nr)
{
	return chip->regs[nr / GSTA_GPIO_PER_BLOCK];
}

static inline u32 __bit(int nr)
{
	return 1U << (nr % GSTA_GPIO_PER_BLOCK);
}

/*
 * gpio methods
 */

static void gsta_gpio_set(struct gpio_chip *gpio, unsigned nr, int val)
{
	struct gsta_gpio *chip = container_of(gpio, struct gsta_gpio, gpio);
	struct gsta_regs __iomem *regs = __regs(chip, nr);
	u32 bit = __bit(nr);

	if (val)
		writel(bit, &regs->dats);
	else
		writel(bit, &regs->datc);
}

static int gsta_gpio_get(struct gpio_chip *gpio, unsigned nr)
{
	struct gsta_gpio *chip = container_of(gpio, struct gsta_gpio, gpio);
	struct gsta_regs __iomem *regs = __regs(chip, nr);
	u32 bit = __bit(nr);

	return readl(&regs->dat) & bit;
}

static int gsta_gpio_direction_output(struct gpio_chip *gpio, unsigned nr,
				      int val)
{
	struct gsta_gpio *chip = container_of(gpio, struct gsta_gpio, gpio);
	struct gsta_regs __iomem *regs = __regs(chip, nr);
	u32 bit = __bit(nr);

	writel(bit, &regs->dirs);
	/* Data register after direction, otherwise pullup/down is selected
	   방향 이후의 데이터 레지스터. 그렇지 않으면 풀업 / 다운이 선택된다.		 */
	if (val)
		writel(bit, &regs->dats);
	else
		writel(bit, &regs->datc);
	return 0;
}

static int gsta_gpio_direction_input(struct gpio_chip *gpio, unsigned nr)
{
	struct gsta_gpio *chip = container_of(gpio, struct gsta_gpio, gpio);
	struct gsta_regs __iomem *regs = __regs(chip, nr);
	u32 bit = __bit(nr);

	writel(bit, &regs->dirc);
	return 0;
}

static int gsta_gpio_to_irq(struct gpio_chip *gpio, unsigned offset)
{
	struct gsta_gpio *chip = container_of(gpio, struct gsta_gpio, gpio);
	return chip->irq_base + offset;
}

static void gsta_gpio_setup(struct gsta_gpio *chip) /* called from probe */
{
	struct gpio_chip *gpio = &chip->gpio;

	/*
	 * ARCH_NR_GPIOS is currently 256 and dynamic allocation starts
	 * from the end. However, for compatibility, we need the first
	 * ConneXt device to start from gpio 0: it's the main chipset
	 * on most boards so documents and drivers assume gpio0..gpio127
	   
ARCH_NR_GPIOS는 현재 256이고 동적 할당은 끝에서부터 시작됩니다. 그러나 호환성을 위해 첫 번째 ConneXt 디바이스가 gpio 0에서 시작해야합니다. 대부분의 보드에서 메인 칩셋이므로 문서 및 드라이버는 gpio0..gpio127을 사용합니다.
	 */
	static int gpio_base;

	gpio->label = dev_name(chip->dev);
	gpio->owner = THIS_MODULE;
	gpio->direction_input = gsta_gpio_direction_input;
	gpio->get = gsta_gpio_get;
	gpio->direction_output = gsta_gpio_direction_output;
	gpio->set = gsta_gpio_set;
	gpio->dbg_show = NULL;
	gpio->base = gpio_base;
	gpio->ngpio = GSTA_NR_GPIO;
	gpio->can_sleep = false;
	gpio->to_irq = gsta_gpio_to_irq;

	/*
	 * After the first device, turn to dynamic gpio numbers.
	 * For example, with ARCH_NR_GPIOS = 256 we can fit two cards
	 
	 * 첫 번째 기기가 끝나면 동적 GPIO 번호로 전환합니다.
	 * 예를 들어 ARCH_NR_GPIOS = 256 인 경우 두 개의 카드를 넣을 수 있습니다.
	 */
	if (!gpio_base)
		gpio_base = -1;
}

/*
 * Special method: alternate functions and pullup/pulldown. This is only
 * invoked on startup to configure gpio's according to platform data.
 * FIXME : this functionality shall be managed (and exported to other drivers)
 * via the pin control subsystem.

 * 특수 방법 : 대체 기능 및 풀업 / 풀다운. 시작시에만 플랫폼 데이터에 따라 gpio를 구성하기 위해 호출됩니다.
 * FIXME :이 기능은 핀 제어 서브 시스템을 통해 관리되고 (다른 드라이버로 내보내 짐)
 */
static void gsta_set_config(struct gsta_gpio *chip, int nr, unsigned cfg)
{
	struct gsta_regs __iomem *regs = __regs(chip, nr);
	unsigned long flags;
	u32 bit = __bit(nr);
	u32 val;
	int err = 0;

	pr_info("%s: %p %i %i\n", __func__, chip, nr, cfg);

	if (cfg == PINMUX_TYPE_NONE)
		return;

	/* Alternate function or not? 	대체 기능인가요?	*/
	spin_lock_irqsave(&chip->lock, flags);
	val = readl(&regs->afsela);
	if (cfg == PINMUX_TYPE_FUNCTION)
		val |= bit;
	else
		val &= ~bit;
	writel(val | bit, &regs->afsela);
	if (cfg == PINMUX_TYPE_FUNCTION) {
		spin_unlock_irqrestore(&chip->lock, flags);
		return;
	}

	/* not alternate function: set details	대체 기능 없음 : 세부 정보 설정	 */
	switch (cfg) {
	case PINMUX_TYPE_OUTPUT_LOW:
		writel(bit, &regs->dirs);
		writel(bit, &regs->datc);
		break;
	case PINMUX_TYPE_OUTPUT_HIGH:
		writel(bit, &regs->dirs);
		writel(bit, &regs->dats);
		break;
	case PINMUX_TYPE_INPUT:
		writel(bit, &regs->dirc);
		val = readl(&regs->pdis) | bit;
		writel(val, &regs->pdis);
		break;
	case PINMUX_TYPE_INPUT_PULLUP:
		writel(bit, &regs->dirc);
		val = readl(&regs->pdis) & ~bit;
		writel(val, &regs->pdis);
		writel(bit, &regs->dats);
		break;
	case PINMUX_TYPE_INPUT_PULLDOWN:
		writel(bit, &regs->dirc);
		val = readl(&regs->pdis) & ~bit;
		writel(val, &regs->pdis);
		writel(bit, &regs->datc);
		break;
	default:
		err = 1;
	}
	spin_unlock_irqrestore(&chip->lock, flags);
	if (err)
		pr_err("%s: chip %p, pin %i, cfg %i is invalid\n",
		       __func__, chip, nr, cfg);
}

/*
 * Irq methods
 */

static void gsta_irq_disable(struct irq_data *data)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(data);
	struct gsta_gpio *chip = gc->private;
	int nr = data->irq - chip->irq_base;
	struct gsta_regs __iomem *regs = __regs(chip, nr);
	u32 bit = __bit(nr);
	u32 val;
	unsigned long flags;

	spin_lock_irqsave(&chip->lock, flags);
	if (chip->irq_type[nr] & IRQ_TYPE_EDGE_RISING) {
		val = readl(&regs->rimsc) & ~bit;
		writel(val, &regs->rimsc);
	}
	if (chip->irq_type[nr] & IRQ_TYPE_EDGE_FALLING) {
		val = readl(&regs->fimsc) & ~bit;
		writel(val, &regs->fimsc);
	}
	spin_unlock_irqrestore(&chip->lock, flags);
	return;
}

static void gsta_irq_enable(struct irq_data *data)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(data);
	struct gsta_gpio *chip = gc->private;
	int nr = data->irq - chip->irq_base;
	struct gsta_regs __iomem *regs = __regs(chip, nr);
	u32 bit = __bit(nr);
	u32 val;
	int type;
	unsigned long flags;

	type = chip->irq_type[nr];

	spin_lock_irqsave(&chip->lock, flags);
	val = readl(&regs->rimsc);
	if (type & IRQ_TYPE_EDGE_RISING)
		writel(val | bit, &regs->rimsc);
	else
		writel(val & ~bit, &regs->rimsc);
	val = readl(&regs->rimsc);
	if (type & IRQ_TYPE_EDGE_FALLING)
		writel(val | bit, &regs->fimsc);
	else
		writel(val & ~bit, &regs->fimsc);
	spin_unlock_irqrestore(&chip->lock, flags);
	return;
}

static int gsta_irq_type(struct irq_data *d, unsigned int type)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct gsta_gpio *chip = gc->private;
	int nr = d->irq - chip->irq_base;

	/* We only support edge interrupts 
	   에지 인터럽트만 지원			*/
	if (!(type & (IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING))) {
		pr_debug("%s: unsupported type 0x%x\n", __func__, type);
		return -EINVAL;
	}

	chip->irq_type[nr] = type; /* used for enable/disable 
				       활성화 / 비활성화에 사용		*/

	gsta_irq_enable(d);
	return 0;
}

static irqreturn_t gsta_gpio_handler(int irq, void *dev_id)
{
	struct gsta_gpio *chip = dev_id;
	struct gsta_regs __iomem *regs;
	u32 is;
	int i, nr, base;
	irqreturn_t ret = IRQ_NONE;

	for (i = 0; i < GSTA_NR_BLOCKS; i++) {
		regs = chip->regs[i];
		base = chip->irq_base + i * GSTA_GPIO_PER_BLOCK;
		while ((is = readl(&regs->is))) {
			nr = __ffs(is);
			irq = base + nr;
			generic_handle_irq(irq);
			writel(1 << nr, &regs->ic);
			ret = IRQ_HANDLED;
		}
	}
	return ret;
}

static void gsta_alloc_irq_chip(struct gsta_gpio *chip)
{
	struct irq_chip_generic *gc;
	struct irq_chip_type *ct;

	gc = irq_alloc_generic_chip(KBUILD_MODNAME, 1, chip->irq_base,
				     chip->reg_base, handle_simple_irq);
	gc->private = chip;
	ct = gc->chip_types;

	ct->chip.irq_set_type = gsta_irq_type;
	ct->chip.irq_disable = gsta_irq_disable;
	ct->chip.irq_enable = gsta_irq_enable;

	/* FIXME: this makes at most 32 interrupts. Request 0 by now
	   FIXME : 최대 32 개의 인터럽트를 발생시킵니다. 지금 0 번 요청	 */
	irq_setup_generic_chip(gc, 0 /* IRQ_MSK(GSTA_GPIO_PER_BLOCK) */, 0,
			       IRQ_NOREQUEST | IRQ_NOPROBE, 0);

	/* Set up all all 128 interrupts: code from setup_generic_chip
	   모든 128 개의 모든 인터럽트 설정 : setup_generic_chip의 코드	 */
	{
		struct irq_chip_type *ct = gc->chip_types;
		int i, j;
		for (j = 0; j < GSTA_NR_GPIO; j++) {
			i = chip->irq_base + j;
			irq_set_chip_and_handler(i, &ct->chip, ct->handler);
			irq_set_chip_data(i, gc);
			irq_clear_status_flags(i, IRQ_NOREQUEST | IRQ_NOPROBE);
		}
		gc->irq_cnt = i - gc->irq_base;
	}
}

/* The platform device used here is instantiated by the MFD device
   여기서 사용 된 플랫폼 장치는 MFD 장치에 의해 인스턴스화됩니다.		 */
static int gsta_probe(struct platform_device *dev)
{
	int i, err;
	struct pci_dev *pdev;
	struct sta2x11_gpio_pdata *gpio_pdata;
	struct gsta_gpio *chip;
	struct resource *res;

	pdev = *(struct pci_dev **)dev_get_platdata(&dev->dev);
	gpio_pdata = dev_get_platdata(&pdev->dev);

	if (gpio_pdata == NULL)
		dev_err(&dev->dev, "no gpio config\n");
	pr_debug("gpio config: %p\n", gpio_pdata);

	res = platform_get_resource(dev, IORESOURCE_MEM, 0);

	chip = devm_kzalloc(&dev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;
	chip->dev = &dev->dev;
	chip->reg_base = devm_ioremap_resource(&dev->dev, res);
	if (IS_ERR(chip->reg_base))
		return PTR_ERR(chip->reg_base);

	for (i = 0; i < GSTA_NR_BLOCKS; i++) {
		chip->regs[i] = chip->reg_base + i * 4096;
		/* disable all irqs */
		writel(0, &chip->regs[i]->rimsc);
		writel(0, &chip->regs[i]->fimsc);
		writel(~0, &chip->regs[i]->ic);
	}
	spin_lock_init(&chip->lock);
	gsta_gpio_setup(chip);
	if (gpio_pdata)
		for (i = 0; i < GSTA_NR_GPIO; i++)
			gsta_set_config(chip, i, gpio_pdata->pinconfig[i]);

	/* 384 was used in previous code: be compatible for other drivers
	   이전 코드에서 384가 사용되었습니다 : 다른 드라이버와 호환 가능		 */
	err = irq_alloc_descs(-1, 384, GSTA_NR_GPIO, NUMA_NO_NODE);
	if (err < 0) {
		dev_warn(&dev->dev, "sta2x11 gpio: Can't get irq base (%i)\n",
			 -err);
		return err;
	}
	chip->irq_base = err;
	gsta_alloc_irq_chip(chip);

	err = request_irq(pdev->irq, gsta_gpio_handler,
			     IRQF_SHARED, KBUILD_MODNAME, chip);
	if (err < 0) {
		dev_err(&dev->dev, "sta2x11 gpio: Can't request irq (%i)\n",
			-err);
		goto err_free_descs;
	}

	err = gpiochip_add(&chip->gpio);
	if (err < 0) {
		dev_err(&dev->dev, "sta2x11 gpio: Can't register (%i)\n",
			-err);
		goto err_free_irq;
	}

	platform_set_drvdata(dev, chip);
	return 0;

err_free_irq:
	free_irq(pdev->irq, chip);
err_free_descs:
	irq_free_descs(chip->irq_base, GSTA_NR_GPIO);
	return err;
}

static struct platform_driver sta2x11_gpio_platform_driver = {
	.driver = {
		.name	= "sta2x11-gpio",
	},
	.probe = gsta_probe,
};

module_platform_driver(sta2x11_gpio_platform_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("sta2x11_gpio GPIO driver");
