/*
 * drivers/gpio/devres.c - managed gpio resources
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This file is based on kernel/irq/devres.c
 *
 * Copyright (c) 2011 John Crispin <blogic@openwrt.org>
   drivers / gpio / devres.c - 관리되는 GPIO 리소스
  
  이 프로그램은 무료 소프트웨어입니다. 자유 소프트웨어 재단이 발표 한 GNU General Public License 버전 2의 조건에 따라 재배포 및 / 또는 수정할 수 있습니다.
  
  이 프로그램과 함께 GNU 일반 공중 사용 허가서 사본을 받아야합니다. 그렇지 않다면 자유 소프트웨어에 글을 씁니다.
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 미국
  
  이 파일은 kernel / irq / devres.c를 기반으로합니다.
  
  Copyright (c) 2011 John Crispin <blogic@openwrt.org>
  * /
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/device.h>
#include <linux/gfp.h>

static void devm_gpiod_release(struct device *dev, void *res)
{
	struct gpio_desc **desc = res;

	gpiod_put(*desc);
}

static int devm_gpiod_match(struct device *dev, void *res, void *data)
{
	struct gpio_desc **this = res, **gpio = data;

	return *this == *gpio;
}

static void devm_gpiod_release_array(struct device *dev, void *res)
{
	struct gpio_descs **descs = res;

	gpiod_put_array(*descs);
}

static int devm_gpiod_match_array(struct device *dev, void *res, void *data)
{
	struct gpio_descs **this = res, **gpios = data;

	return *this == *gpios;
}

/**
 * devm_gpiod_get - Resource-managed gpiod_get()
 * @dev:	GPIO consumer
 * @con_id:	function within the GPIO consumer
 * @flags:	optional GPIO initialization flags
 *
 * Managed gpiod_get(). GPIO descriptors returned from this function are
 * automatically disposed on driver detach. See gpiod_get() for detailed
 * information about behavior and return values.

  * devm_gpiod_get - 자원 관리 gpiod_get ()
  * @dev : GPIO 소비자
  * @con_id : GPIO 소비자 내의 기능
  * @ 플래그 : 선택적 GPIO 초기화 플래그
  *
  * 관리 된 gpiod_get (). 이 함수에서 반환 된 GPIO 설명자는 드라이버 분리시 자동으로 삭제됩니다. 
    동작 및 반환 값에 대한 자세한 내용은 gpiod_get ()을 참조하십시오.
 */
struct gpio_desc *__must_check devm_gpiod_get(struct device *dev,
					      const char *con_id,
					      enum gpiod_flags flags)
{
	return devm_gpiod_get_index(dev, con_id, 0, flags);
}
EXPORT_SYMBOL(devm_gpiod_get);

/**
 * devm_gpiod_get_optional - Resource-managed gpiod_get_optional()
 * @dev: GPIO consumer
 * @con_id: function within the GPIO consumer
 * @flags: optional GPIO initialization flags
 *
 * Managed gpiod_get_optional(). GPIO descriptors returned from this function
 * are automatically disposed on driver detach. See gpiod_get_optional() for
 * detailed information about behavior and return values.
  * devm_gpiod_get_optional - 자원 관리 gpiod_get_optional ()
  * @dev : GPIO 소비자
  * @con_id : GPIO 소비자 내의 기능
  * @ 플래그 : 선택적 GPIO 초기화 플래그
  *
  * 관리 된 gpiod_get_optional (). 이 함수에서 반환 된 GPIO 설명자는 드라이버 분리시 자동으로 삭제됩니다.
    동작 및 반환 값에 대한 자세한 내용은 gpiod_get_optional ()을 참조하십시오.
 */
struct gpio_desc *__must_check devm_gpiod_get_optional(struct device *dev,
						       const char *con_id,
						       enum gpiod_flags flags)
{
	return devm_gpiod_get_index_optional(dev, con_id, 0, flags);
}
EXPORT_SYMBOL(devm_gpiod_get_optional);

/**
 * devm_gpiod_get_index - Resource-managed gpiod_get_index()
 * @dev:	GPIO consumer
 * @con_id:	function within the GPIO consumer
 * @idx:	index of the GPIO to obtain in the consumer
 * @flags:	optional GPIO initialization flags
 *
 * Managed gpiod_get_index(). GPIO descriptors returned from this function are
 * automatically disposed on driver detach. See gpiod_get_index() for detailed
 * information about behavior and return values.
  * devm_gpiod_get_index - 리소스 관리 gpiod_get_index ()
  * @dev : GPIO 소비자
  * @con_id : GPIO 소비자 내의 기능
  * @idx : 소비자에서 얻을 GPIO의 인덱스
  * @ 플래그 : 선택적 GPIO 초기화 플래그
  * 관리 된 gpiod_get_index (). 이 함수에서 반환 된 GPIO 설명자는 드라이버 분리시 자동으로 삭제됩니다.
    동작 및 반환 값에 대한 자세한 내용은 gpiod_get_index ()를 참조하십시오.
 */
struct gpio_desc *__must_check devm_gpiod_get_index(struct device *dev,
						    const char *con_id,
						    unsigned int idx,
						    enum gpiod_flags flags)
{
	struct gpio_desc **dr;
	struct gpio_desc *desc;

	dr = devres_alloc(devm_gpiod_release, sizeof(struct gpio_desc *),
			  GFP_KERNEL);
	if (!dr)
		return ERR_PTR(-ENOMEM);

	desc = gpiod_get_index(dev, con_id, idx, flags);
	if (IS_ERR(desc)) {
		devres_free(dr);
		return desc;
	}

	*dr = desc;
	devres_add(dev, dr);

	return desc;
}
EXPORT_SYMBOL(devm_gpiod_get_index);

/**
 * devm_get_gpiod_from_child - get a GPIO descriptor from a device's child node
 * @dev:	GPIO consumer
 * @con_id:	function within the GPIO consumer
 * @child:	firmware node (child of @dev)
 *
 * GPIO descriptors returned from this function are automatically disposed on
 * driver detach.

  * devm_get_gpiod_from_child - 장치의 자식 노드로부터 GPIO 디스크립터 가져 오기
  * @dev : GPIO 소비자
  * @con_id : GPIO 소비자 내의 기능
  * @child : 펌웨어 노드 (@dev의 자식)
  
  *이 함수에서 반환 된 GPIO 설명자는 드라이버 분리시 자동으로 삭제됩니다.
 */
struct gpio_desc *devm_get_gpiod_from_child(struct device *dev,
					    const char *con_id,
					    struct fwnode_handle *child)
{
	static const char * const suffixes[] = { "gpios", "gpio" };
	char prop_name[32]; /* 32 is max size of property name */
	struct gpio_desc **dr;
	struct gpio_desc *desc;
	unsigned int i;

	dr = devres_alloc(devm_gpiod_release, sizeof(struct gpio_desc *),
			  GFP_KERNEL);
	if (!dr)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < ARRAY_SIZE(suffixes); i++) {
		if (con_id)
			snprintf(prop_name, sizeof(prop_name), "%s-%s",
					    con_id, suffixes[i]);
		else
			snprintf(prop_name, sizeof(prop_name), "%s",
							       suffixes[i]);

		desc = fwnode_get_named_gpiod(child, prop_name);
		if (!IS_ERR(desc) || (PTR_ERR(desc) == -EPROBE_DEFER))
			break;
	}
	if (IS_ERR(desc)) {
		devres_free(dr);
		return desc;
	}

	*dr = desc;
	devres_add(dev, dr);

	return desc;
}
EXPORT_SYMBOL(devm_get_gpiod_from_child);

/**
 * devm_gpiod_get_index_optional - Resource-managed gpiod_get_index_optional()
 * @dev: GPIO consumer
 * @con_id: function within the GPIO consumer
 * @index: index of the GPIO to obtain in the consumer
 * @flags: optional GPIO initialization flags
 *
 * Managed gpiod_get_index_optional(). GPIO descriptors returned from this
 * function are automatically disposed on driver detach. See
 * gpiod_get_index_optional() for detailed information about behavior and
 * return values.
  * devm_gpiod_get_index_optional - 자원 관리 gpiod_get_index_optional ()
  * @dev : GPIO 소비자
  * @con_id : GPIO 소비자 내의 기능
  * @ 인덱스 : 소비자에서 얻는 GPIO의 인덱스
  * @ 플래그 : 선택적 GPIO 초기화 플래그
  *
  * 관리 된 gpiod_get_index_optional (). 이 함수에서 반환 된 GPIO 설명자는 드라이버 분리시 자동으로 삭제됩니다.
    동작 및 반환 값에 대한 자세한 내용은 gpiod_get_index_optional ()을 참조하십시오.
 */
struct gpio_desc *__must_check devm_gpiod_get_index_optional(struct device *dev,
							     const char *con_id,
							     unsigned int index,
							     enum gpiod_flags flags)
{
	struct gpio_desc *desc;

	desc = devm_gpiod_get_index(dev, con_id, index, flags);
	if (IS_ERR(desc)) {
		if (PTR_ERR(desc) == -ENOENT)
			return NULL;
	}

	return desc;
}
EXPORT_SYMBOL(devm_gpiod_get_index_optional);

/**
 * devm_gpiod_get_array - Resource-managed gpiod_get_array()
 * @dev:	GPIO consumer
 * @con_id:	function within the GPIO consumer
 * @flags:	optional GPIO initialization flags
 *
 * Managed gpiod_get_array(). GPIO descriptors returned from this function are
 * automatically disposed on driver detach. See gpiod_get_array() for detailed
 * information about behavior and return values.
  * devm_gpiod_get_array - 리소스 관리 gpiod_get_array ()
  * @dev : GPIO 소비자
  * @con_id : GPIO 소비자 내의 기능
  * @ 플래그 : 선택적 GPIO 초기화 플래그
  *
  * 관리되는 gpiod_get_array (). 이 함수에서 반환 된 GPIO 설명자는 드라이버 분리시 자동으로 삭제됩니다. 
    동작 및 반환 값에 대한 자세한 내용은 gpiod_get_array ()를 참조하십시오.
 */
struct gpio_descs *__must_check devm_gpiod_get_array(struct device *dev,
						     const char *con_id,
						     enum gpiod_flags flags)
{
	struct gpio_descs **dr;
	struct gpio_descs *descs;

	dr = devres_alloc(devm_gpiod_release_array,
			  sizeof(struct gpio_descs *), GFP_KERNEL);
	if (!dr)
		return ERR_PTR(-ENOMEM);

	descs = gpiod_get_array(dev, con_id, flags);
	if (IS_ERR(descs)) {
		devres_free(dr);
		return descs;
	}

	*dr = descs;
	devres_add(dev, dr);

	return descs;
}
EXPORT_SYMBOL(devm_gpiod_get_array);

/**
 * devm_gpiod_get_array_optional - Resource-managed gpiod_get_array_optional()
 * @dev:	GPIO consumer
 * @con_id:	function within the GPIO consumer
 * @flags:	optional GPIO initialization flags
 *
 * Managed gpiod_get_array_optional(). GPIO descriptors returned from this
 * function are automatically disposed on driver detach.
 * See gpiod_get_array_optional() for detailed information about behavior and
 * return values.

  * devm_gpiod_get_array_optional - 자원 관리 gpiod_get_array_optional ()
  * @dev : GPIO 소비자
  * @con_id : GPIO 소비자 내의 기능
  * @ 플래그 : 선택적 GPIO 초기화 플래그
  *
  * 관리되는 gpiod_get_array_optional (). 이 함수에서 반환 된 GPIO 설명자는 드라이버 분리시 자동으로 삭제됩니다.
  * 동작 및 반환 값에 대한 자세한 내용은 gpiod_get_array_optional ()을 참조하십시오.
 */
struct gpio_descs *__must_check
devm_gpiod_get_array_optional(struct device *dev, const char *con_id,
			      enum gpiod_flags flags)
{
	struct gpio_descs *descs;

	descs = devm_gpiod_get_array(dev, con_id, flags);
	if (IS_ERR(descs) && (PTR_ERR(descs) == -ENOENT))
		return NULL;

	return descs;
}
EXPORT_SYMBOL(devm_gpiod_get_array_optional);

/**
 * devm_gpiod_put - Resource-managed gpiod_put()
 * @desc:	GPIO descriptor to dispose of
 *
 * Dispose of a GPIO descriptor obtained with devm_gpiod_get() or
 * devm_gpiod_get_index(). Normally this function will not be called as the GPIO
 * will be disposed of by the resource management code.
  * devm_gpiod_put - 리소스 관리 gpiod_put ()
  * @desc : 처리 할 GPIO 설명자
  *
  * devm_gpiod_get () 또는 devm_gpiod_get_index ()로 얻은 GPIO 디스크립터를 삭제하십시오.
    일반적으로 GPIO가 리소스 관리 코드에 의해 처리되므로이 함수는 호출되지 않습니다.
 */
void devm_gpiod_put(struct device *dev, struct gpio_desc *desc)
{
	WARN_ON(devres_release(dev, devm_gpiod_release, devm_gpiod_match,
		&desc));
}
EXPORT_SYMBOL(devm_gpiod_put);

/**
 * devm_gpiod_put_array - Resource-managed gpiod_put_array()
 * @descs:	GPIO descriptor array to dispose of
 *
 * Dispose of an array of GPIO descriptors obtained with devm_gpiod_get_array().
 * Normally this function will not be called as the GPIOs will be disposed of
 * by the resource management code.
  * devm_gpiod_put_array - 리소스 관리 gpiod_put_array ()
  * @descs : 처분 할 GPIO 디스크립터 배열
  *
  * devm_gpiod_get_array ()로 얻은 GPIO 디스크립터의 배열을 삭제하십시오.
  * 일반적으로 GPIO는 자원 관리 코드에 의해 처리되므로이 함수는 호출되지 않습니다.
 */
void devm_gpiod_put_array(struct device *dev, struct gpio_descs *descs)
{
	WARN_ON(devres_release(dev, devm_gpiod_release_array,
			       devm_gpiod_match_array, &descs));
}
EXPORT_SYMBOL(devm_gpiod_put_array);




static void devm_gpio_release(struct device *dev, void *res)
{
	unsigned *gpio = res;

	gpio_free(*gpio);
}

static int devm_gpio_match(struct device *dev, void *res, void *data)
{
	unsigned *this = res, *gpio = data;

	return *this == *gpio;
}

/**
 *      devm_gpio_request - request a GPIO for a managed device
 *      @dev: device to request the GPIO for
 *      @gpio: GPIO to allocate
 *      @label: the name of the requested GPIO
 *
 *      Except for the extra @dev argument, this function takes the
 *      same arguments and performs the same function as
 *      gpio_request().  GPIOs requested with this function will be
 *      automatically freed on driver detach.
 *
 *      If an GPIO allocated with this function needs to be freed
 *      separately, devm_gpio_free() must be used.
  * devm_gpio_request - 관리 대상 장치에 대한 GPIO 요청
  * @dev : GPIO를 요청하는 장치
  * @gpio : 할당 할 GPIO
  * @label : 요청 된 GPIO의 이름
  *
  * 추가 @dev 인수를 제외하고이 함수는 동일한 인수를 사용하고 gpio_request ()와 같은 기능을 수행합니다.
    이 기능으로 요청 된 GPIO는 드라이버 분리시 자동으로 해제됩니다.
  *
  * 이 함수로 할당 된 GPIO를 별도로 해제해야하는 경우 devm_gpio_free ()를 사용해야합니다.
 */

int devm_gpio_request(struct device *dev, unsigned gpio, const char *label)
{
	unsigned *dr;
	int rc;

	dr = devres_alloc(devm_gpio_release, sizeof(unsigned), GFP_KERNEL);
	if (!dr)
		return -ENOMEM;

	rc = gpio_request(gpio, label);
	if (rc) {
		devres_free(dr);
		return rc;
	}

	*dr = gpio;
	devres_add(dev, dr);

	return 0;
}
EXPORT_SYMBOL(devm_gpio_request);

/**
 *	devm_gpio_request_one - request a single GPIO with initial setup
 *	@dev:   device to request for
 *	@gpio:	the GPIO number
 *	@flags:	GPIO configuration as specified by GPIOF_*
 *	@label:	a literal description string of this GPIO
  * devm_gpio_request_one - 초기 설정으로 단일 GPIO 요청
  * @dev : 요청할 장치
  * @gpio : GPIO 번호
  * @ 플래그 : GPIOF_ *로 지정된 GPIO 구성
  * @label :이 GPIO의 리터럴 설명 문자열
 */
int devm_gpio_request_one(struct device *dev, unsigned gpio,
			  unsigned long flags, const char *label)
{
	unsigned *dr;
	int rc;

	dr = devres_alloc(devm_gpio_release, sizeof(unsigned), GFP_KERNEL);
	if (!dr)
		return -ENOMEM;

	rc = gpio_request_one(gpio, flags, label);
	if (rc) {
		devres_free(dr);
		return rc;
	}

	*dr = gpio;
	devres_add(dev, dr);

	return 0;
}
EXPORT_SYMBOL(devm_gpio_request_one);

/**
 *      devm_gpio_free - free a GPIO
 *      @dev: device to free GPIO for
 *      @gpio: GPIO to free
 *
 *      Except for the extra @dev argument, this function takes the
 *      same arguments and performs the same function as gpio_free().
 *      This function instead of gpio_free() should be used to manually
 *      free GPIOs allocated with devm_gpio_request().
  * devm_gpio_free - 무료 GPIO
  * @dev : 무료 GPIO 장치
  * @gpio : 무료 GPIO
  *
  * 추가 @dev 인수를 제외하고이 함수는 동일한 인수를 사용하고 gpio_free ()와 동일한 기능을 수행합니다.
  * devm_gpio_request ()로 할당 된 GPIO를 수동으로 해제하려면 gpio_free () 대신이 함수를 사용해야합니다.
 */
void devm_gpio_free(struct device *dev, unsigned int gpio)
{

	WARN_ON(devres_release(dev, devm_gpio_release, devm_gpio_match,
		&gpio));
}
EXPORT_SYMBOL(devm_gpio_free);
