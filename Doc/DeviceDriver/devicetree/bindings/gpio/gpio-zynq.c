/*
 * Xilinx Zynq GPIO device driver
 *
 * Copyright (C) 2009 - 2014 Xilinx, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/gpio/driver.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>

#define DRIVER_NAME "zynq-gpio"

/* Maximum banks */
#define ZYNQ_GPIO_MAX_BANK	4
#define ZYNQMP_GPIO_MAX_BANK	6

#define ZYNQ_GPIO_BANK0_NGPIO	32
#define ZYNQ_GPIO_BANK1_NGPIO	22
#define ZYNQ_GPIO_BANK2_NGPIO	32
#define ZYNQ_GPIO_BANK3_NGPIO	32

#define ZYNQMP_GPIO_BANK0_NGPIO 26
#define ZYNQMP_GPIO_BANK1_NGPIO 26
#define ZYNQMP_GPIO_BANK2_NGPIO 26
#define ZYNQMP_GPIO_BANK3_NGPIO 32
#define ZYNQMP_GPIO_BANK4_NGPIO 32
#define ZYNQMP_GPIO_BANK5_NGPIO 32

#define	ZYNQ_GPIO_NR_GPIOS	118
#define	ZYNQMP_GPIO_NR_GPIOS	174

#define ZYNQ_GPIO_BANK0_PIN_MIN(str)	0
#define ZYNQ_GPIO_BANK0_PIN_MAX(str)	(ZYNQ_GPIO_BANK0_PIN_MIN(str) + \
					ZYNQ##str##_GPIO_BANK0_NGPIO - 1)
#define ZYNQ_GPIO_BANK1_PIN_MIN(str)	(ZYNQ_GPIO_BANK0_PIN_MAX(str) + 1)
#define ZYNQ_GPIO_BANK1_PIN_MAX(str)	(ZYNQ_GPIO_BANK1_PIN_MIN(str) + \
					ZYNQ##str##_GPIO_BANK1_NGPIO - 1)
#define ZYNQ_GPIO_BANK2_PIN_MIN(str)	(ZYNQ_GPIO_BANK1_PIN_MAX(str) + 1)
#define ZYNQ_GPIO_BANK2_PIN_MAX(str)	(ZYNQ_GPIO_BANK2_PIN_MIN(str) + \
					ZYNQ##str##_GPIO_BANK2_NGPIO - 1)
#define ZYNQ_GPIO_BANK3_PIN_MIN(str)	(ZYNQ_GPIO_BANK2_PIN_MAX(str) + 1)
#define ZYNQ_GPIO_BANK3_PIN_MAX(str)	(ZYNQ_GPIO_BANK3_PIN_MIN(str) + \
					ZYNQ##str##_GPIO_BANK3_NGPIO - 1)
#define ZYNQ_GPIO_BANK4_PIN_MIN(str)	(ZYNQ_GPIO_BANK3_PIN_MAX(str) + 1)
#define ZYNQ_GPIO_BANK4_PIN_MAX(str)	(ZYNQ_GPIO_BANK4_PIN_MIN(str) + \
					ZYNQ##str##_GPIO_BANK4_NGPIO - 1)
#define ZYNQ_GPIO_BANK5_PIN_MIN(str)	(ZYNQ_GPIO_BANK4_PIN_MAX(str) + 1)
#define ZYNQ_GPIO_BANK5_PIN_MAX(str)	(ZYNQ_GPIO_BANK5_PIN_MIN(str) + \
					ZYNQ##str##_GPIO_BANK5_NGPIO - 1)


/* Register offsets for the GPIO device
   GPIO 디바이스에 대한 레지스터 오프셋		 */
/* LSW Mask & Data -WO */
#define ZYNQ_GPIO_DATA_LSW_OFFSET(BANK)	(0x000 + (8 * BANK))
/* MSW Mask & Data -WO */
#define ZYNQ_GPIO_DATA_MSW_OFFSET(BANK)	(0x004 + (8 * BANK))
/* Data Register-RW */
#define ZYNQ_GPIO_DATA_RO_OFFSET(BANK)	(0x060 + (4 * BANK))
/* Direction mode reg-RW */
#define ZYNQ_GPIO_DIRM_OFFSET(BANK)	(0x204 + (0x40 * BANK))
/* Output enable reg-RW */
#define ZYNQ_GPIO_OUTEN_OFFSET(BANK)	(0x208 + (0x40 * BANK))
/* Interrupt mask reg-RO */
#define ZYNQ_GPIO_INTMASK_OFFSET(BANK)	(0x20C + (0x40 * BANK))
/* Interrupt enable reg-WO */
#define ZYNQ_GPIO_INTEN_OFFSET(BANK)	(0x210 + (0x40 * BANK))
/* Interrupt disable reg-WO */
#define ZYNQ_GPIO_INTDIS_OFFSET(BANK)	(0x214 + (0x40 * BANK))
/* Interrupt status reg-RO */
#define ZYNQ_GPIO_INTSTS_OFFSET(BANK)	(0x218 + (0x40 * BANK))
/* Interrupt type reg-RW */
#define ZYNQ_GPIO_INTTYPE_OFFSET(BANK)	(0x21C + (0x40 * BANK))
/* Interrupt polarity reg-RW */
#define ZYNQ_GPIO_INTPOL_OFFSET(BANK)	(0x220 + (0x40 * BANK))
/* Interrupt on any, reg-RW */
#define ZYNQ_GPIO_INTANY_OFFSET(BANK)	(0x224 + (0x40 * BANK))

/* Disable all interrupts mask */
#define ZYNQ_GPIO_IXR_DISABLE_ALL	0xFFFFFFFF

/* Mid pin number of a bank */
#define ZYNQ_GPIO_MID_PIN_NUM 16

/* GPIO upper 16 bit mask */
#define ZYNQ_GPIO_UPPER_MASK 0xFFFF0000

/**
 * struct zynq_gpio - gpio device private data structure
 * @chip:	instance of the gpio_chip
 * @base_addr:	base address of the GPIO device
 * @clk:	clock resource for this controller
 * @irq:	interrupt for the GPIO device
 * @p_data:	pointer to platform data
  * struct zynq_gpio - gpio 장치 개인 데이터 구조
  * @chip : gpio_chip의 인스턴스
  * @base_addr : GPIO 장치의 기본 주소
  * @clk :이 컨트롤러의 시계 리소스
  * @irq : GPIO 장치에 대한 인터럽트
  * @p_data : 플랫폼 데이터 포인터
 */
struct zynq_gpio {
	struct gpio_chip chip;
	void __iomem *base_addr;
	struct clk *clk;
	int irq;
	const struct zynq_platform_data *p_data;
};

/**
 * struct zynq_platform_data -  zynq gpio platform data structure
 * @label:	string to store in gpio->label
 * @ngpio:	max number of gpio pins
 * @max_bank:	maximum number of gpio banks
 * @bank_min:	this array represents bank's min pin
 * @bank_max:	this array represents bank's max pin
  * struct zynq_platform_data - zynq gpio 플랫폼 데이터 구조
  * @ 레이블 : gpio-> 레이블에 저장할 문자열
  * @ngpio : gpio 핀의 최대 수
  * @max_bank : gpio 은행의 최대 수
  * @bank_min :이 배열은 뱅크의 최소 핀을 나타냅니다.
  * @bank_max :이 배열은 뱅크의 최대 핀을 나타냅니다.
*/
struct zynq_platform_data {
	const char *label;
	u16 ngpio;
	int max_bank;
	int bank_min[ZYNQMP_GPIO_MAX_BANK];
	int bank_max[ZYNQMP_GPIO_MAX_BANK];
};

static struct irq_chip zynq_gpio_level_irqchip;
static struct irq_chip zynq_gpio_edge_irqchip;

static struct zynq_gpio *to_zynq_gpio(struct gpio_chip *gc)
{
	return container_of(gc, struct zynq_gpio, chip);
}

/**
 * zynq_gpio_get_bank_pin - Get the bank number and pin number within that bank
 * for a given pin in the GPIO device
 * @pin_num:	gpio pin number within the device
 * @bank_num:	an output parameter used to return the bank number of the gpio
 *		pin
 * @bank_pin_num: an output parameter used to return pin number within a bank
 *		  for the given gpio pin
 *
 * Returns the bank number and pin offset within the bank.
 
  * zynq_gpio_get_bank_pin - GPIO 디바이스의 해당 핀에 대해 해당 bank 내의 bank 번호와 핀 번호를 가져옵니다.
  * @ 핀 _ 번호 : 장치 내의 gpio 핀 번호
  * @bank_num : gpio 핀의 bank 번호를 반환하는 데 사용되는 출력 매개 변수
  * @bank_pin_num : 지정된 gpio 핀에 대해 bank 내의 핀 번호를 반환하는 데 사용되는 출력 매개 변수입니다.
  *
  * bank 내의 bank 번호와 핀 오프셋을 반환합니다.
 */
static inline void zynq_gpio_get_bank_pin(unsigned int pin_num,
					  unsigned int *bank_num,
					  unsigned int *bank_pin_num,
					  struct zynq_gpio *gpio)
{
	int bank;

	for (bank = 0; bank < gpio->p_data->max_bank; bank++) {
		if ((pin_num >= gpio->p_data->bank_min[bank]) &&
			(pin_num <= gpio->p_data->bank_max[bank])) {
				*bank_num = bank;
				*bank_pin_num = pin_num -
						gpio->p_data->bank_min[bank];
				return;
		}
	}

	/* default */
	WARN(true, "invalid GPIO pin number: %u", pin_num);
	*bank_num = 0;
	*bank_pin_num = 0;
}

/**
 * zynq_gpio_get_value - Get the state of the specified pin of GPIO device
 * @chip:	gpio_chip instance to be worked on
 * @pin:	gpio pin number within the device
 *
 * This function reads the state of the specified pin of the GPIO device.
 *
 * Return: 0 if the pin is low, 1 if pin is high.

  * zynq_gpio_get_value - GPIO 디바이스의 지정된 핀의 상태를 가져옵니다.
  * @chip : 작업 할 gpio_chip 인스턴스
  * @ 핀 : 장치 내의 gpio 핀 번호
  *
  *이 함수는 GPIO 디바이스의 지정된 핀의 상태를 읽습니다.
  *
  * Return : 핀이 로우이면 0, 핀이 하이이면 1.
 */
static int zynq_gpio_get_value(struct gpio_chip *chip, unsigned int pin)
{
	u32 data;
	unsigned int bank_num, bank_pin_num;
	struct zynq_gpio *gpio = to_zynq_gpio(chip);

	zynq_gpio_get_bank_pin(pin, &bank_num, &bank_pin_num, gpio);

	data = readl_relaxed(gpio->base_addr +
			     ZYNQ_GPIO_DATA_RO_OFFSET(bank_num));

	return (data >> bank_pin_num) & 1;
}

/**
 * zynq_gpio_set_value - Modify the state of the pin with specified value
 * @chip:	gpio_chip instance to be worked on
 * @pin:	gpio pin number within the device
 * @state:	value used to modify the state of the specified pin
 *
 * This function calculates the register offset (i.e to lower 16 bits or
 * upper 16 bits) based on the given pin number and sets the state of a
 * gpio pin to the specified value. The state is either 0 or non-zero.

  * zynq_gpio_set_value - 지정된 값으로 핀의 상태를 수정합니다.
  * @chip : 작업 할 gpio_chip 인스턴스
  * @ 핀 : 장치 내의 gpio 핀 번호
  * @state : 지정된 핀의 상태를 수정하는 데 사용되는 값
  *
  *이 함수는 지정된 핀 번호를 기준으로 레지스터 오프셋 (즉, 하위 16 비트 또는 상위 16 비트)을 계산하고 gpio 핀의 상태를 지정된 값으로 설정합니다. 상태는 0 또는 0이 아닙니다.
 */
static void zynq_gpio_set_value(struct gpio_chip *chip, unsigned int pin,
				int state)
{
	unsigned int reg_offset, bank_num, bank_pin_num;
	struct zynq_gpio *gpio = to_zynq_gpio(chip);

	zynq_gpio_get_bank_pin(pin, &bank_num, &bank_pin_num, gpio);

	if (bank_pin_num >= ZYNQ_GPIO_MID_PIN_NUM) {
		/* only 16 data bits in bit maskable reg */
		bank_pin_num -= ZYNQ_GPIO_MID_PIN_NUM;
		reg_offset = ZYNQ_GPIO_DATA_MSW_OFFSET(bank_num);
	} else {
		reg_offset = ZYNQ_GPIO_DATA_LSW_OFFSET(bank_num);
	}

	/*
	 * get the 32 bit value to be written to the mask/data register where
	 * the upper 16 bits is the mask and lower 16 bits is the data
	   상위 16 비트가 마스크이고 하위 16 비트가 데이터 인 마스크 / 데이터 레지스터에 기록 할 32 비트 값을 가져옵니다.
	 */
	state = !!state;
	state = ~(1 << (bank_pin_num + ZYNQ_GPIO_MID_PIN_NUM)) &
		((state << bank_pin_num) | ZYNQ_GPIO_UPPER_MASK);

	writel_relaxed(state, gpio->base_addr + reg_offset);
}

/**
 * zynq_gpio_dir_in - Set the direction of the specified GPIO pin as input
 * @chip:	gpio_chip instance to be worked on
 * @pin:	gpio pin number within the device
 *
 * This function uses the read-modify-write sequence to set the direction of
 * the gpio pin as input.
 *
 * Return: 0 always

  * zynq_gpio_dir_in - 지정된 GPIO 핀의 방향을 입력으로 설정합니다
  * @chip : 작업 할 gpio_chip 인스턴스
  * @ 핀 : 장치 내의 gpio 핀 번호
  *
  * 이 기능은 읽기 - 수정 - 쓰기 시퀀스를 사용하여 gpio 핀의 방향을 입력으로 설정합니다.
  *
  * 돌아 오는 길 : 0 항상
 */
static int zynq_gpio_dir_in(struct gpio_chip *chip, unsigned int pin)
{
	u32 reg;
	unsigned int bank_num, bank_pin_num;
	struct zynq_gpio *gpio = to_zynq_gpio(chip);

	zynq_gpio_get_bank_pin(pin, &bank_num, &bank_pin_num, gpio);

	/* bank 0 pins 7 and 8 are special and cannot be used as inputs 
	   뱅크 0 핀 7 및 8은 특수하며 입력으로 사용할 수 없습니다			*/
	if (bank_num == 0 && (bank_pin_num == 7 || bank_pin_num == 8))
		return -EINVAL;

	/* clear the bit in direction mode reg to set the pin as input 
	   핀을 입력으로 설정하기 위해 방향 모드의 비트를 클리어한다. 		*/
	reg = readl_relaxed(gpio->base_addr + ZYNQ_GPIO_DIRM_OFFSET(bank_num));
	reg &= ~BIT(bank_pin_num);
	writel_relaxed(reg, gpio->base_addr + ZYNQ_GPIO_DIRM_OFFSET(bank_num));

	return 0;
}

/**
 * zynq_gpio_dir_out - Set the direction of the specified GPIO pin as output
 * @chip:	gpio_chip instance to be worked on
 * @pin:	gpio pin number within the device
 * @state:	value to be written to specified pin
 *
 * This function sets the direction of specified GPIO pin as output, configures
 * the Output Enable register for the pin and uses zynq_gpio_set to set
 * the state of the pin to the value specified.
 *
 * Return: 0 always

  * zynq_gpio_dir_out - 지정된 GPIO 핀의 방향을 출력으로 설정합니다
  * @chip : 작업 할 gpio_chip 인스턴스
  * @ 핀 : 장치 내의 gpio 핀 번호
  * @state : 지정된 핀에 쓰여지는 값
  *
  * 이 함수는 지정된 GPIO 핀의 방향을 출력으로 설정하고, 핀에 대해 Output Enable 레지스터를 구성하고, zynq_gpio_set을 사용하여 핀의 상태를 지정된 값으로 설정합니다.
  *
  * 돌아 오는 길 : 0 항상
 */
static int zynq_gpio_dir_out(struct gpio_chip *chip, unsigned int pin,
			     int state)
{
	u32 reg;
	unsigned int bank_num, bank_pin_num;
	struct zynq_gpio *gpio = to_zynq_gpio(chip);

	zynq_gpio_get_bank_pin(pin, &bank_num, &bank_pin_num, gpio);

	/* set the GPIO pin as output
	    GPIO 핀을 출력으로 설정	 */
	reg = readl_relaxed(gpio->base_addr + ZYNQ_GPIO_DIRM_OFFSET(bank_num));
	reg |= BIT(bank_pin_num);
	writel_relaxed(reg, gpio->base_addr + ZYNQ_GPIO_DIRM_OFFSET(bank_num));

	/* configure the output enable reg for the pin
	    핀의 출력 인 에이블 (reg)을 설정한다		 */
	reg = readl_relaxed(gpio->base_addr + ZYNQ_GPIO_OUTEN_OFFSET(bank_num));
	reg |= BIT(bank_pin_num);
	writel_relaxed(reg, gpio->base_addr + ZYNQ_GPIO_OUTEN_OFFSET(bank_num));

	/* set the state of the pin 
	   핀의 상태를 설정한다. 		*/
	zynq_gpio_set_value(chip, pin, state);
	return 0;
}

/**
 * zynq_gpio_irq_mask - Disable the interrupts for a gpio pin
 * @irq_data:	per irq and chip data passed down to chip functions
 *
 * This function calculates gpio pin number from irq number and sets the
 * bit in the Interrupt Disable register of the corresponding bank to disable
 * interrupts for that pin.

  * zynq_gpio_irq_mask - GPIO 핀의 인터럽트를 비활성화합니다.
  * @irq_data : 칩 기능에 전달되는 irq 및 칩 데이터 당
  *
  * 이 함수는 irq 번호에서 gpio 핀 번호를 계산하고 해당 핀의 인터럽트를 비활성화하기 위해 해당 뱅크의 인터럽트 비활성화 레지스터의 비트를 설정합니다.
 */
static void zynq_gpio_irq_mask(struct irq_data *irq_data)
{
	unsigned int device_pin_num, bank_num, bank_pin_num;
	struct zynq_gpio *gpio =
		to_zynq_gpio(irq_data_get_irq_chip_data(irq_data));

	device_pin_num = irq_data->hwirq;
	zynq_gpio_get_bank_pin(device_pin_num, &bank_num, &bank_pin_num, gpio);
	writel_relaxed(BIT(bank_pin_num),
		       gpio->base_addr + ZYNQ_GPIO_INTDIS_OFFSET(bank_num));
}

/**
 * zynq_gpio_irq_unmask - Enable the interrupts for a gpio pin
 * @irq_data:	irq data containing irq number of gpio pin for the interrupt
 *		to enable
 *
 * This function calculates the gpio pin number from irq number and sets the
 * bit in the Interrupt Enable register of the corresponding bank to enable
 * interrupts for that pin.

  * zynq_gpio_irq_unmask - gpio 핀에 대한 인터럽트를 활성화합니다.
  * @irq_data : 인터럽트를위한 irq 번호의 gpio 핀을 포함하는 irq 데이터
  * 사용하려면
  *
  * 이 함수는 irq 번호에서 gpio 핀 번호를 계산하고 해당 핀의 인터럽트를 활성화하기 위해 해당 뱅크의 인터럽트 인 에이블 레지스터의 비트를 설정합니다.
 */
static void zynq_gpio_irq_unmask(struct irq_data *irq_data)
{
	unsigned int device_pin_num, bank_num, bank_pin_num;
	struct zynq_gpio *gpio =
		to_zynq_gpio(irq_data_get_irq_chip_data(irq_data));

	device_pin_num = irq_data->hwirq;
	zynq_gpio_get_bank_pin(device_pin_num, &bank_num, &bank_pin_num, gpio);
	writel_relaxed(BIT(bank_pin_num),
		       gpio->base_addr + ZYNQ_GPIO_INTEN_OFFSET(bank_num));
}

/**
 * zynq_gpio_irq_ack - Acknowledge the interrupt of a gpio pin
 * @irq_data:	irq data containing irq number of gpio pin for the interrupt
 *		to ack
 *
 * This function calculates gpio pin number from irq number and sets the bit
 * in the Interrupt Status Register of the corresponding bank, to ACK the irq.

  * zynq_gpio_irq_ack - gpio 핀의 인터럽트를 확인합니다.
  * @irq_data : 인터럽트를위한 irq 번호의 gpio 핀을 포함하는 irq 데이터
  * ack에
  *
  * 이 함수는 irq 번호에서 GPIO 핀 번호를 계산하고 해당 뱅크의 인터럽트 상태 레지스터에 비트를 설정하여 IRq에 ACK를 전송합니다.
 */
static void zynq_gpio_irq_ack(struct irq_data *irq_data)
{
	unsigned int device_pin_num, bank_num, bank_pin_num;
	struct zynq_gpio *gpio =
		to_zynq_gpio(irq_data_get_irq_chip_data(irq_data));

	device_pin_num = irq_data->hwirq;
	zynq_gpio_get_bank_pin(device_pin_num, &bank_num, &bank_pin_num, gpio);
	writel_relaxed(BIT(bank_pin_num),
		       gpio->base_addr + ZYNQ_GPIO_INTSTS_OFFSET(bank_num));
}

/**
 * zynq_gpio_irq_enable - Enable the interrupts for a gpio pin
 * @irq_data:	irq data containing irq number of gpio pin for the interrupt
 *		to enable
 *
 * Clears the INTSTS bit and unmasks the given interrupt.

  * zynq_gpio_irq_enable - gpio 핀에 대한 인터럽트를 활성화합니다.
  * @irq_data : irq 데이터를 포함하는 인터럽트를위한 gpio 핀을 포함
  *
  * INTSTS 비트를 클리어하고 주어진 인터럽트를 마스크 해제합니다.
 */
static void zynq_gpio_irq_enable(struct irq_data *irq_data)
{
	/*
	 * The Zynq GPIO controller does not disable interrupt detection when
	 * the interrupt is masked and only disables the propagation of the
	 * interrupt. This means when the controller detects an interrupt
	 * condition while the interrupt is logically disabled it will propagate
	 * that interrupt event once the interrupt is enabled. This will cause
	 * the interrupt consumer to see spurious interrupts to prevent this
	 * first make sure that the interrupt is not asserted and then enable
	 * it.
	
	* Zynq GPIO 컨트롤러는 인터럽트가 마스크되어있을 때 인터럽트 탐지를 비활성화하지 않고 인터럽트 전파 만 비활성화합니다. 
	즉, 인터럽트가 논리적으로 비활성화되어있는 동안 컨트롤러가 인터럽트 조건을 감지하면 인터럽트가 활성화되면 해당 인터럽트 이벤트를 전파합니다.
	이렇게하면 인터럽트 소비자가 가짜 인터럽트를보고 이로 인해 먼저 인터럽트가 어서 트되지 않았는지 확인한 다음 활성화 할 수 있습니다.
	 */
	zynq_gpio_irq_ack(irq_data);
	zynq_gpio_irq_unmask(irq_data);
}

/**
 * zynq_gpio_set_irq_type - Set the irq type for a gpio pin
 * @irq_data:	irq data containing irq number of gpio pin
 * @type:	interrupt type that is to be set for the gpio pin
 *
 * This function gets the gpio pin number and its bank from the gpio pin number
 * and configures the INT_TYPE, INT_POLARITY and INT_ANY registers.
   이 함수는 gpio 핀 번호에서 gpio 핀 번호와 해당 뱅크를 가져오고 INT_TYPE, INT_POLARITY 및 INT_ANY 레지스터를 구성합니다.
 *
 * Return: 0, negative error otherwise.
 * TYPE-EDGE_RISING,  INT_TYPE - 1, INT_POLARITY - 1,  INT_ANY - 0;
 * TYPE-EDGE_FALLING, INT_TYPE - 1, INT_POLARITY - 0,  INT_ANY - 0;
 * TYPE-EDGE_BOTH,    INT_TYPE - 1, INT_POLARITY - NA, INT_ANY - 1;
 * TYPE-LEVEL_HIGH,   INT_TYPE - 0, INT_POLARITY - 1,  INT_ANY - NA;
 * TYPE-LEVEL_LOW,    INT_TYPE - 0, INT_POLARITY - 0,  INT_ANY - NA
 */
static int zynq_gpio_set_irq_type(struct irq_data *irq_data, unsigned int type)
{
	u32 int_type, int_pol, int_any;
	unsigned int device_pin_num, bank_num, bank_pin_num;
	struct zynq_gpio *gpio =
		to_zynq_gpio(irq_data_get_irq_chip_data(irq_data));

	device_pin_num = irq_data->hwirq;
	zynq_gpio_get_bank_pin(device_pin_num, &bank_num, &bank_pin_num, gpio);

	int_type = readl_relaxed(gpio->base_addr +
				 ZYNQ_GPIO_INTTYPE_OFFSET(bank_num));
	int_pol = readl_relaxed(gpio->base_addr +
				ZYNQ_GPIO_INTPOL_OFFSET(bank_num));
	int_any = readl_relaxed(gpio->base_addr +
				ZYNQ_GPIO_INTANY_OFFSET(bank_num));

	/*
	 * based on the type requested, configure the INT_TYPE, INT_POLARITY
	 * and INT_ANY registers
	   요청 된 유형에 따라 INT_TYPE, INT_POLARITY 및 INT_ANY 레지스터를 구성하십시오
	 */
	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
		int_type |= BIT(bank_pin_num);
		int_pol |= BIT(bank_pin_num);
		int_any &= ~BIT(bank_pin_num);
		break;
	case IRQ_TYPE_EDGE_FALLING:
		int_type |= BIT(bank_pin_num);
		int_pol &= ~BIT(bank_pin_num);
		int_any &= ~BIT(bank_pin_num);
		break;
	case IRQ_TYPE_EDGE_BOTH:
		int_type |= BIT(bank_pin_num);
		int_any |= BIT(bank_pin_num);
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		int_type &= ~BIT(bank_pin_num);
		int_pol |= BIT(bank_pin_num);
		break;
	case IRQ_TYPE_LEVEL_LOW:
		int_type &= ~BIT(bank_pin_num);
		int_pol &= ~BIT(bank_pin_num);
		break;
	default:
		return -EINVAL;
	}

	writel_relaxed(int_type,
		       gpio->base_addr + ZYNQ_GPIO_INTTYPE_OFFSET(bank_num));
	writel_relaxed(int_pol,
		       gpio->base_addr + ZYNQ_GPIO_INTPOL_OFFSET(bank_num));
	writel_relaxed(int_any,
		       gpio->base_addr + ZYNQ_GPIO_INTANY_OFFSET(bank_num));

	if (type & IRQ_TYPE_LEVEL_MASK) {
		irq_set_chip_handler_name_locked(irq_data,
			&zynq_gpio_level_irqchip, handle_fasteoi_irq, NULL);
	} else {
		irq_set_chip_handler_name_locked(irq_data,
			&zynq_gpio_edge_irqchip, handle_level_irq, NULL);
	}

	return 0;
}

static int zynq_gpio_set_wake(struct irq_data *data, unsigned int on)
{
	struct zynq_gpio *gpio =
		to_zynq_gpio(irq_data_get_irq_chip_data(data));

	irq_set_irq_wake(gpio->irq, on);

	return 0;
}

/* irq chip descriptor */
static struct irq_chip zynq_gpio_level_irqchip = {
	.name		= DRIVER_NAME,
	.irq_enable	= zynq_gpio_irq_enable,
	.irq_eoi	= zynq_gpio_irq_ack,
	.irq_mask	= zynq_gpio_irq_mask,
	.irq_unmask	= zynq_gpio_irq_unmask,
	.irq_set_type	= zynq_gpio_set_irq_type,
	.irq_set_wake	= zynq_gpio_set_wake,
	.flags		= IRQCHIP_EOI_THREADED | IRQCHIP_EOI_IF_HANDLED |
			  IRQCHIP_MASK_ON_SUSPEND,
};

static struct irq_chip zynq_gpio_edge_irqchip = {
	.name		= DRIVER_NAME,
	.irq_enable	= zynq_gpio_irq_enable,
	.irq_ack	= zynq_gpio_irq_ack,
	.irq_mask	= zynq_gpio_irq_mask,
	.irq_unmask	= zynq_gpio_irq_unmask,
	.irq_set_type	= zynq_gpio_set_irq_type,
	.irq_set_wake	= zynq_gpio_set_wake,
	.flags		= IRQCHIP_MASK_ON_SUSPEND,
};

static void zynq_gpio_handle_bank_irq(struct zynq_gpio *gpio,
				      unsigned int bank_num,
				      unsigned long pending)
{
	unsigned int bank_offset = gpio->p_data->bank_min[bank_num];
	struct irq_domain *irqdomain = gpio->chip.irqdomain;
	int offset;

	if (!pending)
		return;

	for_each_set_bit(offset, &pending, 32) {
		unsigned int gpio_irq;

		gpio_irq = irq_find_mapping(irqdomain, offset + bank_offset);
		generic_handle_irq(gpio_irq);
	}
}

/**
 * zynq_gpio_irqhandler - IRQ handler for the gpio banks of a gpio device
 * @irq:	irq number of the gpio bank where interrupt has occurred
 * @desc:	irq descriptor instance of the 'irq'
 *
 * This function reads the Interrupt Status Register of each bank to get the
 * gpio pin number which has triggered an interrupt. It then acks the triggered
 * interrupt and calls the pin specific handler set by the higher layer
 * application for that pin.
 * Note: A bug is reported if no handler is set for the gpio pin.

  * zynq_gpio_irqhandler - GPIO 디바이스의 GPIO 뱅크 용 IRQ 핸들러
  * @irq : 인터럽트가 발생한 GPIO의 irq 번호
  * @desc : 'irq'의 irq 디스크립터 인스턴스
  *
  * 이 기능은 각 뱅크의 인터럽트 상태 레지스터를 읽고 인터럽트를 트리거 한 GPIO 핀 번호를 얻습니다. 
    그런 다음 트리거 된 인터럽트를 확인하고 해당 핀에 대한 상위 계층 응용 프로그램에 의해 설정된 핀 특정 핸들러를 호출합니다.
  * 참고 : gpio 핀에 대한 핸들러가 설정되지 않으면 버그가보고됩니다.
 */
static void zynq_gpio_irqhandler(struct irq_desc *desc)
{
	u32 int_sts, int_enb;
	unsigned int bank_num;
	struct zynq_gpio *gpio =
		to_zynq_gpio(irq_desc_get_handler_data(desc));
	struct irq_chip *irqchip = irq_desc_get_chip(desc);

	chained_irq_enter(irqchip, desc);

	for (bank_num = 0; bank_num < gpio->p_data->max_bank; bank_num++) {
		int_sts = readl_relaxed(gpio->base_addr +
					ZYNQ_GPIO_INTSTS_OFFSET(bank_num));
		int_enb = readl_relaxed(gpio->base_addr +
					ZYNQ_GPIO_INTMASK_OFFSET(bank_num));
		zynq_gpio_handle_bank_irq(gpio, bank_num, int_sts & ~int_enb);
	}

	chained_irq_exit(irqchip, desc);
}

static int __maybe_unused zynq_gpio_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	int irq = platform_get_irq(pdev, 0);
	struct irq_data *data = irq_get_irq_data(irq);

	if (!irqd_is_wakeup_set(data))
		return pm_runtime_force_suspend(dev);

	return 0;
}

static int __maybe_unused zynq_gpio_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	int irq = platform_get_irq(pdev, 0);
	struct irq_data *data = irq_get_irq_data(irq);

	if (!irqd_is_wakeup_set(data))
		return pm_runtime_force_resume(dev);

	return 0;
}

static int __maybe_unused zynq_gpio_runtime_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct zynq_gpio *gpio = platform_get_drvdata(pdev);

	clk_disable_unprepare(gpio->clk);

	return 0;
}

static int __maybe_unused zynq_gpio_runtime_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct zynq_gpio *gpio = platform_get_drvdata(pdev);

	return clk_prepare_enable(gpio->clk);
}

static int zynq_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	int ret;

	ret = pm_runtime_get_sync(chip->dev);

	/*
	 * If the device is already active pm_runtime_get() will return 1 on
	 * success, but gpio_request still needs to return 0.
	   * 장치가 이미 활성화되어 있으면 pm_runtime_get ()은 성공시 1을 반환하지만 gpio_request는 여전히 0을 반환해야합니다.
	 */
	return ret < 0 ? ret : 0;
}

static void zynq_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	pm_runtime_put(chip->dev);
}

static const struct dev_pm_ops zynq_gpio_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(zynq_gpio_suspend, zynq_gpio_resume)
	SET_RUNTIME_PM_OPS(zynq_gpio_runtime_suspend,
			zynq_gpio_runtime_resume, NULL)
};

static const struct zynq_platform_data zynqmp_gpio_def = {
	.label = "zynqmp_gpio",
	.ngpio = ZYNQMP_GPIO_NR_GPIOS,
	.max_bank = ZYNQMP_GPIO_MAX_BANK,
	.bank_min[0] = ZYNQ_GPIO_BANK0_PIN_MIN(MP),
	.bank_max[0] = ZYNQ_GPIO_BANK0_PIN_MAX(MP),
	.bank_min[1] = ZYNQ_GPIO_BANK1_PIN_MIN(MP),
	.bank_max[1] = ZYNQ_GPIO_BANK1_PIN_MAX(MP),
	.bank_min[2] = ZYNQ_GPIO_BANK2_PIN_MIN(MP),
	.bank_max[2] = ZYNQ_GPIO_BANK2_PIN_MAX(MP),
	.bank_min[3] = ZYNQ_GPIO_BANK3_PIN_MIN(MP),
	.bank_max[3] = ZYNQ_GPIO_BANK3_PIN_MAX(MP),
	.bank_min[4] = ZYNQ_GPIO_BANK4_PIN_MIN(MP),
	.bank_max[4] = ZYNQ_GPIO_BANK4_PIN_MAX(MP),
	.bank_min[5] = ZYNQ_GPIO_BANK5_PIN_MIN(MP),
	.bank_max[5] = ZYNQ_GPIO_BANK5_PIN_MAX(MP),
};

static const struct zynq_platform_data zynq_gpio_def = {
	.label = "zynq_gpio",
	.ngpio = ZYNQ_GPIO_NR_GPIOS,
	.max_bank = ZYNQ_GPIO_MAX_BANK,
	.bank_min[0] = ZYNQ_GPIO_BANK0_PIN_MIN(),
	.bank_max[0] = ZYNQ_GPIO_BANK0_PIN_MAX(),
	.bank_min[1] = ZYNQ_GPIO_BANK1_PIN_MIN(),
	.bank_max[1] = ZYNQ_GPIO_BANK1_PIN_MAX(),
	.bank_min[2] = ZYNQ_GPIO_BANK2_PIN_MIN(),
	.bank_max[2] = ZYNQ_GPIO_BANK2_PIN_MAX(),
	.bank_min[3] = ZYNQ_GPIO_BANK3_PIN_MIN(),
	.bank_max[3] = ZYNQ_GPIO_BANK3_PIN_MAX(),
};

static const struct of_device_id zynq_gpio_of_match[] = {
	{ .compatible = "xlnx,zynq-gpio-1.0", .data = (void *)&zynq_gpio_def },
	{ .compatible = "xlnx,zynqmp-gpio-1.0",
					.data = (void *)&zynqmp_gpio_def },
	{ /* end of table */ }
};
MODULE_DEVICE_TABLE(of, zynq_gpio_of_match);

/**
 * zynq_gpio_probe - Initialization method for a zynq_gpio device
 * @pdev:	platform device instance
 *
 * This function allocates memory resources for the gpio device and registers
 * all the banks of the device. It will also set up interrupts for the gpio
 * pins.
 * Note: Interrupts are disabled for all the banks during initialization.
 *
 * Return: 0 on success, negative error otherwise.

  * zynq_gpio_probe - zynq_gpio 장치의 초기화 메소드
  * @ pdev : 플랫폼 장치 인스턴스
  *
  * 이 기능은 gpio 장치에 대한 메모리 자원을 할당하고 장치의 모든 뱅크를 등록합니다. 
    또한 GPIO 핀에 대한 인터럽트를 설정합니다.
  * 참고 : 초기화하는 동안 모든 뱅크에 대해 인터럽트가 비활성화됩니다.
  *
  * 반환 : 성공시 0, 그렇지 않으면 음수 오류.
 */
static int zynq_gpio_probe(struct platform_device *pdev)
{
	int ret, bank_num;
	struct zynq_gpio *gpio;
	struct gpio_chip *chip;
	struct resource *res;
	const struct of_device_id *match;

	gpio = devm_kzalloc(&pdev->dev, sizeof(*gpio), GFP_KERNEL);
	if (!gpio)
		return -ENOMEM;

	match = of_match_node(zynq_gpio_of_match, pdev->dev.of_node);
	if (!match) {
		dev_err(&pdev->dev, "of_match_node() failed\n");
		return -EINVAL;
	}
	gpio->p_data = match->data;
	platform_set_drvdata(pdev, gpio);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	gpio->base_addr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(gpio->base_addr))
		return PTR_ERR(gpio->base_addr);

	gpio->irq = platform_get_irq(pdev, 0);
	if (gpio->irq < 0) {
		dev_err(&pdev->dev, "invalid IRQ\n");
		return gpio->irq;
	}

	/* configure the gpio chip */
	chip = &gpio->chip;
	chip->label = gpio->p_data->label;
	chip->owner = THIS_MODULE;
	chip->dev = &pdev->dev;
	chip->get = zynq_gpio_get_value;
	chip->set = zynq_gpio_set_value;
	chip->request = zynq_gpio_request;
	chip->free = zynq_gpio_free;
	chip->direction_input = zynq_gpio_dir_in;
	chip->direction_output = zynq_gpio_dir_out;
	chip->base = -1;
	chip->ngpio = gpio->p_data->ngpio;

	/* Enable GPIO clock */
	gpio->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(gpio->clk)) {
		dev_err(&pdev->dev, "input clock not found.\n");
		return PTR_ERR(gpio->clk);
	}
	ret = clk_prepare_enable(gpio->clk);
	if (ret) {
		dev_err(&pdev->dev, "Unable to enable clock.\n");
		return ret;
	}

	/* report a bug if gpio chip registration fails
	   GPIO 칩 등록 실패시 버그보고				 */
	ret = gpiochip_add(chip);
	if (ret) {
		dev_err(&pdev->dev, "Failed to add gpio chip\n");
		goto err_disable_clk;
	}

	/* disable interrupts for all banks
	   모든 뱅크에 대한 인터럽트 금지				 */
	for (bank_num = 0; bank_num < gpio->p_data->max_bank; bank_num++)
		writel_relaxed(ZYNQ_GPIO_IXR_DISABLE_ALL, gpio->base_addr +
			       ZYNQ_GPIO_INTDIS_OFFSET(bank_num));

	ret = gpiochip_irqchip_add(chip, &zynq_gpio_edge_irqchip, 0,
				   handle_level_irq, IRQ_TYPE_NONE);
	if (ret) {
		dev_err(&pdev->dev, "Failed to add irq chip\n");
		goto err_rm_gpiochip;
	}

	gpiochip_set_chained_irqchip(chip, &zynq_gpio_edge_irqchip, gpio->irq,
				     zynq_gpio_irqhandler);

	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	return 0;

err_rm_gpiochip:
	gpiochip_remove(chip);
err_disable_clk:
	clk_disable_unprepare(gpio->clk);

	return ret;
}

/**
 * zynq_gpio_remove - Driver removal function
 * @pdev:	platform device instance
 *
 * Return: 0 always

  * zynq_gpio_remove - 드라이버 제거 기능
  * @ pdev : 플랫폼 장치 인스턴스
  *
  * 돌아 오는 길 : 0 항상
 */
static int zynq_gpio_remove(struct platform_device *pdev)
{
	struct zynq_gpio *gpio = platform_get_drvdata(pdev);

	pm_runtime_get_sync(&pdev->dev);
	gpiochip_remove(&gpio->chip);
	clk_disable_unprepare(gpio->clk);
	device_set_wakeup_capable(&pdev->dev, 0);
	pm_runtime_disable(&pdev->dev);
	return 0;
}

static struct platform_driver zynq_gpio_driver = {
	.driver	= {
		.name = DRIVER_NAME,
		.pm = &zynq_gpio_dev_pm_ops,
		.of_match_table = zynq_gpio_of_match,
	},
	.probe = zynq_gpio_probe,
	.remove = zynq_gpio_remove,
};

/**
 * zynq_gpio_init - Initial driver registration call
 *
 * Return: value from platform_driver_register

  * zynq_gpio_init - 초기 드라이버 등록 호출
  *
  * 반환 : platform_driver_registe의 값
 */
static int __init zynq_gpio_init(void)
{
	return platform_driver_register(&zynq_gpio_driver);
}
postcore_initcall(zynq_gpio_init);

static void __exit zynq_gpio_exit(void)
{
	platform_driver_unregister(&zynq_gpio_driver);
}
module_exit(zynq_gpio_exit);

MODULE_AUTHOR("Xilinx Inc.");
MODULE_DESCRIPTION("Zynq GPIO driver");
MODULE_LICENSE("GPL");
