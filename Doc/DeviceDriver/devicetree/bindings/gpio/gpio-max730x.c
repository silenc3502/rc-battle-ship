/**
 * Copyright (C) 2006 Juergen Beisert, Pengutronix
 * Copyright (C) 2008 Guennadi Liakhovetski, Pengutronix
 * Copyright (C) 2009 Wolfram Sang, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * The Maxim MAX7300/1 device is an I2C/SPI driven GPIO expander. There are
 * 28 GPIOs. 8 of them can trigger an interrupt. See datasheet for more
 * details
 * Note:
 * - DIN must be stable at the rising edge of clock.
 * - when writing:
 *   - always clock in 16 clocks at once
 *   - at DIN: D15 first, D0 last
 *   - D0..D7 = databyte, D8..D14 = commandbyte
 *   - D15 = low -> write command
 * - when reading
 *   - always clock in 16 clocks at once
 *   - at DIN: D15 first, D0 last
 *   - D0..D7 = dummy, D8..D14 = register address
 *   - D15 = high -> read command
 *   - raise CS and assert it again
 *   - always clock in 16 clocks at once
 *   - at DOUT: D15 first, D0 last
 *   - D0..D7 contains the data from the first cycle
 *
 * The driver exports a standard gpiochip interface
  * Maxim MAX7300 / 1 디바이스는 I2C / SPI 기반 GPIO 확장기입니다. 28 개의 GPIO가 있습니다. 8 개가 인터럽트를 유발할 수 있습니다. 자세한 내용은 데이터 시트를 참조하십시오.
  * 노트:
  * - DIN은 클럭의 상승 에지에서 안정적이어야합니다.
  * - 글쓰기 :
  * - 항상 한 번에 16 클록 시계
  * - DIN : D15 첫 번째, D0 마지막
  * - D0..D7 = 데이터 바이트, D8..D14 = commandbyte
  * - D15 = 낮음 -> 쓰기 명령
  * - 읽을 때
  * - 항상 한 번에 16 클록 시계
  * - DIN : D15 첫 번째, D0 마지막
  * - D0..D7 = 더미, D8..D14 = 레지스터 주소
  * - D15 = 하이 -> 읽기 명령
  * - CS를 올리고 다시 선언하십시오.
  * - 항상 한 번에 16 클록 시계
  * - DOUT : D15 첫 번째, D0 마지막
  * - D0..D7에는 첫 번째주기의 데이터가 포함됩니다.
  *
  * 드라이버가 표준 gpiochip 인터페이스를 내 보냅니다.
  * /
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/spi/max7301.h>
#include <linux/gpio.h>
#include <linux/slab.h>

/*
 * Pin configurations, see MAX7301 datasheet page 6
   핀 구성, MAX7301 데이터 시트 페이지 6 참조
 */
#define PIN_CONFIG_MASK 0x03
#define PIN_CONFIG_IN_PULLUP 0x03
#define PIN_CONFIG_IN_WO_PULLUP 0x02
#define PIN_CONFIG_OUT 0x01

#define PIN_NUMBER 28

static int max7301_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct max7301 *ts = container_of(chip, struct max7301, chip);
	u8 *config;
	u8 offset_bits, pin_config;
	int ret;

	/* First 4 pins are unused in the controller 
	   처음 4 개의 핀은 컨트롤러에서 사용되지 않습니다.	*/
	offset += 4;
	offset_bits = (offset & 3) << 1;

	config = &ts->port_config[offset >> 2];

	if (ts->input_pullup_active & BIT(offset))
		pin_config = PIN_CONFIG_IN_PULLUP;
	else
		pin_config = PIN_CONFIG_IN_WO_PULLUP;

	mutex_lock(&ts->lock);

	*config = (*config & ~(PIN_CONFIG_MASK << offset_bits))
			   | (pin_config << offset_bits);

	ret = ts->write(ts->dev, 0x08 + (offset >> 2), *config);

	mutex_unlock(&ts->lock);

	return ret;
}

static int __max7301_set(struct max7301 *ts, unsigned offset, int value)
{
	if (value) {
		ts->out_level |= 1 << offset;
		return ts->write(ts->dev, 0x20 + offset, 0x01);
	} else {
		ts->out_level &= ~(1 << offset);
		return ts->write(ts->dev, 0x20 + offset, 0x00);
	}
}

static int max7301_direction_output(struct gpio_chip *chip, unsigned offset,
				    int value)
{
	struct max7301 *ts = container_of(chip, struct max7301, chip);
	u8 *config;
	u8 offset_bits;
	int ret;

	/* First 4 pins are unused in the controller
	   처음 4 개의 핀은 컨트롤러에서 사용되지 않습니다.	 */
	offset += 4;
	offset_bits = (offset & 3) << 1;

	config = &ts->port_config[offset >> 2];

	mutex_lock(&ts->lock);

	*config = (*config & ~(PIN_CONFIG_MASK << offset_bits))
			   | (PIN_CONFIG_OUT << offset_bits);

	ret = __max7301_set(ts, offset, value);

	if (!ret)
		ret = ts->write(ts->dev, 0x08 + (offset >> 2), *config);

	mutex_unlock(&ts->lock);

	return ret;
}

static int max7301_get(struct gpio_chip *chip, unsigned offset)
{
	struct max7301 *ts = container_of(chip, struct max7301, chip);
	int config, level = -EINVAL;

	/* First 4 pins are unused in the controller
	   처음 4 개의 핀은 컨트롤러에서 사용되지 않습니다.	 */
	offset += 4;

	mutex_lock(&ts->lock);

	config = (ts->port_config[offset >> 2] >> ((offset & 3) << 1))
			& PIN_CONFIG_MASK;

	switch (config) {
	case PIN_CONFIG_OUT:
		/* Output: return cached level */
		level =  !!(ts->out_level & (1 << offset));
		break;
	case PIN_CONFIG_IN_WO_PULLUP:
	case PIN_CONFIG_IN_PULLUP:
		/* Input: read out */
		level = ts->read(ts->dev, 0x20 + offset) & 0x01;
	}
	mutex_unlock(&ts->lock);

	return level;
}

static void max7301_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct max7301 *ts = container_of(chip, struct max7301, chip);

	/* First 4 pins are unused in the controller */
	offset += 4;

	mutex_lock(&ts->lock);

	__max7301_set(ts, offset, value);

	mutex_unlock(&ts->lock);
}

int __max730x_probe(struct max7301 *ts)
{
	struct device *dev = ts->dev;
	struct max7301_platform_data *pdata;
	int i, ret;

	pdata = dev_get_platdata(dev);

	mutex_init(&ts->lock);
	dev_set_drvdata(dev, ts);

	/* Power up the chip and disable IRQ output
	   칩 전원을 켜고 IRQ 출력을 비활성화하십시오.	 */
	ts->write(dev, 0x04, 0x01);

	if (pdata) {
		ts->input_pullup_active = pdata->input_pullup_active;
		ts->chip.base = pdata->base;
	} else {
		ts->chip.base = -1;
	}
	ts->chip.label = dev->driver->name;

	ts->chip.direction_input = max7301_direction_input;
	ts->chip.get = max7301_get;
	ts->chip.direction_output = max7301_direction_output;
	ts->chip.set = max7301_set;

	ts->chip.ngpio = PIN_NUMBER;
	ts->chip.can_sleep = true;
	ts->chip.dev = dev;
	ts->chip.owner = THIS_MODULE;

	/*
	 * initialize pullups according to platform data and cache the
	 * register values for later use.
	   플랫폼 데이터에 따라 풀업을 초기화하고 나중에 사용할 수 있도록 레지스터 값을 캐시하십시오.
	 */
	for (i = 1; i < 8; i++) {
		int j;
		/*
		 * initialize port_config with "0xAA", which means
		 * input with internal pullup disabled. This is needed
		 * to avoid writing zeros (in the inner for loop),
		 * which is not allowed according to the datasheet.
		   port_config를 "0xAA"로 초기화하십시오. 이는 내부 풀업이 비활성화 된 입력을 의미합니다. 
		   이것은 데이터 시트에 따라 허용되지 않는 0을 쓰는 것을 피하기 위해 필요합니다 (inner for 루프에서).
		 */
		ts->port_config[i] = 0xAA;
		for (j = 0; j < 4; j++) {
			int offset = (i - 1) * 4 + j;
			ret = max7301_direction_input(&ts->chip, offset);
			if (ret)
				goto exit_destroy;
		}
	}

	ret = gpiochip_add(&ts->chip);
	if (ret)
		goto exit_destroy;

	return ret;

exit_destroy:
	mutex_destroy(&ts->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(__max730x_probe);

int __max730x_remove(struct device *dev)
{
	struct max7301 *ts = dev_get_drvdata(dev);

	if (ts == NULL)
		return -ENODEV;

	/* Power down the chip and disable IRQ output 
	   칩의 전원을 끄고 IRQ 출력을 비활성화하십시오.	*/
	ts->write(dev, 0x04, 0x00);
	gpiochip_remove(&ts->chip);
	mutex_destroy(&ts->lock);
	return 0;
}
EXPORT_SYMBOL_GPL(__max730x_remove);

MODULE_AUTHOR("Juergen Beisert, Wolfram Sang");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MAX730x GPIO-Expanders, generic parts");
