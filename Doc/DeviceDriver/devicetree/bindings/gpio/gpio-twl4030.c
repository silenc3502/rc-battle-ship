/*
 * Access to GPIOs on TWL4030/TPS659x0 chips
 *
 * Copyright (C) 2006-2007 Texas Instruments, Inc.
 * Copyright (C) 2006 MontaVista Software, Inc.
 *
 * Code re-arranged and cleaned up by:
 *	Syed Mohammed Khasim <x0khasim@ti.com>
 *
 * Initial Code:
 *	Andy Lowe / Nishanth Menon
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/irqdomain.h>

#include <linux/i2c/twl.h>

/*
 * The GPIO "subchip" supports 18 GPIOs which can be configured as
 * inputs or outputs, with pullups or pulldowns on each pin.  Each
 * GPIO can trigger interrupts on either or both edges.
 *
 * GPIO interrupts can be fed to either of two IRQ lines; this is
 * intended to support multiple hosts.
 *
 * There are also two LED pins used sometimes as output-only GPIOs.
 
  * GPIO "서브 칩"은 18 핀의 GPIO를 지원하며, 각 핀에 풀업 또는 풀다운이있는 입력 또는 출력으로 구성 할 수 있습니다. 각 GPIO는 양쪽 또는 양쪽 에지에서 인터럽트를 트리거 할 수 있습니다.
  *
  * GPIO 인터럽트는 두 개의 IRQ 라인 중 하나에 공급 될 수 있습니다. 이는 여러 호스트를 지원하기위한 것입니다.
  *
  * 출력 전용 GPIO로 사용되는 두 개의 LED 핀이 있습니다.*/

/* genirq interfaces are not available to modules 
   모듈에 genirq 인터페이스를 사용할 수 없습니다	    */
#ifdef MODULE
#define is_module()	true
#else
#define is_module()	false
#endif

/* GPIO_CTRL Fields */
#define MASK_GPIO_CTRL_GPIO0CD1		BIT(0)
#define MASK_GPIO_CTRL_GPIO1CD2		BIT(1)
#define MASK_GPIO_CTRL_GPIO_ON		BIT(2)

/* Mask for GPIO registers when aggregated into a 32-bit integer
   32 비트 정수로 집계 될 때 GPIO 레지스터 Mask			 */
#define GPIO_32_MASK			0x0003ffff

struct gpio_twl4030_priv {
	struct gpio_chip gpio_chip;
	struct mutex mutex;
	int irq_base;

	/* Bitfields for state caching */
	unsigned int usage_count;
	unsigned int direction;
	unsigned int out_state;
};

/*----------------------------------------------------------------------*/

static inline struct gpio_twl4030_priv *to_gpio_twl4030(struct gpio_chip *chip)
{
	return container_of(chip, struct gpio_twl4030_priv, gpio_chip);
}

/*
 * To configure TWL4030 GPIO module registers
   TWL4030 GPIO 모듈 레지스터를 구성하려면
 */
static inline int gpio_twl4030_write(u8 address, u8 data)
{
	return twl_i2c_write_u8(TWL4030_MODULE_GPIO, data, address);
}

/*----------------------------------------------------------------------*/

/*
 * LED register offsets from TWL_MODULE_LED base
 * PWMs A and B are dedicated to LEDs A and B, respectively.
TWL_MODULE_LED베이스 PWM으로부터의 LED 레지스터 오프셋 A 및 B는 각각 LED A 및 B 전용이다.
 */

#define TWL4030_LED_LEDEN_REG	0x00
#define TWL4030_PWMAON_REG	0x01
#define TWL4030_PWMAOFF_REG	0x02
#define TWL4030_PWMBON_REG	0x03
#define TWL4030_PWMBOFF_REG	0x04

/* LEDEN bits */
#define LEDEN_LEDAON		BIT(0)
#define LEDEN_LEDBON		BIT(1)
#define LEDEN_LEDAEXT		BIT(2)
#define LEDEN_LEDBEXT		BIT(3)
#define LEDEN_LEDAPWM		BIT(4)
#define LEDEN_LEDBPWM		BIT(5)
#define LEDEN_PWM_LENGTHA	BIT(6)
#define LEDEN_PWM_LENGTHB	BIT(7)

#define PWMxON_LENGTH		BIT(7)

/*----------------------------------------------------------------------*/

/*
 * To read a TWL4030 GPIO module register
 */
static inline int gpio_twl4030_read(u8 address)
{
	u8 data;
	int ret = 0;

	ret = twl_i2c_read_u8(TWL4030_MODULE_GPIO, &data, address);
	return (ret < 0) ? ret : data;
}

/*----------------------------------------------------------------------*/

static u8 cached_leden;

/* The LED lines are open drain outputs ... a FET pulls to GND, so an
 * external pullup is needed.  We could also expose the integrated PWM
 * as a LED brightness control; we initialize it as "always on".
LED 라인은 오픈 드레인 출력입니다 ... FET는 GND로 당겨서 외부 풀업이 필요합니다. 우리는 또한 LED 휘도 제어로서 통합 PWM을 노출 할 수있다. 우리는 그것을 "항상 켜기"로 초기화합니다.
 */
static void twl4030_led_set_value(int led, int value)
{
	u8 mask = LEDEN_LEDAON | LEDEN_LEDAPWM;

	if (led)
		mask <<= 1;

	if (value)
		cached_leden &= ~mask;
	else
		cached_leden |= mask;

	WARN_ON_ONCE(twl_i2c_write_u8(TWL4030_MODULE_LED, cached_leden,
				      TWL4030_LED_LEDEN_REG));
}

static int twl4030_set_gpio_direction(int gpio, int is_input)
{
	u8 d_bnk = gpio >> 3;
	u8 d_msk = BIT(gpio & 0x7);
	u8 reg = 0;
	u8 base = REG_GPIODATADIR1 + d_bnk;
	int ret = 0;

	ret = gpio_twl4030_read(base);
	if (ret >= 0) {
		if (is_input)
			reg = ret & ~d_msk;
		else
			reg = ret | d_msk;

		ret = gpio_twl4030_write(base, reg);
	}
	return ret;
}

static int twl4030_set_gpio_dataout(int gpio, int enable)
{
	u8 d_bnk = gpio >> 3;
	u8 d_msk = BIT(gpio & 0x7);
	u8 base = 0;

	if (enable)
		base = REG_SETGPIODATAOUT1 + d_bnk;
	else
		base = REG_CLEARGPIODATAOUT1 + d_bnk;

	return gpio_twl4030_write(base, d_msk);
}

static int twl4030_get_gpio_datain(int gpio)
{
	u8 d_bnk = gpio >> 3;
	u8 d_off = gpio & 0x7;
	u8 base = 0;
	int ret = 0;

	base = REG_GPIODATAIN1 + d_bnk;
	ret = gpio_twl4030_read(base);
	if (ret > 0)
		ret = (ret >> d_off) & 0x1;

	return ret;
}

/*----------------------------------------------------------------------*/

static int twl_request(struct gpio_chip *chip, unsigned offset)
{
	struct gpio_twl4030_priv *priv = to_gpio_twl4030(chip);
	int status = 0;

	mutex_lock(&priv->mutex);

	/* Support the two LED outputs as output-only GPIOs.
	   두 개의 LED 출력을 출력 전용 GPIO로 지원하십시오.		 */
	if (offset >= TWL4030_GPIO_MAX) {
		u8	ledclr_mask = LEDEN_LEDAON | LEDEN_LEDAEXT
				| LEDEN_LEDAPWM | LEDEN_PWM_LENGTHA;
		u8	reg = TWL4030_PWMAON_REG;

		offset -= TWL4030_GPIO_MAX;
		if (offset) {
			ledclr_mask <<= 1;
			reg = TWL4030_PWMBON_REG;
		}

		/* initialize PWM to always-drive 
	  	   PWM을 항상 구동하도록 초기화		*/
		/* Configure PWM OFF register first 
	 	   먼저 PWM OFF 레지스터를 구성하십시오	*/
		status = twl_i2c_write_u8(TWL4030_MODULE_LED, 0x7f, reg + 1);
		if (status < 0)
			goto done;

		/* Followed by PWM ON register */
		status = twl_i2c_write_u8(TWL4030_MODULE_LED, 0x7f, reg);
		if (status < 0)
			goto done;

		/* init LED to not-driven (high) */
		status = twl_i2c_read_u8(TWL4030_MODULE_LED, &cached_leden,
					 TWL4030_LED_LEDEN_REG);
		if (status < 0)
			goto done;
		cached_leden &= ~ledclr_mask;
		status = twl_i2c_write_u8(TWL4030_MODULE_LED, cached_leden,
					  TWL4030_LED_LEDEN_REG);
		if (status < 0)
			goto done;

		status = 0;
		goto done;
	}

	/* on first use, turn GPIO module "on" 
	   처음 사용시에는 GPIO 모듈을 "켜짐"		*/
	if (!priv->usage_count) {
		struct twl4030_gpio_platform_data *pdata;
		u8 value = MASK_GPIO_CTRL_GPIO_ON;

		/* optionally have the first two GPIOs switch vMMC1
		 * and vMMC2 power supplies based on card presence.
		 * 선택적으로 첫 번째 두 GPIO는 카드 존재 여부에 따라 vMMC1 및 vMMC2 전원 공급 장치를 전환합니다.
		 */
		pdata = dev_get_platdata(chip->dev);
		if (pdata)
			value |= pdata->mmc_cd & 0x03;

		status = gpio_twl4030_write(REG_GPIO_CTRL, value);
	}

done:
	if (!status)
		priv->usage_count |= BIT(offset);

	mutex_unlock(&priv->mutex);
	return status;
}

static void twl_free(struct gpio_chip *chip, unsigned offset)
{
	struct gpio_twl4030_priv *priv = to_gpio_twl4030(chip);

	mutex_lock(&priv->mutex);
	if (offset >= TWL4030_GPIO_MAX) {
		twl4030_led_set_value(offset - TWL4030_GPIO_MAX, 1);
		goto out;
	}

	priv->usage_count &= ~BIT(offset);

	/* on last use, switch off GPIO module
	   마지막 사용시에는 GPIO 모듈을 끄십시오.	 */
	if (!priv->usage_count)
		gpio_twl4030_write(REG_GPIO_CTRL, 0x0);

out:
	mutex_unlock(&priv->mutex);
}

static int twl_direction_in(struct gpio_chip *chip, unsigned offset)
{
	struct gpio_twl4030_priv *priv = to_gpio_twl4030(chip);
	int ret;

	mutex_lock(&priv->mutex);
	if (offset < TWL4030_GPIO_MAX)
		ret = twl4030_set_gpio_direction(offset, 1);
	else
		ret = -EINVAL;	/* LED outputs can't be set as input
			   	   LED 출력을 입력으로 설정할 수 없습니다.	 */

	if (!ret)
		priv->direction &= ~BIT(offset);

	mutex_unlock(&priv->mutex);

	return ret;
}

static int twl_get(struct gpio_chip *chip, unsigned offset)
{
	struct gpio_twl4030_priv *priv = to_gpio_twl4030(chip);
	int ret;
	int status = 0;

	mutex_lock(&priv->mutex);
	if (!(priv->usage_count & BIT(offset))) {
		ret = -EPERM;
		goto out;
	}

	if (priv->direction & BIT(offset))
		status = priv->out_state & BIT(offset);
	else
		status = twl4030_get_gpio_datain(offset);

	ret = (status <= 0) ? 0 : 1;
out:
	mutex_unlock(&priv->mutex);
	return ret;
}

static void twl_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct gpio_twl4030_priv *priv = to_gpio_twl4030(chip);

	mutex_lock(&priv->mutex);
	if (offset < TWL4030_GPIO_MAX)
		twl4030_set_gpio_dataout(offset, value);
	else
		twl4030_led_set_value(offset - TWL4030_GPIO_MAX, value);

	if (value)
		priv->out_state |= BIT(offset);
	else
		priv->out_state &= ~BIT(offset);

	mutex_unlock(&priv->mutex);
}

static int twl_direction_out(struct gpio_chip *chip, unsigned offset, int value)
{
	struct gpio_twl4030_priv *priv = to_gpio_twl4030(chip);
	int ret = 0;

	mutex_lock(&priv->mutex);
	if (offset < TWL4030_GPIO_MAX) {
		ret = twl4030_set_gpio_direction(offset, 0);
		if (ret) {
			mutex_unlock(&priv->mutex);
			return ret;
		}
	}

	/*
	 *  LED gpios i.e. offset >= TWL4030_GPIO_MAX are always output
	    LED gpios, 즉 오프셋> = TWL4030_GPIO_MAX가 항상 출력됩니다.
	 */

	priv->direction |= BIT(offset);
	mutex_unlock(&priv->mutex);

	twl_set(chip, offset, value);

	return ret;
}

static int twl_to_irq(struct gpio_chip *chip, unsigned offset)
{
	struct gpio_twl4030_priv *priv = to_gpio_twl4030(chip);

	return (priv->irq_base && (offset < TWL4030_GPIO_MAX))
		? (priv->irq_base + offset)
		: -EINVAL;
}

static struct gpio_chip template_chip = {
	.label			= "twl4030",
	.owner			= THIS_MODULE,
	.request		= twl_request,
	.free			= twl_free,
	.direction_input	= twl_direction_in,
	.get			= twl_get,
	.direction_output	= twl_direction_out,
	.set			= twl_set,
	.to_irq			= twl_to_irq,
	.can_sleep		= true,
};

/*----------------------------------------------------------------------*/

static int gpio_twl4030_pulls(u32 ups, u32 downs)
{
	u8		message[5];
	unsigned	i, gpio_bit;

	/* For most pins, a pulldown was enabled by default.
	 * We should have data that's specific to this board.
	대부분의 핀에서 기본적으로 풀다운이 사용 설정되었습니다.
	이 게시판에 특정한 데이터가 있어야합니다.
	 */
	for (gpio_bit = 1, i = 0; i < 5; i++) {
		u8		bit_mask;
		unsigned	j;

		for (bit_mask = 0, j = 0; j < 8; j += 2, gpio_bit <<= 1) {
			if (ups & gpio_bit)
				bit_mask |= 1 << (j + 1);
			else if (downs & gpio_bit)
				bit_mask |= 1 << (j + 0);
		}
		message[i] = bit_mask;
	}

	return twl_i2c_write(TWL4030_MODULE_GPIO, message,
				REG_GPIOPUPDCTR1, 5);
}

static int gpio_twl4030_debounce(u32 debounce, u8 mmc_cd)
{
	u8		message[3];

	/* 30 msec of debouncing is always used for MMC card detect,
	 * and is optional for everything else.
	   디 바운싱의 30msec는 항상 MMC 카드 감지에 사용되며,
	   다른 모든 것에 대해서는 선택 사항입니다.
	 */
	message[0] = (debounce & 0xff) | (mmc_cd & 0x03);
	debounce >>= 8;
	message[1] = (debounce & 0xff);
	debounce >>= 8;
	message[2] = (debounce & 0x03);

	return twl_i2c_write(TWL4030_MODULE_GPIO, message,
				REG_GPIO_DEBEN1, 3);
}

static int gpio_twl4030_remove(struct platform_device *pdev);

static struct twl4030_gpio_platform_data *of_gpio_twl4030(struct device *dev,
				struct twl4030_gpio_platform_data *pdata)
{
	struct twl4030_gpio_platform_data *omap_twl_info;

	omap_twl_info = devm_kzalloc(dev, sizeof(*omap_twl_info), GFP_KERNEL);
	if (!omap_twl_info)
		return NULL;

	if (pdata)
		*omap_twl_info = *pdata;

	omap_twl_info->use_leds = of_property_read_bool(dev->of_node,
			"ti,use-leds");

	of_property_read_u32(dev->of_node, "ti,debounce",
			     &omap_twl_info->debounce);
	of_property_read_u32(dev->of_node, "ti,mmc-cd",
			     (u32 *)&omap_twl_info->mmc_cd);
	of_property_read_u32(dev->of_node, "ti,pullups",
			     &omap_twl_info->pullups);
	of_property_read_u32(dev->of_node, "ti,pulldowns",
			     &omap_twl_info->pulldowns);

	return omap_twl_info;
}

static int gpio_twl4030_probe(struct platform_device *pdev)
{
	struct twl4030_gpio_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct device_node *node = pdev->dev.of_node;
	struct gpio_twl4030_priv *priv;
	int ret, irq_base;

	priv = devm_kzalloc(&pdev->dev, sizeof(struct gpio_twl4030_priv),
			    GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	/* maybe setup IRQs */
	if (is_module()) {
		dev_err(&pdev->dev, "can't dispatch IRQs from modules\n");
		goto no_irqs;
	}

	irq_base = irq_alloc_descs(-1, 0, TWL4030_GPIO_MAX, 0);
	if (irq_base < 0) {
		dev_err(&pdev->dev, "Failed to alloc irq_descs\n");
		return irq_base;
	}

	irq_domain_add_legacy(node, TWL4030_GPIO_MAX, irq_base, 0,
			      &irq_domain_simple_ops, NULL);

	ret = twl4030_sih_setup(&pdev->dev, TWL4030_MODULE_GPIO, irq_base);
	if (ret < 0)
		return ret;

	priv->irq_base = irq_base;

no_irqs:
	priv->gpio_chip = template_chip;
	priv->gpio_chip.base = -1;
	priv->gpio_chip.ngpio = TWL4030_GPIO_MAX;
	priv->gpio_chip.dev = &pdev->dev;

	mutex_init(&priv->mutex);

	if (node)
		pdata = of_gpio_twl4030(&pdev->dev, pdata);

	if (pdata == NULL) {
		dev_err(&pdev->dev, "Platform data is missing\n");
		return -ENXIO;
	}

	/*
	 * NOTE:  boards may waste power if they don't set pullups
	 * and pulldowns correctly ... default for non-ULPI pins is
	 * pulldown, and some other pins may have external pullups
	 * or pulldowns.  Careful!
	  참고 : 풀업 및 풀다운을 올바르게 설정하지 않으면 보드가 전력을 낭비 할 수 있습니다 ... 
	  ULPI가 아닌 핀의 기본값은 풀다운이고 다른 일부 핀은 외부 풀업 또는 풀다운이있을 수 있습니다. 
	 */
	ret = gpio_twl4030_pulls(pdata->pullups, pdata->pulldowns);
	if (ret)
		dev_dbg(&pdev->dev, "pullups %.05x %.05x --> %d\n",
			pdata->pullups, pdata->pulldowns, ret);

	ret = gpio_twl4030_debounce(pdata->debounce, pdata->mmc_cd);
	if (ret)
		dev_dbg(&pdev->dev, "debounce %.03x %.01x --> %d\n",
			pdata->debounce, pdata->mmc_cd, ret);

	/*
	 * NOTE: we assume VIBRA_CTL.VIBRA_EN, in MODULE_AUDIO_VOICE,
	 * is (still) clear if use_leds is set.
	   참고 : MODULE_AUDIO_VOICE의 VIBRA_CTL.VIBRA_EN은 use_leds가 설정된 경우 (여전히) 분명합니다.
	 */
	if (pdata->use_leds)
		priv->gpio_chip.ngpio += 2;

	ret = gpiochip_add(&priv->gpio_chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "could not register gpiochip, %d\n", ret);
		priv->gpio_chip.ngpio = 0;
		gpio_twl4030_remove(pdev);
		goto out;
	}

	platform_set_drvdata(pdev, priv);

	if (pdata->setup) {
		int status;

		status = pdata->setup(&pdev->dev, priv->gpio_chip.base,
				      TWL4030_GPIO_MAX);
		if (status)
			dev_dbg(&pdev->dev, "setup --> %d\n", status);
	}

out:
	return ret;
}

/* Cannot use as gpio_twl4030_probe() calls us */
static int gpio_twl4030_remove(struct platform_device *pdev)
{
	struct twl4030_gpio_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct gpio_twl4030_priv *priv = platform_get_drvdata(pdev);
	int status;

	if (pdata && pdata->teardown) {
		status = pdata->teardown(&pdev->dev, priv->gpio_chip.base,
					 TWL4030_GPIO_MAX);
		if (status) {
			dev_dbg(&pdev->dev, "teardown --> %d\n", status);
			return status;
		}
	}

	gpiochip_remove(&priv->gpio_chip);

	if (is_module())
		return 0;

	/* REVISIT no support yet for deregistering all the IRQs 
	   모든 IRQ의 등록을 취소 할 수있는 권한이 아직 없습니다.		*/
	WARN_ON(1);
	return -EIO;
}

static const struct of_device_id twl_gpio_match[] = {
	{ .compatible = "ti,twl4030-gpio", },
	{ },
};
MODULE_DEVICE_TABLE(of, twl_gpio_match);

/* Note:  this hardware lives inside an I2C-based multi-function device.
   참고 :이 하드웨어는 I2C 기반 다기능 장치 안에 있습니다.			 */
MODULE_ALIAS("platform:twl4030_gpio");

static struct platform_driver gpio_twl4030_driver = {
	.driver = {
		.name	= "twl4030_gpio",
		.of_match_table = twl_gpio_match,
	},
	.probe		= gpio_twl4030_probe,
	.remove		= gpio_twl4030_remove,
};

static int __init gpio_twl4030_init(void)
{
	return platform_driver_register(&gpio_twl4030_driver);
}
subsys_initcall(gpio_twl4030_init);

static void __exit gpio_twl4030_exit(void)
{
	platform_driver_unregister(&gpio_twl4030_driver);
}
module_exit(gpio_twl4030_exit);

MODULE_AUTHOR("Texas Instruments, Inc.");
MODULE_DESCRIPTION("GPIO interface for TWL4030");
MODULE_LICENSE("GPL");
