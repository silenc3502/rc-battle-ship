/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2012 John Crispin <blogic@openwrt.org>
 *
 */

#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/of_platform.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <lantiq_soc.h>

/*
 * The Serial To Parallel (STP) is found on MIPS based Lantiq socs. It is a
 * peripheral controller used to drive external shift register cascades. At most
 * 3 groups of 8 bits can be driven. The hardware is able to allow the DSL modem
 * to drive the 2 LSBs of the cascade automatically.
직렬 대 병렬 (STP)은 MIPS 기반 Lantiq socs에서 찾을 수 있습니다. 외부 시프트 레지스터 캐스케이드를 구동하는 데 사용되는 주변 장치 컨트롤러입니다. 최대 3 개의 8 비트 그룹이 구동 될 수 있습니다. 하드웨어는 DSL 모뎀이 캐스케이드의 2 LSB를 자동으로 구동하도록 허용 할 수 있습니다.
 */

/* control register 0 */
#define XWAY_STP_CON0		0x00
/* control register 1 */
#define XWAY_STP_CON1		0x04
/* data register 0 */
#define XWAY_STP_CPU0		0x08
/* data register 1 */
#define XWAY_STP_CPU1		0x0C
/* access register */
#define XWAY_STP_AR		0x10

/* software or hardware update select bit
   소프트웨어 또는 하드웨어 업데이트 선택 비트	 */
#define XWAY_STP_CON_SWU	BIT(31)

/* automatic update rates
   자동 업데이트 속도				*/
#define XWAY_STP_2HZ		0
#define XWAY_STP_4HZ		BIT(23)
#define XWAY_STP_8HZ		BIT(24)
#define XWAY_STP_10HZ		(BIT(24) | BIT(23))
#define XWAY_STP_SPEED_MASK	(0xf << 23)

/* clock source for automatic update
    자동 업데이트를위한 클럭 소스			 */
#define XWAY_STP_UPD_FPI	BIT(31)
#define XWAY_STP_UPD_MASK	(BIT(31) | BIT(30))

/* let the adsl core drive the 2 LSBs
   adsl 코어가 2 LSB를 구동하게합니다 		*/
#define XWAY_STP_ADSL_SHIFT	24
#define XWAY_STP_ADSL_MASK	0x3

/* 2 groups of 3 bits can be driven by the phys 
   3 비트의 2 그룹은 phys에 의해 구동 될 수 있습니다 */
#define XWAY_STP_PHY_MASK	0x7
#define XWAY_STP_PHY1_SHIFT	27
#define XWAY_STP_PHY2_SHIFT	15

/* STP has 3 groups of 8 bits
   STP에는 8 비트의 3 그룹이 있습니다		 */
#define XWAY_STP_GROUP0		BIT(0)
#define XWAY_STP_GROUP1		BIT(1)
#define XWAY_STP_GROUP2		BIT(2)
#define XWAY_STP_GROUP_MASK	(0x7)

/* Edge configuration bits
   가장자리 구성 비트				 */
#define XWAY_STP_FALLING	BIT(26)
#define XWAY_STP_EDGE_MASK	BIT(26)

#define xway_stp_r32(m, reg)		__raw_readl(m + reg)
#define xway_stp_w32(m, val, reg)	__raw_writel(val, m + reg)
#define xway_stp_w32_mask(m, clear, set, reg) \
		ltq_w32((ltq_r32(m + reg) & ~(clear)) | (set), \
		m + reg)

struct xway_stp {
	struct gpio_chip gc;
	void __iomem *virt;
	u32 edge;	/* rising or falling edge triggered shift register 
			   상승 또는 하강 에지 트리거 시프트 레지스터		*/
	u32 shadow;	/* shadow the shift registers state 
			   섀도우 시프트 레지스터 상태				*/
	u8 groups;	/* we can drive 1-3 groups of 8bit each
		  	   우리는 각각 8 비트의 1-3 그룹을 운전할 수 있습니다 	 */
	u8 dsl;		/* the 2 LSBs can be driven by the dsl core 
			   2 LSB는 dsl 코어에 의해 구동 될 수 있습니다 		*/
	u8 phy1;	/* 3 bits can be driven by phy1
			   phy1에 의해 3 비트를 구동 할 수 있습니다		 */
	u8 phy2;	/* 3 bits can be driven by phy2 
			   phy2로 3 비트를 구동 할 수 있습니다 			*/
	u8 reserved;	/* mask out the hw driven bits in gpio_request 
			   gpio_request에서 hw 구동 비트를 마스크 아웃합니다 	*/
};

/**
 * xway_stp_set() - gpio_chip->set - set gpios.
 * @gc:     Pointer to gpio_chip device structure.
 * @gpio:   GPIO signal number.
 * @val:    Value to be written to specified signal.
 *
 * Set the shadow value and call ltq_ebu_apply.

  * xway_stp_set () - gpio_chip-> set - gpios를 설정합니다.
  * @gc : gpio_chip 장치 구조의 포인터.
  * @gpio : GPIO 신호 번호.
  * @val : 지정된 신호에 기록 할 값.
  *
  * 그림자 값을 설정하고 ltq_ebu_apply를 호출하십시오.
 */
static void xway_stp_set(struct gpio_chip *gc, unsigned gpio, int val)
{
	struct xway_stp *chip =
		container_of(gc, struct xway_stp, gc);

	if (val)
		chip->shadow |= BIT(gpio);
	else
		chip->shadow &= ~BIT(gpio);
	xway_stp_w32(chip->virt, chip->shadow, XWAY_STP_CPU0);
	xway_stp_w32_mask(chip->virt, 0, XWAY_STP_CON_SWU, XWAY_STP_CON0);
}

/**
 * xway_stp_dir_out() - gpio_chip->dir_out - set gpio direction.
 * @gc:     Pointer to gpio_chip device structure.
 * @gpio:   GPIO signal number.
 * @val:    Value to be written to specified signal.
 *
 * Same as xway_stp_set, always returns 0.

  * xway_stp_dir_out () - gpio_chip-> dir_out - gpio 방향을 설정합니다.
  * @gc : gpio_chip 장치 구조의 포인터.
  * @gpio : GPIO 신호 번호.
  * @val : 지정된 신호에 기록 할 값.
  *
  * xway_stp_set과 동일하지만 항상 0을 반환합니다.
 */
static int xway_stp_dir_out(struct gpio_chip *gc, unsigned gpio, int val)
{
	xway_stp_set(gc, gpio, val);

	return 0;
}

/**
 * xway_stp_request() - gpio_chip->request
 * @gc:     Pointer to gpio_chip device structure.
 * @gpio:   GPIO signal number.
 *
 * We mask out the HW driven pins

  * xway_stp_request () - gpio_chip-> 요청
  * @gc : gpio_chip 장치 구조의 포인터.
  * @gpio : GPIO 신호 번호.
  *
  * HW 구동 핀을 가려냅니다.
 */
static int xway_stp_request(struct gpio_chip *gc, unsigned gpio)
{
	struct xway_stp *chip =
		container_of(gc, struct xway_stp, gc);

	if ((gpio < 8) && (chip->reserved & BIT(gpio))) {
		dev_err(gc->dev, "GPIO %d is driven by hardware\n", gpio);
		return -ENODEV;
	}

	return 0;
}

/**
 * xway_stp_hw_init() - Configure the STP unit and enable the clock gate
 * @virt: pointer to the remapped register range
   
   xway_stp_hw_init () - STP 장치를 구성하고 클럭 게이트를 활성화합니다.
   @virt : 재배치 된 레지스터 범위에 대한 포인터
 */
static int xway_stp_hw_init(struct xway_stp *chip)
{
	/* sane defaults 
	   정상적인 기본값	*/
	xway_stp_w32(chip->virt, 0, XWAY_STP_AR);
	xway_stp_w32(chip->virt, 0, XWAY_STP_CPU0);
	xway_stp_w32(chip->virt, 0, XWAY_STP_CPU1);
	xway_stp_w32(chip->virt, XWAY_STP_CON_SWU, XWAY_STP_CON0);
	xway_stp_w32(chip->virt, 0, XWAY_STP_CON1);

	/* apply edge trigger settings for the shift register 
	   시프트 레지스터에 에지 트리거 설정 적용			*/
	xway_stp_w32_mask(chip->virt, XWAY_STP_EDGE_MASK,
				chip->edge, XWAY_STP_CON0);

	/* apply led group settings 
	   적용된 그룹 설정 적용					*/
	xway_stp_w32_mask(chip->virt, XWAY_STP_GROUP_MASK,
				chip->groups, XWAY_STP_CON1);

	/* tell the hardware which pins are controlled by the dsl modem 
	   어떤 핀이 dsl 모뎀에 의해 제어되는지 하드웨어에 알린다. 	*/
	xway_stp_w32_mask(chip->virt,
			XWAY_STP_ADSL_MASK << XWAY_STP_ADSL_SHIFT,
			chip->dsl << XWAY_STP_ADSL_SHIFT,
			XWAY_STP_CON0);

	/* tell the hardware which pins are controlled by the phys
	   어떤 핀이 phys에 의해 제어되는지를 하드웨어에 알려준다.	 */
	xway_stp_w32_mask(chip->virt,
			XWAY_STP_PHY_MASK << XWAY_STP_PHY1_SHIFT,
			chip->phy1 << XWAY_STP_PHY1_SHIFT,
			XWAY_STP_CON0);
	xway_stp_w32_mask(chip->virt,
			XWAY_STP_PHY_MASK << XWAY_STP_PHY2_SHIFT,
			chip->phy2 << XWAY_STP_PHY2_SHIFT,
			XWAY_STP_CON1);

	/* mask out the hw driven bits in gpio_request
	   gpio_request에서 hw 구동 비트를 마스크 아웃합니다	 */
	chip->reserved = (chip->phy2 << 5) | (chip->phy1 << 2) | chip->dsl;

	/*
	 * if we have pins that are driven by hw, we need to tell the stp what
	 * clock to use as a timer.
	   hw에 의해 구동되는 핀이 있다면, 타이머로 사용할 클럭을 stp에 알려줘야합니다.
	 */
	if (chip->reserved)
		xway_stp_w32_mask(chip->virt, XWAY_STP_UPD_MASK,
			XWAY_STP_UPD_FPI, XWAY_STP_CON1);

	return 0;
}

static int xway_stp_probe(struct platform_device *pdev)
{
	struct resource *res;
	u32 shadow, groups, dsl, phy;
	struct xway_stp *chip;
	struct clk *clk;
	int ret = 0;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	chip->virt = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(chip->virt))
		return PTR_ERR(chip->virt);

	chip->gc.dev = &pdev->dev;
	chip->gc.label = "stp-xway";
	chip->gc.direction_output = xway_stp_dir_out;
	chip->gc.set = xway_stp_set;
	chip->gc.request = xway_stp_request;
	chip->gc.base = -1;
	chip->gc.owner = THIS_MODULE;

	/* store the shadow value if one was passed by the devicetree
	   devicetree가 전달한 경우 그림자 값 저장 			 */
	if (!of_property_read_u32(pdev->dev.of_node, "lantiq,shadow", &shadow))
		chip->shadow = shadow;

	/* find out which gpio groups should be enabled 
	   어떤 gpio 그룹이 활성화되어야하는지 알아 낸다.			*/
	if (!of_property_read_u32(pdev->dev.of_node, "lantiq,groups", &groups))
		chip->groups = groups & XWAY_STP_GROUP_MASK;
	else
		chip->groups = XWAY_STP_GROUP0;
	chip->gc.ngpio = fls(chip->groups) * 8;

	/* find out which gpios are controlled by the dsl core
	   어떤 gpios가 dsl core에 의해 제어되는지 알아 낸다.		 */
	if (!of_property_read_u32(pdev->dev.of_node, "lantiq,dsl", &dsl))
		chip->dsl = dsl & XWAY_STP_ADSL_MASK;

	/* find out which gpios are controlled by the phys 
	   어떤 gpios가 phys에 의해 제어되는지 알아 내기			*/
	if (of_machine_is_compatible("lantiq,ar9") ||
			of_machine_is_compatible("lantiq,gr9") ||
			of_machine_is_compatible("lantiq,vr9")) {
		if (!of_property_read_u32(pdev->dev.of_node, "lantiq,phy1", &phy))
			chip->phy1 = phy & XWAY_STP_PHY_MASK;
		if (!of_property_read_u32(pdev->dev.of_node, "lantiq,phy2", &phy))
			chip->phy2 = phy & XWAY_STP_PHY_MASK;
	}

	/* check which edge trigger we should use, default to a falling edge
	   우리가 사용해야하는 에지 트리거를 체크하고, 디폴트는 하강 에지에 체크한다. */
	if (!of_find_property(pdev->dev.of_node, "lantiq,rising", NULL))
		chip->edge = XWAY_STP_FALLING;

	clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "Failed to get clock\n");
		return PTR_ERR(clk);
	}
	clk_enable(clk);

	ret = xway_stp_hw_init(chip);
	if (!ret)
		ret = gpiochip_add(&chip->gc);

	if (!ret)
		dev_info(&pdev->dev, "Init done\n");

	return ret;
}

static const struct of_device_id xway_stp_match[] = {
	{ .compatible = "lantiq,gpio-stp-xway" },
	{},
};
MODULE_DEVICE_TABLE(of, xway_stp_match);

static struct platform_driver xway_stp_driver = {
	.probe = xway_stp_probe,
	.driver = {
		.name = "gpio-stp-xway",
		.of_match_table = xway_stp_match,
	},
};

static int __init xway_stp_init(void)
{
	return platform_driver_register(&xway_stp_driver);
}

subsys_initcall(xway_stp_init);
