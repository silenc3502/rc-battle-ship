/*
 *	Video for Linux Two
 *
 *	A generic video device interface for the LINUX operating system
 *	using a set of device structures/vectors for low level operations.
 *	LINUX 운영 체제 용 일반 비디오 장치 인터페이스 낮은 수준의 작업을 위해 일련의 장치 구조 
	/ 벡터	를 사용합니다.
 *	This file replaces the videodev.c file that comes with the
 *	regular kernel distribution.
 *	이 파일은 일반 커널 배포와 함께 제공되는 videodev.c 파일을 대체합니다.

 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 * Author:	Bill Dirks <bill@thedirks.org>
 *		based on code by Alan Cox, <alan@cymru.net>
 *
 */

/*
 * Video capture interface for Linux
 * Linux 용 비디오 캡처 인터페이스

 *	A generic video device interface for the LINUX operating system
 *	using a set of device structures/vectors for low level operations.
 *	저수준 작업을위한 일련의 장치 구조 / 벡터를 사용하는 LINUX 운영 체제 용 일반 비디오 
	장치 인터페이스.

 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Author:	Alan Cox, <alan@lxorguk.ukuu.org.uk>
 *
 * Fixes:
 */

/*
 * Video4linux 1/2 integration by Justin Schoeman
 * <justin@suntiger.ee.up.ac.za>
 * 2.4 PROCFS support ported from 2.4 kernels by
 *  Iñaki García Etxebarria <garetxe@euskalnet.net>
 * Makefile fix by "W. Michael Petullo" <mike@flyn.org>
 * 2.4 devfs support ported from 2.4 kernels by
 *  Dan Merillat <dan@merillat.org>
 * Added Gerd Knorrs v4l1 enhancements (Justin Schoeman)
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#if defined(CONFIG_SPI)
#include <linux/spi/spi.h>
#endif
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/io.h>
#include <asm/div64.h>
#include <media/v4l2-common.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>

#include <linux/videodev2.h>

MODULE_AUTHOR("Bill Dirks, Justin Schoeman, Gerd Knorr");
MODULE_DESCRIPTION("misc helper functions for v4l2 device drivers");
MODULE_LICENSE("GPL");

/*
 *
 *	V 4 L 2   D R I V E R   H E L P E R   A P I
 *
 */

/*
 *  Video Standard Operations (contributed by Michael Schimek)
	비디오 표준 업무 (Michael Schimek 제공)
 */

/* Helper functions for control handling			     */

/* Fill in a struct v4l2_queryctrl */
/* 구조체를 채우십시오. v4l2_queryctrl */
int v4l2_ctrl_query_fill(struct v4l2_queryctrl *qctrl, s32 _min, s32 _max, s32 _step, s32 _def)
{
	const char *name;
	s64 min = _min;
	s64 max = _max;
	u64 step = _step;
	s64 def = _def;

	v4l2_ctrl_fill(qctrl->id, &name, &qctrl->type,
		       &min, &max, &step, &def, &qctrl->flags);

	if (name == NULL)
		return -EINVAL;

	qctrl->minimum = min;
	qctrl->maximum = max;
	qctrl->step = step;
	qctrl->default_value = def;
	qctrl->reserved[0] = qctrl->reserved[1] = 0;
	strlcpy(qctrl->name, name, sizeof(qctrl->name));
	return 0;
}
EXPORT_SYMBOL(v4l2_ctrl_query_fill);

/* I2C Helper functions */

#if IS_ENABLED(CONFIG_I2C)

void v4l2_i2c_subdev_init(struct v4l2_subdev *sd, struct i2c_client *client,
		const struct v4l2_subdev_ops *ops)
{
	v4l2_subdev_init(sd, ops);
	sd->flags |= V4L2_SUBDEV_FL_IS_I2C;
	/* the owner is the same as the i2c_client's driver owner */
        /* 소유자가 i2c_client의 드라이버 소유자와 동일합니다 */
	sd->owner = client->dev.driver->owner;
	sd->dev = &client->dev;
	/* i2c_client and v4l2_subdev point to one another */
	/* i2c_client와 v4l2_subdev가 서로를 가리킴 */
	/* 이 두가지 함수호출은 인라인함수호출로, client를 => subdev의 *dev_priv 멤버변수에(sd->dev_priv) 넣는 작업을 수행한다. dev_priv == Pointer to private data*/
	v4l2_set_subdevdata(sd, client);
	/* 인라인함수호출 dev_set_drvdata(&dev->dev, sd) 로 대체되며, sd를 => dev의 멤버변수 driver_data에 넣는 작업을 수행한다. */
	i2c_set_clientdata(client, sd);

	/* initialize name */
        /* 이름 초기화 */
	snprintf(sd->name, sizeof(sd->name), "%s %d-%04x",
		client->dev.driver->name, i2c_adapter_id(client->adapter),
		client->addr);
}
EXPORT_SYMBOL_GPL(v4l2_i2c_subdev_init);

/* Load an i2c sub-device. */
/* i2c 하위 장치를로드하십시오. */
struct v4l2_subdev *v4l2_i2c_new_subdev_board(struct v4l2_device *v4l2_dev,
		struct i2c_adapter *adapter, struct i2c_board_info *info,
		const unsigned short *probe_addrs)
{
	struct v4l2_subdev *sd = NULL;
	struct i2c_client *client;

	BUG_ON(!v4l2_dev);

	request_module(I2C_MODULE_PREFIX "%s", info->type);

	/* Create the i2c client */
	/* i2c 클라이언트 만들기 */
	if (info->addr == 0 && probe_addrs)
		client = i2c_new_probed_device(adapter, info, probe_addrs,
					       NULL);
	else
		client = i2c_new_device(adapter, info);

	/* Note: by loading the module first we are certain that c->driver
	   will be set if the driver was found. If the module was not loaded
	   first, then the i2c core tries to delay-load the module for us,
	   and then c->driver is still NULL until the module is finally
	   loaded. This delay-load mechanism doesn't work if other drivers
	   want to use the i2c device, so explicitly loading the module
	   is the best alternative. */
	/*주의 : 먼저 모듈을 로딩하면 드라이버가 발견되며 c-> driver가 설정 될 것입니다. 
	모듈이 먼저로드되지 않으면 i2c 코어가 모듈을 지연로드하려고 시도하고 모듈이 마지막으로로드
	 될 때까지 c-> driver가 여전히 NULL입니다. 이 지연로드 메커니즘은 다른 드라이버가 i2c 장치를
	 사용하려는 경우에는 작동하지 않으므로 모듈을 명시 적으로로드하는 것이 가장 좋습니다. */
	if (client == NULL || client->dev.driver == NULL)
		goto error;

	/* Lock the module so we can safely get the v4l2_subdev pointer */
	/* 우리가 안전하게 v4l2_subdev 포인터를 얻을 수 있도록 모듈을 잠그십시오 */
	if (!try_module_get(client->dev.driver->owner))
		goto error;
	sd = i2c_get_clientdata(client);

	/* Register with the v4l2_device which increases the module's
	   use count as well. */
	/* 모듈의 사용 횟수를 증가시키는 v4l2_device에 등록하십시오. */
	if (v4l2_device_register_subdev(v4l2_dev, sd))
		sd = NULL;
	/* Decrease the module use count to match the first try_module_get. */
	/* 첫 번째 try_module_get과 일치하도록 모듈 사용 횟수를 줄입니다. */
	module_put(client->dev.driver->owner);

error:
	/* If we have a client but no subdev, then something went wrong and
	   we must unregister the client. */
	if (client && sd == NULL)
		i2c_unregister_device(client);
	return sd;
}
EXPORT_SYMBOL_GPL(v4l2_i2c_new_subdev_board);

struct v4l2_subdev *v4l2_i2c_new_subdev(struct v4l2_device *v4l2_dev,
		struct i2c_adapter *adapter, const char *client_type,
		u8 addr, const unsigned short *probe_addrs)
{
	struct i2c_board_info info;

	/* Setup the i2c board info with the device type and
	   the device address. */
	/* 우리가 클라이언트가 있지만 subdev가 없다면, 뭔가 잘못되어 
	클라이언트의 등록을 취소해야합니다. */
	memset(&info, 0, sizeof(info));
	strlcpy(info.type, client_type, sizeof(info.type));
	info.addr = addr;

	return v4l2_i2c_new_subdev_board(v4l2_dev, adapter, &info, probe_addrs);
}
EXPORT_SYMBOL_GPL(v4l2_i2c_new_subdev);

/* Return i2c client address of v4l2_subdev. */
/* v4l2_subdev의 i2c 클라이언트 주소를 반환합니다. */
unsigned short v4l2_i2c_subdev_addr(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return client ? client->addr : I2C_CLIENT_END;
}
EXPORT_SYMBOL_GPL(v4l2_i2c_subdev_addr);

/* Return a list of I2C tuner addresses to probe. Use only if the tuner
   addresses are unknown. */
/* 조사 할 I2C 튜너 주소 목록을 반환합니다. 튜너 주소를 알 수없는 경우에만 사용하십시오. */
const unsigned short *v4l2_i2c_tuner_addrs(enum v4l2_i2c_tuner_type type)
{
	static const unsigned short radio_addrs[] = {
#if IS_ENABLED(CONFIG_MEDIA_TUNER_TEA5761)
		0x10,
#endif
		0x60,
		I2C_CLIENT_END
	};
	static const unsigned short demod_addrs[] = {
		0x42, 0x43, 0x4a, 0x4b,
		I2C_CLIENT_END
	};
	static const unsigned short tv_addrs[] = {
		0x42, 0x43, 0x4a, 0x4b,		/* tda8290 */
		0x60, 0x61, 0x62, 0x63, 0x64,
		I2C_CLIENT_END
	};

	switch (type) {
	case ADDRS_RADIO:
		return radio_addrs;
	case ADDRS_DEMOD:
		return demod_addrs;
	case ADDRS_TV:
		return tv_addrs;
	case ADDRS_TV_WITH_DEMOD:
		return tv_addrs + 4;
	}
	return NULL;
}
EXPORT_SYMBOL_GPL(v4l2_i2c_tuner_addrs);

#endif /* defined(CONFIG_I2C) */

#if defined(CONFIG_SPI)

/* Load an spi sub-device. */

void v4l2_spi_subdev_init(struct v4l2_subdev *sd, struct spi_device *spi,
		const struct v4l2_subdev_ops *ops)
{
	v4l2_subdev_init(sd, ops);
	sd->flags |= V4L2_SUBDEV_FL_IS_SPI;
	/* the owner is the same as the spi_device's driver owner */
	/* 소유자가 spi_device의 드라이버 소유자와 동일합니다 */
	sd->owner = spi->dev.driver->owner;
	sd->dev = &spi->dev;
	/* spi_device and v4l2_subdev point to one another */
	/* spi_device와 v4l2_subdev가 서로를 가리킴 */
	v4l2_set_subdevdata(sd, spi);
	spi_set_drvdata(spi, sd);
	/* initialize name */
	strlcpy(sd->name, spi->dev.driver->name, sizeof(sd->name));
}
EXPORT_SYMBOL_GPL(v4l2_spi_subdev_init);

struct v4l2_subdev *v4l2_spi_new_subdev(struct v4l2_device *v4l2_dev,
		struct spi_master *master, struct spi_board_info *info)
{
	struct v4l2_subdev *sd = NULL;
	struct spi_device *spi = NULL;

	BUG_ON(!v4l2_dev);

	if (info->modalias[0])
		request_module(info->modalias);

	spi = spi_new_device(master, info);

	if (spi == NULL || spi->dev.driver == NULL)
		goto error;

	if (!try_module_get(spi->dev.driver->owner))
		goto error;

	sd = spi_get_drvdata(spi);

	/* Register with the v4l2_device which increases the module's
	   use count as well. */
	/* 모듈의 사용 횟수를 증가시키는 v4l2_device에 등록하십시오. */
	if (v4l2_device_register_subdev(v4l2_dev, sd))
		sd = NULL;

	/* Decrease the module use count to match the first try_module_get. */
	/* 첫 번째 try_module_get과 일치하도록 모듈 사용 횟수를 줄입니다. */
	module_put(spi->dev.driver->owner);

error:
	/* If we have a client but no subdev, then something went wrong and
	   we must unregister the client. */
	/* 우리가 클라이언트가 있지만 subdev가 없다면, 뭔가 잘못되어 클라이언트의 등록을 취소해야합니다. */
	if (spi && sd == NULL)
		spi_unregister_device(spi);

	return sd;
}
EXPORT_SYMBOL_GPL(v4l2_spi_new_subdev);

#endif /* defined(CONFIG_SPI) */

/* Clamp x to be between min and max, aligned to a multiple of 2^align.  min
 * and max don't have to be aligned, but there must be at least one valid
 * value.  E.g., min=17,max=31,align=4 is not allowed as there are no multiples
 * of 16 between 17 and 31.  */
/* x를 최소값과 최대 값 사이에 두어 2의 배수로 정렬합니다. 최소 및 최대 값은 정렬 할 필요는
 없지만 하나 이상의 유효한 값이 있어야합니다. 17과 31 사이에 16의 배수가 없으므로 E.g., min = 17, 
max = 31, align = 4는 허용되지 않습니다. */

static unsigned int clamp_align(unsigned int x, unsigned int min,
				unsigned int max, unsigned int align)
{
	/* Bits that must be zero to be aligned */
	/* 정렬되기 위해 0이어야하는 비트 */
	unsigned int mask = ~((1 << align) - 1);

	/* Clamp to aligned min and max */
	/* 정렬 된 최소 및 최대 클램프 */
	x = clamp(x, (min + ~mask) & mask, max & mask);

	/* Round to nearest aligned value */
	/* 가장 가까운 정렬 값으로 반올림 */
	if (align)
		x = (x + (1 << (align - 1))) & mask;

	return x;
}

/* Bound an image to have a width between wmin and wmax, and height between
 * hmin and hmax, inclusive.  Additionally, the width will be a multiple of
 * 2^walign, the height will be a multiple of 2^halign, and the overall size
 * (width*height) will be a multiple of 2^salign.  The image may be shrunk
 * or enlarged to fit the alignment constraints.
 *
 * The width or height maximum must not be smaller than the corresponding
 * minimum.  The alignments must not be so high there are no possible image
 * sizes within the allowed bounds.  wmin and hmin must be at least 1
 * (don't use 0).  If you don't care about a certain alignment, specify 0,
 * as 2^0 is 1 and one byte alignment is equivalent to no alignment.  If
 * you only want to adjust downward, specify a maximum that's the same as
 * the initial value.
 */
/* 이미지를 wmin과 wmax 사이의 폭으로, hmin과 hmax 사이의 높이로 바인딩합니다. 또한 
너비는 2 ^ walign의 배수가되고 높이는 2 ^ halign의 배수가되며 전체 크기 (너비 * 높이)는 
2의 배수가됩니다. 정렬 제약 조건에 맞게 이미지가 축소되거나 확대 될 수 있습니다.
   너비 또는 높이 최대 값은 해당 최소값보다 작아서는 안됩니다. 정렬은 너무 높아서는 안되며 
허용 된 범위 내에 가능한 이미지 크기가 없습니다. wmin과 hmin은 적어도 1이어야합니다 (0은 사용하지 마십시오). 
특정 정렬에 대해 신경 쓰지 않으면 2 ^ 0이 1이고 1 바이트 정렬이 정렬 없음과 동일하므로 0을 지정하십시오. 
아래쪽으로 만 조정하려면 초기 값과 동일한 최대 값을 지정하십시오.
  */

void v4l_bound_align_image(u32 *w, unsigned int wmin, unsigned int wmax,
			   unsigned int walign,
			   u32 *h, unsigned int hmin, unsigned int hmax,
			   unsigned int halign, unsigned int salign)
{
	*w = clamp_align(*w, wmin, wmax, walign);
	*h = clamp_align(*h, hmin, hmax, halign);

	/* Usually we don't need to align the size and are done now. */
	/* 보통 우리는 크기를 조정할 필요가 없으며 지금 완료되었습니다. */
	if (!salign)
		return;

	/* How much alignment do we have? */
	/* 우리는 얼마나 정렬되어 있습니까? */
	walign = __ffs(*w);
	halign = __ffs(*h);
	/* Enough to satisfy the image alignment? */
	/* 이미지 정렬을 만족시키기에 충분합니까? */
	if (walign + halign < salign) {
		/* Max walign where there is still a valid width */
		/* 여전히 유효한 너비가있는 곳의 최대 길이 */
		unsigned int wmaxa = __fls(wmax ^ (wmin - 1));
		/* Max halign where there is still a valid height */
		/* 여전히 유효한 높이가있는 곳의 최대 halign */
		unsigned int hmaxa = __fls(hmax ^ (hmin - 1));

		/* up the smaller alignment until we have enough */
		/* 우리가 충분할 때까지 더 작은 정렬을 올리십시오 */
		do {
			if (halign >= hmaxa ||
			    (walign <= halign && walign < wmaxa)) {
				*w = clamp_align(*w, wmin, wmax, walign + 1);
				walign = __ffs(*w);
			} else {
				*h = clamp_align(*h, hmin, hmax, halign + 1);
				halign = __ffs(*h);
			}
		} while (halign + walign < salign);
	}
}
EXPORT_SYMBOL_GPL(v4l_bound_align_image);

const struct v4l2_frmsize_discrete *v4l2_find_nearest_format(
		const struct v4l2_discrete_probe *probe,
		s32 width, s32 height)
{
	int i;
	u32 error, min_error = UINT_MAX;
	const struct v4l2_frmsize_discrete *size, *best = NULL;

	if (!probe)
		return best;

	for (i = 0, size = probe->sizes; i < probe->num_sizes; i++, size++) {
		error = abs(size->width - width) + abs(size->height - height);
		if (error < min_error) {
			min_error = error;
			best = size;
		}
		if (!error)
			break;
	}

	return best;
}
EXPORT_SYMBOL_GPL(v4l2_find_nearest_format);

void v4l2_get_timestamp(struct timeval *tv)
{
	struct timespec ts;

	ktime_get_ts(&ts);
	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec / NSEC_PER_USEC;
}
EXPORT_SYMBOL_GPL(v4l2_get_timestamp);
