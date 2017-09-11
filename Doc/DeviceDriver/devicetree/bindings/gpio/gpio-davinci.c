/*
 * TI DaVinci GPIO Support
 *
 * Copyright (c) 2006-2007 David Brownell
 * Copyright (c) 2007, MontaVista Software, Inc. <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/platform_data/gpio-davinci.h>
#include <linux/irqchip/chained_irq.h>

struct davinci_gpio_regs {
	u32	dir;
	u32	out_data;
	u32	set_data;
	u32	clr_data;
	u32	in_data;
	u32	set_rising;
	u32	clr_rising;
	u32	set_falling;
	u32	clr_falling;
	u32	intstat;
};

typedef struct irq_chip *(*gpio_get_irq_chip_cb_t)(unsigned int irq);

#define BINTEN	0x8 /* GPIO Interrupt Per-Bank Enable Register */

#define chip2controller(chip)	\
	container_of(chip, struct davinci_gpio_controller, chip)

static void __iomem *gpio_base;
static unsigned offset_array[5] = {0x10, 0x38, 0x60, 0x88, 0xb0};

static inline struct davinci_gpio_regs __iomem *irq2regs(struct irq_data *d)
{
	struct davinci_gpio_regs __iomem *g;

	g = (__force struct davinci_gpio_regs __iomem *)irq_data_get_irq_chip_data(d);

	return g;
}

static int davinci_gpio_irq_setup(struct platform_device *pdev);

/*--------------------------------------------------------------------------*/

/* board setup code *MUST* setup pinmux and enable the GPIO clock.
   보드 셋업 코드 * 핀셋을 설정하고 GPIO 클록을 활성화해야합니다. */
static inline int __davinci_direction(struct gpio_chip *chip,
			unsigned offset, bool out, int value)
{
	struct davinci_gpio_controller *d = chip2controller(chip);
	struct davinci_gpio_regs __iomem *g = d->regs;
	unsigned long flags;
	u32 temp;
	u32 mask = 1 << offset;

	spin_lock_irqsave(&d->lock, flags);
	temp = readl_relaxed(&g->dir);
	if (out) {
		temp &= ~mask;
		writel_relaxed(mask, value ? &g->set_data : &g->clr_data);
	} else {
		temp |= mask;
	}
	writel_relaxed(temp, &g->dir);
	spin_unlock_irqrestore(&d->lock, flags);

	return 0;
}

static int davinci_direction_in(struct gpio_chip *chip, unsigned offset)
{
	return __davinci_direction(chip, offset, false, 0);
}

static int
davinci_direction_out(struct gpio_chip *chip, unsigned offset, int value)
{
	return __davinci_direction(chip, offset, true, value);
}

/*
 * Read the pin's value (works even if it's set up as output);
 * returns zero/nonzero.
 *
 * Note that changes are synched to the GPIO clock, so reading values back
 * right after you've set them may give old values.
  
  * 핀의 값을 읽습니다 (출력으로 설정 한 경우에도 작동 함). 0 / 0이 아닌 값을 반환합니다.
  * 변경 사항은 GPIO 클록과 동기화되므로, 설정 한 직후에 값을 읽으면 이전 값이 제공 될 수 있습니다.
 */
static int davinci_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct davinci_gpio_controller *d = chip2controller(chip);
	struct davinci_gpio_regs __iomem *g = d->regs;

	return (1 << offset) & readl_relaxed(&g->in_data);
}

/*
 * Assuming the pin is muxed as a gpio output, set its output value.
   핀이 gpio 출력으로 혼합되었다고 가정하면 출력 값을 설정하십시오.
 */
static void
davinci_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct davinci_gpio_controller *d = chip2controller(chip);
	struct davinci_gpio_regs __iomem *g = d->regs;

	writel_relaxed((1 << offset), value ? &g->set_data : &g->clr_data);
}

static struct davinci_gpio_platform_data *
davinci_gpio_get_pdata(struct platform_device *pdev)
{
	struct device_node *dn = pdev->dev.of_node;
	struct davinci_gpio_platform_data *pdata;
	int ret;
	u32 val;

	if (!IS_ENABLED(CONFIG_OF) || !pdev->dev.of_node)
		return pdev->dev.platform_data;

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return NULL;

	ret = of_property_read_u32(dn, "ti,ngpio", &val);
	if (ret)
		goto of_err;

	pdata->ngpio = val;

	ret = of_property_read_u32(dn, "ti,davinci-gpio-unbanked", &val);
	if (ret)
		goto of_err;

	pdata->gpio_unbanked = val;

	return pdata;

of_err:
	dev_err(&pdev->dev, "Populating pdata from DT failed: err %d\n", ret);
	return NULL;
}

#ifdef CONFIG_OF_GPIO
static int davinci_gpio_of_xlate(struct gpio_chip *gc,
			     const struct of_phandle_args *gpiospec,
			     u32 *flags)
{
	struct davinci_gpio_controller *chips = dev_get_drvdata(gc->dev);
	struct davinci_gpio_platform_data *pdata = dev_get_platdata(gc->dev);

	if (gpiospec->args[0] > pdata->ngpio)
		return -EINVAL;

	if (gc != &chips[gpiospec->args[0] / 32].chip)
		return -EINVAL;

	if (flags)
		*flags = gpiospec->args[1];

	return gpiospec->args[0] % 32;
}
#endif

static int davinci_gpio_probe(struct platform_device *pdev)
{
	int i, base, temp_ctrl_base;
	unsigned ngpio, nbank;
	struct davinci_gpio_controller *chips;
	struct davinci_gpio_platform_data *pdata;
	struct davinci_gpio_regs __iomem *regs;
	struct device *dev = &pdev->dev;
	struct resource *res;
	static int bank_base;

	pdata = davinci_gpio_get_pdata(pdev);
	if (!pdata) {
		dev_err(dev, "No platform data found\n");
		return -EINVAL;
	}

	dev->platform_data = pdata;

	/*
	 * The gpio banks conceptually expose a segmented bitmap,
	 * and "ngpio" is one more than the largest zero-based
	 * bit index that's valid.
	   gpio 은행은 개념적으로 세그먼트 화 된 비트 맵을 노출하며 
	  "ngpio"는 유효한 0부터 시작하는 최대 비트 인덱스 중 하나입니다.
	 */
	ngpio = pdata->ngpio;
	if (ngpio == 0) {
		dev_err(dev, "How many GPIOs?\n");
		return -EINVAL;
	}

	if (WARN_ON(ARCH_NR_GPIOS < ngpio))
		ngpio = ARCH_NR_GPIOS;

	nbank = DIV_ROUND_UP(ngpio, 32);
	chips = devm_kzalloc(dev,
			     nbank * sizeof(struct davinci_gpio_controller),
			     GFP_KERNEL);
	if (!chips)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	gpio_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(gpio_base))
		return PTR_ERR(gpio_base);

	temp_ctrl_base = bank_base;

	for (i = 0, base = 0; base < ngpio; i++, base += 32) {
		chips[i].chip.label = "DaVinci";

		chips[i].chip.direction_input = davinci_direction_in;
		chips[i].chip.get = davinci_gpio_get;
		chips[i].chip.direction_output = davinci_direction_out;
		chips[i].chip.set = davinci_gpio_set;

		chips[i].chip.base = bank_base;
		chips[i].ctrl_base = temp_ctrl_base;
		bank_base += 32;
		chips[i].chip.ngpio = ngpio - base;
		if (chips[i].chip.ngpio > 32)
			chips[i].chip.ngpio = 32;
		else
			bank_base = ngpio;

#ifdef CONFIG_OF_GPIO
		chips[i].chip.of_gpio_n_cells = 2;
		chips[i].chip.of_xlate = davinci_gpio_of_xlate;
		chips[i].chip.dev = dev;
		chips[i].chip.of_node = dev->of_node;
#endif
		spin_lock_init(&chips[i].lock);

		regs = gpio_base + offset_array[i];
		chips[i].regs = regs;
		chips[i].set_data = &regs->set_data;
		chips[i].clr_data = &regs->clr_data;
		chips[i].in_data = &regs->in_data;

		gpiochip_add(&chips[i].chip);
	}

	platform_set_drvdata(pdev, chips);
	davinci_gpio_irq_setup(pdev);
	return 0;
}

/*--------------------------------------------------------------------------*/
/*
 * We expect irqs will normally be set up as input pins, but they can also be
 * used as output pins ... which is convenient for testing.
 *
 * NOTE:  The first few GPIOs also have direct INTC hookups in addition
 * to their GPIOBNK0 irq, with a bit less overhead.
 *
 * All those INTC hookups (direct, plus several IRQ banks) can also
 * serve as EDMA event triggers.
  * irq는 일반적으로 입력 핀으로 설정 될 것으로 예상되지만 출력 핀으로도 사용될 수 있습니다 ... 테스트에 편리합니다.
  * 참고 : 처음 몇 GPIO는 GPIOBNK0 irq 외에도 직접적인 INTC 연결을 가지고 있으며 약간의 오버 헤드가 있습니다.
  * 모든 INTC 연결 (직접 연결 및 여러 IRQ 뱅크)은 EDMA 이벤트 트리거 역할을 할 수 있습니다.
 */

static void gpio_irq_disable(struct irq_data *d)
{
	struct davinci_gpio_regs __iomem *g = irq2regs(d);
	u32 mask = (u32) irq_data_get_irq_handler_data(d);

	writel_relaxed(mask, &g->clr_falling);
	writel_relaxed(mask, &g->clr_rising);
}

static void gpio_irq_enable(struct irq_data *d)
{
	struct davinci_gpio_regs __iomem *g = irq2regs(d);
	u32 mask = (u32) irq_data_get_irq_handler_data(d);
	unsigned status = irqd_get_trigger_type(d);

	status &= IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING;
	if (!status)
		status = IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING;

	if (status & IRQ_TYPE_EDGE_FALLING)
		writel_relaxed(mask, &g->set_falling);
	if (status & IRQ_TYPE_EDGE_RISING)
		writel_relaxed(mask, &g->set_rising);
}

static int gpio_irq_type(struct irq_data *d, unsigned trigger)
{
	if (trigger & ~(IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING))
		return -EINVAL;

	return 0;
}

static struct irq_chip gpio_irqchip = {
	.name		= "GPIO",
	.irq_enable	= gpio_irq_enable,
	.irq_disable	= gpio_irq_disable,
	.irq_set_type	= gpio_irq_type,
	.flags		= IRQCHIP_SET_TYPE_MASKED,
};

static void gpio_irq_handler(struct irq_desc *desc)
{
	unsigned int irq = irq_desc_get_irq(desc);
	struct davinci_gpio_regs __iomem *g;
	u32 mask = 0xffff;
	struct davinci_gpio_controller *d;

	d = (struct davinci_gpio_controller *)irq_desc_get_handler_data(desc);
	g = (struct davinci_gpio_regs __iomem *)d->regs;

	/* we only care about one bank
	    우리는 하나의 bank에만 관심이있다. */
	if (irq == d->birq2)
		mask <<= 16;

	/* temporarily mask (level sensitive) parent IRQ 
	 	일시적으로 마스크 (레벨 감도) 부모 IRQ  	*/
	chained_irq_enter(irq_desc_get_chip(desc), desc);
	while (1) {
		u32		status;
		int		bit;
		irq_hw_number_t hw_irq;

		/* ack any irqs 
	           모든 irqs를 응답	*/
		status = readl_relaxed(&g->intstat) & mask;
		if (!status)
			break;
		writel_relaxed(status, &g->intstat);

		/* now demux them to the right lowlevel handler
		   이제 그것들을 적절한 로우 레벨 처리기에 demux	 */

		while (status) {
			bit = __ffs(status);
			status &= ~BIT(bit);
			/* Max number of gpios per controller is 144 so
			 * hw_irq will be in [0..143]
			   컨트롤러 당 최대 gpios 수는 144입니다.
			   hw_irq는 [0..143]에 있습니다.
			 */
			hw_irq = (d->chip.base - d->ctrl_base) + bit;

			generic_handle_irq(
				irq_find_mapping(d->irq_domain, hw_irq));
		}
	}
	chained_irq_exit(irq_desc_get_chip(desc), desc);
	/* now it may re-trigger
	   이제 다시 트리거 할 수 있습니다. */
}

static int gpio_to_irq_banked(struct gpio_chip *chip, unsigned offset)
{
	struct davinci_gpio_controller *d = chip2controller(chip);
	irq_hw_number_t hw_irq;

	if (d->irq_domain) {
		/* Max number of gpios per controller is 144 so
		 * hw_irq will be in [0..143]
		   컨트롤러 당 최대 gpios 수는 144입니다.
	  	   hw_irq는 [0..143]에 있습니다.
		 */
		hw_irq = (d->chip.base - d->ctrl_base) + offset;
		return irq_create_mapping(d->irq_domain, hw_irq);
	} else {
		return -ENXIO;
	}
}

static int gpio_to_irq_unbanked(struct gpio_chip *chip, unsigned offset)
{
	struct davinci_gpio_controller *d = chip2controller(chip);

	/*
	 * NOTE:  we assume for now that only irqs in the first gpio_chip
	 * can provide direct-mapped IRQs to AINTC (up to 32 GPIOs).
	   참고 : 이제 첫 번째 gpio_chip의 irq만이 AINTC (최대 32 개의 GPIO)에 
 	   직접 매핑 된 IRQ를 제공 할 수 있다고 가정합니다.
	 */
	if (offset < d->gpio_unbanked)
		return d->gpio_irq + offset;
	else
		return -ENODEV;
}

static int gpio_irq_type_unbanked(struct irq_data *data, unsigned trigger)
{
	struct davinci_gpio_controller *d;
	struct davinci_gpio_regs __iomem *g;
	u32 mask;

	d = (struct davinci_gpio_controller *)irq_data_get_irq_handler_data(data);
	g = (struct davinci_gpio_regs __iomem *)d->regs;
	mask = __gpio_mask(data->irq - d->gpio_irq);

	if (trigger & ~(IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING))
		return -EINVAL;

	writel_relaxed(mask, (trigger & IRQ_TYPE_EDGE_FALLING)
		     ? &g->set_falling : &g->clr_falling);
	writel_relaxed(mask, (trigger & IRQ_TYPE_EDGE_RISING)
		     ? &g->set_rising : &g->clr_rising);

	return 0;
}

static int
davinci_gpio_irq_map(struct irq_domain *d, unsigned int irq,
		     irq_hw_number_t hw)
{
	struct davinci_gpio_controller *chips =
				(struct davinci_gpio_controller *)d->host_data;
	struct davinci_gpio_regs __iomem *g = chips[hw / 32].regs;

	irq_set_chip_and_handler_name(irq, &gpio_irqchip, handle_simple_irq,
				"davinci_gpio");
	irq_set_irq_type(irq, IRQ_TYPE_NONE);
	irq_set_chip_data(irq, (__force void *)g);
	irq_set_handler_data(irq, (void *)__gpio_mask(hw));

	return 0;
}

static const struct irq_domain_ops davinci_gpio_irq_ops = {
	.map = davinci_gpio_irq_map,
	.xlate = irq_domain_xlate_onetwocell,
};

static struct irq_chip *davinci_gpio_get_irq_chip(unsigned int irq)
{
	static struct irq_chip_type gpio_unbanked;

	gpio_unbanked = *container_of(irq_get_chip(irq),
				      struct irq_chip_type, chip);

	return &gpio_unbanked.chip;
};

static struct irq_chip *keystone_gpio_get_irq_chip(unsigned int irq)
{
	static struct irq_chip gpio_unbanked;

	gpio_unbanked = *irq_get_chip(irq);
	return &gpio_unbanked;
};

static const struct of_device_id davinci_gpio_ids[];

struct gpio_driver_data {
	gpio_get_irq_chip_cb_t gpio_get_irq_chip;
	bool clk_optional;
};

static struct gpio_driver_data davinci_data = {
	.gpio_get_irq_chip = davinci_gpio_get_irq_chip,
	.clk_optional = false,
};

static struct gpio_driver_data keystone_data = {
	.gpio_get_irq_chip = keystone_gpio_get_irq_chip,
	.clk_optional = false,
};

static struct gpio_driver_data k2g_data = {
	.gpio_get_irq_chip = keystone_gpio_get_irq_chip,
	.clk_optional = true,
};

/*
 * NOTE:  for suspend/resume, probably best to make a platform_device with
 * suspend_late/resume_resume calls hooking into results of the set_wake()
 * calls ... so if no gpios are wakeup events the clock can be disabled,
 * with outputs left at previously set levels, and so that VDD3P3V.IOPWDN0
 * (dm6446) can be set appropriately for GPIOV33 pins.
   참고 : suspend / resume의 경우 suspend_late / resume_resume 호출로 platform_device를 set_wake () 호출 결과에 연결하는 것이 가장 좋습니다 
   ... gpios가 wakeup 이벤트가 아니면 시계를 비활성화 할 수 있으며 출력은 이전에 설정되어 있습니다
   레벨로 설정하여 GPIOV33 핀에 대해 VDD3P3V.IOPWDN0 (dm6446)을 적절하게 설정할 수 있습니다.
 */

static int davinci_gpio_irq_setup(struct platform_device *pdev)
{
	unsigned	gpio, bank;
	int		irq;
	struct clk	*clk;
	u32		binten = 0;
	unsigned	ngpio, bank_irq;
	struct device *dev = &pdev->dev;
	struct resource	*res;
	struct davinci_gpio_controller *chips = platform_get_drvdata(pdev);
	struct davinci_gpio_platform_data *pdata = dev->platform_data;
	struct davinci_gpio_regs __iomem *g;
	struct irq_domain	*irq_domain = NULL;
	const struct of_device_id *match;
	struct irq_chip *irq_chip;
	struct gpio_driver_data *driver_data = NULL;
	gpio_get_irq_chip_cb_t gpio_get_irq_chip;

	/*
	 * Use davinci_gpio_get_irq_chip by default to handle non DT cases
	   비 DT 사례를 처리하려면 기본적으로 davinci_gpio_get_irq_chip을 사용하십시오.
	 */
	gpio_get_irq_chip = davinci_gpio_get_irq_chip;
	match = of_match_device(of_match_ptr(davinci_gpio_ids),
				dev);
	if (match) {
		driver_data = (struct gpio_driver_data *)match->data;
		gpio_get_irq_chip = driver_data->gpio_get_irq_chip;
	}

	ngpio = pdata->ngpio;
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(dev, "Invalid IRQ resource\n");
		return -EBUSY;
	}

	bank_irq = res->start;

	if (!bank_irq) {
		dev_err(dev, "Invalid IRQ resource\n");
		return -ENODEV;
	}

	if (driver_data && driver_data->clk_optional)
		goto skip_clk_handling;

	clk = devm_clk_get(dev, "gpio");
	if (IS_ERR(clk)) {
		printk(KERN_ERR "Error %ld getting gpio clock?\n",
		       PTR_ERR(clk));
		return PTR_ERR(clk);
	}
	clk_prepare_enable(clk);

skip_clk_handling:
	if (!pdata->gpio_unbanked) {
		irq = irq_alloc_descs(-1, 0, ngpio, 0);
		if (irq < 0) {
			dev_err(dev, "Couldn't allocate IRQ numbers\n");
			return irq;
		}

		irq_domain = irq_domain_add_legacy(dev->of_node, ngpio, irq, 0,
							&davinci_gpio_irq_ops,
							chips);
		if (!irq_domain) {
			dev_err(dev, "Couldn't register an IRQ domain\n");
			return -ENODEV;
		}
	}

	/*
	 * Arrange gpio_to_irq() support, handling either direct IRQs or
	 * banked IRQs.  Having GPIOs in the first GPIO bank use direct
	 * IRQs, while the others use banked IRQs, would need some setup
	 * tweaks to recognize hardware which can do that.
	   gpio_to_irq () 지원을 준비하고 직접 IRQ 또는 뱅킹 된 IRQ를 처리합니다.
           첫 번째 GPIO 뱅크에있는 GPIO를 사용하면 직접 IRQ를 사용하고, 
	   다른 하나는 뱅크 된 IRQ를 사용하기 때문에 하드웨어를 인식 할 수있는 설정이 필요합니다.
	 */
	for (gpio = 0, bank = 0; gpio < ngpio; bank++, gpio += 32) {
		chips[bank].chip.to_irq = gpio_to_irq_banked;
		chips[bank].irq_domain = irq_domain;
	}

	/*
	 * AINTC can handle direct/unbanked IRQs for GPIOs, with the GPIO
	 * controller only handling trigger modes.  We currently assume no
	 * IRQ mux conflicts; gpio_irq_type_unbanked() is only for GPIOs.
	   AINTC는 GPIO 컨트롤러에 대해서만 트리거 모드를 처리하는 GPIO 용 다이렉트 / 언 바 킹 IRQ를 처리 할 수 있습니다.
	   우리는 현재 IRQ mux 충돌을 가정하지 않습니다. gpio_irq_type_unbanked ()는 GPIO에만 해당됩니다.
	 */
	if (pdata->gpio_unbanked) {
		/* pass "bank 0" GPIO IRQs to AINTC 
	   	   "bank 0"GPIO IRQ를 AINTC에 전달 */
		chips[0].chip.to_irq = gpio_to_irq_unbanked;
		chips[0].gpio_irq = bank_irq;
		chips[0].gpio_unbanked = pdata->gpio_unbanked;
		binten = GENMASK(pdata->gpio_unbanked / 16, 0);

		/* AINTC handles mask/unmask; GPIO handles triggering
	  	   AINTC는 마스크 / 마스크 해제를 처리합니다. GPIO가 트리거링 처리	 */
		irq = bank_irq;
		irq_chip = gpio_get_irq_chip(irq);
		irq_chip->name = "GPIO-AINTC";
		irq_chip->irq_set_type = gpio_irq_type_unbanked;

		/* default trigger: both edges
	 	  기본 트리거 : 양쪽 가장자리	 */
		g = chips[0].regs;
		writel_relaxed(~0, &g->set_falling);
		writel_relaxed(~0, &g->set_rising);

		/* set the direct IRQs up to use that irqchip
		   irqchip을 사용하도록 직접 IRQ를 설정		 */
		for (gpio = 0; gpio < pdata->gpio_unbanked; gpio++, irq++) {
			irq_set_chip(irq, irq_chip);
			irq_set_handler_data(irq, &chips[gpio / 32]);
			irq_set_status_flags(irq, IRQ_TYPE_EDGE_BOTH);
		}

		goto done;
	}

	/*
	 * Or, AINTC can handle IRQs for banks of 16 GPIO IRQs, which we
	 * then chain through our own handler.
	   또는 AINTC는 16 개의 GPIO IRQ 뱅크에 대해 IRQ를 처리 할 수 있습니다. 그런 다음 자체 처리기를 통해 체인을 연결합니다.
	 */
	for (gpio = 0, bank = 0; gpio < ngpio; bank++, bank_irq++, gpio += 16) {
		/* disabled by default, enabled only as needed 
	 	   기본적으로 비활성화되어 있으며 필요에 따라 활성화되어 있습니다 	*/
		g = chips[bank / 2].regs;
		writel_relaxed(~0, &g->clr_falling);
		writel_relaxed(~0, &g->clr_rising);

		bank_irq = platform_get_irq(pdev, bank);
		if (bank % 2)
			chips[bank / 2].birq2 = bank_irq;
		else
			chips[bank / 2].birq1 = bank_irq;

		/*
		 * Each chip handles 32 gpios, and each irq bank consists of 16
		 * gpio irqs. Pass the irq bank's corresponding controller to
		 * the chained irq handler.
		   각 칩은 32 gpios를 처리하며 각 irq 은행은 16 gpio irq로 구성됩니다.
		   irq 은행의 해당 컨트롤러를 체인화 된 irq 핸들러에 전달하십시오.
		 */
		irq_set_chained_handler_and_data(bank_irq, gpio_irq_handler,
						 &chips[gpio / 32]);

		binten |= BIT(bank);
	}

done:
	/*
	 * BINTEN -- per-bank interrupt enable. genirq would also let these
	 * bits be set/cleared dynamically.
	   BINTEN - 뱅크 당 인터럽트 가능. genirq는 또한 이 비트들이 동적으로 설정 / 지워지도록합니다.
	 */
	writel_relaxed(binten, gpio_base + BINTEN);

	return 0;
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id davinci_gpio_ids[] = {
	{
		.compatible = "ti,keystone-gpio",
		.data = &keystone_data,
	},
	{
		.compatible = "ti,dm6441-gpio",
		.data = &davinci_data,
	},
	{
		.compatible = "ti,k2g-gpio",
		.data = &k2g_data,
	},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, davinci_gpio_ids);
#endif

static struct platform_driver davinci_gpio_driver = {
	.probe		= davinci_gpio_probe,
	.driver		= {
		.name		= "davinci_gpio",
		.of_match_table	= of_match_ptr(davinci_gpio_ids),
	},
};

/**
 * GPIO driver registration needs to be done before machine_init functions
 * access GPIO. Hence davinci_gpio_drv_reg() is a postcore_initcall.
   machine_init 함수가 GPIO에 액세스하기 전에 GPIO 드라이버 등록을 완료해야합니다. 따라서 davinci_gpio_drv_reg ()는 postcore_init 호출입니다.
 */
static int __init davinci_gpio_drv_reg(void)
{
	return platform_driver_register(&davinci_gpio_driver);
}
postcore_initcall(davinci_gpio_drv_reg);
