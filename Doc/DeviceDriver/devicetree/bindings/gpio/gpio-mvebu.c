/*
 * GPIO driver for Marvell SoCs
 *
 * Copyright (C) 2012 Marvell
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 * Andrew Lunn <andrew@lunn.ch>
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 * This driver is a fairly straightforward GPIO driver for the
 * complete family of Marvell EBU SoC platforms (Orion, Dove,
 * Kirkwood, Discovery, Armada 370/XP). The only complexity of this
 * driver is the different register layout that exists between the
 * non-SMP platforms (Orion, Dove, Kirkwood, Armada 370) and the SMP
 * platforms (MV78200 from the Discovery family and the Armada
 * XP). Therefore, this driver handles three variants of the GPIO
 * block:
 * - the basic variant, called "orion-gpio", with the simplest
 *   register set. Used on Orion, Dove, Kirkwoord, Armada 370 and
 *   non-SMP Discovery systems
 * - the mv78200 variant for MV78200 Discovery systems. This variant
 *   turns the edge mask and level mask registers into CPU0 edge
 *   mask/level mask registers, and adds CPU1 edge mask/level mask
 *   registers.
 * - the armadaxp variant for Armada XP systems. This variant keeps
 *   the normal cause/edge mask/level mask registers when the global
 *   interrupts are used, but adds per-CPU cause/edge mask/level mask
 *   registers n a separate memory area for the per-CPU GPIO
 *   interrupts.
  * 블록:
  - "orion-gpio"라고하는 기본 변형으로 가장 간단한 레지스터 세트입니다. Orion, Dove, Kirkwoord, Armada 370 및 비 SMP 디스커버리 시스템에서 사용
  - MV78200 발견 시스템 용 mv78200 변형. 이 변형은 에지 마스크 및 레벨 마스크 레지스터를 CPU0 에지 마스크 / 레벨 마스크 레지스터로 전환하고 CPU1 에지 마스크 / 레벨 마스크 레지스터를 추가합니다.
  - Armada XP 시스템 용 armadaxp 변형. 이 변형은 전역 인터럽트가 사용될 때 일반적인 원인 / 에지 마스크 / 레벨 마스크 레지스터를 유지하지만 CPU 당 GPIO 인터럽트를위한 별도의 메모리 영역에는 CPU 당 원인 / 에지 마스크 / 레벨 마스크 레지스터를 추가합니다.
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/irqdomain.h>
#include <linux/io.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/clk.h>
#include <linux/pinctrl/consumer.h>
#include <linux/irqchip/chained_irq.h>

/*
 * GPIO unit register offsets. GPIO 단위 레지스터 오프셋.
 */
#define GPIO_OUT_OFF		0x0000
#define GPIO_IO_CONF_OFF	0x0004
#define GPIO_BLINK_EN_OFF	0x0008
#define GPIO_IN_POL_OFF		0x000c
#define GPIO_DATA_IN_OFF	0x0010
#define GPIO_EDGE_CAUSE_OFF	0x0014
#define GPIO_EDGE_MASK_OFF	0x0018
#define GPIO_LEVEL_MASK_OFF	0x001c

/* The MV78200 has per-CPU registers for edge mask and level mask 
   MV78200에는 에지 마스크와 레벨 마스크를위한 CPU 별 레지스터가 있습니다 */
#define GPIO_EDGE_MASK_MV78200_OFF(cpu)	  ((cpu) ? 0x30 : 0x18)
#define GPIO_LEVEL_MASK_MV78200_OFF(cpu)  ((cpu) ? 0x34 : 0x1C)

/* The Armada XP has per-CPU registers for interrupt cause, interrupt
 * mask and interrupt level mask. Those are relative to the
 * percpu_membase. 
 Armada XP에는 인터럽트 원인, 인터럽트 마스크 및 인터럽트 레벨 마스크에 대한 CPU 당 레지스터가 있습니다. 그것들은 percpu_membase와 관련이 있습니다.*/
#define GPIO_EDGE_CAUSE_ARMADAXP_OFF(cpu) ((cpu) * 0x4)
#define GPIO_EDGE_MASK_ARMADAXP_OFF(cpu)  (0x10 + (cpu) * 0x4)
#define GPIO_LEVEL_MASK_ARMADAXP_OFF(cpu) (0x20 + (cpu) * 0x4)

#define MVEBU_GPIO_SOC_VARIANT_ORION	0x1
#define MVEBU_GPIO_SOC_VARIANT_MV78200	0x2
#define MVEBU_GPIO_SOC_VARIANT_ARMADAXP 0x3

#define MVEBU_MAX_GPIO_PER_BANK		32

struct mvebu_gpio_chip {
	struct gpio_chip   chip;
	spinlock_t	   lock;
	void __iomem	  *membase;
	void __iomem	  *percpu_membase;
	int		   irqbase;
	struct irq_domain *domain;
	int		   soc_variant;

	/* Used to preserve GPIO registers across suspend/resume
	   일시 중지 / 다시 시작을 통해 GPIO 레지스터를 보존하는 데 사용됩니다. */
	u32                out_reg;
	u32                io_conf_reg;
	u32                blink_en_reg;
	u32                in_pol_reg;
	u32                edge_mask_regs[4];
	u32                level_mask_regs[4];
};

/*
 * Functions returning addresses of individual registers for a given
 * GPIO controller.
   주어진 GPIO 컨트롤러에 대한 개별 레지스터의 주소를 반환하는 함수.
 */
static inline void __iomem *mvebu_gpioreg_out(struct mvebu_gpio_chip *mvchip)
{
	return mvchip->membase + GPIO_OUT_OFF;
}

static inline void __iomem *mvebu_gpioreg_blink(struct mvebu_gpio_chip *mvchip)
{
	return mvchip->membase + GPIO_BLINK_EN_OFF;
}

static inline void __iomem *
mvebu_gpioreg_io_conf(struct mvebu_gpio_chip *mvchip)
{
	return mvchip->membase + GPIO_IO_CONF_OFF;
}

static inline void __iomem *mvebu_gpioreg_in_pol(struct mvebu_gpio_chip *mvchip)
{
	return mvchip->membase + GPIO_IN_POL_OFF;
}

static inline void __iomem *
mvebu_gpioreg_data_in(struct mvebu_gpio_chip *mvchip)
{
	return mvchip->membase + GPIO_DATA_IN_OFF;
}

static inline void __iomem *
mvebu_gpioreg_edge_cause(struct mvebu_gpio_chip *mvchip)
{
	int cpu;

	switch (mvchip->soc_variant) {
	case MVEBU_GPIO_SOC_VARIANT_ORION:
	case MVEBU_GPIO_SOC_VARIANT_MV78200:
		return mvchip->membase + GPIO_EDGE_CAUSE_OFF;
	case MVEBU_GPIO_SOC_VARIANT_ARMADAXP:
		cpu = smp_processor_id();
		return mvchip->percpu_membase +
			GPIO_EDGE_CAUSE_ARMADAXP_OFF(cpu);
	default:
		BUG();
	}
}

static inline void __iomem *
mvebu_gpioreg_edge_mask(struct mvebu_gpio_chip *mvchip)
{
	int cpu;

	switch (mvchip->soc_variant) {
	case MVEBU_GPIO_SOC_VARIANT_ORION:
		return mvchip->membase + GPIO_EDGE_MASK_OFF;
	case MVEBU_GPIO_SOC_VARIANT_MV78200:
		cpu = smp_processor_id();
		return mvchip->membase + GPIO_EDGE_MASK_MV78200_OFF(cpu);
	case MVEBU_GPIO_SOC_VARIANT_ARMADAXP:
		cpu = smp_processor_id();
		return mvchip->percpu_membase +
			GPIO_EDGE_MASK_ARMADAXP_OFF(cpu);
	default:
		BUG();
	}
}

static void __iomem *mvebu_gpioreg_level_mask(struct mvebu_gpio_chip *mvchip)
{
	int cpu;

	switch (mvchip->soc_variant) {
	case MVEBU_GPIO_SOC_VARIANT_ORION:
		return mvchip->membase + GPIO_LEVEL_MASK_OFF;
	case MVEBU_GPIO_SOC_VARIANT_MV78200:
		cpu = smp_processor_id();
		return mvchip->membase + GPIO_LEVEL_MASK_MV78200_OFF(cpu);
	case MVEBU_GPIO_SOC_VARIANT_ARMADAXP:
		cpu = smp_processor_id();
		return mvchip->percpu_membase +
			GPIO_LEVEL_MASK_ARMADAXP_OFF(cpu);
	default:
		BUG();
	}
}

/*
 * Functions implementing the gpio_chip methods 
   gpio_chip 메소드를 구현하는 함수
 */

static void mvebu_gpio_set(struct gpio_chip *chip, unsigned pin, int value)
{
	struct mvebu_gpio_chip *mvchip =
		container_of(chip, struct mvebu_gpio_chip, chip);
	unsigned long flags;
	u32 u;

	spin_lock_irqsave(&mvchip->lock, flags);
	u = readl_relaxed(mvebu_gpioreg_out(mvchip));
	if (value)
		u |= 1 << pin;
	else
		u &= ~(1 << pin);
	writel_relaxed(u, mvebu_gpioreg_out(mvchip));
	spin_unlock_irqrestore(&mvchip->lock, flags);
}

static int mvebu_gpio_get(struct gpio_chip *chip, unsigned pin)
{
	struct mvebu_gpio_chip *mvchip =
		container_of(chip, struct mvebu_gpio_chip, chip);
	u32 u;

	if (readl_relaxed(mvebu_gpioreg_io_conf(mvchip)) & (1 << pin)) {
		u = readl_relaxed(mvebu_gpioreg_data_in(mvchip)) ^
			readl_relaxed(mvebu_gpioreg_in_pol(mvchip));
	} else {
		u = readl_relaxed(mvebu_gpioreg_out(mvchip));
	}

	return (u >> pin) & 1;
}

static void mvebu_gpio_blink(struct gpio_chip *chip, unsigned pin, int value)
{
	struct mvebu_gpio_chip *mvchip =
		container_of(chip, struct mvebu_gpio_chip, chip);
	unsigned long flags;
	u32 u;

	spin_lock_irqsave(&mvchip->lock, flags);
	u = readl_relaxed(mvebu_gpioreg_blink(mvchip));
	if (value)
		u |= 1 << pin;
	else
		u &= ~(1 << pin);
	writel_relaxed(u, mvebu_gpioreg_blink(mvchip));
	spin_unlock_irqrestore(&mvchip->lock, flags);
}

static int mvebu_gpio_direction_input(struct gpio_chip *chip, unsigned pin)
{
	struct mvebu_gpio_chip *mvchip =
		container_of(chip, struct mvebu_gpio_chip, chip);
	unsigned long flags;
	int ret;
	u32 u;

	/* Check with the pinctrl driver whether this pin is usable as
	 * an input GPIO 
	   이 핀이 입력 GPIO로 사용 가능한지 여부는 pinctrl 드라이버로 확인하십시오.*/
	ret = pinctrl_gpio_direction_input(chip->base + pin);
	if (ret)
		return ret;

	spin_lock_irqsave(&mvchip->lock, flags);
	u = readl_relaxed(mvebu_gpioreg_io_conf(mvchip));
	u |= 1 << pin;
	writel_relaxed(u, mvebu_gpioreg_io_conf(mvchip));
	spin_unlock_irqrestore(&mvchip->lock, flags);

	return 0;
}

static int mvebu_gpio_direction_output(struct gpio_chip *chip, unsigned pin,
				       int value)
{
	struct mvebu_gpio_chip *mvchip =
		container_of(chip, struct mvebu_gpio_chip, chip);
	unsigned long flags;
	int ret;
	u32 u;

	/* Check with the pinctrl driver whether this pin is usable as
	 * an output GPIO 
	   이 핀이 출력 GPIO로 사용 가능한지 여부는 pinctrl 드라이버로 확인하십시오.*/
	ret = pinctrl_gpio_direction_output(chip->base + pin);
	if (ret)
		return ret;

	mvebu_gpio_blink(chip, pin, 0);
	mvebu_gpio_set(chip, pin, value);

	spin_lock_irqsave(&mvchip->lock, flags);
	u = readl_relaxed(mvebu_gpioreg_io_conf(mvchip));
	u &= ~(1 << pin);
	writel_relaxed(u, mvebu_gpioreg_io_conf(mvchip));
	spin_unlock_irqrestore(&mvchip->lock, flags);

	return 0;
}

static int mvebu_gpio_to_irq(struct gpio_chip *chip, unsigned pin)
{
	struct mvebu_gpio_chip *mvchip =
		container_of(chip, struct mvebu_gpio_chip, chip);
	return irq_create_mapping(mvchip->domain, pin);
}

/*
 * Functions implementing the irq_chip methods
   irq_chip 메소드를 구현하는 함수
 */
static void mvebu_gpio_irq_ack(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct mvebu_gpio_chip *mvchip = gc->private;
	u32 mask = ~(1 << (d->irq - gc->irq_base));

	irq_gc_lock(gc);
	writel_relaxed(mask, mvebu_gpioreg_edge_cause(mvchip));
	irq_gc_unlock(gc);
}

static void mvebu_gpio_edge_irq_mask(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct mvebu_gpio_chip *mvchip = gc->private;
	struct irq_chip_type *ct = irq_data_get_chip_type(d);
	u32 mask = 1 << (d->irq - gc->irq_base);

	irq_gc_lock(gc);
	ct->mask_cache_priv &= ~mask;

	writel_relaxed(ct->mask_cache_priv, mvebu_gpioreg_edge_mask(mvchip));
	irq_gc_unlock(gc);
}

static void mvebu_gpio_edge_irq_unmask(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct mvebu_gpio_chip *mvchip = gc->private;
	struct irq_chip_type *ct = irq_data_get_chip_type(d);

	u32 mask = 1 << (d->irq - gc->irq_base);

	irq_gc_lock(gc);
	ct->mask_cache_priv |= mask;
	writel_relaxed(ct->mask_cache_priv, mvebu_gpioreg_edge_mask(mvchip));
	irq_gc_unlock(gc);
}

static void mvebu_gpio_level_irq_mask(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct mvebu_gpio_chip *mvchip = gc->private;
	struct irq_chip_type *ct = irq_data_get_chip_type(d);

	u32 mask = 1 << (d->irq - gc->irq_base);

	irq_gc_lock(gc);
	ct->mask_cache_priv &= ~mask;
	writel_relaxed(ct->mask_cache_priv, mvebu_gpioreg_level_mask(mvchip));
	irq_gc_unlock(gc);
}

static void mvebu_gpio_level_irq_unmask(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct mvebu_gpio_chip *mvchip = gc->private;
	struct irq_chip_type *ct = irq_data_get_chip_type(d);

	u32 mask = 1 << (d->irq - gc->irq_base);

	irq_gc_lock(gc);
	ct->mask_cache_priv |= mask;
	writel_relaxed(ct->mask_cache_priv, mvebu_gpioreg_level_mask(mvchip));
	irq_gc_unlock(gc);
}

/*****************************************************************************
 * MVEBU GPIO IRQ
 *
 * GPIO_IN_POL register controls whether GPIO_DATA_IN will hold the same
 * value of the line or the opposite value.
 *
 * Level IRQ handlers: DATA_IN is used directly as cause register.
 *		       Interrupt are masked by LEVEL_MASK registers.
 * Edge IRQ handlers:  Change in DATA_IN are latched in EDGE_CAUSE.
 *		       Interrupt are masked by EDGE_MASK registers.
 * Both-edge handlers: Similar to regular Edge handlers, but also swaps
 *		       the polarity to catch the next line transaction.
 *		       This is a race condition that might not perfectly
 *		       work on some use cases.
 *
 * Every eight GPIO lines are grouped (OR'ed) before going up to main
 * cause register.
 *
 *		      EDGE  cause    mask
 *	  data-in   /--------| |-----| |----\
 *     -----| |-----			     ---- to main cause reg
 *	     X	    \----------------| |----/
 *	  polarity    LEVEL	     mask
 *
 ****************************************************************************/
/*  MVEBU GPIO IRQ
  *
  * GPIO_IN_POL 레지스터는 GPIO_DATA_IN이 라인의 값이나 반대 값을 유지할지 여부를 제어합니다.
  *
  * 레벨 IRQ 처리기 : DATA_IN은 원인 레지스터로 직접 사용됩니다.
  * 인터럽트는 LEVEL_MASK 레지스터에 의해 마스킹됩니다.
  * Edge IRQ 핸들러 : DATA_IN의 변경 사항은 EDGE_CAUSE에 래치됩니다.
  * 인터럽트는 EDGE_MASK 레지스터에 의해 마스크됩니다.
  * 양쪽 가장자리 핸들러 : 일반 가장자리 핸들러와 비슷하지만 다음 줄 트랜잭션을 잡기 위해 극성을 바꿉니다.
  * 일부 사용 사례에서는 완벽하게 작동하지 않을 수있는 경쟁 조건입니다.
  *
  * 주 원인 레지스터로 이동하기 전에 8 개의 GPIO 라인이 그룹화됩니다 (OR'ed). */

static int mvebu_gpio_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct irq_chip_type *ct = irq_data_get_chip_type(d);
	struct mvebu_gpio_chip *mvchip = gc->private;
	int pin;
	u32 u;

	pin = d->hwirq;

	u = readl_relaxed(mvebu_gpioreg_io_conf(mvchip)) & (1 << pin);
	if (!u)
		return -EINVAL;

	type &= IRQ_TYPE_SENSE_MASK;
	if (type == IRQ_TYPE_NONE)
		return -EINVAL;

	/* Check if we need to change chip and handler
	   우리가 칩과 핸들러를 바꿀 필요가 있는지 확인	 */
	if (!(ct->type & type))
		if (irq_setup_alt_chip(d, type))
			return -EINVAL;

	/*
	 * Configure interrupt polarity. 인터럽트 극성을 구성하십시오.
	 */
	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
	case IRQ_TYPE_LEVEL_HIGH:
		u = readl_relaxed(mvebu_gpioreg_in_pol(mvchip));
		u &= ~(1 << pin);
		writel_relaxed(u, mvebu_gpioreg_in_pol(mvchip));
		break;
	case IRQ_TYPE_EDGE_FALLING:
	case IRQ_TYPE_LEVEL_LOW:
		u = readl_relaxed(mvebu_gpioreg_in_pol(mvchip));
		u |= 1 << pin;
		writel_relaxed(u, mvebu_gpioreg_in_pol(mvchip));
		break;
	case IRQ_TYPE_EDGE_BOTH: {
		u32 v;

		v = readl_relaxed(mvebu_gpioreg_in_pol(mvchip)) ^
			readl_relaxed(mvebu_gpioreg_data_in(mvchip));

		/*
		 * set initial polarity based on current input level
		    현재 입력 레벨을 기준으로 초기 극성을 설정합니다.
		 */
		u = readl_relaxed(mvebu_gpioreg_in_pol(mvchip));
		if (v & (1 << pin))
			u |= 1 << pin;		/* falling */
		else
			u &= ~(1 << pin);	/* rising */
		writel_relaxed(u, mvebu_gpioreg_in_pol(mvchip));
		break;
	}
	}
	return 0;
}

static void mvebu_gpio_irq_handler(struct irq_desc *desc)
{
	struct mvebu_gpio_chip *mvchip = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	u32 cause, type;
	int i;

	if (mvchip == NULL)
		return;

	chained_irq_enter(chip, desc);

	cause = readl_relaxed(mvebu_gpioreg_data_in(mvchip)) &
		readl_relaxed(mvebu_gpioreg_level_mask(mvchip));
	cause |= readl_relaxed(mvebu_gpioreg_edge_cause(mvchip)) &
		readl_relaxed(mvebu_gpioreg_edge_mask(mvchip));

	for (i = 0; i < mvchip->chip.ngpio; i++) {
		int irq;

		irq = mvchip->irqbase + i;

		if (!(cause & (1 << i)))
			continue;

		type = irq_get_trigger_type(irq);
		if ((type & IRQ_TYPE_SENSE_MASK) == IRQ_TYPE_EDGE_BOTH) {
			/* Swap polarity (race with GPIO line)
			   극성 스왑 (GPIO 라인과 경쟁)		 */
			u32 polarity;

			polarity = readl_relaxed(mvebu_gpioreg_in_pol(mvchip));
			polarity ^= 1 << i;
			writel_relaxed(polarity, mvebu_gpioreg_in_pol(mvchip));
		}

		generic_handle_irq(irq);
	}

	chained_irq_exit(chip, desc);
}

#ifdef CONFIG_DEBUG_FS
#include <linux/seq_file.h>

static void mvebu_gpio_dbg_show(struct seq_file *s, struct gpio_chip *chip)
{
	struct mvebu_gpio_chip *mvchip =
		container_of(chip, struct mvebu_gpio_chip, chip);
	u32 out, io_conf, blink, in_pol, data_in, cause, edg_msk, lvl_msk;
	int i;

	out	= readl_relaxed(mvebu_gpioreg_out(mvchip));
	io_conf	= readl_relaxed(mvebu_gpioreg_io_conf(mvchip));
	blink	= readl_relaxed(mvebu_gpioreg_blink(mvchip));
	in_pol	= readl_relaxed(mvebu_gpioreg_in_pol(mvchip));
	data_in	= readl_relaxed(mvebu_gpioreg_data_in(mvchip));
	cause	= readl_relaxed(mvebu_gpioreg_edge_cause(mvchip));
	edg_msk	= readl_relaxed(mvebu_gpioreg_edge_mask(mvchip));
	lvl_msk	= readl_relaxed(mvebu_gpioreg_level_mask(mvchip));

	for (i = 0; i < chip->ngpio; i++) {
		const char *label;
		u32 msk;
		bool is_out;

		label = gpiochip_is_requested(chip, i);
		if (!label)
			continue;

		msk = 1 << i;
		is_out = !(io_conf & msk);

		seq_printf(s, " gpio-%-3d (%-20.20s)", chip->base + i, label);

		if (is_out) {
			seq_printf(s, " out %s %s\n",
				   out & msk ? "hi" : "lo",
				   blink & msk ? "(blink )" : "");
			continue;
		}

		seq_printf(s, " in  %s (act %s) - IRQ",
			   (data_in ^ in_pol) & msk  ? "hi" : "lo",
			   in_pol & msk ? "lo" : "hi");
		if (!((edg_msk | lvl_msk) & msk)) {
			seq_puts(s, " disabled\n");
			continue;
		}
		if (edg_msk & msk)
			seq_puts(s, " edge ");
		if (lvl_msk & msk)
			seq_puts(s, " level");
		seq_printf(s, " (%s)\n", cause & msk ? "pending" : "clear  ");
	}
}
#else
#define mvebu_gpio_dbg_show NULL
#endif

static const struct of_device_id mvebu_gpio_of_match[] = {
	{
		.compatible = "marvell,orion-gpio",
		.data	    = (void *) MVEBU_GPIO_SOC_VARIANT_ORION,
	},
	{
		.compatible = "marvell,mv78200-gpio",
		.data	    = (void *) MVEBU_GPIO_SOC_VARIANT_MV78200,
	},
	{
		.compatible = "marvell,armadaxp-gpio",
		.data	    = (void *) MVEBU_GPIO_SOC_VARIANT_ARMADAXP,
	},
	{
		/* sentinel */
	},
};
MODULE_DEVICE_TABLE(of, mvebu_gpio_of_match);

static int mvebu_gpio_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct mvebu_gpio_chip *mvchip = platform_get_drvdata(pdev);
	int i;

	mvchip->out_reg = readl(mvebu_gpioreg_out(mvchip));
	mvchip->io_conf_reg = readl(mvebu_gpioreg_io_conf(mvchip));
	mvchip->blink_en_reg = readl(mvebu_gpioreg_blink(mvchip));
	mvchip->in_pol_reg = readl(mvebu_gpioreg_in_pol(mvchip));

	switch (mvchip->soc_variant) {
	case MVEBU_GPIO_SOC_VARIANT_ORION:
		mvchip->edge_mask_regs[0] =
			readl(mvchip->membase + GPIO_EDGE_MASK_OFF);
		mvchip->level_mask_regs[0] =
			readl(mvchip->membase + GPIO_LEVEL_MASK_OFF);
		break;
	case MVEBU_GPIO_SOC_VARIANT_MV78200:
		for (i = 0; i < 2; i++) {
			mvchip->edge_mask_regs[i] =
				readl(mvchip->membase +
				      GPIO_EDGE_MASK_MV78200_OFF(i));
			mvchip->level_mask_regs[i] =
				readl(mvchip->membase +
				      GPIO_LEVEL_MASK_MV78200_OFF(i));
		}
		break;
	case MVEBU_GPIO_SOC_VARIANT_ARMADAXP:
		for (i = 0; i < 4; i++) {
			mvchip->edge_mask_regs[i] =
				readl(mvchip->membase +
				      GPIO_EDGE_MASK_ARMADAXP_OFF(i));
			mvchip->level_mask_regs[i] =
				readl(mvchip->membase +
				      GPIO_LEVEL_MASK_ARMADAXP_OFF(i));
		}
		break;
	default:
		BUG();
	}

	return 0;
}

static int mvebu_gpio_resume(struct platform_device *pdev)
{
	struct mvebu_gpio_chip *mvchip = platform_get_drvdata(pdev);
	int i;

	writel(mvchip->out_reg, mvebu_gpioreg_out(mvchip));
	writel(mvchip->io_conf_reg, mvebu_gpioreg_io_conf(mvchip));
	writel(mvchip->blink_en_reg, mvebu_gpioreg_blink(mvchip));
	writel(mvchip->in_pol_reg, mvebu_gpioreg_in_pol(mvchip));

	switch (mvchip->soc_variant) {
	case MVEBU_GPIO_SOC_VARIANT_ORION:
		writel(mvchip->edge_mask_regs[0],
		       mvchip->membase + GPIO_EDGE_MASK_OFF);
		writel(mvchip->level_mask_regs[0],
		       mvchip->membase + GPIO_LEVEL_MASK_OFF);
		break;
	case MVEBU_GPIO_SOC_VARIANT_MV78200:
		for (i = 0; i < 2; i++) {
			writel(mvchip->edge_mask_regs[i],
			       mvchip->membase + GPIO_EDGE_MASK_MV78200_OFF(i));
			writel(mvchip->level_mask_regs[i],
			       mvchip->membase +
			       GPIO_LEVEL_MASK_MV78200_OFF(i));
		}
		break;
	case MVEBU_GPIO_SOC_VARIANT_ARMADAXP:
		for (i = 0; i < 4; i++) {
			writel(mvchip->edge_mask_regs[i],
			       mvchip->membase +
			       GPIO_EDGE_MASK_ARMADAXP_OFF(i));
			writel(mvchip->level_mask_regs[i],
			       mvchip->membase +
			       GPIO_LEVEL_MASK_ARMADAXP_OFF(i));
		}
		break;
	default:
		BUG();
	}

	return 0;
}

static int mvebu_gpio_probe(struct platform_device *pdev)
{
	struct mvebu_gpio_chip *mvchip;
	const struct of_device_id *match;
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;
	struct irq_chip_generic *gc;
	struct irq_chip_type *ct;
	struct clk *clk;
	unsigned int ngpios;
	int soc_variant;
	int i, cpu, id;
	int err;

	match = of_match_device(mvebu_gpio_of_match, &pdev->dev);
	if (match)
		soc_variant = (int) match->data;
	else
		soc_variant = MVEBU_GPIO_SOC_VARIANT_ORION;

	mvchip = devm_kzalloc(&pdev->dev, sizeof(struct mvebu_gpio_chip),
			      GFP_KERNEL);
	if (!mvchip)
		return -ENOMEM;

	platform_set_drvdata(pdev, mvchip);

	if (of_property_read_u32(pdev->dev.of_node, "ngpios", &ngpios)) {
		dev_err(&pdev->dev, "Missing ngpios OF property\n");
		return -ENODEV;
	}

	id = of_alias_get_id(pdev->dev.of_node, "gpio");
	if (id < 0) {
		dev_err(&pdev->dev, "Couldn't get OF id\n");
		return id;
	}

	clk = devm_clk_get(&pdev->dev, NULL);
	/* Not all SoCs require a clock.*/
	if (!IS_ERR(clk))
		clk_prepare_enable(clk);

	mvchip->soc_variant = soc_variant;
	mvchip->chip.label = dev_name(&pdev->dev);
	mvchip->chip.dev = &pdev->dev;
	mvchip->chip.request = gpiochip_generic_request;
	mvchip->chip.free = gpiochip_generic_free;
	mvchip->chip.direction_input = mvebu_gpio_direction_input;
	mvchip->chip.get = mvebu_gpio_get;
	mvchip->chip.direction_output = mvebu_gpio_direction_output;
	mvchip->chip.set = mvebu_gpio_set;
	mvchip->chip.to_irq = mvebu_gpio_to_irq;
	mvchip->chip.base = id * MVEBU_MAX_GPIO_PER_BANK;
	mvchip->chip.ngpio = ngpios;
	mvchip->chip.can_sleep = false;
	mvchip->chip.of_node = np;
	mvchip->chip.dbg_show = mvebu_gpio_dbg_show;

	spin_lock_init(&mvchip->lock);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mvchip->membase = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mvchip->membase))
		return PTR_ERR(mvchip->membase);

	/* The Armada XP has a second range of registers for the
	 * per-CPU registers
	   Armada XP에는 CPU 당 레지스터에 대해 두 번째 범위의 레지스터가 있습니다. */
	if (soc_variant == MVEBU_GPIO_SOC_VARIANT_ARMADAXP) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
		mvchip->percpu_membase = devm_ioremap_resource(&pdev->dev,
							       res);
		if (IS_ERR(mvchip->percpu_membase))
			return PTR_ERR(mvchip->percpu_membase);
	}

	/*
	 * Mask and clear GPIO interrupts. GPIO 인터럽트를 마스크 및 클리어합니다.
	 */
	switch (soc_variant) {
	case MVEBU_GPIO_SOC_VARIANT_ORION:
		writel_relaxed(0, mvchip->membase + GPIO_EDGE_CAUSE_OFF);
		writel_relaxed(0, mvchip->membase + GPIO_EDGE_MASK_OFF);
		writel_relaxed(0, mvchip->membase + GPIO_LEVEL_MASK_OFF);
		break;
	case MVEBU_GPIO_SOC_VARIANT_MV78200:
		writel_relaxed(0, mvchip->membase + GPIO_EDGE_CAUSE_OFF);
		for (cpu = 0; cpu < 2; cpu++) {
			writel_relaxed(0, mvchip->membase +
				       GPIO_EDGE_MASK_MV78200_OFF(cpu));
			writel_relaxed(0, mvchip->membase +
				       GPIO_LEVEL_MASK_MV78200_OFF(cpu));
		}
		break;
	case MVEBU_GPIO_SOC_VARIANT_ARMADAXP:
		writel_relaxed(0, mvchip->membase + GPIO_EDGE_CAUSE_OFF);
		writel_relaxed(0, mvchip->membase + GPIO_EDGE_MASK_OFF);
		writel_relaxed(0, mvchip->membase + GPIO_LEVEL_MASK_OFF);
		for (cpu = 0; cpu < 4; cpu++) {
			writel_relaxed(0, mvchip->percpu_membase +
				       GPIO_EDGE_CAUSE_ARMADAXP_OFF(cpu));
			writel_relaxed(0, mvchip->percpu_membase +
				       GPIO_EDGE_MASK_ARMADAXP_OFF(cpu));
			writel_relaxed(0, mvchip->percpu_membase +
				       GPIO_LEVEL_MASK_ARMADAXP_OFF(cpu));
		}
		break;
	default:
		BUG();
	}

	gpiochip_add(&mvchip->chip);

	/* Some gpio controllers do not provide irq support 
	   일부 gpio 컨트롤러는 irq 지원을 제공하지 않습니다		*/
	if (!of_irq_count(np))
		return 0;

	/* Setup the interrupt handlers. Each chip can have up to 4
	 * interrupt handlers, with each handler dealing with 8 GPIO
	 * pins. 
	  인터럽트 핸들러를 설정합니다. 각 칩은 최대 4 개의 인터럽트 핸들러를 가질 수 있으며 각 핸들러는 8 개의 GPIO 핀을 처리합니다. */
	for (i = 0; i < 4; i++) {
		int irq = platform_get_irq(pdev, i);

		if (irq < 0)
			continue;
		irq_set_chained_handler_and_data(irq, mvebu_gpio_irq_handler,
						 mvchip);
	}

	mvchip->irqbase = irq_alloc_descs(-1, 0, ngpios, -1);
	if (mvchip->irqbase < 0) {
		dev_err(&pdev->dev, "no irqs\n");
		err = mvchip->irqbase;
		goto err_gpiochip_add;
	}

	gc = irq_alloc_generic_chip("mvebu_gpio_irq", 2, mvchip->irqbase,
				    mvchip->membase, handle_level_irq);
	if (!gc) {
		dev_err(&pdev->dev, "Cannot allocate generic irq_chip\n");
		err = -ENOMEM;
		goto err_gpiochip_add;
	}

	gc->private = mvchip;
	ct = &gc->chip_types[0];
	ct->type = IRQ_TYPE_LEVEL_HIGH | IRQ_TYPE_LEVEL_LOW;
	ct->chip.irq_mask = mvebu_gpio_level_irq_mask;
	ct->chip.irq_unmask = mvebu_gpio_level_irq_unmask;
	ct->chip.irq_set_type = mvebu_gpio_irq_set_type;
	ct->chip.name = mvchip->chip.label;

	ct = &gc->chip_types[1];
	ct->type = IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING;
	ct->chip.irq_ack = mvebu_gpio_irq_ack;
	ct->chip.irq_mask = mvebu_gpio_edge_irq_mask;
	ct->chip.irq_unmask = mvebu_gpio_edge_irq_unmask;
	ct->chip.irq_set_type = mvebu_gpio_irq_set_type;
	ct->handler = handle_edge_irq;
	ct->chip.name = mvchip->chip.label;

	irq_setup_generic_chip(gc, IRQ_MSK(ngpios), 0,
			       IRQ_NOREQUEST, IRQ_LEVEL | IRQ_NOPROBE);

	/* Setup irq domain on top of the generic chip. 
	   일반 칩 위에 irq 도메인을 설정하십시오.		*/
	mvchip->domain = irq_domain_add_simple(np, mvchip->chip.ngpio,
					       mvchip->irqbase,
					       &irq_domain_simple_ops,
					       mvchip);
	if (!mvchip->domain) {
		dev_err(&pdev->dev, "couldn't allocate irq domain %s (DT).\n",
			mvchip->chip.label);
		err = -ENODEV;
		goto err_generic_chip;
	}

	return 0;

err_generic_chip:
	irq_remove_generic_chip(gc, IRQ_MSK(ngpios), IRQ_NOREQUEST,
				IRQ_LEVEL | IRQ_NOPROBE);
	kfree(gc);

err_gpiochip_add:
	gpiochip_remove(&mvchip->chip);

	return err;
}

static struct platform_driver mvebu_gpio_driver = {
	.driver		= {
		.name		= "mvebu-gpio",
		.of_match_table = mvebu_gpio_of_match,
	},
	.probe		= mvebu_gpio_probe,
	.suspend        = mvebu_gpio_suspend,
	.resume         = mvebu_gpio_resume,
};
module_platform_driver(mvebu_gpio_driver);
