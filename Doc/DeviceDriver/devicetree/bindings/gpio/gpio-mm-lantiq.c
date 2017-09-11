/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2012 John Crispin <blogic@openwrt.org>
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/slab.h>

#include <lantiq_soc.h>

/*
 * By attaching hardware latches to the EBU it is possible to create output
 * only gpios. This driver configures a special memory address, which when
 * written to outputs 16 bit to the latches.
   하드웨어 래치를 EBU에 연결하면 gpios 만 출력 할 수 있습니다. 이 드라이버는 래치에 16 비트를 출력하도록 작성된 특수 메모리 주소를 구성합니다.
 */

#define LTQ_EBU_BUSCON	0x1e7ff		/* 16 bit access, slowest timing 
					   16 비트 액세스, 가장 느린 타이밍	*/
#define LTQ_EBU_WP	0x80000000	/* write protect bit 
					    쓰기 방지 비트			*/

struct ltq_mm {
	struct of_mm_gpio_chip mmchip;
	u16 shadow;	/* shadow the latches state */
};

/**
 * ltq_mm_apply() - write the shadow value to the ebu address.
 * @chip:     Pointer to our private data structure.
 *
 * Write the shadow value to the EBU to set the gpios. We need to set the
 * global EBU lock to make sure that PCI/MTD dont break.
  * ltq_mm_apply () - 그림자 값을 ebu 주소에 씁니다.
  * @chip : 개인 데이터 구조의 포인터.
  *
  * 섀도우 값을 EBU에 써서 gpios를 설정하십시오. 우리는
  PCI / MTD가 중단되지 않도록 전역 EBU 잠금.
 */
static void ltq_mm_apply(struct ltq_mm *chip)
{
	unsigned long flags;

	spin_lock_irqsave(&ebu_lock, flags);
	ltq_ebu_w32(LTQ_EBU_BUSCON, LTQ_EBU_BUSCON1);
	__raw_writew(chip->shadow, chip->mmchip.regs);
	ltq_ebu_w32(LTQ_EBU_BUSCON | LTQ_EBU_WP, LTQ_EBU_BUSCON1);
	spin_unlock_irqrestore(&ebu_lock, flags);
}

/**
 * ltq_mm_set() - gpio_chip->set - set gpios.
 * @gc:     Pointer to gpio_chip device structure.
 * @gpio:   GPIO signal number.
 * @val:    Value to be written to specified signal.
 *
 * Set the shadow value and call ltq_mm_apply.
  * ltq_mm_set () - gpio_chip-> set - gpios를 설정합니다.
  * @gc : gpio_chip 장치 구조의 포인터.
  * @gpio : GPIO 신호 번호.
  * @val : 지정된 신호에 기록 할 값.
  *
  그림자 값을 설정하고 ltq_mm_apply를 호출하십시오.
 */
static void ltq_mm_set(struct gpio_chip *gc, unsigned offset, int value)
{
	struct of_mm_gpio_chip *mm_gc = to_of_mm_gpio_chip(gc);
	struct ltq_mm *chip =
		container_of(mm_gc, struct ltq_mm, mmchip);

	if (value)
		chip->shadow |= (1 << offset);
	else
		chip->shadow &= ~(1 << offset);
	ltq_mm_apply(chip);
}

/**
 * ltq_mm_dir_out() - gpio_chip->dir_out - set gpio direction.
 * @gc:     Pointer to gpio_chip device structure.
 * @gpio:   GPIO signal number.
 * @val:    Value to be written to specified signal.
 *
 * Same as ltq_mm_set, always returns 0.
  * ltq_mm_dir_out () - gpio_chip-> dir_out - gpio 방향을 설정합니다.
  * @gc : gpio_chip 장치 구조의 포인터.
  * @gpio : GPIO 신호 번호.
  * @val : 지정된 신호에 기록 할 값.
  *
  * ltq_mm_set과 같으며 항상 0을 반환합니다.
 */
static int ltq_mm_dir_out(struct gpio_chip *gc, unsigned offset, int value)
{
	ltq_mm_set(gc, offset, value);

	return 0;
}

/**
 * ltq_mm_save_regs() - Set initial values of GPIO pins
 * @mm_gc: pointer to memory mapped GPIO chip structure
  * ltq_mm_save_regs () - GPIO 핀의 초기 값을 설정합니다.
  * @ mm_gc : 메모리 매핑 된 GPIO 칩 구조에 대한 포인터
 */
static void ltq_mm_save_regs(struct of_mm_gpio_chip *mm_gc)
{
	struct ltq_mm *chip =
		container_of(mm_gc, struct ltq_mm, mmchip);

	/* tell the ebu controller which memory address we will be using
	   우리가 사용할 메모리 주소를 ebu 컨트롤러에 알려주십시오.		 */
	ltq_ebu_w32(CPHYSADDR(chip->mmchip.regs) | 0x1, LTQ_EBU_ADDRSEL1);

	ltq_mm_apply(chip);
}

static int ltq_mm_probe(struct platform_device *pdev)
{
	struct ltq_mm *chip;
	u32 shadow;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	platform_set_drvdata(pdev, chip);

	chip->mmchip.gc.ngpio = 16;
	chip->mmchip.gc.direction_output = ltq_mm_dir_out;
	chip->mmchip.gc.set = ltq_mm_set;
	chip->mmchip.save_regs = ltq_mm_save_regs;

	/* store the shadow value if one was passed by the devicetree
	   하나가 devicetree에 의해 전달 된 경우 그림자 값을 저장		 */
	if (!of_property_read_u32(pdev->dev.of_node, "lantiq,shadow", &shadow))
		chip->shadow = shadow;

	return of_mm_gpiochip_add(pdev->dev.of_node, &chip->mmchip);
}

static int ltq_mm_remove(struct platform_device *pdev)
{
	struct ltq_mm *chip = platform_get_drvdata(pdev);

	of_mm_gpiochip_remove(&chip->mmchip);

	return 0;
}

static const struct of_device_id ltq_mm_match[] = {
	{ .compatible = "lantiq,gpio-mm" },
	{},
};
MODULE_DEVICE_TABLE(of, ltq_mm_match);

static struct platform_driver ltq_mm_driver = {
	.probe = ltq_mm_probe,
	.remove = ltq_mm_remove,
	.driver = {
		.name = "gpio-mm-ltq",
		.of_match_table = ltq_mm_match,
	},
};

static int __init ltq_mm_init(void)
{
	return platform_driver_register(&ltq_mm_driver);
}

subsys_initcall(ltq_mm_init);

static void __exit ltq_mm_exit(void)
{
	platform_driver_unregister(&ltq_mm_driver);
}
module_exit(ltq_mm_exit);
