/*
 * MC33880 high-side/low-side switch GPIO driver
 * Copyright (c) 2009 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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

/* Supports:
 * Freescale MC33880 high-side/low-side switch
    지원 :
  * 프리 스케일 MC33880 하이 사이드 / 로우 사이드 스위치
 */

#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/spi/spi.h>
#include <linux/spi/mc33880.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/module.h>

#define DRIVER_NAME "mc33880"

/*
 * Pin configurations, see MAX7301 datasheet page 6
   핀 구성, MAX7301 데이터 시트 페이지 6 참조
 */
#define PIN_CONFIG_MASK 0x03
#define PIN_CONFIG_IN_PULLUP 0x03
#define PIN_CONFIG_IN_WO_PULLUP 0x02
#define PIN_CONFIG_OUT 0x01

#define PIN_NUMBER 8


/*
 * Some registers must be read back to modify.
 * To save time we cache them here in memory
   수정하려면 일부 레지스터를 다시 읽어야합니다.
  * 시간을 절약하기 위해 메모리에 여기에 캐시합니다.
 */
struct mc33880 {
	struct mutex	lock;	/* protect from simultaneous accesses
				   동시 액세스로부터 보호  */
	u8		port_config;
	struct gpio_chip chip;
	struct spi_device *spi;
};

static int mc33880_write_config(struct mc33880 *mc)
{
	return spi_write(mc->spi, &mc->port_config, sizeof(mc->port_config));
}


static int __mc33880_set(struct mc33880 *mc, unsigned offset, int value)
{
	if (value)
		mc->port_config |= 1 << offset;
	else
		mc->port_config &= ~(1 << offset);

	return mc33880_write_config(mc);
}


static void mc33880_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct mc33880 *mc = container_of(chip, struct mc33880, chip);

	mutex_lock(&mc->lock);

	__mc33880_set(mc, offset, value);

	mutex_unlock(&mc->lock);
}

static int mc33880_probe(struct spi_device *spi)
{
	struct mc33880 *mc;
	struct mc33880_platform_data *pdata;
	int ret;

	pdata = dev_get_platdata(&spi->dev);
	if (!pdata || !pdata->base) {
		dev_dbg(&spi->dev, "incorrect or missing platform data\n");
		return -EINVAL;
	}

	/*
	 * bits_per_word cannot be configured in platform data
	   플랫폼 데이터에 bits_per_word를 구성 할 수 없습니다.
	 */
	spi->bits_per_word = 8;

	ret = spi_setup(spi);
	if (ret < 0)
		return ret;

	mc = devm_kzalloc(&spi->dev, sizeof(struct mc33880), GFP_KERNEL);
	if (!mc)
		return -ENOMEM;

	mutex_init(&mc->lock);

	spi_set_drvdata(spi, mc);

	mc->spi = spi;

	mc->chip.label = DRIVER_NAME,
	mc->chip.set = mc33880_set;
	mc->chip.base = pdata->base;
	mc->chip.ngpio = PIN_NUMBER;
	mc->chip.can_sleep = true;
	mc->chip.dev = &spi->dev;
	mc->chip.owner = THIS_MODULE;

	mc->port_config = 0x00;
	/* write twice, because during initialisation the first setting
	 * is just for testing SPI communication, and the second is the
	 * "real" configuration
	   초기화하는 동안 첫 번째 설정은 SPI 통신을 테스트하기위한 것이고 두 번째 설정은 "실제"구성이기 때문에 두 번 쓰십시오
	 */
	ret = mc33880_write_config(mc);
	mc->port_config = 0x00;
	if (!ret)
		ret = mc33880_write_config(mc);

	if (ret) {
		dev_err(&spi->dev, "Failed writing to " DRIVER_NAME ": %d\n",
			ret);
		goto exit_destroy;
	}

	ret = gpiochip_add(&mc->chip);
	if (ret)
		goto exit_destroy;

	return ret;

exit_destroy:
	mutex_destroy(&mc->lock);
	return ret;
}

static int mc33880_remove(struct spi_device *spi)
{
	struct mc33880 *mc;

	mc = spi_get_drvdata(spi);
	if (!mc)
		return -ENODEV;

	gpiochip_remove(&mc->chip);
	mutex_destroy(&mc->lock);

	return 0;
}

static struct spi_driver mc33880_driver = {
	.driver = {
		.name		= DRIVER_NAME,
	},
	.probe		= mc33880_probe,
	.remove		= mc33880_remove,
};

static int __init mc33880_init(void)
{
	return spi_register_driver(&mc33880_driver);
}
/* register after spi postcore initcall and before
 * subsys initcalls that may rely on these GPIOs
   spi postcore initcall 이후에 그리고이 GPIO에 의존 할 수있는 서브 시스템 시작 전에 등록하십시오
 */
subsys_initcall(mc33880_init);

static void __exit mc33880_exit(void)
{
	spi_unregister_driver(&mc33880_driver);
}
module_exit(mc33880_exit);

MODULE_AUTHOR("Mocean Laboratories <info@mocean-labs.com>");
MODULE_LICENSE("GPL v2");

