/*
 * OF helpers for the GPIO API
 *
 * Copyright (c) 2007-2008  MontaVista Software, Inc.
 *
 * Author: Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/slab.h>
#include <linux/gpio/machine.h>

#include "gpiolib.h"

/* Private data structure for of_gpiochip_find_and_xlate
  of_gpiochip_find_and_xlate에 대한 개인 데이터 구조 		 */
struct gg_data {
	enum of_gpio_flags *flags;
	struct of_phandle_args gpiospec;

	struct gpio_desc *out_gpio;
};

/* Private function for resolving node pointer to gpio_chip
    gpio_chip에 대한 노드 포인터를 해석하는 개인 함수 		 */
static int of_gpiochip_find_and_xlate(struct gpio_chip *gc, void *data)
{
	struct gg_data *gg_data = data;
	int ret;

	if ((gc->of_node != gg_data->gpiospec.np) ||
	    (gc->of_gpio_n_cells != gg_data->gpiospec.args_count) ||
	    (!gc->of_xlate))
		return false;

	ret = gc->of_xlate(gc, &gg_data->gpiospec, gg_data->flags);
	if (ret < 0) {
		/* We've found a gpio chip, but the translation failed.
		 * Store translation error in out_gpio.
		 * Return false to keep looking, as more than one gpio chip
		 * could be registered per of-node.
	  	 * gpio 칩을 발견했지만 번역에 실패했습니다.
		 * out_gpio에 상점 번역 오류가 있습니다.
		 * 노드 당 하나 이상의 gpio 칩을 등록 할 수 있으므로 계속보고하려면 false를 반환합니다.
		 */
		gg_data->out_gpio = ERR_PTR(ret);
		return false;
	 }

	gg_data->out_gpio = gpiochip_get_desc(gc, ret);
	return true;
}

/**
 * of_get_named_gpiod_flags() - Get a GPIO descriptor and flags for GPIO API
 * @np:		device node to get GPIO from
 * @propname:	property name containing gpio specifier(s)
 * @index:	index of the GPIO
 * @flags:	a flags pointer to fill in
 *
 * Returns GPIO descriptor to use with Linux GPIO API, or one of the errno
 * value on the error condition. If @flags is not NULL the function also fills
 * in flags for the GPIO.

  * of_get_named_gpiod_flags () - GPIO API에 대한 GPIO 설명자 및 플래그 가져 오기
  * @np : GPIO를 얻을 장치 노드
  * @propname : gpio 지정자를 포함하는 속성 이름
  * @ 인덱스 : GPIO 인덱스
  * @flags : 채울 플래그 포인터
  *
  * Linux GPIO API와 함께 사용할 GPIO 설명자를 반환하거나 오류 조건의 errno 값 중 하나를 반환합니다. @flags가 NULL이 아닌 경우 함수는 GPIO 플래그도 채 웁니다.
 */
struct gpio_desc *of_get_named_gpiod_flags(struct device_node *np,
		     const char *propname, int index, enum of_gpio_flags *flags)
{
	/* Return -EPROBE_DEFER to support probe() functions to be called
	 * later when the GPIO actually becomes available
	   GPIO가 실제로 사용 가능할 때 나중에 호출 할 probe () 함수를 지원하는 -EPROBE_DEFER를 반환합니다.
	 */
	struct gg_data gg_data = {
		.flags = flags,
		.out_gpio = ERR_PTR(-EPROBE_DEFER)
	};
	int ret;

	/* .of_xlate might decide to not fill in the flags, so clear it. 
	    .of_xlate는 플래그를 채우지 않기로 결정할 수 있습니다.			*/
	if (flags)
		*flags = 0;

	ret = of_parse_phandle_with_args(np, propname, "#gpio-cells", index,
					 &gg_data.gpiospec);
	if (ret) {
		pr_debug("%s: can't parse '%s' property of node '%s[%d]'\n",
			__func__, propname, np->full_name, index);
		return ERR_PTR(ret);
	}

	gpiochip_find(&gg_data, of_gpiochip_find_and_xlate);

	of_node_put(gg_data.gpiospec.np);
	pr_debug("%s: parsed '%s' property of node '%s[%d]' - status (%d)\n",
		 __func__, propname, np->full_name, index,
		 PTR_ERR_OR_ZERO(gg_data.out_gpio));
	return gg_data.out_gpio;
}

int of_get_named_gpio_flags(struct device_node *np, const char *list_name,
			    int index, enum of_gpio_flags *flags)
{
	struct gpio_desc *desc;

	desc = of_get_named_gpiod_flags(np, list_name, index, flags);

	if (IS_ERR(desc))
		return PTR_ERR(desc);
	else
		return desc_to_gpio(desc);
}
EXPORT_SYMBOL(of_get_named_gpio_flags);

/**
 * of_parse_own_gpio() - Get a GPIO hog descriptor, names and flags for GPIO API
 * @np:		device node to get GPIO from
 * @name:	GPIO line name
 * @lflags:	gpio_lookup_flags - returned from of_find_gpio() or
 *		of_parse_own_gpio()
 * @dflags:	gpiod_flags - optional GPIO initialization flags
 *
 * Returns GPIO descriptor to use with Linux GPIO API, or one of the errno
 * value on the error condition.

  * of_parse_own_gpio () - GPIO API에 대한 GPIO 돼지 설명자, 이름 및 플래그 가져 오기
  * @np : GPIO를 얻을 장치 노드
  * @name : GPIO 라인 이름
  * @lflags : gpio_lookup_flags - of_find_gpio () 또는에서 반환되었습니다.
  * of_parse_own_gpio ()
  * @dflags : gpiod_flags - 선택적 GPIO 초기화 플래그
  *
  * Linux GPIO API와 함께 사용할 GPIO 설명자를 반환하거나 오류 조건의 errno 값 중 하나를 반환합니다.
 */
static struct gpio_desc *of_parse_own_gpio(struct device_node *np,
					   const char **name,
					   enum gpio_lookup_flags *lflags,
					   enum gpiod_flags *dflags)
{
	struct device_node *chip_np;
	enum of_gpio_flags xlate_flags;
	struct gg_data gg_data = {
		.flags = &xlate_flags,
	};
	u32 tmp;
	int i, ret;

	chip_np = np->parent;
	if (!chip_np)
		return ERR_PTR(-EINVAL);

	xlate_flags = 0;
	*lflags = 0;
	*dflags = 0;

	ret = of_property_read_u32(chip_np, "#gpio-cells", &tmp);
	if (ret)
		return ERR_PTR(ret);

	if (tmp > MAX_PHANDLE_ARGS)
		return ERR_PTR(-EINVAL);

	gg_data.gpiospec.args_count = tmp;
	gg_data.gpiospec.np = chip_np;
	for (i = 0; i < tmp; i++) {
		ret = of_property_read_u32_index(np, "gpios", i,
					   &gg_data.gpiospec.args[i]);
		if (ret)
			return ERR_PTR(ret);
	}

	gpiochip_find(&gg_data, of_gpiochip_find_and_xlate);
	if (!gg_data.out_gpio) {
		if (np->parent == np)
			return ERR_PTR(-ENXIO);
		else
			return ERR_PTR(-EINVAL);
	}

	if (xlate_flags & OF_GPIO_ACTIVE_LOW)
		*lflags |= GPIO_ACTIVE_LOW;

	if (of_property_read_bool(np, "input"))
		*dflags |= GPIOD_IN;
	else if (of_property_read_bool(np, "output-low"))
		*dflags |= GPIOD_OUT_LOW;
	else if (of_property_read_bool(np, "output-high"))
		*dflags |= GPIOD_OUT_HIGH;
	else {
		pr_warn("GPIO line %d (%s): no hogging state specified, bailing out\n",
			desc_to_gpio(gg_data.out_gpio), np->name);
		return ERR_PTR(-EINVAL);
	}

	if (name && of_property_read_string(np, "line-name", name))
		*name = np->name;

	return gg_data.out_gpio;
}

/**
 * of_gpiochip_scan_gpios - Scan gpio-controller for gpio definitions
 * @chip:	gpio chip to act on
 *
 * This is only used by of_gpiochip_add to request/set GPIO initial
 * configuration.

 * of_gpiochip_scan_gpios - gpio 정의에 대한 gpio-controller 검색
 * @chip : gpio 칩이 작동합니다.
 *
 * 이것은 of_gpiochip_add가 GPIO 초기 설정을 요청 / 설정하는 용도로만 사용됩니다.
 */
static void of_gpiochip_scan_gpios(struct gpio_chip *chip)
{
	struct gpio_desc *desc = NULL;
	struct device_node *np;
	const char *name;
	enum gpio_lookup_flags lflags;
	enum gpiod_flags dflags;

	for_each_child_of_node(chip->of_node, np) {
		if (!of_property_read_bool(np, "gpio-hog"))
			continue;

		desc = of_parse_own_gpio(np, &name, &lflags, &dflags);
		if (IS_ERR(desc))
			continue;

		if (gpiod_hog(desc, name, lflags, dflags))
			continue;
	}
}

/**
 * of_gpio_simple_xlate - translate gpio_spec to the GPIO number and flags
 * @gc:		pointer to the gpio_chip structure
 * @np:		device node of the GPIO chip
 * @gpio_spec:	gpio specifier as found in the device tree
 * @flags:	a flags pointer to fill in
 *
 * This is simple translation function, suitable for the most 1:1 mapped
 * gpio chips. This function performs only one sanity check: whether gpio
 * is less than ngpios (that is specified in the gpio_chip).

 * of_gpio_simple_xlate - gpio_spec을 GPIO 번호 및 플래그로 변환합니다.
 * @gc : gpio_chip 구조체에 대한 포인터
 * @np : GPIO 칩의 디바이스 노드
 * @gpio_spec : 장치 트리에있는 gpio 지정자
 * @flags : 채울 플래그 포인터
 *
 * 이것은 1 : 1 매핑 된 gpio 칩에 가장 적합한 간단한 변환 기능입니다. 이 함수는 gpio가 ngpios (gpio_chip에 지정된 값)보다 작은 지 여부를 한 번만 점검합니다.
 */
int of_gpio_simple_xlate(struct gpio_chip *gc,
			 const struct of_phandle_args *gpiospec, u32 *flags)
{
	/*
	 * We're discouraging gpio_cells < 2, since that way you'll have to
	 * write your own xlate function (that will have to retrieve the GPIO
	 * number and the flags from a single gpio cell -- this is possible,
	 * but not recommended).
 	 * gpio_cells <2를 사용하면 사용자의 xlate 함수 (단일 gpio 셀에서 GPIO 번호와 플래그를 가져와야 함 - 가능하지만 권장되지 않음)를 작성해야하므로이 방법을 사용하지 않는 것이 좋습니다.
	 */
	if (gc->of_gpio_n_cells < 2) {
		WARN_ON(1);
		return -EINVAL;
	}

	if (WARN_ON(gpiospec->args_count < gc->of_gpio_n_cells))
		return -EINVAL;

	if (gpiospec->args[0] >= gc->ngpio)
		return -EINVAL;

	if (flags)
		*flags = gpiospec->args[1];

	return gpiospec->args[0];
}
EXPORT_SYMBOL(of_gpio_simple_xlate);

/**
 * of_mm_gpiochip_add - Add memory mapped GPIO chip (bank)
 * @np:		device node of the GPIO chip
 * @mm_gc:	pointer to the of_mm_gpio_chip allocated structure
 *
 * To use this function you should allocate and fill mm_gc with:
 *
 * 1) In the gpio_chip structure:
 *    - all the callbacks
 *    - of_gpio_n_cells
 *    - of_xlate callback (optional)
 *
 * 3) In the of_mm_gpio_chip structure:
 *    - save_regs callback (optional)
 *
 * If succeeded, this function will map bank's memory and will
 * do all necessary work for you. Then you'll able to use .regs
 * to manage GPIOs from the callbacks.

  * of_mm_gpiochip_add - 메모리 매핑 된 GPIO 칩 (뱅크) 추가
  * @np : GPIO 칩의 디바이스 노드
  * @ mm_gc : of_mm_gpio_chip 할당 구조체에 대한 포인터
  *
  *이 함수를 사용하려면 mm_gc를 다음과 같이 할당하고 채워야합니다.
  *
  * 1) gpio_chip 구조에서 :
  * - 모든 콜백
  * - of_gpio_n_cells
  * - of_xlate 콜백 (선택 사항)
  *
  * 3) of_mm_gpio_chip 구조에서 :
  * - save_regs 콜백 (선택 사항)
  *
  * 성공하면이 기능이 은행의 메모리를 매핑하여 필요한 모든 작업을 수행합니다. 그런 다음 .regs를 사용하여 콜백에서 GPIO를 관리 할 수 있습니다.

 */
int of_mm_gpiochip_add(struct device_node *np,
		       struct of_mm_gpio_chip *mm_gc)
{
	int ret = -ENOMEM;
	struct gpio_chip *gc = &mm_gc->gc;

	gc->label = kstrdup(np->full_name, GFP_KERNEL);
	if (!gc->label)
		goto err0;

	mm_gc->regs = of_iomap(np, 0);
	if (!mm_gc->regs)
		goto err1;

	gc->base = -1;

	if (mm_gc->save_regs)
		mm_gc->save_regs(mm_gc);

	mm_gc->gc.of_node = np;

	ret = gpiochip_add(gc);
	if (ret)
		goto err2;

	return 0;
err2:
	iounmap(mm_gc->regs);
err1:
	kfree(gc->label);
err0:
	pr_err("%s: GPIO chip registration failed with status %d\n",
	       np->full_name, ret);
	return ret;
}
EXPORT_SYMBOL(of_mm_gpiochip_add);

/**
 * of_mm_gpiochip_remove - Remove memory mapped GPIO chip (bank)
 * @mm_gc:	pointer to the of_mm_gpio_chip allocated structure
  
  * of_mm_gpiochip_remove - 메모리 매핑 된 GPIO 칩 제거 (뱅크)
  * @ mm_gc : of_mm_gpio_chip 할당 구조체에 대한 포인터
 */
void of_mm_gpiochip_remove(struct of_mm_gpio_chip *mm_gc)
{
	struct gpio_chip *gc = &mm_gc->gc;

	if (!mm_gc)
		return;

	gpiochip_remove(gc);
	iounmap(mm_gc->regs);
	kfree(gc->label);
}
EXPORT_SYMBOL(of_mm_gpiochip_remove);

#ifdef CONFIG_PINCTRL
static int of_gpiochip_add_pin_range(struct gpio_chip *chip)
{
	struct device_node *np = chip->of_node;
	struct of_phandle_args pinspec;
	struct pinctrl_dev *pctldev;
	int index = 0, ret;
	const char *name;
	static const char group_names_propname[] = "gpio-ranges-group-names";
	struct property *group_names;

	if (!np)
		return 0;

	group_names = of_find_property(np, group_names_propname, NULL);

	for (;; index++) {
		ret = of_parse_phandle_with_fixed_args(np, "gpio-ranges", 3,
				index, &pinspec);
		if (ret)
			break;

		pctldev = of_pinctrl_get(pinspec.np);
		if (!pctldev)
			return -EPROBE_DEFER;

		if (pinspec.args[2]) {
			if (group_names) {
				of_property_read_string_index(np,
						group_names_propname,
						index, &name);
				if (strlen(name)) {
					pr_err("%s: Group name of numeric GPIO ranges must be the empty string.\n",
						np->full_name);
					break;
				}
			}
			/* npins != 0: linear range */
			ret = gpiochip_add_pin_range(chip,
					pinctrl_dev_get_devname(pctldev),
					pinspec.args[0],
					pinspec.args[1],
					pinspec.args[2]);
			if (ret)
				return ret;
		} else {
			/* npins == 0: special range */
			if (pinspec.args[1]) {
				pr_err("%s: Illegal gpio-range format.\n",
					np->full_name);
				break;
			}

			if (!group_names) {
				pr_err("%s: GPIO group range requested but no %s property.\n",
					np->full_name, group_names_propname);
				break;
			}

			ret = of_property_read_string_index(np,
						group_names_propname,
						index, &name);
			if (ret)
				break;

			if (!strlen(name)) {
				pr_err("%s: Group name of GPIO group range cannot be the empty string.\n",
				np->full_name);
				break;
			}

			ret = gpiochip_add_pingroup_range(chip, pctldev,
						pinspec.args[0], name);
			if (ret)
				return ret;
		}
	}

	return 0;
}

#else
static int of_gpiochip_add_pin_range(struct gpio_chip *chip) { return 0; }
#endif

int of_gpiochip_add(struct gpio_chip *chip)
{
	int status;

	if ((!chip->of_node) && (chip->dev))
		chip->of_node = chip->dev->of_node;

	if (!chip->of_node)
		return 0;

	if (!chip->of_xlate) {
		chip->of_gpio_n_cells = 2;
		chip->of_xlate = of_gpio_simple_xlate;
	}

	status = of_gpiochip_add_pin_range(chip);
	if (status)
		return status;

	of_node_get(chip->of_node);

	of_gpiochip_scan_gpios(chip);

	return 0;
}

void of_gpiochip_remove(struct gpio_chip *chip)
{
	gpiochip_remove_pin_ranges(chip);
	of_node_put(chip->of_node);
}
