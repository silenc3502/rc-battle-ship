#include <linux/idr.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/interrupt.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>

#include "gpiolib.h"

#define GPIO_IRQF_TRIGGER_FALLING	BIT(0)
#define GPIO_IRQF_TRIGGER_RISING	BIT(1)
#define GPIO_IRQF_TRIGGER_BOTH		(GPIO_IRQF_TRIGGER_FALLING | \
					 GPIO_IRQF_TRIGGER_RISING)

struct gpiod_data {
	struct gpio_desc *desc;

	struct mutex mutex;
	struct kernfs_node *value_kn;
	int irq;
	unsigned char irq_flags;

	bool direction_can_change;
};

/*
 * Lock to serialise gpiod export and unexport, and prevent re-export of
 * gpiod whose chip is being unregistered.
 * 잠금을 사용하여 gpiod 내보내기 및 unexport를 직렬화하고 칩이 등록 해제 된 gpiod를 다시 내보낼 수 없도록합니다.
 */
static DEFINE_MUTEX(sysfs_lock);

/*
 * /sys/class/gpio/gpioN... only for GPIOs that are exported
 *   /direction
 *      * MAY BE OMITTED if kernel won't allow direction changes
 *      * is read/write as "in" or "out"
 *      * may also be written as "high" or "low", initializing
 *        output value as specified ("out" implies "low")
 *   /value
 *      * always readable, subject to hardware behavior
 *      * may be writable, as zero/nonzero
 *   /edge
 *      * configures behavior of poll(2) on /value
 *      * available only if pin can generate IRQs on input
 *      * is read/write as "none", "falling", "rising", or "both"
 *   /active_low
 *      * configures polarity of /value
 *      * is read/write as zero/nonzero
 *      * also affects existing and subsequent "falling" and "rising"
 *        /edge configuration

  * / sys / class / gpio / gpioN ... 내 보낸 GPIO에만 해당
  * / 방향
  커널이 방향 전환을 허용하지 않으면 "in"또는 "out"이 "high"또는 "low"로 쓰여 지거나 출력 값을 지정된대로 초기화 할 수 있습니다 ( "out"은 "low"를 의미 함). ")
  * / 값
  * * 항상 읽을 수 있고, 하드웨어 동작에 따라 쓰기 가능할 수 있습니다. 0 / 0이 아닙니다.
  * / 가장자리
  핀이 입력에서 IRQ를 생성 할 수있는 경우에만 "none", "falling", "rising"또는 "both"로 읽기 / 쓰기가 가능한 경우에만 poll * (2) on /
  * / active_low
  * * / 극성의 구성은 읽기 / 쓰기가 0 / 0이 아니기 때문에 기존 및 이후의 "하강"및 "상승"
  * / 가장자리 구성

 */

static ssize_t direction_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct gpiod_data *data = dev_get_drvdata(dev);
	struct gpio_desc *desc = data->desc;
	ssize_t			status;

	mutex_lock(&data->mutex);

	gpiod_get_direction(desc);
	status = sprintf(buf, "%s\n",
			test_bit(FLAG_IS_OUT, &desc->flags)
				? "out" : "in");

	mutex_unlock(&data->mutex);

	return status;
}

static ssize_t direction_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct gpiod_data *data = dev_get_drvdata(dev);
	struct gpio_desc *desc = data->desc;
	ssize_t			status;

	mutex_lock(&data->mutex);

	if (sysfs_streq(buf, "high"))
		status = gpiod_direction_output_raw(desc, 1);
	else if (sysfs_streq(buf, "out") || sysfs_streq(buf, "low"))
		status = gpiod_direction_output_raw(desc, 0);
	else if (sysfs_streq(buf, "in"))
		status = gpiod_direction_input(desc);
	else
		status = -EINVAL;

	mutex_unlock(&data->mutex);

	return status ? : size;
}
static DEVICE_ATTR_RW(direction);

static ssize_t value_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct gpiod_data *data = dev_get_drvdata(dev);
	struct gpio_desc *desc = data->desc;
	ssize_t			status;

	mutex_lock(&data->mutex);

	status = sprintf(buf, "%d\n", gpiod_get_value_cansleep(desc));

	mutex_unlock(&data->mutex);

	return status;
}

static ssize_t value_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct gpiod_data *data = dev_get_drvdata(dev);
	struct gpio_desc *desc = data->desc;
	ssize_t			status;

	mutex_lock(&data->mutex);

	if (!test_bit(FLAG_IS_OUT, &desc->flags)) {
		status = -EPERM;
	} else {
		long		value;

		status = kstrtol(buf, 0, &value);
		if (status == 0) {
			gpiod_set_value_cansleep(desc, value);
			status = size;
		}
	}

	mutex_unlock(&data->mutex);

	return status;
}
static DEVICE_ATTR_RW(value);

static irqreturn_t gpio_sysfs_irq(int irq, void *priv)
{
	struct gpiod_data *data = priv;

	sysfs_notify_dirent(data->value_kn);

	return IRQ_HANDLED;
}

/* Caller holds gpiod-data mutex. */
static int gpio_sysfs_request_irq(struct device *dev, unsigned char flags)
{
	struct gpiod_data	*data = dev_get_drvdata(dev);
	struct gpio_desc	*desc = data->desc;
	unsigned long		irq_flags;
	int			ret;

	data->irq = gpiod_to_irq(desc);
	if (data->irq < 0)
		return -EIO;

	data->value_kn = sysfs_get_dirent(dev->kobj.sd, "value");
	if (!data->value_kn)
		return -ENODEV;

	irq_flags = IRQF_SHARED;
	if (flags & GPIO_IRQF_TRIGGER_FALLING)
		irq_flags |= test_bit(FLAG_ACTIVE_LOW, &desc->flags) ?
			IRQF_TRIGGER_RISING : IRQF_TRIGGER_FALLING;
	if (flags & GPIO_IRQF_TRIGGER_RISING)
		irq_flags |= test_bit(FLAG_ACTIVE_LOW, &desc->flags) ?
			IRQF_TRIGGER_FALLING : IRQF_TRIGGER_RISING;

	/*
	 * FIXME: This should be done in the irq_request_resources callback
	 *        when the irq is requested, but a few drivers currently fail
	 *        to do so.
	 *
	 *        Remove this redundant call (along with the corresponding
	 *        unlock) when those drivers have been fixed.
	 
	 * FIXME : 이것은 irq가 요청 될 때 irq_request_resources 콜백에서 수행되어야하지만, 현재 몇몇 드라이버는 그렇게하지 못하고있다.

	 * 드라이버가 수정되면이 중복 호출을 제거하십시오 (해당 잠금 해제와 함께).
	 */
	ret = gpiochip_lock_as_irq(desc->chip, gpio_chip_hwgpio(desc));
	if (ret < 0)
		goto err_put_kn;

	ret = request_any_context_irq(data->irq, gpio_sysfs_irq, irq_flags,
				"gpiolib", data);
	if (ret < 0)
		goto err_unlock;

	data->irq_flags = flags;

	return 0;

err_unlock:
	gpiochip_unlock_as_irq(desc->chip, gpio_chip_hwgpio(desc));
err_put_kn:
	sysfs_put(data->value_kn);

	return ret;
}

/*
 * Caller holds gpiod-data mutex (unless called after class-device
 * deregistration).
 * 호출자는 gpiod-data 뮤텍스를 보유합니다 (클래스 장치 등록 취소 후 호출되지 않는 한).
 */
static void gpio_sysfs_free_irq(struct device *dev)
{
	struct gpiod_data *data = dev_get_drvdata(dev);
	struct gpio_desc *desc = data->desc;

	data->irq_flags = 0;
	free_irq(data->irq, data);
	gpiochip_unlock_as_irq(desc->chip, gpio_chip_hwgpio(desc));
	sysfs_put(data->value_kn);
}

static const struct {
	const char *name;
	unsigned char flags;
} trigger_types[] = {
	{ "none",    0 },
	{ "falling", GPIO_IRQF_TRIGGER_FALLING },
	{ "rising",  GPIO_IRQF_TRIGGER_RISING },
	{ "both",    GPIO_IRQF_TRIGGER_BOTH },
};

static ssize_t edge_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct gpiod_data *data = dev_get_drvdata(dev);
	ssize_t	status = 0;
	int i;

	mutex_lock(&data->mutex);

	for (i = 0; i < ARRAY_SIZE(trigger_types); i++) {
		if (data->irq_flags == trigger_types[i].flags) {
			status = sprintf(buf, "%s\n", trigger_types[i].name);
			break;
		}
	}

	mutex_unlock(&data->mutex);

	return status;
}

static ssize_t edge_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct gpiod_data *data = dev_get_drvdata(dev);
	unsigned char flags;
	ssize_t	status = size;
	int i;

	for (i = 0; i < ARRAY_SIZE(trigger_types); i++) {
		if (sysfs_streq(trigger_types[i].name, buf))
			break;
	}

	if (i == ARRAY_SIZE(trigger_types))
		return -EINVAL;

	flags = trigger_types[i].flags;

	mutex_lock(&data->mutex);

	if (flags == data->irq_flags) {
		status = size;
		goto out_unlock;
	}

	if (data->irq_flags)
		gpio_sysfs_free_irq(dev);

	if (flags) {
		status = gpio_sysfs_request_irq(dev, flags);
		if (!status)
			status = size;
	}

out_unlock:
	mutex_unlock(&data->mutex);

	return status;
}
static DEVICE_ATTR_RW(edge);

/* Caller holds gpiod-data mutex. */
static int gpio_sysfs_set_active_low(struct device *dev, int value)
{
	struct gpiod_data	*data = dev_get_drvdata(dev);
	struct gpio_desc	*desc = data->desc;
	int			status = 0;
	unsigned int		flags = data->irq_flags;

	if (!!test_bit(FLAG_ACTIVE_LOW, &desc->flags) == !!value)
		return 0;

	if (value)
		set_bit(FLAG_ACTIVE_LOW, &desc->flags);
	else
		clear_bit(FLAG_ACTIVE_LOW, &desc->flags);

	/* reconfigure poll(2) support if enabled on one edge only
	   한쪽 가장자리에서만 활성화 된 경우 poll (2) 지원을 다시 구성합니다.		 */
	if (flags == GPIO_IRQF_TRIGGER_FALLING ||
					flags == GPIO_IRQF_TRIGGER_RISING) {
		gpio_sysfs_free_irq(dev);
		status = gpio_sysfs_request_irq(dev, flags);
	}

	return status;
}

static ssize_t active_low_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct gpiod_data *data = dev_get_drvdata(dev);
	struct gpio_desc *desc = data->desc;
	ssize_t			status;

	mutex_lock(&data->mutex);

	status = sprintf(buf, "%d\n",
				!!test_bit(FLAG_ACTIVE_LOW, &desc->flags));

	mutex_unlock(&data->mutex);

	return status;
}

static ssize_t active_low_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct gpiod_data	*data = dev_get_drvdata(dev);
	ssize_t			status;
	long			value;

	mutex_lock(&data->mutex);

	status = kstrtol(buf, 0, &value);
	if (status == 0)
		status = gpio_sysfs_set_active_low(dev, value);

	mutex_unlock(&data->mutex);

	return status ? : size;
}
static DEVICE_ATTR_RW(active_low);

static umode_t gpio_is_visible(struct kobject *kobj, struct attribute *attr,
			       int n)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct gpiod_data *data = dev_get_drvdata(dev);
	struct gpio_desc *desc = data->desc;
	umode_t mode = attr->mode;
	bool show_direction = data->direction_can_change;

	if (attr == &dev_attr_direction.attr) {
		if (!show_direction)
			mode = 0;
	} else if (attr == &dev_attr_edge.attr) {
		if (gpiod_to_irq(desc) < 0)
			mode = 0;
		if (!show_direction && test_bit(FLAG_IS_OUT, &desc->flags))
			mode = 0;
	}

	return mode;
}

static struct attribute *gpio_attrs[] = {
	&dev_attr_direction.attr,
	&dev_attr_edge.attr,
	&dev_attr_value.attr,
	&dev_attr_active_low.attr,
	NULL,
};

static const struct attribute_group gpio_group = {
	.attrs = gpio_attrs,
	.is_visible = gpio_is_visible,
};

static const struct attribute_group *gpio_groups[] = {
	&gpio_group,
	NULL
};

/*
 * /sys/class/gpio/gpiochipN/
 *   /base ... matching gpio_chip.base (N)
 *   /label ... matching gpio_chip.label
 *   /ngpio ... matching gpio_chip.ngpio
 */

static ssize_t base_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	const struct gpio_chip	*chip = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", chip->base);
}
static DEVICE_ATTR_RO(base);

static ssize_t label_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	const struct gpio_chip	*chip = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", chip->label ? : "");
}
static DEVICE_ATTR_RO(label);

static ssize_t ngpio_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	const struct gpio_chip	*chip = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", chip->ngpio);
}
static DEVICE_ATTR_RO(ngpio);

static struct attribute *gpiochip_attrs[] = {
	&dev_attr_base.attr,
	&dev_attr_label.attr,
	&dev_attr_ngpio.attr,
	NULL,
};
ATTRIBUTE_GROUPS(gpiochip);

/*
 * /sys/class/gpio/export ... write-only
 *	integer N ... number of GPIO to export (full access)
 * /sys/class/gpio/unexport ... write-only
 *	integer N ... number of GPIO to unexport
 */
static ssize_t export_store(struct class *class,
				struct class_attribute *attr,
				const char *buf, size_t len)
{
	long			gpio;
	struct gpio_desc	*desc;
	int			status;

	status = kstrtol(buf, 0, &gpio);
	if (status < 0)
		goto done;

	desc = gpio_to_desc(gpio);
	/* reject invalid GPIOs */
	if (!desc) {
		pr_warn("%s: invalid GPIO %ld\n", __func__, gpio);
		return -EINVAL;
	}

	/* No extra locking here; FLAG_SYSFS just signifies that the
	 * request and export were done by on behalf of userspace, so
	 * they may be undone on its behalf too.
	   여분의 잠금 장치가 없습니다. FLAG_SYSFS는 사용자 공간을 대신하여 요청 및 내보내기가 완료되었음을 나타냅니다. 따라서 FLAG_SYSFS는 사용자 공간을 대신하여 실행 취소 될 수 있습니다.
	 */

	status = gpiod_request(desc, "sysfs");
	if (status < 0) {
		if (status == -EPROBE_DEFER)
			status = -ENODEV;
		goto done;
	}
	status = gpiod_export(desc, true);
	if (status < 0)
		gpiod_free(desc);
	else
		set_bit(FLAG_SYSFS, &desc->flags);

done:
	if (status)
		pr_debug("%s: status %d\n", __func__, status);
	return status ? : len;
}

static ssize_t unexport_store(struct class *class,
				struct class_attribute *attr,
				const char *buf, size_t len)
{
	long			gpio;
	struct gpio_desc	*desc;
	int			status;

	status = kstrtol(buf, 0, &gpio);
	if (status < 0)
		goto done;

	desc = gpio_to_desc(gpio);
	/* reject bogus commands (gpio_unexport ignores them) 
	   위조 명령을 거부합니다 (gpio_unexport는 무시합니다)		*/
	if (!desc) {
		pr_warn("%s: invalid GPIO %ld\n", __func__, gpio);
		return -EINVAL;
	}

	status = -EINVAL;

	/* No extra locking here; FLAG_SYSFS just signifies that the
	 * request and export were done by on behalf of userspace, so
	 * they may be undone on its behalf too.

	 여분의 잠금 장치가 없습니다. FLAG_SYSFS는 사용자 공간을 대신하여 요청 및 내보내기가 완료되었음을 나타냅니다. 따라서 FLAG_SYSFS는 사용자 공간을 대신하여 실행 취소 될 수 있습니다.
	 */
	if (test_and_clear_bit(FLAG_SYSFS, &desc->flags)) {
		status = 0;
		gpiod_free(desc);
	}
done:
	if (status)
		pr_debug("%s: status %d\n", __func__, status);
	return status ? : len;
}

static struct class_attribute gpio_class_attrs[] = {
	__ATTR(export, 0200, NULL, export_store),
	__ATTR(unexport, 0200, NULL, unexport_store),
	__ATTR_NULL,
};

static struct class gpio_class = {
	.name =		"gpio",
	.owner =	THIS_MODULE,

	.class_attrs =	gpio_class_attrs,
};


/**
 * gpiod_export - export a GPIO through sysfs
 * @gpio: gpio to make available, already requested
 * @direction_may_change: true if userspace may change gpio direction
 * Context: arch_initcall or later
 *
 * When drivers want to make a GPIO accessible to userspace after they
 * have requested it -- perhaps while debugging, or as part of their
 * public interface -- they may use this routine.  If the GPIO can
 * change direction (some can't) and the caller allows it, userspace
 * will see "direction" sysfs attribute which may be used to change
 * the gpio's direction.  A "value" attribute will always be provided.
 *
 * Returns zero on success, else an error.

  * gpiod_export - sysfs를 통해 GPIO 내보내기
  * @ gpio : gpio를 사용 가능하게하고 이미 요청했습니다.
  * @direction_may_change : 사용자 공간이 gpio 방향을 변경할 수 있으면 true입니다.
  * 컨텍스트 : arch_initcall 이상
  *
  * 드라이버가 요청한 후 (예 : 디버깅 중일 때 또는 공개 인터페이스의 일부로) 사용자 공간에서 GPIO에 액세스 할 수 있도록하려는 경우이 루틴을 사용할 수 있습니다. GPIO가 방향을 변경할 수 있고 (일부는 할 수 없음) 호출자가 허용하는 경우 사용자 공간에 GPIO의 방향을 변경하는 데 사용할 수있는 "direction"sysfs 속성이 표시됩니다. '가치'속성은 항상 제공됩니다.
  *
  * 성공하면 0을 반환하고, 그렇지 않으면 오류를 반환합니다.
 */
int gpiod_export(struct gpio_desc *desc, bool direction_may_change)
{
	struct gpio_chip	*chip;
	struct gpiod_data	*data;
	unsigned long		flags;
	int			status;
	const char		*ioname = NULL;
	struct device		*dev;
	int			offset;

	/* can't export until sysfs is available ... 
	    sysfs가 사용 가능할 때까지 내보낼 수 없습니다 ...		*/
	if (!gpio_class.p) {
		pr_debug("%s: called too early!\n", __func__);
		return -ENOENT;
	}

	if (!desc) {
		pr_debug("%s: invalid gpio descriptor\n", __func__);
		return -EINVAL;
	}

	chip = desc->chip;

	mutex_lock(&sysfs_lock);

	/* check if chip is being removed 
	    칩 제거 중인지 확인			*/
	if (!chip || !chip->cdev) {
		status = -ENODEV;
		goto err_unlock;
	}

	spin_lock_irqsave(&gpio_lock, flags);
	if (!test_bit(FLAG_REQUESTED, &desc->flags) ||
	     test_bit(FLAG_EXPORT, &desc->flags)) {
		spin_unlock_irqrestore(&gpio_lock, flags);
		gpiod_dbg(desc, "%s: unavailable (requested=%d, exported=%d)\n",
				__func__,
				test_bit(FLAG_REQUESTED, &desc->flags),
				test_bit(FLAG_EXPORT, &desc->flags));
		status = -EPERM;
		goto err_unlock;
	}
	spin_unlock_irqrestore(&gpio_lock, flags);

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		status = -ENOMEM;
		goto err_unlock;
	}

	data->desc = desc;
	mutex_init(&data->mutex);
	if (chip->direction_input && chip->direction_output)
		data->direction_can_change = direction_may_change;
	else
		data->direction_can_change = false;

	offset = gpio_chip_hwgpio(desc);
	if (chip->names && chip->names[offset])
		ioname = chip->names[offset];

	dev = device_create_with_groups(&gpio_class, chip->dev,
					MKDEV(0, 0), data, gpio_groups,
					ioname ? ioname : "gpio%u",
					desc_to_gpio(desc));
	if (IS_ERR(dev)) {
		status = PTR_ERR(dev);
		goto err_free_data;
	}

	set_bit(FLAG_EXPORT, &desc->flags);
	mutex_unlock(&sysfs_lock);
	return 0;

err_free_data:
	kfree(data);
err_unlock:
	mutex_unlock(&sysfs_lock);
	gpiod_dbg(desc, "%s: status %d\n", __func__, status);
	return status;
}
EXPORT_SYMBOL_GPL(gpiod_export);

static int match_export(struct device *dev, const void *desc)
{
	struct gpiod_data *data = dev_get_drvdata(dev);

	return data->desc == desc;
}

/**
 * gpiod_export_link - create a sysfs link to an exported GPIO node
 * @dev: device under which to create symlink
 * @name: name of the symlink
 * @gpio: gpio to create symlink to, already exported
 *
 * Set up a symlink from /sys/.../dev/name to /sys/class/gpio/gpioN
 * node. Caller is responsible for unlinking.
 *
 * Returns zero on success, else an error.

  * gpiod_export_link - 내 보낸 GPIO 노드에 대한 sysfs 링크를 생성합니다.
  * @dev : 심볼릭 링크를 생성 할 장치
  * @name : 심볼릭 링크의 이름
  * @gpio : gpio는 심볼릭 링크를 만들고, 이미 내 보낸
  *
  * /sys/.../dev/name에서 / sys / class / gpio / gpioN 노드로 심볼릭 링크를 설정하십시오. 발신자는 링크 해제를 담당합니다.
  *
  * 성공하면 0을 반환하고, 그렇지 않으면 오류를 반환합니다.
 */
int gpiod_export_link(struct device *dev, const char *name,
		      struct gpio_desc *desc)
{
	struct device *cdev;
	int ret;

	if (!desc) {
		pr_warn("%s: invalid GPIO\n", __func__);
		return -EINVAL;
	}

	cdev = class_find_device(&gpio_class, NULL, desc, match_export);
	if (!cdev)
		return -ENODEV;

	ret = sysfs_create_link(&dev->kobj, &cdev->kobj, name);
	put_device(cdev);

	return ret;
}
EXPORT_SYMBOL_GPL(gpiod_export_link);

/**
 * gpiod_unexport - reverse effect of gpio_export()
 * @gpio: gpio to make unavailable
 *
 * This is implicit on gpio_free().

  * gpiod_unexport - gpio_export ()의 반대 효과
  * @gpio : gpio를 사용할 수 없도록 설정합니다.
  *
  * 이것은 gpio_free ()에서 암시 적입니다.
 */
void gpiod_unexport(struct gpio_desc *desc)
{
	struct gpiod_data *data;
	struct device *dev;

	if (!desc) {
		pr_warn("%s: invalid GPIO\n", __func__);
		return;
	}

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		goto err_unlock;

	dev = class_find_device(&gpio_class, NULL, desc, match_export);
	if (!dev)
		goto err_unlock;

	data = dev_get_drvdata(dev);

	clear_bit(FLAG_EXPORT, &desc->flags);

	device_unregister(dev);

	/*
	 * Release irq after deregistration to prevent race with edge_store.
	   탈퇴 후에 irq를 해제하여 edge_store와의 경쟁을 방지하십시오.
	 */
	if (data->irq_flags)
		gpio_sysfs_free_irq(dev);

	mutex_unlock(&sysfs_lock);

	put_device(dev);
	kfree(data);

	return;

err_unlock:
	mutex_unlock(&sysfs_lock);
}
EXPORT_SYMBOL_GPL(gpiod_unexport);

int gpiochip_sysfs_register(struct gpio_chip *chip)
{
	struct device	*dev;

	/*
	 * Many systems add gpio chips for SOC support very early,
	 * before driver model support is available.  In those cases we
	 * register later, in gpiolib_sysfs_init() ... here we just
	 * verify that _some_ field of gpio_class got initialized.
	 * 많은 시스템은 드라이버 모델 지원이 가능하기 전에 SOC 지원을 위해 gpio 칩을 아주 일찍 추가합니다. 나중에 gpiolib_sysfs_init ()에 등록하는 경우 gpio_class의 _some_ 필드가 초기화되었는지 확인합니다.
	 */
	if (!gpio_class.p)
		return 0;

	/* use chip->base for the ID; it's already known to be unique 
	    ID에 chip-> base를 사용하십시오. 그것은 이미 독특하다고 알려져 있습니다 */
	dev = device_create_with_groups(&gpio_class, chip->dev, MKDEV(0, 0),
					chip, gpiochip_groups,
					"gpiochip%d", chip->base);
	if (IS_ERR(dev))
		return PTR_ERR(dev);

	mutex_lock(&sysfs_lock);
	chip->cdev = dev;
	mutex_unlock(&sysfs_lock);

	return 0;
}

void gpiochip_sysfs_unregister(struct gpio_chip *chip)
{
	struct gpio_desc *desc;
	unsigned int i;

	if (!chip->cdev)
		return;

	device_unregister(chip->cdev);

	/* prevent further gpiod exports 
	    추가 gpiod 내보내기 방지				*/
	mutex_lock(&sysfs_lock);
	chip->cdev = NULL;
	mutex_unlock(&sysfs_lock);

	/* unregister gpiod class devices owned by sysfs
	   sysfs가 소유 한 gpiod 클래스 장치의 등록을 취소합니다	 */
	for (i = 0; i < chip->ngpio; i++) {
		desc = &chip->desc[i];
		if (test_and_clear_bit(FLAG_SYSFS, &desc->flags))
			gpiod_free(desc);
	}
}

static int __init gpiolib_sysfs_init(void)
{
	int		status;
	unsigned long	flags;
	struct gpio_chip *chip;

	status = class_register(&gpio_class);
	if (status < 0)
		return status;

	/* Scan and register the gpio_chips which registered very
	 * early (e.g. before the class_register above was called).
	 *
	 * We run before arch_initcall() so chip->dev nodes can have
	 * registered, and so arch_initcall() can always gpio_export().
	
	 * 매우 일찍 등록 된 gpio_chips를 검색하고 등록하십시오 (예 : 위의 class_register가 호출되기 전에).

	 * arch_initcall () 전에 실행하여 칩 -> dev 노드를 등록 할 수 있으므로 arch_initcall ()은 항상 gpio_export ()를 수행 할 수 있습니다.
	 */
	spin_lock_irqsave(&gpio_lock, flags);
	list_for_each_entry(chip, &gpio_chips, list) {
		if (chip->cdev)
			continue;

		/*
		 * TODO we yield gpio_lock here because
		 * gpiochip_sysfs_register() acquires a mutex. This is unsafe
		 * and needs to be fixed.
		 *
		 * Also it would be nice to use gpiochip_find() here so we
		 * can keep gpio_chips local to gpiolib.c, but the yield of
		 * gpio_lock prevents us from doing this.

		* TODO gpiochip_sysfs_register ()가 뮤텍스를 획득하기 때문에 여기서 gpio_lock을 산출합니다. 이것은 안전하지 않으므로 수정해야합니다.

		* gpio_chips를 gpiolib.c에 로컬로 유지할 수 있도록 gpiochip_find ()를 사용하는 것이 좋겠지 만 gpio_lock의 수율로 인해이 작업을 수행 할 수 없습니다.
		 */
		spin_unlock_irqrestore(&gpio_lock, flags);
		status = gpiochip_sysfs_register(chip);
		spin_lock_irqsave(&gpio_lock, flags);
	}
	spin_unlock_irqrestore(&gpio_lock, flags);


	return status;
}
postcore_initcall(gpiolib_sysfs_init);
