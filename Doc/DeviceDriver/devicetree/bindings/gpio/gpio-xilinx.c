/*
 * Xilinx gpio driver for xps/axi_gpio IP.
 *
 * Copyright 2008 - 2013 Xilinx, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/slab.h>

/* Register Offset Definitions */
#define XGPIO_DATA_OFFSET   (0x0)	/* Data register  */
#define XGPIO_TRI_OFFSET    (0x4)	/* I/O direction register  */

#define XGPIO_CHANNEL_OFFSET	0x8

/* Read/Write access to the GPIO registers */
#if defined(CONFIG_ARCH_ZYNQ) || defined(CONFIG_X86)
# define xgpio_readreg(offset)		readl(offset)
# define xgpio_writereg(offset, val)	writel(val, offset)
#else
# define xgpio_readreg(offset)		__raw_readl(offset)
# define xgpio_writereg(offset, val)	__raw_writel(val, offset)
#endif

/**
 * struct xgpio_instance - Stores information about GPIO device
 * @mmchip: OF GPIO chip for memory mapped banks
 * @gpio_width: GPIO width for every channel
 * @gpio_state: GPIO state shadow register
 * @gpio_dir: GPIO direction shadow register
 * @gpio_lock: Lock used for synchronization

  * struct xgpio_instance - GPIO 디바이스에 대한 정보를 저장합니다.
  * @ mmchip : 메모리 매핑 된 뱅크 용 GPIO 칩
  * @gpio_width : 모든 채널에 대한 GPIO 너비
  * @gpio_state : GPIO 상태 쉐도우 레지스터
  * @gpio_dir : GPIO 방향 음영 레지스터
  * @gpio_lock : 동기화에 사용되는 잠금
 */
struct xgpio_instance {
	struct of_mm_gpio_chip mmchip;
	unsigned int gpio_width[2];
	u32 gpio_state[2];
	u32 gpio_dir[2];
	spinlock_t gpio_lock[2];
};

static inline int xgpio_index(struct xgpio_instance *chip, int gpio)
{
	if (gpio >= chip->gpio_width[0])
		return 1;

	return 0;
}

static inline int xgpio_regoffset(struct xgpio_instance *chip, int gpio)
{
	if (xgpio_index(chip, gpio))
		return XGPIO_CHANNEL_OFFSET;

	return 0;
}

static inline int xgpio_offset(struct xgpio_instance *chip, int gpio)
{
	if (xgpio_index(chip, gpio))
		return gpio - chip->gpio_width[0];

	return gpio;
}

/**
 * xgpio_get - Read the specified signal of the GPIO device.
 * @gc:     Pointer to gpio_chip device structure.
 * @gpio:   GPIO signal number.
 *
 * This function reads the specified signal of the GPIO device.
 *
 * Return:
 * 0 if direction of GPIO signals is set as input otherwise it
 * returns negative error value.

  * xgpio_get - GPIO 장치의 지정된 신호를 읽습니다.
  * @gc : gpio_chip 장치 구조의 포인터.
  * @gpio : GPIO 신호 번호.
  *
  *이 함수는 GPIO 장치의 지정된 신호를 읽습니다.
  *
  * 귀국 :
  * GPIO 신호의 방향이 입력으로 설정되면 0, 그렇지 않으면 음의 오류 값을 반환합니다.
 */
static int xgpio_get(struct gpio_chip *gc, unsigned int gpio)
{
	struct of_mm_gpio_chip *mm_gc = to_of_mm_gpio_chip(gc);
	struct xgpio_instance *chip =
	    container_of(mm_gc, struct xgpio_instance, mmchip);
	u32 val;

	val = xgpio_readreg(mm_gc->regs + XGPIO_DATA_OFFSET +
			    xgpio_regoffset(chip, gpio));

	return !!(val & BIT(xgpio_offset(chip, gpio)));
}

/**
 * xgpio_set - Write the specified signal of the GPIO device.
 * @gc:     Pointer to gpio_chip device structure.
 * @gpio:   GPIO signal number.
 * @val:    Value to be written to specified signal.
 *
 * This function writes the specified value in to the specified signal of the
 * GPIO device.

  * xgpio_set - GPIO 장치의 지정된 신호를 씁니다.
  * @gc : gpio_chip 장치 구조의 포인터.
  * @gpio : GPIO 신호 번호.
  * @val : 지정된 신호에 기록 할 값.
  *
  *이 함수는 지정된 값을 GPIO 장치의 지정된 신호에 씁니다.
 */
static void xgpio_set(struct gpio_chip *gc, unsigned int gpio, int val)
{
	unsigned long flags;
	struct of_mm_gpio_chip *mm_gc = to_of_mm_gpio_chip(gc);
	struct xgpio_instance *chip =
	    container_of(mm_gc, struct xgpio_instance, mmchip);
	int index =  xgpio_index(chip, gpio);
	int offset =  xgpio_offset(chip, gpio);

	spin_lock_irqsave(&chip->gpio_lock[index], flags);

	/* Write to GPIO signal and set its direction to output 
	   GPIO 신호에 쓰고 방향을 출력으로 설정				*/
	if (val)
		chip->gpio_state[index] |= BIT(offset);
	else
		chip->gpio_state[index] &= ~BIT(offset);

	xgpio_writereg(mm_gc->regs + XGPIO_DATA_OFFSET +
		       xgpio_regoffset(chip, gpio), chip->gpio_state[index]);

	spin_unlock_irqrestore(&chip->gpio_lock[index], flags);
}

/**
 * xgpio_dir_in - Set the direction of the specified GPIO signal as input.
 * @gc:     Pointer to gpio_chip device structure.
 * @gpio:   GPIO signal number.
 *
 * Return:
 * 0 - if direction of GPIO signals is set as input
 * otherwise it returns negative error value.

  * xgpio_dir_in - 지정된 GPIO 신호의 방향을 입력으로 설정합니다.
  * @gc : gpio_chip 장치 구조의 포인터.
  * @gpio : GPIO 신호 번호.
  *
  * 귀국 :
  * 0 - GPIO 신호의 방향이 입력으로 설정된 경우
  그렇지 않으면 음의 오류 값을 반환합니다.
 */
static int xgpio_dir_in(struct gpio_chip *gc, unsigned int gpio)
{
	unsigned long flags;
	struct of_mm_gpio_chip *mm_gc = to_of_mm_gpio_chip(gc);
	struct xgpio_instance *chip =
	    container_of(mm_gc, struct xgpio_instance, mmchip);
	int index =  xgpio_index(chip, gpio);
	int offset =  xgpio_offset(chip, gpio);

	spin_lock_irqsave(&chip->gpio_lock[index], flags);

	/* Set the GPIO bit in shadow register and set direction as input 
	   쉐도우 레지스터에 GPIO 비트를 설정하고 방향을 입력으로 설정		*/
	chip->gpio_dir[index] |= BIT(offset);
	xgpio_writereg(mm_gc->regs + XGPIO_TRI_OFFSET +
		       xgpio_regoffset(chip, gpio), chip->gpio_dir[index]);

	spin_unlock_irqrestore(&chip->gpio_lock[index], flags);

	return 0;
}

/**
 * xgpio_dir_out - Set the direction of the specified GPIO signal as output.
 * @gc:     Pointer to gpio_chip device structure.
 * @gpio:   GPIO signal number.
 * @val:    Value to be written to specified signal.
 *
 * This function sets the direction of specified GPIO signal as output.
 *
 * Return:
 * If all GPIO signals of GPIO chip is configured as input then it returns
 * error otherwise it returns 0.


  * xgpio_dir_out - 지정된 GPIO 신호의 방향을 출력으로 설정합니다.
  * @gc : gpio_chip 장치 구조의 포인터.
  * @gpio : GPIO 신호 번호.
  * @val : 지정된 신호에 기록 할 값.
  *
  *이 함수는 지정된 GPIO 신호의 방향을 출력으로 설정합니다.
  *
  * 귀국 :
  * GPIO 칩의 모든 GPIO 신호가 입력으로 설정된 경우 오류를 반환하고 그렇지 않으면 0을 반환합니다.
 */
static int xgpio_dir_out(struct gpio_chip *gc, unsigned int gpio, int val)
{
	unsigned long flags;
	struct of_mm_gpio_chip *mm_gc = to_of_mm_gpio_chip(gc);
	struct xgpio_instance *chip =
	    container_of(mm_gc, struct xgpio_instance, mmchip);
	int index =  xgpio_index(chip, gpio);
	int offset =  xgpio_offset(chip, gpio);

	spin_lock_irqsave(&chip->gpio_lock[index], flags);

	/* Write state of GPIO signal 
	   GPIO 신호 상태 쓰기			*/
	if (val)
		chip->gpio_state[index] |= BIT(offset);
	else
		chip->gpio_state[index] &= ~BIT(offset);
	xgpio_writereg(mm_gc->regs + XGPIO_DATA_OFFSET +
			xgpio_regoffset(chip, gpio), chip->gpio_state[index]);

	/* Clear the GPIO bit in shadow register and set direction as output 
	   섀도우 레지스터의 GPIO 비트를 지우고 방향을 출력으로 설정 		*/
	chip->gpio_dir[index] &= ~BIT(offset);
	xgpio_writereg(mm_gc->regs + XGPIO_TRI_OFFSET +
			xgpio_regoffset(chip, gpio), chip->gpio_dir[index]);

	spin_unlock_irqrestore(&chip->gpio_lock[index], flags);

	return 0;
}

/**
 * xgpio_save_regs - Set initial values of GPIO pins
 * @mm_gc: Pointer to memory mapped GPIO chip structure

  * xgpio_save_regs - GPIO 핀의 초기 값 설정
  * @ mm_gc : 메모리 매핑 된 GPIO 칩 구조에 대한 포인터
 */
static void xgpio_save_regs(struct of_mm_gpio_chip *mm_gc)
{
	struct xgpio_instance *chip =
	    container_of(mm_gc, struct xgpio_instance, mmchip);

	xgpio_writereg(mm_gc->regs + XGPIO_DATA_OFFSET,	chip->gpio_state[0]);
	xgpio_writereg(mm_gc->regs + XGPIO_TRI_OFFSET, chip->gpio_dir[0]);

	if (!chip->gpio_width[1])
		return;

	xgpio_writereg(mm_gc->regs + XGPIO_DATA_OFFSET + XGPIO_CHANNEL_OFFSET,
		       chip->gpio_state[1]);
	xgpio_writereg(mm_gc->regs + XGPIO_TRI_OFFSET + XGPIO_CHANNEL_OFFSET,
		       chip->gpio_dir[1]);
}

/**
 * xgpio_remove - Remove method for the GPIO device.
 * @pdev: pointer to the platform device
 *
 * This function remove gpiochips and frees all the allocated resources.
 *
 * Return: 0 always


  * xgpio_remove - GPIO 디바이스의 메소드를 제거합니다.
  * @pdev : 플랫폼 장치에 대한 포인터
  *
  *이 기능은 gpiochips를 제거하고 할당 된 모든 자원을 해제합니다.
  *
  * 돌아 오는 길 : 0 항상
 */
static int xgpio_remove(struct platform_device *pdev)
{
	struct xgpio_instance *chip = platform_get_drvdata(pdev);

	of_mm_gpiochip_remove(&chip->mmchip);

	return 0;
}

/**
 * xgpio_of_probe - Probe method for the GPIO device.
 * @pdev: pointer to the platform device
 *
 * Return:
 * It returns 0, if the driver is bound to the GPIO device, or
 * a negative value if there is an error.

  * xgpio_of_probe - GPIO 장치의 프로브 방법입니다.
  * @pdev : 플랫폼 장치에 대한 포인터
  *
  * 귀국 :
  * 드라이버가 GPIO 장치에 바인딩되어 있으면 0을 반환하고 오류가 있으면 음수 값을 반환합니다.
 */
static int xgpio_probe(struct platform_device *pdev)
{
	struct xgpio_instance *chip;
	int status = 0;
	struct device_node *np = pdev->dev.of_node;
	u32 is_dual;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	platform_set_drvdata(pdev, chip);

	/* Update GPIO state shadow register with default value 
	   GPIO 상태 쉐도우 레지스터를 기본값으로 업데이트			*/
	of_property_read_u32(np, "xlnx,dout-default", &chip->gpio_state[0]);

	/* Update GPIO direction shadow register with default value 
	   GPIO 방향 쉐도우 레지스터를 기본값으로 업데이트			*/
	if (of_property_read_u32(np, "xlnx,tri-default", &chip->gpio_dir[0]))
		chip->gpio_dir[0] = 0xFFFFFFFF;

	/*
	 * Check device node and parent device node for device width
	 * and assume default width of 32
	   장치 노드와 상위 장치 노드에서 장치 너비를 확인하고 기본 너비를 32로 가정합니다.
	 */
	if (of_property_read_u32(np, "xlnx,gpio-width", &chip->gpio_width[0]))
		chip->gpio_width[0] = 32;

	spin_lock_init(&chip->gpio_lock[0]);

	if (of_property_read_u32(np, "xlnx,is-dual", &is_dual))
		is_dual = 0;

	if (is_dual) {
		/* Update GPIO state shadow register with default value
	 	   GPIO 상태 쉐도우 레지스터를 기본값으로 업데이트			 */
		of_property_read_u32(np, "xlnx,dout-default-2",
				     &chip->gpio_state[1]);

		/* Update GPIO direction shadow register with default value
		   GPIO 방향 쉐도우 레지스터를 기본값으로 업데이트			 */
		if (of_property_read_u32(np, "xlnx,tri-default-2",
					 &chip->gpio_dir[1]))
			chip->gpio_dir[1] = 0xFFFFFFFF;

		/*
		 * Check device node and parent device node for device width
		 * and assume default width of 32
		  장치 노드와 상위 장치 노드에서 장치 너비를 확인하고 기본 너비를 32로 가정합니다.
		 */
		if (of_property_read_u32(np, "xlnx,gpio2-width",
					 &chip->gpio_width[1]))
			chip->gpio_width[1] = 32;

		spin_lock_init(&chip->gpio_lock[1]);
	}

	chip->mmchip.gc.ngpio = chip->gpio_width[0] + chip->gpio_width[1];
	chip->mmchip.gc.dev = &pdev->dev;
	chip->mmchip.gc.direction_input = xgpio_dir_in;
	chip->mmchip.gc.direction_output = xgpio_dir_out;
	chip->mmchip.gc.get = xgpio_get;
	chip->mmchip.gc.set = xgpio_set;

	chip->mmchip.save_regs = xgpio_save_regs;

	/* Call the OF gpio helper to setup and register the GPIO device
	   GPIO 장치를 설정하고 등록하려면 OF gpio 헬퍼를 호출하십시오.		 */
	status = of_mm_gpiochip_add(np, &chip->mmchip);
	if (status) {
		pr_err("%s: error in probe function with status %d\n",
		       np->full_name, status);
		return status;
	}

	return 0;
}

static const struct of_device_id xgpio_of_match[] = {
	{ .compatible = "xlnx,xps-gpio-1.00.a", },
	{ /* end of list */ },
};

MODULE_DEVICE_TABLE(of, xgpio_of_match);

static struct platform_driver xgpio_plat_driver = {
	.probe		= xgpio_probe,
	.remove		= xgpio_remove,
	.driver		= {
			.name = "gpio-xilinx",
			.of_match_table	= xgpio_of_match,
	},
};

static int __init xgpio_init(void)
{
	return platform_driver_register(&xgpio_plat_driver);
}

subsys_initcall(xgpio_init);

static void __exit xgpio_exit(void)
{
	platform_driver_unregister(&xgpio_plat_driver);
}
module_exit(xgpio_exit);

MODULE_AUTHOR("Xilinx, Inc.");
MODULE_DESCRIPTION("Xilinx GPIO driver");
MODULE_LICENSE("GPL");
