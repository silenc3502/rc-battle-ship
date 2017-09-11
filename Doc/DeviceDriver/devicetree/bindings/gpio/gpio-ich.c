/*
 * Intel ICH6-10, Series 5 and 6, Atom C2000 (Avoton/Rangeley) GPIO driver
 *
 * Copyright (C) 2010 Extreme Engineering Solutions.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/mfd/lpc_ich.h>

#define DRV_NAME "gpio_ich"

/*
 * GPIO register offsets in GPIO I/O space.
 * Each chunk of 32 GPIOs is manipulated via its own USE_SELx, IO_SELx, and
 * LVLx registers.  Logic in the read/write functions takes a register and
 * an absolute bit number and determines the proper register offset and bit
 * number in that register.  For example, to read the value of GPIO bit 50
 * the code would access offset ichx_regs[2(=GPIO_LVL)][1(=50/32)],
 * bit 18 (50%32).
  * GPIO I / O 공간에서 GPIO 레지스터 오프셋.
  * 32 개의 GPIO로 구성된 각 청크는 자체 USE_SELx, IO_SELx 및 LVLx 레지스터를 통해 조작됩니다. 
    읽기 / 쓰기 기능의 논리는 레지스터와 절대 비트 수를 취해 해당 레지스터에서 적절한 레지스터 오프셋과 비트 수를 결정합니다. 
    예를 들어, GPIO 비트 50의 값을 읽으려면 코드는 오프셋 ichx_regs [2 (= GPIO_LVL)] [1 (= 50 / 32)], 비트 18 (50 % 32)에 액세스합니다.
 */
enum GPIO_REG {
	GPIO_USE_SEL = 0,
	GPIO_IO_SEL,
	GPIO_LVL,
	GPO_BLINK
};

static const u8 ichx_regs[4][3] = {
	{0x00, 0x30, 0x40},	/* USE_SEL[1-3] offsets */
	{0x04, 0x34, 0x44},	/* IO_SEL[1-3] offsets */
	{0x0c, 0x38, 0x48},	/* LVL[1-3] offsets */
	{0x18, 0x18, 0x18},	/* BLINK offset */
};

static const u8 ichx_reglen[3] = {
	0x30, 0x10, 0x10,
};

static const u8 avoton_regs[4][3] = {
	{0x00, 0x80, 0x00},
	{0x04, 0x84, 0x00},
	{0x08, 0x88, 0x00},
};

static const u8 avoton_reglen[3] = {
	0x10, 0x10, 0x00,
};

#define ICHX_WRITE(val, reg, base_res)	outl(val, (reg) + (base_res)->start)
#define ICHX_READ(reg, base_res)	inl((reg) + (base_res)->start)

struct ichx_desc {
	/* Max GPIO pins the chipset can have 
	   칩셋이 가질 수있는 최대 GPIO 핀		*/
	uint ngpio;

	/* chipset registers 
	   칩셋 레지스터		*/
	const u8 (*regs)[3];
	const u8 *reglen;

	/* GPO_BLINK is available on this chipset
	   이 칩셋에서 GPO_BLINK를 사용할 수 있습니다. 	 */
	bool have_blink;

	/* Whether the chipset has GPIO in GPE0_STS in the PM IO region 
	   GPIO가 PM IO 영역의 GPE0_STS에 있는지 여부			*/
	bool uses_gpe0;

	/* USE_SEL is bogus on some chipsets, eg 3100 
	   USE_SEL은 일부 칩셋에서 가짜입니다 (예 : 3,100 			*/
	u32 use_sel_ignore[3];

	/* Some chipsets have quirks, let these use their own request/get 
	   일부 칩셋에는 단점이 있으며, 이것들은 그들 자신의 요청을 사용하게한다.	*/
	int (*request)(struct gpio_chip *chip, unsigned offset);
	int (*get)(struct gpio_chip *chip, unsigned offset);

	/*
	 * Some chipsets don't let reading output values on GPIO_LVL register
	 * this option allows driver caching written output values
	   일부 칩셋은 GPIO_LVL 레지스터에서 출력 값을 읽을 수 없도록합니다.
	   이 옵션을 사용하면 드라이버가 기록 된 출력 값을 캐싱 할 수 있습니다
	 */
	bool use_outlvl_cache;
};

static struct {
	spinlock_t lock;
	struct platform_device *dev;
	struct gpio_chip chip;
	struct resource *gpio_base;	/* GPIO IO base 
					   GPIO IO 기반	*/
	struct resource *pm_base;	/* Power Mangagment IO base */
	struct ichx_desc *desc;	/* Pointer to chipset-specific description 
				   칩셋 별 설명에 대한 포인터 		*/
	u32 orig_gpio_ctrl;	/* Orig CTRL value, used to restore on exit 
				   종료시 복원에 사용되는 Orig CTRL 값	*/
	u8 use_gpio;		/* Which GPIO groups are usable 
				   사용 가능한 GPIO 그룹		*/
	int outlvl_cache[3];	/* cached output values 
				    캐시 된 출력 값		*/
} ichx_priv;

static int modparam_gpiobase = -1;	/* dynamic */
module_param_named(gpiobase, modparam_gpiobase, int, 0444);
MODULE_PARM_DESC(gpiobase, "The GPIO number base. -1 means dynamic, "
			   "which is the default.");

static int ichx_write_bit(int reg, unsigned nr, int val, int verify)
{
	unsigned long flags;
	u32 data, tmp;
	int reg_nr = nr / 32;
	int bit = nr & 0x1f;
	int ret = 0;

	spin_lock_irqsave(&ichx_priv.lock, flags);

	if (reg == GPIO_LVL && ichx_priv.desc->use_outlvl_cache)
		data = ichx_priv.outlvl_cache[reg_nr];
	else
		data = ICHX_READ(ichx_priv.desc->regs[reg][reg_nr],
				 ichx_priv.gpio_base);

	if (val)
		data |= 1 << bit;
	else
		data &= ~(1 << bit);
	ICHX_WRITE(data, ichx_priv.desc->regs[reg][reg_nr],
			 ichx_priv.gpio_base);
	if (reg == GPIO_LVL && ichx_priv.desc->use_outlvl_cache)
		ichx_priv.outlvl_cache[reg_nr] = data;

	tmp = ICHX_READ(ichx_priv.desc->regs[reg][reg_nr],
			ichx_priv.gpio_base);
	if (verify && data != tmp)
		ret = -EPERM;

	spin_unlock_irqrestore(&ichx_priv.lock, flags);

	return ret;
}

static int ichx_read_bit(int reg, unsigned nr)
{
	unsigned long flags;
	u32 data;
	int reg_nr = nr / 32;
	int bit = nr & 0x1f;

	spin_lock_irqsave(&ichx_priv.lock, flags);

	data = ICHX_READ(ichx_priv.desc->regs[reg][reg_nr],
			 ichx_priv.gpio_base);

	if (reg == GPIO_LVL && ichx_priv.desc->use_outlvl_cache)
		data = ichx_priv.outlvl_cache[reg_nr] | data;

	spin_unlock_irqrestore(&ichx_priv.lock, flags);

	return data & (1 << bit) ? 1 : 0;
}

static bool ichx_gpio_check_available(struct gpio_chip *gpio, unsigned nr)
{
	return !!(ichx_priv.use_gpio & (1 << (nr / 32)));
}

static int ichx_gpio_get_direction(struct gpio_chip *gpio, unsigned nr)
{
	return ichx_read_bit(GPIO_IO_SEL, nr) ? GPIOF_DIR_IN : GPIOF_DIR_OUT;
}

static int ichx_gpio_direction_input(struct gpio_chip *gpio, unsigned nr)
{
	/*
	 * Try setting pin as an input and verify it worked since many pins
	 * are output-only.
	   핀을 입력으로 설정해보고 많은 핀이 출력 전용이므로 제대로 작동하는지 확인하십시오.
	 */
	if (ichx_write_bit(GPIO_IO_SEL, nr, 1, 1))
		return -EINVAL;

	return 0;
}

static int ichx_gpio_direction_output(struct gpio_chip *gpio, unsigned nr,
					int val)
{
	/* Disable blink hardware which is available for GPIOs from 0 to 31.
	   0에서 31까지 GPIO에 사용할 수있는 깜박임 하드웨어를 비활성화합니다	 */
	if (nr < 32 && ichx_priv.desc->have_blink)
		ichx_write_bit(GPO_BLINK, nr, 0, 0);

	/* Set GPIO output value. */
	ichx_write_bit(GPIO_LVL, nr, val, 0);

	/*
	 * Try setting pin as an output and verify it worked since many pins
	 * are input-only.
	   핀을 출력으로 설정해보고 많은 핀이 입력 전용이므로 동작하는지 확인하십시오.
	 */
	if (ichx_write_bit(GPIO_IO_SEL, nr, 0, 1))
		return -EINVAL;

	return 0;
}

static int ichx_gpio_get(struct gpio_chip *chip, unsigned nr)
{
	return ichx_read_bit(GPIO_LVL, nr);
}

static int ich6_gpio_get(struct gpio_chip *chip, unsigned nr)
{
	unsigned long flags;
	u32 data;

	/*
	 * GPI 0 - 15 need to be read from the power management registers on
	 * a ICH6/3100 bridge.
	   GPI 0-15는 ICH6 / 3100 브리지의 전원 관리 레지스터에서 읽어야합니다.
	 */
	if (nr < 16) {
		if (!ichx_priv.pm_base)
			return -ENXIO;

		spin_lock_irqsave(&ichx_priv.lock, flags);

		/* GPI 0 - 15 are latched, write 1 to clear
		    GPI 0-15가 latched되고, clear하기 위해 1을 쓰십시오	 */
		ICHX_WRITE(1 << (16 + nr), 0, ichx_priv.pm_base);
		data = ICHX_READ(0, ichx_priv.pm_base);

		spin_unlock_irqrestore(&ichx_priv.lock, flags);

		return (data >> 16) & (1 << nr) ? 1 : 0;
	} else {
		return ichx_gpio_get(chip, nr);
	}
}

static int ichx_gpio_request(struct gpio_chip *chip, unsigned nr)
{
	if (!ichx_gpio_check_available(chip, nr))
		return -ENXIO;

	/*
	 * Note we assume the BIOS properly set a bridge's USE value.  Some
	 * chips (eg Intel 3100) have bogus USE values though, so first see if
	 * the chipset's USE value can be trusted for this specific bit.
	 * If it can't be trusted, assume that the pin can be used as a GPIO.
	 */
	if (ichx_priv.desc->use_sel_ignore[nr / 32] & (1 << (nr & 0x1f)))
		return 0;

	return ichx_read_bit(GPIO_USE_SEL, nr) ? 0 : -ENODEV;
}

static int ich6_gpio_request(struct gpio_chip *chip, unsigned nr)
{
	/*
	 * Fixups for bits 16 and 17 are necessary on the Intel ICH6/3100
	 * bridge as they are controlled by USE register bits 0 and 1.  See
	 * "Table 704 GPIO_USE_SEL1 register" in the i3100 datasheet for
	 * additional info.
	   BIOS가 브리지의 USE 값을 올바르게 설정했다고 가정합니다.
	   일부 칩 (예 : 인텔 3100)은 가짜 USE 값을 가지고 있으므로 먼저이 특정 비트에 대해 칩셋의 USE 값을 신뢰할 수 있는지 확인하십시오.
           신뢰할 수없는 경우 핀을 GPIO로 사용할 수 있다고 가정합니다.
	 */
	if (nr == 16 || nr == 17)
		nr -= 16;

	return ichx_gpio_request(chip, nr);
}

static void ichx_gpio_set(struct gpio_chip *chip, unsigned nr, int val)
{
	ichx_write_bit(GPIO_LVL, nr, val, 0);
}

static void ichx_gpiolib_setup(struct gpio_chip *chip)
{
	chip->owner = THIS_MODULE;
	chip->label = DRV_NAME;
	chip->dev = &ichx_priv.dev->dev;

	/* Allow chip-specific overrides of request()/get()
	   request () / get ()에 대한 칩 특정 재 지정 허용	 */
	chip->request = ichx_priv.desc->request ?
		ichx_priv.desc->request : ichx_gpio_request;
	chip->get = ichx_priv.desc->get ?
		ichx_priv.desc->get : ichx_gpio_get;

	chip->set = ichx_gpio_set;
	chip->get_direction = ichx_gpio_get_direction;
	chip->direction_input = ichx_gpio_direction_input;
	chip->direction_output = ichx_gpio_direction_output;
	chip->base = modparam_gpiobase;
	chip->ngpio = ichx_priv.desc->ngpio;
	chip->can_sleep = false;
	chip->dbg_show = NULL;
}

/* ICH6-based, 631xesb-based */
static struct ichx_desc ich6_desc = {
	/* Bridges using the ICH6 controller need fixups for GPIO 0 - 17
	    ICH6 컨트롤러를 사용하는 브리지는 GPIO 0 - 17			 */
	.request = ich6_gpio_request,
	.get = ich6_gpio_get,

	/* GPIO 0-15 are read in the GPE0_STS PM register 
	   GPIO0-15는 GPE0_STS PM 레지스터에서 읽습니다. 			*/
	.uses_gpe0 = true,

	.ngpio = 50,
	.have_blink = true,
	.regs = ichx_regs,
	.reglen = ichx_reglen,
};

/* Intel 3100 */
static struct ichx_desc i3100_desc = {
	/*
	 * Bits 16,17, 20 of USE_SEL and bit 16 of USE_SEL2 always read 0 on
	 * the Intel 3100.  See "Table 712. GPIO Summary Table" of 3100
	 * Datasheet for more info.
	   USE_SEL의 16,17,20 비트와 USE_SEL2의 16 비트는 항상 Intel 3100에서 0을 읽습니다. 
	 */
	.use_sel_ignore = {0x00130000, 0x00010000, 0x0},

	/* The 3100 needs fixups for GPIO 0 - 17 
	   3100에는 GPIO 0 - 17 에 대한 픽스 업이 필요합니다.*/
	.request = ich6_gpio_request,
	.get = ich6_gpio_get,

	/* GPIO 0-15 are read in the GPE0_STS PM register 
	   GPIO0-15는 GPE0_STS PM 레지스터에서 읽습니다. */
	.uses_gpe0 = true,

	.ngpio = 50,
	.regs = ichx_regs,
	.reglen = ichx_reglen,
};

/* ICH7 and ICH8-based */
static struct ichx_desc ich7_desc = {
	.ngpio = 50,
	.have_blink = true,
	.regs = ichx_regs,
	.reglen = ichx_reglen,
};

/* ICH9-based */
static struct ichx_desc ich9_desc = {
	.ngpio = 61,
	.have_blink = true,
	.regs = ichx_regs,
	.reglen = ichx_reglen,
};

/* ICH10-based - Consumer/corporate versions have different amount of GPIO 
	         소비자 / 기업 버전의 GPIO 양은 다릅니다.*/
static struct ichx_desc ich10_cons_desc = {
	.ngpio = 61,
	.have_blink = true,
	.regs = ichx_regs,
	.reglen = ichx_reglen,
};
static struct ichx_desc ich10_corp_desc = {
	.ngpio = 72,
	.have_blink = true,
	.regs = ichx_regs,
	.reglen = ichx_reglen,
};

/* Intel 5 series, 6 series, 3400 series, and C200 series */
static struct ichx_desc intel5_desc = {
	.ngpio = 76,
	.regs = ichx_regs,
	.reglen = ichx_reglen,
};

/* Avoton */
static struct ichx_desc avoton_desc = {
	/* Avoton has only 59 GPIOs, but we assume the first set of register
	 * (Core) has 32 instead of 31 to keep gpio-ich compliance
	  Avoton은 단지 59 개의 GPIO를 가지고 있지만 gpio-ich를 준수하기 위해서는 첫 번째 레지스터 세트 (코어)가 31 대신 32 개가 있다고 가정합니다.
	 */
	.ngpio = 60,
	.regs = avoton_regs,
	.reglen = avoton_reglen,
	.use_outlvl_cache = true,
};

static int ichx_gpio_request_regions(struct resource *res_base,
						const char *name, u8 use_gpio)
{
	int i;

	if (!res_base || !res_base->start || !res_base->end)
		return -ENODEV;

	for (i = 0; i < ARRAY_SIZE(ichx_priv.desc->regs[0]); i++) {
		if (!(use_gpio & (1 << i)))
			continue;
		if (!request_region(
				res_base->start + ichx_priv.desc->regs[0][i],
				ichx_priv.desc->reglen[i], name))
			goto request_err;
	}
	return 0;

request_err:
	/* Clean up: release already requested regions, if any 
	   정리 : 이미 요청한 영역이 있으면이를 해제합니다.			*/
	for (i--; i >= 0; i--) {
		if (!(use_gpio & (1 << i)))
			continue;
		release_region(res_base->start + ichx_priv.desc->regs[0][i],
			       ichx_priv.desc->reglen[i]);
	}
	return -EBUSY;
}

static void ichx_gpio_release_regions(struct resource *res_base, u8 use_gpio)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ichx_priv.desc->regs[0]); i++) {
		if (!(use_gpio & (1 << i)))
			continue;
		release_region(res_base->start + ichx_priv.desc->regs[0][i],
			       ichx_priv.desc->reglen[i]);
	}
}

static int ichx_gpio_probe(struct platform_device *pdev)
{
	struct resource *res_base, *res_pm;
	int err;
	struct lpc_ich_info *ich_info = dev_get_platdata(&pdev->dev);

	if (!ich_info)
		return -ENODEV;

	ichx_priv.dev = pdev;

	switch (ich_info->gpio_version) {
	case ICH_I3100_GPIO:
		ichx_priv.desc = &i3100_desc;
		break;
	case ICH_V5_GPIO:
		ichx_priv.desc = &intel5_desc;
		break;
	case ICH_V6_GPIO:
		ichx_priv.desc = &ich6_desc;
		break;
	case ICH_V7_GPIO:
		ichx_priv.desc = &ich7_desc;
		break;
	case ICH_V9_GPIO:
		ichx_priv.desc = &ich9_desc;
		break;
	case ICH_V10CORP_GPIO:
		ichx_priv.desc = &ich10_corp_desc;
		break;
	case ICH_V10CONS_GPIO:
		ichx_priv.desc = &ich10_cons_desc;
		break;
	case AVOTON_GPIO:
		ichx_priv.desc = &avoton_desc;
		break;
	default:
		return -ENODEV;
	}

	spin_lock_init(&ichx_priv.lock);
	res_base = platform_get_resource(pdev, IORESOURCE_IO, ICH_RES_GPIO);
	ichx_priv.use_gpio = ich_info->use_gpio;
	err = ichx_gpio_request_regions(res_base, pdev->name,
					ichx_priv.use_gpio);
	if (err)
		return err;

	ichx_priv.gpio_base = res_base;

	/*
	 * If necessary, determine the I/O address of ACPI/power management
	 * registers which are needed to read the the GPE0 register for GPI pins
	 * 0 - 15 on some chipsets.
	   필요한 경우 일부 칩셋의 GPI 핀 0-15에 대한 GPE0 레지스터를 읽는 데 필요한 ACPI / 전원 관리 레지스터의 I / O 주소를 결정합니다.
	 */
	if (!ichx_priv.desc->uses_gpe0)
		goto init;

	res_pm = platform_get_resource(pdev, IORESOURCE_IO, ICH_RES_GPE0);
	if (!res_pm) {
		pr_warn("ACPI BAR is unavailable, GPI 0 - 15 unavailable\n");
		goto init;
	}

	if (!request_region(res_pm->start, resource_size(res_pm),
			pdev->name)) {
		pr_warn("ACPI BAR is busy, GPI 0 - 15 unavailable\n");
		goto init;
	}

	ichx_priv.pm_base = res_pm;

init:
	ichx_gpiolib_setup(&ichx_priv.chip);
	err = gpiochip_add(&ichx_priv.chip);
	if (err) {
		pr_err("Failed to register GPIOs\n");
		goto add_err;
	}

	pr_info("GPIO from %d to %d on %s\n", ichx_priv.chip.base,
	       ichx_priv.chip.base + ichx_priv.chip.ngpio - 1, DRV_NAME);

	return 0;

add_err:
	ichx_gpio_release_regions(ichx_priv.gpio_base, ichx_priv.use_gpio);
	if (ichx_priv.pm_base)
		release_region(ichx_priv.pm_base->start,
				resource_size(ichx_priv.pm_base));
	return err;
}

static int ichx_gpio_remove(struct platform_device *pdev)
{
	gpiochip_remove(&ichx_priv.chip);

	ichx_gpio_release_regions(ichx_priv.gpio_base, ichx_priv.use_gpio);
	if (ichx_priv.pm_base)
		release_region(ichx_priv.pm_base->start,
				resource_size(ichx_priv.pm_base));

	return 0;
}

static struct platform_driver ichx_gpio_driver = {
	.driver		= {
		.name	= DRV_NAME,
	},
	.probe		= ichx_gpio_probe,
	.remove		= ichx_gpio_remove,
};

module_platform_driver(ichx_gpio_driver);

MODULE_AUTHOR("Peter Tyser <ptyser@xes-inc.com>");
MODULE_DESCRIPTION("GPIO interface for Intel ICH series");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:"DRV_NAME);
