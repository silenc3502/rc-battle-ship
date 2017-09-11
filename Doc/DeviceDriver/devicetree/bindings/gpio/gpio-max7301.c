/*
 * Copyright (C) 2006 Juergen Beisert, Pengutronix
 * Copyright (C) 2008 Guennadi Liakhovetski, Pengutronix
 * Copyright (C) 2009 Wolfram Sang, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Check max730x.c for further details.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/spi/max7301.h>

/* A write to the MAX7301 means one message with one transfer
   MAX7301에 대한 쓰기는 하나의 메시지가 하나의 전송		 */
static int max7301_spi_write(struct device *dev, unsigned int reg,
				unsigned int val)
{
	struct spi_device *spi = to_spi_device(dev);
	u16 word = ((reg & 0x7F) << 8) | (val & 0xFF);

	return spi_write(spi, (const u8 *)&word, sizeof(word));
}

/* A read from the MAX7301 means two transfers; here, one message each 
   MAX7301에서 읽은 것은 2 개의 전송을 의미한다. 여기, 각각 하나의 메시지 	*/

static int max7301_spi_read(struct device *dev, unsigned int reg)
{
	int ret;
	u16 word;
	struct spi_device *spi = to_spi_device(dev);

	word = 0x8000 | (reg << 8);
	ret = spi_write(spi, (const u8 *)&word, sizeof(word));
	if (ret)
		return ret;
	/*
	 * This relies on the fact, that a transfer with NULL tx_buf shifts out
	 * zero bytes (=NOOP for MAX7301)
 	   이것은 NULL tx_buf가있는 전송이 0 바이트를 이동한다는 사실에 달려 있습니다 	   (MAX7301의 경우 = NOOP)
	 */
	ret = spi_read(spi, (u8 *)&word, sizeof(word));
	if (ret)
		return ret;
	return word & 0xff;
}

static int max7301_probe(struct spi_device *spi)
{
	struct max7301 *ts;
	int ret;

	/* bits_per_word cannot be configured in platform data 
	   플랫폼 데이터에 bits_per_word를 구성 할 수 없습니다.	*/
	spi->bits_per_word = 16;
	ret = spi_setup(spi);
	if (ret < 0)
		return ret;

	ts = devm_kzalloc(&spi->dev, sizeof(struct max7301), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	ts->read = max7301_spi_read;
	ts->write = max7301_spi_write;
	ts->dev = &spi->dev;

	ret = __max730x_probe(ts);
	return ret;
}

static int max7301_remove(struct spi_device *spi)
{
	return __max730x_remove(&spi->dev);
}

static const struct spi_device_id max7301_id[] = {
	{ "max7301", 0 },
	{ }
};
MODULE_DEVICE_TABLE(spi, max7301_id);

static struct spi_driver max7301_driver = {
	.driver = {
		.name = "max7301",
	},
	.probe = max7301_probe,
	.remove = max7301_remove,
	.id_table = max7301_id,
};

static int __init max7301_init(void)
{
	return spi_register_driver(&max7301_driver);
}
/* register after spi postcore initcall and before
 * subsys initcalls that may rely on these GPIOs
   spi postcore initcall 이후에 그리고이 GPIO에 의존 할 수있는 서브 시스템 시작 전에 등록하십시오
 */
subsys_initcall(max7301_init);

static void __exit max7301_exit(void)
{
	spi_unregister_driver(&max7301_driver);
}
module_exit(max7301_exit);

MODULE_AUTHOR("Juergen Beisert, Wolfram Sang");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MAX7301 GPIO-Expander");
