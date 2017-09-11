#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/idr.h>
#include <linux/slab.h>
#include <linux/acpi.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/machine.h>
#include <linux/pinctrl/consumer.h>

#include "gpiolib.h"

#define CREATE_TRACE_POINTS
#include <trace/events/gpio.h>

/* Implementation infrastructure for GPIO interfaces.
 *
 * The GPIO programming interface allows for inlining speed-critical
 * get/set operations for common cases, so that access to SOC-integrated
 * GPIOs can sometimes cost only an instruction or two per bit.

  * GPIO 인터페이스 구현 인프라.
  *
  * GPIO 프로그래밍 인터페이스는 일반적인 경우에 속도가 중요한 get / set 동작을 인라이닝 할 수 있으므로 SOC 통합 GPIO에 대한 액세스는 종종 명령어 당 1 비트 또는 2 비트에 불과합니다.
 */


/* When debugging, extend minimal trust to callers and platform code.
 * Also emit diagnostic messages that may help initial bringup, when
 * board setup or driver bugs are most common.
 *
 * Otherwise, minimize overhead in what may be bitbanging codepaths.
  
  디버깅 할 때 호출자와 플랫폼 코드에 대한 최소한의 신뢰를 확장하십시오.
  보드 초기화 또는 드라이버 버그가 가장 흔한 초기 진단에 도움이되는 진단 메시지를 내 보냅니다.
  
  그렇지 않으면 비트 밴딩 코드 패스가 될 수있는 오버 헤드를 최소화하십시오.
 */
#ifdef	DEBUG
#define	extra_checks	1
#else
#define	extra_checks	0
#endif

/* gpio_lock prevents conflicts during gpio_desc[] table updates.
 * While any GPIO is requested, its gpio_chip is not removable;
 * each GPIO's "requested" flag serves as a lock and refcount.

  * gpio_lock은 gpio_desc [] 테이블 업데이트 중 충돌을 방지합니다.
  * GPIO가 요청되는 동안 gpio_chip은 제거 할 수 없습니다.
  * 각 GPIO의 "requested"플래그는 잠금 및 참조 횟수로 사용됩니다.
 */
DEFINE_SPINLOCK(gpio_lock);

static DEFINE_MUTEX(gpio_lookup_lock);
static LIST_HEAD(gpio_lookup_list);
LIST_HEAD(gpio_chips);


static void gpiochip_free_hogs(struct gpio_chip *chip);
static void gpiochip_irqchip_remove(struct gpio_chip *gpiochip);


static inline void desc_set_label(struct gpio_desc *d, const char *label)
{
	d->label = label;
}

/**
 * Convert a GPIO number to its descriptor
   GPIO 번호를 해당 설명자로 변환
 */
struct gpio_desc *gpio_to_desc(unsigned gpio)
{
	struct gpio_chip *chip;
	unsigned long flags;

	spin_lock_irqsave(&gpio_lock, flags);

	list_for_each_entry(chip, &gpio_chips, list) {
		if (chip->base <= gpio && chip->base + chip->ngpio > gpio) {
			spin_unlock_irqrestore(&gpio_lock, flags);
			return &chip->desc[gpio - chip->base];
		}
	}

	spin_unlock_irqrestore(&gpio_lock, flags);

	if (!gpio_is_valid(gpio))
		WARN(1, "invalid GPIO %d\n", gpio);

	return NULL;
}
EXPORT_SYMBOL_GPL(gpio_to_desc);

/**
 * Get the GPIO descriptor corresponding to the given hw number for this chip.
   이 칩에 대해 주어진 hw 번호에 해당하는 GPIO 설명자를 가져옵니다.
 */
struct gpio_desc *gpiochip_get_desc(struct gpio_chip *chip,
				    u16 hwnum)
{
	if (hwnum >= chip->ngpio)
		return ERR_PTR(-EINVAL);

	return &chip->desc[hwnum];
}

/**
 * Convert a GPIO descriptor to the integer namespace.
 * This should disappear in the future but is needed since we still
 * use GPIO numbers for error messages and sysfs nodes

 * GPIO 기술자를 정수 네임 스페이스로 변환하십시오.
 * 앞으로는 사라지 겠지만 오류 메시지와 sysfs 노드에 GPIO 번호를 사용하기 때문에 필요합니다.
 */
int desc_to_gpio(const struct gpio_desc *desc)
{
	return desc->chip->base + (desc - &desc->chip->desc[0]);
}
EXPORT_SYMBOL_GPL(desc_to_gpio);


/**
 * gpiod_to_chip - Return the GPIO chip to which a GPIO descriptor belongs
 * @desc:	descriptor to return the chip of

  * gpiod_to_chip - GPIO 디스크립터가 속한 GPIO 칩 반환
  * @desc : 칩을 반환하는 설명자
 */
struct gpio_chip *gpiod_to_chip(const struct gpio_desc *desc)
{
	return desc ? desc->chip : NULL;
}
EXPORT_SYMBOL_GPL(gpiod_to_chip);

/* dynamic allocation of GPIOs, e.g. on a hotplugged device
   GPIO의 동적 할당. 핫 플러그 장치				 */
static int gpiochip_find_base(int ngpio)
{
	struct gpio_chip *chip;
	int base = ARCH_NR_GPIOS - ngpio;

	list_for_each_entry_reverse(chip, &gpio_chips, list) {
		/* found a free space? */
		if (chip->base + chip->ngpio <= base)
			break;
		else
			/* nope, check the space right before the chip */
			base = chip->base - ngpio;
	}

	if (gpio_is_valid(base)) {
		pr_debug("%s: found new base at %d\n", __func__, base);
		return base;
	} else {
		pr_err("%s: cannot find free range\n", __func__);
		return -ENOSPC;
	}
}

/**
 * gpiod_get_direction - return the current direction of a GPIO
 * @desc:	GPIO to get the direction of
 *
 * Return GPIOF_DIR_IN or GPIOF_DIR_OUT, or an error code in case of error.
 *
 * This function may sleep if gpiod_cansleep() is true.

  * gpiod_get_direction - GPIO의 현재 방향을 반환합니다.
  * @desc : GPIO의 방향을 잡으려고
  *
  * GPIOF_DIR_IN 또는 GPIOF_DIR_OUT을 반환하거나 오류가 발생하면 오류 코드를 반환합니다.
  *
  *이 함수는 gpiod_cansleep ()이 참이면 잠자기 될 수 있습니다.
 */
int gpiod_get_direction(struct gpio_desc *desc)
{
	struct gpio_chip	*chip;
	unsigned		offset;
	int			status = -EINVAL;

	chip = gpiod_to_chip(desc);
	offset = gpio_chip_hwgpio(desc);

	if (!chip->get_direction)
		return status;

	status = chip->get_direction(chip, offset);
	if (status > 0) {
		/* GPIOF_DIR_IN, or other positive */
		status = 1;
		clear_bit(FLAG_IS_OUT, &desc->flags);
	}
	if (status == 0) {
		/* GPIOF_DIR_OUT */
		set_bit(FLAG_IS_OUT, &desc->flags);
	}
	return status;
}
EXPORT_SYMBOL_GPL(gpiod_get_direction);

/*
 * Add a new chip to the global chips list, keeping the list of chips sorted
 * by base order.
 *
 * Return -EBUSY if the new chip overlaps with some other chip's integer
 * space.

  * 글로벌 칩 목록에 새 칩을 추가하여 기본 주문별로 정렬 된 칩 목록을 유지합니다.
  *
  * Return - 새로운 칩이 다른 칩의 정수 공간과 겹치면 유효합니다.
 */
static int gpiochip_add_to_list(struct gpio_chip *chip)
{
	struct list_head *pos;
	struct gpio_chip *_chip;
	int err = 0;

	/* find where to insert our chip 
	   칩을 삽입 할 위치를 찾는다. 		*/
	list_for_each(pos, &gpio_chips) {
		_chip = list_entry(pos, struct gpio_chip, list);
		/* shall we insert before _chip? 
	  	   우리는 _chip 앞에 삽입해야합니까? 	*/
		if (_chip->base >= chip->base + chip->ngpio)
			break;
	}

	/* are we stepping on the chip right before?
	   우린 칩을 밟고 계십니까?			 */
	if (pos != &gpio_chips && pos->prev != &gpio_chips) {
		_chip = list_entry(pos->prev, struct gpio_chip, list);
		if (_chip->base + _chip->ngpio > chip->base) {
			dev_err(chip->dev,
			       "GPIO integer space overlap, cannot add chip\n");
			err = -EBUSY;
		}
	}

	if (!err)
		list_add_tail(&chip->list, pos);

	return err;
}

/**
 * Convert a GPIO name to its descriptor
   GPIO 이름을 해당 설명자로 변환
 */
static struct gpio_desc *gpio_name_to_desc(const char * const name)
{
	struct gpio_chip *chip;
	unsigned long flags;

	spin_lock_irqsave(&gpio_lock, flags);

	list_for_each_entry(chip, &gpio_chips, list) {
		int i;

		for (i = 0; i != chip->ngpio; ++i) {
			struct gpio_desc *gpio = &chip->desc[i];

			if (!gpio->name || !name)
				continue;

			if (!strcmp(gpio->name, name)) {
				spin_unlock_irqrestore(&gpio_lock, flags);
				return gpio;
			}
		}
	}

	spin_unlock_irqrestore(&gpio_lock, flags);

	return NULL;
}

/*
 * Takes the names from gc->names and checks if they are all unique. If they
 * are, they are assigned to their gpio descriptors.
 *
 * Returns -EEXIST if one of the names is already used for a different GPIO.

  * gc-> 이름에서 이름을 가져 와서 모두 고유한지 확인합니다. 그럴 경우 gpio 디스크립터에 할당됩니다.
  
  * 이름 중 하나가 이미 다른 GPIO에 사용 된 경우 -EEXIST를 반환합니다.
 */
static int gpiochip_set_desc_names(struct gpio_chip *gc)
{
	int i;

	if (!gc->names)
		return 0;

	/* First check all names if they are unique 
	   먼저 모든 이름이 고유한지 확인하십시오. 	*/
	for (i = 0; i != gc->ngpio; ++i) {
		struct gpio_desc *gpio;

		gpio = gpio_name_to_desc(gc->names[i]);
		if (gpio)
			dev_warn(gc->dev, "Detected name collision for "
				 "GPIO name '%s'\n",
				 gc->names[i]);
	}

	/* Then add all names to the GPIO descriptors 
	   그런 다음 모든 이름을 GPIO 설명자에 추가하십시오. */
	for (i = 0; i != gc->ngpio; ++i)
		gc->desc[i].name = gc->names[i];

	return 0;
}

/**
 * gpiochip_add() - register a gpio_chip
 * @chip: the chip to register, with chip->base initialized
 * Context: potentially before irqs will work
 *
 * Returns a negative errno if the chip can't be registered, such as
 * because the chip->base is invalid or already associated with a
 * different chip.  Otherwise it returns zero as a success code.
 *
 * When gpiochip_add() is called very early during boot, so that GPIOs
 * can be freely used, the chip->dev device must be registered before
 * the gpio framework's arch_initcall().  Otherwise sysfs initialization
 * for GPIOs will fail rudely.
 *
 * If chip->base is negative, this requests dynamic assignment of
 * a range of valid GPIOs.

  * gpiochip_add () - gpio_chip 등록
  * @ 칩 : 등록 할 칩, 칩 -> 기반 초기화 됨
  * 컨텍스트 : irq가 작동하기 전에 잠재적으로
  * 칩 ->베이스가 유효하지 않거나 다른 칩과 이미 연관되어 있기 때문에 칩을 등록 할 수없는 경우 음의 errno를 반환합니다. 그렇지 않으면 0을 성공 코드로 리턴합니다.
  * gpiochip_add ()가 부팅 중 매우 일찍 호출되어 GPIO를 자유롭게 사용할 수 있도록 gpio 프레임 워크의 arch_initcall () 전에 chip-> dev 장치를 등록해야합니다. 그렇지 않으면 GPIO 용 sysfs 초기화가 엉뚱하게 실패합니다.
  *
  * chip-> base가 음수이면 유효 GPIO 범위의 동적 할당을 요구합니다.
 */
int gpiochip_add(struct gpio_chip *chip)
{
	unsigned long	flags;
	int		status = 0;
	unsigned	id;
	int		base = chip->base;
	struct gpio_desc *descs;

	descs = kcalloc(chip->ngpio, sizeof(descs[0]), GFP_KERNEL);
	if (!descs)
		return -ENOMEM;

	spin_lock_irqsave(&gpio_lock, flags);

	if (base < 0) {
		base = gpiochip_find_base(chip->ngpio);
		if (base < 0) {
			status = base;
			spin_unlock_irqrestore(&gpio_lock, flags);
			goto err_free_descs;
		}
		chip->base = base;
	}

	status = gpiochip_add_to_list(chip);
	if (status) {
		spin_unlock_irqrestore(&gpio_lock, flags);
		goto err_free_descs;
	}

	for (id = 0; id < chip->ngpio; id++) {
		struct gpio_desc *desc = &descs[id];

		desc->chip = chip;

		/* REVISIT: most hardware initializes GPIOs as inputs (often
		 * with pullups enabled) so power usage is minimized. Linux
		 * code should set the gpio direction first thing; but until
		 * it does, and in case chip->get_direction is not set, we may
		 * expose the wrong direction in sysfs.
		   REVISIT : 대부분의 하드웨어는 GPIO를 입력으로 풀 초기화 (풀업 사용 가능)하여 전력 사용을 최소화합니다. 
	           Linux 코드는 gpio 방향을 먼저 설정해야합니다. 
		   하지만 그렇게 될 때까지는 chip-> get_direction이 설정되지 않은 경우 sysfs에서 잘못된 방향을 보여줄 수 있습니다.
		 */
		desc->flags = !chip->direction_input ? (1 << FLAG_IS_OUT) : 0;
	}

	chip->desc = descs;

	spin_unlock_irqrestore(&gpio_lock, flags);

#ifdef CONFIG_PINCTRL
	INIT_LIST_HEAD(&chip->pin_ranges);
#endif

	if (!chip->owner && chip->dev && chip->dev->driver)
		chip->owner = chip->dev->driver->owner;

	status = gpiochip_set_desc_names(chip);
	if (status)
		goto err_remove_from_list;

	status = of_gpiochip_add(chip);
	if (status)
		goto err_remove_chip;

	acpi_gpiochip_add(chip);

	status = gpiochip_sysfs_register(chip);
	if (status)
		goto err_remove_chip;

	pr_debug("%s: registered GPIOs %d to %d on device: %s\n", __func__,
		chip->base, chip->base + chip->ngpio - 1,
		chip->label ? : "generic");

	return 0;

err_remove_chip:
	acpi_gpiochip_remove(chip);
	gpiochip_free_hogs(chip);
	of_gpiochip_remove(chip);
err_remove_from_list:
	spin_lock_irqsave(&gpio_lock, flags);
	list_del(&chip->list);
	spin_unlock_irqrestore(&gpio_lock, flags);
	chip->desc = NULL;
err_free_descs:
	kfree(descs);

	/* failures here can mean systems won't boot...
	   여기의 실패는 시스템이 부팅되지 않는다는 것을 의미 할 수 있습니다 ... */
	pr_err("%s: GPIOs %d..%d (%s) failed to register\n", __func__,
		chip->base, chip->base + chip->ngpio - 1,
		chip->label ? : "generic");
	return status;
}
EXPORT_SYMBOL_GPL(gpiochip_add);

/**
 * gpiochip_remove() - unregister a gpio_chip
 * @chip: the chip to unregister
 *
 * A gpio_chip with any GPIOs still requested may not be removed.
 
  * gpiochip_remove () - gpio_chip 등록을 취소합니다.
  * @ 칩 : 등록을 취소 할 칩
  *
  * 요청한 GPIO가있는 gpio_chip은 제거 할 수 없습니다.
 */
void gpiochip_remove(struct gpio_chip *chip)
{
	struct gpio_desc *desc;
	unsigned long	flags;
	unsigned	id;
	bool		requested = false;

	gpiochip_sysfs_unregister(chip);

	gpiochip_irqchip_remove(chip);

	acpi_gpiochip_remove(chip);
	gpiochip_remove_pin_ranges(chip);
	gpiochip_free_hogs(chip);
	of_gpiochip_remove(chip);

	spin_lock_irqsave(&gpio_lock, flags);
	for (id = 0; id < chip->ngpio; id++) {
		desc = &chip->desc[id];
		desc->chip = NULL;
		if (test_bit(FLAG_REQUESTED, &desc->flags))
			requested = true;
	}
	list_del(&chip->list);
	spin_unlock_irqrestore(&gpio_lock, flags);

	if (requested)
		dev_crit(chip->dev, "REMOVING GPIOCHIP WITH GPIOS STILL REQUESTED\n");

	kfree(chip->desc);
	chip->desc = NULL;
}
EXPORT_SYMBOL_GPL(gpiochip_remove);

/**
 * gpiochip_find() - iterator for locating a specific gpio_chip
 * @data: data to pass to match function
 * @callback: Callback function to check gpio_chip
 *
 * Similar to bus_find_device.  It returns a reference to a gpio_chip as
 * determined by a user supplied @match callback.  The callback should return
 * 0 if the device doesn't match and non-zero if it does.  If the callback is
 * non-zero, this function will return to the caller and not iterate over any
 * more gpio_chips.

  * gpiochip_find () - 특정 gpio_chip을 찾기위한 반복자
  * @ 데이터 : 일치하는 함수에 전달할 데이터
  * @callback : gpio_chip을 검사하는 콜백 함수
  *
  * bus_find_device와 유사합니다. 사용자가 제공 한 @match 콜백에 의해 결정된대로 gpio_chip에 대한 참조를 반환합니다. 장치가 일치하지 않으면 콜백은 0을 반환하고 그렇지 않으면 콜백은 0이 아닙니다. 콜백이 0이 아니면이 함수는 호출자에게 돌아가고 더 이상 gpio_chips를 반복하지 않습니다.
 */
struct gpio_chip *gpiochip_find(void *data,
				int (*match)(struct gpio_chip *chip,
					     void *data))
{
	struct gpio_chip *chip;
	unsigned long flags;

	spin_lock_irqsave(&gpio_lock, flags);
	list_for_each_entry(chip, &gpio_chips, list)
		if (match(chip, data))
			break;

	/* No match? */
	if (&chip->list == &gpio_chips)
		chip = NULL;
	spin_unlock_irqrestore(&gpio_lock, flags);

	return chip;
}
EXPORT_SYMBOL_GPL(gpiochip_find);

static int gpiochip_match_name(struct gpio_chip *chip, void *data)
{
	const char *name = data;

	return !strcmp(chip->label, name);
}

static struct gpio_chip *find_chip_by_name(const char *name)
{
	return gpiochip_find((void *)name, gpiochip_match_name);
}

#ifdef CONFIG_GPIOLIB_IRQCHIP

/*
 * The following is irqchip helper code for gpiochips.
 */

/**
 * gpiochip_set_chained_irqchip() - sets a chained irqchip to a gpiochip
 * @gpiochip: the gpiochip to set the irqchip chain to
 * @irqchip: the irqchip to chain to the gpiochip
 * @parent_irq: the irq number corresponding to the parent IRQ for this
 * chained irqchip
 * @parent_handler: the parent interrupt handler for the accumulated IRQ
 * coming out of the gpiochip. If the interrupt is nested rather than
 * cascaded, pass NULL in this handler argument

  * 다음은 gpiochips 용 irqchip 도우미 코드입니다.
  
  * gpiochip_set_chained_irqchip () - 연결된 irqchip을 gpiochip으로 설정합니다.
  * @gpiochip : irqchip 체인을 설정하는 gpiochip
  * @irqchip : gpiochip에 연결된 irqchip
  * @parent_irq :이 연결된 irqchip에 대한 부모 IRQ에 해당하는 irq 번호
  * @parent_handler : gpiochip에서 나온 누적 된 IRQ의 부모 인터럽트 처리기입니다. 인터럽트가 계단식이 아닌 중첩 된 경우이 핸들러 인수에 NULL을 전달합니다.
 */
void gpiochip_set_chained_irqchip(struct gpio_chip *gpiochip,
				  struct irq_chip *irqchip,
				  int parent_irq,
				  irq_flow_handler_t parent_handler)
{
	unsigned int offset;

	if (!gpiochip->irqdomain) {
		chip_err(gpiochip, "called %s before setting up irqchip\n",
			 __func__);
		return;
	}

	if (parent_handler) {
		if (gpiochip->can_sleep) {
			chip_err(gpiochip,
				 "you cannot have chained interrupts on a "
				 "chip that may sleep\n");
			return;
		}
		/*
		 * The parent irqchip is already using the chip_data for this
		 * irqchip, so our callbacks simply use the handler_data.
		   부모 irqchip는 이미이 irqchip에 대해 chip_data를 사용하고 있으므로 콜백은 단순히 handler_data를 사용합니다.
		 */
		irq_set_chained_handler_and_data(parent_irq, parent_handler,
						 gpiochip);

		gpiochip->irq_parent = parent_irq;
	}

	/* Set the parent IRQ for all affected IRQs 
	   영향을받는 모든 IRQ에 대해 부모 IRQ 설정	*/
	for (offset = 0; offset < gpiochip->ngpio; offset++)
		irq_set_parent(irq_find_mapping(gpiochip->irqdomain, offset),
			       parent_irq);
}
EXPORT_SYMBOL_GPL(gpiochip_set_chained_irqchip);

/**
 * gpiochip_irq_map() - maps an IRQ into a GPIO irqchip
 * @d: the irqdomain used by this irqchip
 * @irq: the global irq number used by this GPIO irqchip irq
 * @hwirq: the local IRQ/GPIO line offset on this gpiochip
 *
 * This function will set up the mapping for a certain IRQ line on a
 * gpiochip by assigning the gpiochip as chip data, and using the irqchip
 * stored inside the gpiochip.

  * gpiochip_irq_map () - IRQ를 GPIO irqchip으로 매핑합니다.
  * @d :이 irqchip에서 사용하는 irqdomain
  * @irq :이 GPIO에 의해 사용 된 글로벌 irq 번호 irqchip irq
  * @hwirq :이 gpiochip의 로컬 IRQ / GPIO 라인 오프셋
  *
  * 이 함수는 gpiochip을 칩 데이터로 할당하고 gpiochip 내부에 저장된 irqchip을 사용하여 gpiochip의 특정 IRQ 라인에 대한 매핑을 설정합니다.
 */
static int gpiochip_irq_map(struct irq_domain *d, unsigned int irq,
			    irq_hw_number_t hwirq)
{
	struct gpio_chip *chip = d->host_data;

	irq_set_chip_data(irq, chip);
	/*
	 * This lock class tells lockdep that GPIO irqs are in a different
	 * category than their parents, so it won't report false recursion.
	   이 잠금 클래스는 GPIO irq가 부모와 다른 카테고리에 있다는 것을 lockdep에 알려주기 때문에 잘못된 재귀를보고하지 않습니다.
	 */
	irq_set_lockdep_class(irq, chip->lock_key);
	irq_set_chip_and_handler(irq, chip->irqchip, chip->irq_handler);
	/* Chips that can sleep need nested thread handlers 
	   잠들 수있는 칩은 중첩 된 스레드 처리기가 필요합니다   */
	if (chip->can_sleep && !chip->irq_not_threaded)
		irq_set_nested_thread(irq, 1);
	irq_set_noprobe(irq);

	/*
	 * No set-up of the hardware will happen if IRQ_TYPE_NONE
	 * is passed as default type.
	   IRQ_TYPE_NONE이 기본 유형으로 전달되면 하드웨어 설정이 이루어지지 않습니다.
	 */
	if (chip->irq_default_type != IRQ_TYPE_NONE)
		irq_set_irq_type(irq, chip->irq_default_type);

	return 0;
}

static void gpiochip_irq_unmap(struct irq_domain *d, unsigned int irq)
{
	struct gpio_chip *chip = d->host_data;

	if (chip->can_sleep)
		irq_set_nested_thread(irq, 0);
	irq_set_chip_and_handler(irq, NULL, NULL);
	irq_set_chip_data(irq, NULL);
}

static const struct irq_domain_ops gpiochip_domain_ops = {
	.map	= gpiochip_irq_map,
	.unmap	= gpiochip_irq_unmap,
	/* Virtually all GPIO irqchips are twocell:ed
	   거의 모든 GPIO 칩은 두 가지 종류로되어 있습니다. */
	.xlate	= irq_domain_xlate_twocell,
};

static int gpiochip_irq_reqres(struct irq_data *d)
{
	struct gpio_chip *chip = irq_data_get_irq_chip_data(d);

	if (!try_module_get(chip->owner))
		return -ENODEV;

	if (gpiochip_lock_as_irq(chip, d->hwirq)) {
		chip_err(chip,
			"unable to lock HW IRQ %lu for IRQ\n",
			d->hwirq);
		module_put(chip->owner);
		return -EINVAL;
	}
	return 0;
}

static void gpiochip_irq_relres(struct irq_data *d)
{
	struct gpio_chip *chip = irq_data_get_irq_chip_data(d);

	gpiochip_unlock_as_irq(chip, d->hwirq);
	module_put(chip->owner);
}

static int gpiochip_to_irq(struct gpio_chip *chip, unsigned offset)
{
	return irq_find_mapping(chip->irqdomain, offset);
}

/**
 * gpiochip_irqchip_remove() - removes an irqchip added to a gpiochip
 * @gpiochip: the gpiochip to remove the irqchip from
 *
 * This is called only from gpiochip_remove()
  * gpiochip_irqchip_remove () - gpiochip에 추가 된 irqchip을 제거합니다.
  * @gpiochip : irqchip을 제거하는 gpiochip
  *
  * 이것은 gpiochip_remove ()에서만 호출됩니다.
 */
static void gpiochip_irqchip_remove(struct gpio_chip *gpiochip)
{
	unsigned int offset;

	acpi_gpiochip_free_interrupts(gpiochip);

	if (gpiochip->irq_parent) {
		irq_set_chained_handler(gpiochip->irq_parent, NULL);
		irq_set_handler_data(gpiochip->irq_parent, NULL);
	}

	/* Remove all IRQ mappings and delete the domain
	   모든 IRQ 매핑을 제거하고 도메인을 삭제하십시오.	 */
	if (gpiochip->irqdomain) {
		for (offset = 0; offset < gpiochip->ngpio; offset++)
			irq_dispose_mapping(
				irq_find_mapping(gpiochip->irqdomain, offset));
		irq_domain_remove(gpiochip->irqdomain);
	}

	if (gpiochip->irqchip) {
		gpiochip->irqchip->irq_request_resources = NULL;
		gpiochip->irqchip->irq_release_resources = NULL;
		gpiochip->irqchip = NULL;
	}
}

/**
 * gpiochip_irqchip_add() - adds an irqchip to a gpiochip
 * @gpiochip: the gpiochip to add the irqchip to
 * @irqchip: the irqchip to add to the gpiochip
 * @first_irq: if not dynamically assigned, the base (first) IRQ to
 * allocate gpiochip irqs from
 * @handler: the irq handler to use (often a predefined irq core function)
 * @type: the default type for IRQs on this irqchip, pass IRQ_TYPE_NONE
 * to have the core avoid setting up any default type in the hardware.
 * @lock_key: lockdep class
 *
 * This function closely associates a certain irqchip with a certain
 * gpiochip, providing an irq domain to translate the local IRQs to
 * global irqs in the gpiolib core, and making sure that the gpiochip
 * is passed as chip data to all related functions. Driver callbacks
 * need to use container_of() to get their local state containers back
 * from the gpiochip passed as chip data. An irqdomain will be stored
 * in the gpiochip that shall be used by the driver to handle IRQ number
 * translation. The gpiochip will need to be initialized and registered
 * before calling this function.
 *
 * This function will handle two cell:ed simple IRQs and assumes all
 * the pins on the gpiochip can generate a unique IRQ. Everything else
 * need to be open coded.

 * gpiochip_irqchip_add () - irqchip을 gpiochip에 추가합니다.
 * @gpiochip : irqchip을 추가 할 gpiochip
 * @irqchip : gpiochip에 추가 할 irqchip
 * @first_irq : 동적으로 할당되지 않은 경우, gpiochip irqs를 할당하는 기본 (첫 번째) IRQ
 * @handler : 사용할 irq 핸들러 (종종 미리 정의 된 irq 핵심 함수)
 * @type :이 irqchip에있는 IRQ의 기본 유형으로, IRQ_TYPE_NONE을 전달하여 코어가 하드웨어의 기본 유형을 설정하지 않도록합니다.
 * @lock_key : lockdep 클래스
 *
 *이 함수는 밀접 gpiolib 코어 글로벌 IRQ를 로컬 IRQ를 번역 할 수있는 IRQ 도메인을 제공하며 gpiochip 관련된 모든 기능 칩 데이터로서 전달되는 것을 확인하고, 소정 gpiochip와 특정 irqchip 연관시킨다. 드라이버 콜백은 container_of ()를 사용하여 칩 데이터로 전달 된 gpio 칩에서 로컬 상태 컨테이너를 다시 가져와야합니다. IRQ 번호 변환을 처리하기 위해 드라이버가 사용할 irqdomain이 gpiochip에 저장됩니다. 이 기능을 호출하기 전에 gpiochip을 초기화하고 등록해야합니다.
 *
 *이 함수는 두 개의 셀 : ed 단순 IRQ를 처리하고 gpiochip의 모든 핀이 고유 한 IRQ를 생성 할 수 있다고 가정합니다. 다른 모든 것은 공개 된 코드로 작성해야합니다.
 */
int _gpiochip_irqchip_add(struct gpio_chip *gpiochip,
			  struct irq_chip *irqchip,
			  unsigned int first_irq,
			  irq_flow_handler_t handler,
			  unsigned int type,
			  struct lock_class_key *lock_key)
{
	struct device_node *of_node;
	unsigned int offset;
	unsigned irq_base = 0;

	if (!gpiochip || !irqchip)
		return -EINVAL;

	if (!gpiochip->dev) {
		pr_err("missing gpiochip .dev parent pointer\n");
		return -EINVAL;
	}
	of_node = gpiochip->dev->of_node;
#ifdef CONFIG_OF_GPIO
	/*
	 * If the gpiochip has an assigned OF node this takes precedence
	 * FIXME: get rid of this and use gpiochip->dev->of_node everywhere
	 
	* gpiochip에 할당 된 OF 노드가있는 경우이 우선 순위가 적용됩니다.	
	* FIXME : 이것을 없애고 어디서나 gpiochip-> dev-> of_node를 사용하십시오.
	 */
	if (gpiochip->of_node)
		of_node = gpiochip->of_node;
#endif
	gpiochip->irqchip = irqchip;
	gpiochip->irq_handler = handler;
	gpiochip->irq_default_type = type;
	gpiochip->to_irq = gpiochip_to_irq;
	gpiochip->lock_key = lock_key;
	gpiochip->irqdomain = irq_domain_add_simple(of_node,
					gpiochip->ngpio, first_irq,
					&gpiochip_domain_ops, gpiochip);
	if (!gpiochip->irqdomain) {
		gpiochip->irqchip = NULL;
		return -EINVAL;
	}

	/*
	 * It is possible for a driver to override this, but only if the
	 * alternative functions are both implemented.
	   드라이버가이 기능을 무시할 수는 있지만 대체 기능이 모두 구현 된 경우에만 가능합니다.
	 */
	if (!irqchip->irq_request_resources &&
	    !irqchip->irq_release_resources) {
		irqchip->irq_request_resources = gpiochip_irq_reqres;
		irqchip->irq_release_resources = gpiochip_irq_relres;
	}

	/*
	 * Prepare the mapping since the irqchip shall be orthogonal to
	 * any gpiochip calls. If the first_irq was zero, this is
	 * necessary to allocate descriptors for all IRQs.
	   
	   irqchip은 모든 gpiochip 호출과 직교해야하므로 맵핑을 준비하십시오. 
	   first_irq가 0이면 모든 IRQ에 대한 설명자를 할당하는 데 필요합니다.
	 */
	for (offset = 0; offset < gpiochip->ngpio; offset++) {
		irq_base = irq_create_mapping(gpiochip->irqdomain, offset);
		if (offset == 0)
			/*
			 * Store the base into the gpiochip to be used when
			 * unmapping the irqs.

   	 	           irq를 매핑 해제 할 때 사용할베이스를 gpiochip에 저장하십시오.
			 */
			gpiochip->irq_base = irq_base;
	}

	acpi_gpiochip_request_interrupts(gpiochip);

	return 0;
}
EXPORT_SYMBOL_GPL(_gpiochip_irqchip_add);

#else /* CONFIG_GPIOLIB_IRQCHIP */

static void gpiochip_irqchip_remove(struct gpio_chip *gpiochip) {}

#endif /* CONFIG_GPIOLIB_IRQCHIP */

/**
 * gpiochip_generic_request() - request the gpio function for a pin
 * @chip: the gpiochip owning the GPIO
 * @offset: the offset of the GPIO to request for GPIO function

  * gpiochip_generic_request () - 핀에 대한 gpio 함수를 요청합니다.
  * @ 칩 : GPIO를 소유 한 GPIO 칩
  * @offset : GPIO 함수 요청을위한 GPIO 오프셋
 */
int gpiochip_generic_request(struct gpio_chip *chip, unsigned offset)
{
	return pinctrl_request_gpio(chip->base + offset);
}
EXPORT_SYMBOL_GPL(gpiochip_generic_request);

/**
 * gpiochip_generic_free() - free the gpio function from a pin
 * @chip: the gpiochip to request the gpio function for
 * @offset: the offset of the GPIO to free from GPIO function
 */
void gpiochip_generic_free(struct gpio_chip *chip, unsigned offset)
{
	pinctrl_free_gpio(chip->base + offset);
}
EXPORT_SYMBOL_GPL(gpiochip_generic_free);

#ifdef CONFIG_PINCTRL

/**
 * gpiochip_add_pingroup_range() - add a range for GPIO <-> pin mapping
 * @chip: the gpiochip to add the range for
 * @pctldev: the pin controller to map to
 * @gpio_offset: the start offset in the current gpio_chip number space
 * @pin_group: name of the pin group inside the pin controller

  * gpiochip_add_pingroup_range () - GPIO <-> 핀 매핑을위한 범위 추가
  * @ 칩 : 범위를 추가 할 gpiochip
  * @ pctldev : 매핑 할 핀 컨트롤러
  * @gpio_offset : 현재 gpio_chip 번호 공간의 시작 오프셋
  * @ 핀 그룹 : 핀 컨트롤러 내부의 핀 그룹 명
 */
int gpiochip_add_pingroup_range(struct gpio_chip *chip,
			struct pinctrl_dev *pctldev,
			unsigned int gpio_offset, const char *pin_group)
{
	struct gpio_pin_range *pin_range;
	int ret;

	pin_range = kzalloc(sizeof(*pin_range), GFP_KERNEL);
	if (!pin_range) {
		chip_err(chip, "failed to allocate pin ranges\n");
		return -ENOMEM;
	}

	/* Use local offset as range ID 
	   범위 ID로 로컬 오프셋 사용		*/
	pin_range->range.id = gpio_offset;
	pin_range->range.gc = chip;
	pin_range->range.name = chip->label;
	pin_range->range.base = chip->base + gpio_offset;
	pin_range->pctldev = pctldev;

	ret = pinctrl_get_group_pins(pctldev, pin_group,
					&pin_range->range.pins,
					&pin_range->range.npins);
	if (ret < 0) {
		kfree(pin_range);
		return ret;
	}

	pinctrl_add_gpio_range(pctldev, &pin_range->range);

	chip_dbg(chip, "created GPIO range %d->%d ==> %s PINGRP %s\n",
		 gpio_offset, gpio_offset + pin_range->range.npins - 1,
		 pinctrl_dev_get_devname(pctldev), pin_group);

	list_add_tail(&pin_range->node, &chip->pin_ranges);

	return 0;
}
EXPORT_SYMBOL_GPL(gpiochip_add_pingroup_range);

/**
 * gpiochip_add_pin_range() - add a range for GPIO <-> pin mapping
 * @chip: the gpiochip to add the range for
 * @pinctrl_name: the dev_name() of the pin controller to map to
 * @gpio_offset: the start offset in the current gpio_chip number space
 * @pin_offset: the start offset in the pin controller number space
 * @npins: the number of pins from the offset of each pin space (GPIO and
 *	pin controller) to accumulate in this range

  * gpiochip_add_pin_range () - GPIO <-> 핀 매핑을위한 범위 추가
  * @ 칩 : 범위를 추가 할 gpiochip
  * @pinctrl_name : 매핑 할 핀 컨트롤러의 dev_name ()
  * @gpio_offset : 현재 gpio_chip 번호 공간의 시작 오프셋
  * @pin_offset : 핀 콘트롤러 번호 공간의 개시 오프셋 (offset)
  * @npins :이 범위에 누적되는 각 핀 공간 (GPIO 및 핀 컨트롤러) 오프셋의 핀 수
 */
int gpiochip_add_pin_range(struct gpio_chip *chip, const char *pinctl_name,
			   unsigned int gpio_offset, unsigned int pin_offset,
			   unsigned int npins)
{
	struct gpio_pin_range *pin_range;
	int ret;

	pin_range = kzalloc(sizeof(*pin_range), GFP_KERNEL);
	if (!pin_range) {
		chip_err(chip, "failed to allocate pin ranges\n");
		return -ENOMEM;
	}

	/* Use local offset as range ID
	   범위 ID로 로컬 오프셋 사용	 */
	pin_range->range.id = gpio_offset;
	pin_range->range.gc = chip;
	pin_range->range.name = chip->label;
	pin_range->range.base = chip->base + gpio_offset;
	pin_range->range.pin_base = pin_offset;
	pin_range->range.npins = npins;
	pin_range->pctldev = pinctrl_find_and_add_gpio_range(pinctl_name,
			&pin_range->range);
	if (IS_ERR(pin_range->pctldev)) {
		ret = PTR_ERR(pin_range->pctldev);
		chip_err(chip, "could not create pin range\n");
		kfree(pin_range);
		return ret;
	}
	chip_dbg(chip, "created GPIO range %d->%d ==> %s PIN %d->%d\n",
		 gpio_offset, gpio_offset + npins - 1,
		 pinctl_name,
		 pin_offset, pin_offset + npins - 1);

	list_add_tail(&pin_range->node, &chip->pin_ranges);

	return 0;
}
EXPORT_SYMBOL_GPL(gpiochip_add_pin_range);

/**
 * gpiochip_remove_pin_ranges() - remove all the GPIO <-> pin mappings
 * @chip: the chip to remove all the mappings for

  * gpiochip_remove_pin_ranges () - 모든 GPIO <-> 핀 매핑 제거
  * @ 칩 :에 대한 모든 매핑을 제거하는 칩
 */
void gpiochip_remove_pin_ranges(struct gpio_chip *chip)
{
	struct gpio_pin_range *pin_range, *tmp;

	list_for_each_entry_safe(pin_range, tmp, &chip->pin_ranges, node) {
		list_del(&pin_range->node);
		pinctrl_remove_gpio_range(pin_range->pctldev,
				&pin_range->range);
		kfree(pin_range);
	}
}
EXPORT_SYMBOL_GPL(gpiochip_remove_pin_ranges);

#endif /* CONFIG_PINCTRL */

/* These "optional" allocation calls help prevent drivers from stomping
 * on each other, and help provide better diagnostics in debugfs.
 * They're called even less than the "set direction" calls.

이러한 "선택적인"할당 호출은 드라이버가 서로 쾅쾅 거리지 못하게하고 디버그에서보다 나은 진단을 제공하는 데 도움이됩니다. 그것들은 "방향 설정"호출보다 적게 호출됩니다.
 */
static int __gpiod_request(struct gpio_desc *desc, const char *label)
{
	struct gpio_chip	*chip = desc->chip;
	int			status;
	unsigned long		flags;

	spin_lock_irqsave(&gpio_lock, flags);

	/* NOTE:  gpio_request() can be called in early boot,
	 * before IRQs are enabled, for non-sleeping (SOC) GPIOs.
	   참고 : gpio_request ()는 IRQ가 활성화되기 전에 조기 부팅에서 non-sleeping (SOC) GPIO를 호출 할 수 있습니다.
	 */

	if (test_and_set_bit(FLAG_REQUESTED, &desc->flags) == 0) {
		desc_set_label(desc, label ? : "?");
		status = 0;
	} else {
		status = -EBUSY;
		goto done;
	}

	if (chip->request) {
		/* chip->request may sleep */
		spin_unlock_irqrestore(&gpio_lock, flags);
		status = chip->request(chip, gpio_chip_hwgpio(desc));
		spin_lock_irqsave(&gpio_lock, flags);

		if (status < 0) {
			desc_set_label(desc, NULL);
			clear_bit(FLAG_REQUESTED, &desc->flags);
			goto done;
		}
	}
	if (chip->get_direction) {
		/* chip->get_direction may sleep */
		spin_unlock_irqrestore(&gpio_lock, flags);
		gpiod_get_direction(desc);
		spin_lock_irqsave(&gpio_lock, flags);
	}
done:
	spin_unlock_irqrestore(&gpio_lock, flags);
	return status;
}

int gpiod_request(struct gpio_desc *desc, const char *label)
{
	int status = -EPROBE_DEFER;
	struct gpio_chip *chip;

	if (!desc) {
		pr_warn("%s: invalid GPIO\n", __func__);
		return -EINVAL;
	}

	chip = desc->chip;
	if (!chip)
		goto done;

	if (try_module_get(chip->owner)) {
		status = __gpiod_request(desc, label);
		if (status < 0)
			module_put(chip->owner);
	}

done:
	if (status)
		gpiod_dbg(desc, "%s: status %d\n", __func__, status);

	return status;
}

static bool __gpiod_free(struct gpio_desc *desc)
{
	bool			ret = false;
	unsigned long		flags;
	struct gpio_chip	*chip;

	might_sleep();

	gpiod_unexport(desc);

	spin_lock_irqsave(&gpio_lock, flags);

	chip = desc->chip;
	if (chip && test_bit(FLAG_REQUESTED, &desc->flags)) {
		if (chip->free) {
			spin_unlock_irqrestore(&gpio_lock, flags);
			might_sleep_if(chip->can_sleep);
			chip->free(chip, gpio_chip_hwgpio(desc));
			spin_lock_irqsave(&gpio_lock, flags);
		}
		desc_set_label(desc, NULL);
		clear_bit(FLAG_ACTIVE_LOW, &desc->flags);
		clear_bit(FLAG_REQUESTED, &desc->flags);
		clear_bit(FLAG_OPEN_DRAIN, &desc->flags);
		clear_bit(FLAG_OPEN_SOURCE, &desc->flags);
		clear_bit(FLAG_IS_HOGGED, &desc->flags);
		ret = true;
	}

	spin_unlock_irqrestore(&gpio_lock, flags);
	return ret;
}

void gpiod_free(struct gpio_desc *desc)
{
	if (desc && __gpiod_free(desc))
		module_put(desc->chip->owner);
	else
		WARN_ON(extra_checks);
}

/**
 * gpiochip_is_requested - return string iff signal was requested
 * @chip: controller managing the signal
 * @offset: of signal within controller's 0..(ngpio - 1) range
 *
 * Returns NULL if the GPIO is not currently requested, else a string.
 * The string returned is the label passed to gpio_request(); if none has been
 * passed it is a meaningless, non-NULL constant.
 *
 * This function is for use by GPIO controller drivers.  The label can
 * help with diagnostics, and knowing that the signal is used as a GPIO
 * can help avoid accidentally multiplexing it to another controller.

  * gpiochip_is_requested - 신호가 요청 된 경우 문자열 반환
  * @chip : 신호를 관리하는 컨트롤러
  * @offset : 제어기의 0 .. (ngpio - 1) 범위 내의 신호
  *
    GPIO가 현재 요청되지 않은 경우 NULL을 반환하고 그렇지 않으면 문자열을 반환합니다.
  * 반환 된 문자열은 gpio_request ()에 전달 된 레이블입니다. 아무 것도 전달되지 않으면 의미가없고 NULL이 아닌 상수입니다.
  *
  * 이 기능은 GPIO 컨트롤러 드라이버 용입니다. 이 라벨은 진단에 도움이 될 수 있으며 신호가 GPIO로 사용된다는 것을 알고 있으면 실수로 다른 컨트롤러에 신호가 다중화되는 것을 방지 할 수 있습니다.
 */
const char *gpiochip_is_requested(struct gpio_chip *chip, unsigned offset)
{
	struct gpio_desc *desc;

	if (offset >= chip->ngpio)
		return NULL;

	desc = &chip->desc[offset];

	if (test_bit(FLAG_REQUESTED, &desc->flags) == 0)
		return NULL;
	return desc->label;
}
EXPORT_SYMBOL_GPL(gpiochip_is_requested);

/**
 * gpiochip_request_own_desc - Allow GPIO chip to request its own descriptor
 * @desc: GPIO descriptor to request
 * @label: label for the GPIO
 *
 * Function allows GPIO chip drivers to request and use their own GPIO
 * descriptors via gpiolib API. Difference to gpiod_request() is that this
 * function will not increase reference count of the GPIO chip module. This
 * allows the GPIO chip module to be unloaded as needed (we assume that the
 * GPIO chip driver handles freeing the GPIOs it has requested).

  * gpiochip_request_own_desc - GPIO 칩이 자체 디스크립터를 요청하도록 허용
  * @desc : 요청할 GPIO 설명자
  * @ 라벨 : GPIO 라벨
  *
  * 기능은 GPIO 칩 드라이버가 gpiolib API를 통해 자신의 GPIO 설명자를 요청하고 사용할 수있게합니다. gpiod_request ()와 다른 점은이 함수는 GPIO 칩 모듈의 참조 카운트를 증가시키지 않는다는 것입니다. 이를 통해 필요에 따라 GPIO 칩 모듈을 언로드 할 수 있습니다 (GPIO 칩 드라이버가 요청한 GPIO를 해제한다고 가정합니다).
 */
struct gpio_desc *gpiochip_request_own_desc(struct gpio_chip *chip, u16 hwnum,
					    const char *label)
{
	struct gpio_desc *desc = gpiochip_get_desc(chip, hwnum);
	int err;

	if (IS_ERR(desc)) {
		chip_err(chip, "failed to get GPIO descriptor\n");
		return desc;
	}

	err = __gpiod_request(desc, label);
	if (err < 0)
		return ERR_PTR(err);

	return desc;
}
EXPORT_SYMBOL_GPL(gpiochip_request_own_desc);

/**
 * gpiochip_free_own_desc - Free GPIO requested by the chip driver
 * @desc: GPIO descriptor to free
 *
 * Function frees the given GPIO requested previously with
 * gpiochip_request_own_desc().

  * gpiochip_free_own_desc - 칩 드라이버가 요청한 무료 GPIO
  * @desc : 무료 GPIO 디스크립터
  *
  * 함수는 이전에 gpiochip_request_own_desc ()로 요청한 주어진 GPIO를 해제합니다.
 */
void gpiochip_free_own_desc(struct gpio_desc *desc)
{
	if (desc)
		__gpiod_free(desc);
}
EXPORT_SYMBOL_GPL(gpiochip_free_own_desc);

/* Drivers MUST set GPIO direction before making get/set calls.  In
 * some cases this is done in early boot, before IRQs are enabled.
 *
 * As a rule these aren't called more than once (except for drivers
 * using the open-drain emulation idiom) so these are natural places
 * to accumulate extra debugging checks.  Note that we can't (yet)
 * rely on gpio_request() having been called beforehand.
 */

/**
 * gpiod_direction_input - set the GPIO direction to input
 * @desc:	GPIO to set to input
 *
 * Set the direction of the passed GPIO to input, such as gpiod_get_value() can
 * be called safely on it.
 *
 * Return 0 in case of success, else an error code.
 
  * 드라이버는 get / set 호출을하기 전에 반드시 GPIO 방향을 설정해야합니다. 경우에 따라 IRQ를 사용하기 전에 초기 부팅에서 수행되는 경우도 있습니다.
  *
  * 원칙적으로 이들은 오픈 드레 이뮬레이션 에뮬레이션 관용구를 사용하는 드라이버를 제외하고 두 번 이상 호출되지 않으므로 추가 디버깅 검사를 누적하는 자연스러운 장소입니다. 미리 호출 된 gpio_request ()에 의존 할 수는 없습니다.
  
  * gpiod_direction_input - GPIO 방향을 입력으로 설정합니다.
  * @desc : 입력으로 설정할 GPIO
  *
  * 전달 된 GPIO의 방향을 gpiod_get_value ()와 같이 입력으로 설정하면 안전하게 호출 할 수 있습니다.
  *
  * 성공한 경우 0을 반환하고, 그렇지 않으면 오류 코드를 반환합니다.
 */
int gpiod_direction_input(struct gpio_desc *desc)
{
	struct gpio_chip	*chip;
	int			status = -EINVAL;

	if (!desc || !desc->chip) {
		pr_warn("%s: invalid GPIO\n", __func__);
		return -EINVAL;
	}

	chip = desc->chip;
	if (!chip->get || !chip->direction_input) {
		gpiod_warn(desc,
			"%s: missing get() or direction_input() operations\n",
			__func__);
		return -EIO;
	}

	status = chip->direction_input(chip, gpio_chip_hwgpio(desc));
	if (status == 0)
		clear_bit(FLAG_IS_OUT, &desc->flags);

	trace_gpio_direction(desc_to_gpio(desc), 1, status);

	return status;
}
EXPORT_SYMBOL_GPL(gpiod_direction_input);

static int _gpiod_direction_output_raw(struct gpio_desc *desc, int value)
{
	struct gpio_chip	*chip;
	int			status = -EINVAL;

	/* GPIOs used for IRQs shall not be set as output 
	   IRQ에 사용되는 GPIO는 출력으로 설정되지 않습니다.	*/
	if (test_bit(FLAG_USED_AS_IRQ, &desc->flags)) {
		gpiod_err(desc,
			  "%s: tried to set a GPIO tied to an IRQ as output\n",
			  __func__);
		return -EIO;
	}

	/* Open drain pin should not be driven to 1 
	   오픈 드레인 핀은 1					*/
	if (value && test_bit(FLAG_OPEN_DRAIN,  &desc->flags))
		return gpiod_direction_input(desc);

	/* Open source pin should not be driven to 0
	   오픈 소스 핀을 0으로 구동해서는 안됩니다  		*/
	if (!value && test_bit(FLAG_OPEN_SOURCE,  &desc->flags))
		return gpiod_direction_input(desc);

	chip = desc->chip;
	if (!chip->set || !chip->direction_output) {
		gpiod_warn(desc,
		       "%s: missing set() or direction_output() operations\n",
		       __func__);
		return -EIO;
	}

	status = chip->direction_output(chip, gpio_chip_hwgpio(desc), value);
	if (status == 0)
		set_bit(FLAG_IS_OUT, &desc->flags);
	trace_gpio_value(desc_to_gpio(desc), 0, value);
	trace_gpio_direction(desc_to_gpio(desc), 0, status);
	return status;
}

/**
 * gpiod_direction_output_raw - set the GPIO direction to output
 * @desc:	GPIO to set to output
 * @value:	initial output value of the GPIO
 *
 * Set the direction of the passed GPIO to output, such as gpiod_set_value() can
 * be called safely on it. The initial value of the output must be specified
 * as raw value on the physical line without regard for the ACTIVE_LOW status.
 *
 * Return 0 in case of success, else an error code.

  * gpiod_direction_output_raw - GPIO 방향을 출력으로 설정합니다.
  * @desc : 출력으로 설정하는 GPIO
  * @value : GPIO 초기 출력 값
  *
  * gpiod_set_value ()가 안전하게 호출 될 수 있도록 전달 된 GPIO의 방향을 출력으로 설정하십시오. 출력의 초기 값은 ACTIVE_LOW 상태와 상관없이 물리적 라인에서 원래 값으로 지정되어야합니다.
  *
  * 성공한 경우 0을 반환하고, 그렇지 않으면 오류 코드를 반환합니다.
 */
int gpiod_direction_output_raw(struct gpio_desc *desc, int value)
{
	if (!desc || !desc->chip) {
		pr_warn("%s: invalid GPIO\n", __func__);
		return -EINVAL;
	}
	return _gpiod_direction_output_raw(desc, value);
}
EXPORT_SYMBOL_GPL(gpiod_direction_output_raw);

/**
 * gpiod_direction_output - set the GPIO direction to output
 * @desc:	GPIO to set to output
 * @value:	initial output value of the GPIO
 *
 * Set the direction of the passed GPIO to output, such as gpiod_set_value() can
 * be called safely on it. The initial value of the output must be specified
 * as the logical value of the GPIO, i.e. taking its ACTIVE_LOW status into
 * account.
 *
 * Return 0 in case of success, else an error code.

  * gpiod_direction_output - GPIO 방향을 출력으로 설정합니다.
  * @desc : 출력으로 설정하는 GPIO
  * @value : GPIO 초기 출력 값
  *
  * gpiod_set_value ()가 안전하게 호출 될 수 있도록 전달 된 GPIO의 방향을 출력으로 설정하십시오. 출력의 초기 값은 GPIO의 논리 값으로 지정되어야합니다. 즉, ACTIVE_LOW 상태를 고려해야합니다.
  *
  * 성공한 경우 0을 반환하고, 그렇지 않으면 오류 코드를 반환합니다.
 */
int gpiod_direction_output(struct gpio_desc *desc, int value)
{
	if (!desc || !desc->chip) {
		pr_warn("%s: invalid GPIO\n", __func__);
		return -EINVAL;
	}
	if (test_bit(FLAG_ACTIVE_LOW, &desc->flags))
		value = !value;
	return _gpiod_direction_output_raw(desc, value);
}
EXPORT_SYMBOL_GPL(gpiod_direction_output);

/**
 * gpiod_set_debounce - sets @debounce time for a @gpio
 * @gpio: the gpio to set debounce time
 * @debounce: debounce time is microseconds
 *
 * returns -ENOTSUPP if the controller does not support setting
 * debounce.

  * gpiod_set_debounce - @gpio에 대한 @debounce 시간을 설정합니다.
  * @gpio : 디 바운스 시간을 설정하는 gpio
  * @debounce : 디버깅 시간은 마이크로 초입니다.
  *
  * 컨트롤러가 debounce 설정을 지원하지 않으면 -ENOTSUPP를 반환합니다.
 */
int gpiod_set_debounce(struct gpio_desc *desc, unsigned debounce)
{
	struct gpio_chip	*chip;

	if (!desc || !desc->chip) {
		pr_warn("%s: invalid GPIO\n", __func__);
		return -EINVAL;
	}

	chip = desc->chip;
	if (!chip->set || !chip->set_debounce) {
		gpiod_dbg(desc,
			  "%s: missing set() or set_debounce() operations\n",
			  __func__);
		return -ENOTSUPP;
	}

	return chip->set_debounce(chip, gpio_chip_hwgpio(desc), debounce);
}
EXPORT_SYMBOL_GPL(gpiod_set_debounce);

/**
 * gpiod_is_active_low - test whether a GPIO is active-low or not
 * @desc: the gpio descriptor to test
 *
 * Returns 1 if the GPIO is active-low, 0 otherwise.
  * gpiod_is_active_low - GPIO가 액티브 로우인지 아닌지 테스트
  * @desc : 테스트 할 gpio 설명자
  *
  * GPIO가 활성 낮음이면 1을, 그렇지 않으면 0을 반환합니다.
 */
int gpiod_is_active_low(const struct gpio_desc *desc)
{
	return test_bit(FLAG_ACTIVE_LOW, &desc->flags);
}
EXPORT_SYMBOL_GPL(gpiod_is_active_low);

/* I/O calls are only valid after configuration completed; the relevant
 * "is this a valid GPIO" error checks should already have been done.
 *
 * "Get" operations are often inlinable as reading a pin value register,
 * and masking the relevant bit in that register.
 *
 * When "set" operations are inlinable, they involve writing that mask to
 * one register to set a low value, or a different register to set it high.
 * Otherwise locking is needed, so there may be little value to inlining.
 *
 *------------------------------------------------------------------------
 *
 * IMPORTANT!!!  The hot paths -- get/set value -- assume that callers
 * have requested the GPIO.  That can include implicit requesting by
 * a direction setting call.  Marking a gpio as requested locks its chip
 * in memory, guaranteeing that these table lookups need no more locking
 * and that gpiochip_remove() will fail.
 *
 * REVISIT when debugging, consider adding some instrumentation to ensure
 * that the GPIO was actually requested.

  * I / O 호출은 구성이 완료된 후에 만 유효합니다. 관련 "유효한 GPIO입니까?"오류 검사가 이미 완료되어 있어야합니다.
  * "Get"연산은 종종 핀 값 레지스터를 읽고 해당 레지스터의 관련 비트를 마스킹하는 것으로 인라인 할 수 있습니다.
  * "set"연산이 인라인 가능할 때, 낮은 값을 설정하기 위해 하나의 레지스터에 마스크를 쓰거나 높은 값을 설정하기 위해 다른 레지스터를 쓰는 것을 포함합니다.
  * 그렇지 않으면 잠금이 필요하므로 인라인 할 가치가 거의 없습니다.
  *
  * ------------------------------------------------- -----------------------
  *
  * 중요 !!! 핫 경로 - get / set value - 발신자가 GPIO를 요청했다고 가정합니다. 이는 방향 설정 호출에 의한 암시 적 요청을 포함 할 수 있습니다. 요청 된 gpio를 마킹하면 메모리에 칩이 잠기므로 이러한 테이블 조회에 더 이상 잠금이 필요하지 않으며 gpiochip_remove ()가 실패합니다.
  * 디버깅 할 때 다시 작성하십시오. GPIO가 실제로 요청되었는지 확인하기 위해 일부 계측 추가를 고려하십시오.
 */

static int _gpiod_get_raw_value(const struct gpio_desc *desc)
{
	struct gpio_chip	*chip;
	int offset;
	int value;

	chip = desc->chip;
	offset = gpio_chip_hwgpio(desc);
	value = chip->get ? chip->get(chip, offset) : -EIO;
	/*
	 * FIXME: fix all drivers to clamp to [0,1] or return negative,
	 * then change this to:
	 * value = value < 0 ? value : !!value;
	 * so we can properly propagate error codes.
	 
	  * FIXME : 모든 드라이버를 [0,1]에 고정 시키거나 음수를 반환하도록 수정 한 후이를 다음으로 변경하십시오.
	  * 값 = 값 <0? 가치 : !! 가치;
	  * 우리는 오류 코드를 올바르게 전달할 수 있습니다.
	 */
	value = !!value;
	trace_gpio_value(desc_to_gpio(desc), 1, value);
	return value;
}

/**
 * gpiod_get_raw_value() - return a gpio's raw value
 * @desc: gpio whose value will be returned
 *
 * Return the GPIO's raw value, i.e. the value of the physical line disregarding
 * its ACTIVE_LOW status, or negative errno on failure.
 *
 * This function should be called from contexts where we cannot sleep, and will
 * complain if the GPIO chip functions potentially sleep.

  * gpiod_get_raw_value () - 값이 반환 될 gpio의 원시 값 
    @desc : gpio를 반환합니다.
  * GPIO의 원시 값, 즉 ACTIVE_LOW 상태를 무시한 물리적 라인의 값 또는 실패시 음의 errno를 반환합니다.
  * 이 함수는 잠들 수없는 컨텍스트에서 호출해야하며 GPIO 칩이 잠자기 상태 일 경우 불평 할 것입니다.
 */
int gpiod_get_raw_value(const struct gpio_desc *desc)
{
	if (!desc)
		return 0;
	/* Should be using gpio_get_value_cansleep()
	   gpio_get_value_cansleep ()을 사용해야합니다.	 */
	WARN_ON(desc->chip->can_sleep);
	return _gpiod_get_raw_value(desc);
}
EXPORT_SYMBOL_GPL(gpiod_get_raw_value);

/**
 * gpiod_get_value() - return a gpio's value
 * @desc: gpio whose value will be returned
 *
 * Return the GPIO's logical value, i.e. taking the ACTIVE_LOW status into
 * account, or negative errno on failure.
 *
 * This function should be called from contexts where we cannot sleep, and will
 * complain if the GPIO chip functions potentially sleep.

  * gpiod_get_value () - GPIO의 값을 반환합니다.
  * @desc : 값이 반환 될 gpio
  *
  * GPIO의 논리 값을 반환합니다. 즉, ACTIVE_LOW 상태를 고려하거나 실패하면 음수 errno를 반환합니다.
  *
  * 이 함수는 잠들 수없는 컨텍스트에서 호출해야하며 GPIO 칩이 잠자기 상태 일 경우 불평 할 것입니다.
 */
int gpiod_get_value(const struct gpio_desc *desc)
{
	int value;
	if (!desc)
		return 0;
	/* Should be using gpio_get_value_cansleep() 
	   gpio_get_value_cansleep ()을 사용해야합니다.	*/
	WARN_ON(desc->chip->can_sleep);

	value = _gpiod_get_raw_value(desc);
	if (value < 0)
		return value;

	if (test_bit(FLAG_ACTIVE_LOW, &desc->flags))
		value = !value;

	return value;
}
EXPORT_SYMBOL_GPL(gpiod_get_value);

/*
 *  _gpio_set_open_drain_value() - Set the open drain gpio's value.
 * @desc: gpio descriptor whose state need to be set.
 * @value: Non-zero for setting it HIGH otherwise it will set to LOW.

  * _gpio_set_open_drain_value () - 오픈 드레인 gpio의 값을 설정합니다.
  * @desc : 상태를 설정해야하는 gpio 설명자.
  * @value : HIGH로 설정하면 0이 아니고 그렇지 않으면 LOW로 설정됩니다.
 */
static void _gpio_set_open_drain_value(struct gpio_desc *desc, bool value)
{
	int err = 0;
	struct gpio_chip *chip = desc->chip;
	int offset = gpio_chip_hwgpio(desc);

	if (value) {
		err = chip->direction_input(chip, offset);
		if (!err)
			clear_bit(FLAG_IS_OUT, &desc->flags);
	} else {
		err = chip->direction_output(chip, offset, 0);
		if (!err)
			set_bit(FLAG_IS_OUT, &desc->flags);
	}
	trace_gpio_direction(desc_to_gpio(desc), value, err);
	if (err < 0)
		gpiod_err(desc,
			  "%s: Error in set_value for open drain err %d\n",
			  __func__, err);
}

/*
 *  _gpio_set_open_source_value() - Set the open source gpio's value.
 * @desc: gpio descriptor whose state need to be set.
 * @value: Non-zero for setting it HIGH otherwise it will set to LOW.

  * _gpio_set_open_source_value () - 오픈 소스 gpio의 값을 설정합니다.
  * @desc : 상태를 설정해야하는 gpio 설명자.
  * @value : HIGH로 설정하면 0이 아니고 그렇지 않으면 LOW로 설정됩니다.
 */
static void _gpio_set_open_source_value(struct gpio_desc *desc, bool value)
{
	int err = 0;
	struct gpio_chip *chip = desc->chip;
	int offset = gpio_chip_hwgpio(desc);

	if (value) {
		err = chip->direction_output(chip, offset, 1);
		if (!err)
			set_bit(FLAG_IS_OUT, &desc->flags);
	} else {
		err = chip->direction_input(chip, offset);
		if (!err)
			clear_bit(FLAG_IS_OUT, &desc->flags);
	}
	trace_gpio_direction(desc_to_gpio(desc), !value, err);
	if (err < 0)
		gpiod_err(desc,
			  "%s: Error in set_value for open source err %d\n",
			  __func__, err);
}

static void _gpiod_set_raw_value(struct gpio_desc *desc, bool value)
{
	struct gpio_chip	*chip;

	chip = desc->chip;
	trace_gpio_value(desc_to_gpio(desc), 0, value);
	if (test_bit(FLAG_OPEN_DRAIN, &desc->flags))
		_gpio_set_open_drain_value(desc, value);
	else if (test_bit(FLAG_OPEN_SOURCE, &desc->flags))
		_gpio_set_open_source_value(desc, value);
	else
		chip->set(chip, gpio_chip_hwgpio(desc), value);
}

/*
 * set multiple outputs on the same chip;
 * use the chip's set_multiple function if available;
 * otherwise set the outputs sequentially;
 * @mask: bit mask array; one bit per output; BITS_PER_LONG bits per word
 *        defines which outputs are to be changed
 * @bits: bit value array; one bit per output; BITS_PER_LONG bits per word
 *        defines the values the outputs specified by mask are to be set to

  * 동일한 칩에 여러 개의 출력 설정;
  * 가능한 경우 칩의 set_multiple 함수를 사용하십시오.
  * 그렇지 않으면 순차적으로 출력을 설정합니다.
  * @mask : 비트 마스크 배열. 출력 당 1 비트; 한 단어 당 BITS_PER_LONG 비트가 변경 될 출력을 정의합니다.
  * @ 비트 : 비트 값 배열; 출력 당 1 비트; 한 단어 당 BITS_PER_LONG 비트는 mask로 지정된 출력을로 설정하는 값을 정의합니다.
 */
static void gpio_chip_set_multiple(struct gpio_chip *chip,
				   unsigned long *mask, unsigned long *bits)
{
	if (chip->set_multiple) {
		chip->set_multiple(chip, mask, bits);
	} else {
		int i;
		for (i = 0; i < chip->ngpio; i++) {
			if (mask[BIT_WORD(i)] == 0) {
				/* no more set bits in this mask word;
				 * skip ahead to the next word 
				 * 이 마스크 단어에는 더 이상 비트가 설정되지 않습니다.
				 * 다음 단어로 건너 뜁니다. **/
				i = (BIT_WORD(i) + 1) * BITS_PER_LONG - 1;
				continue;
			}
			/* set outputs if the corresponding mask bit is set
	 	           해당 마스크 비트가 설정되면 출력 설정			 */
			if (__test_and_clear_bit(i, mask))
				chip->set(chip, i, test_bit(i, bits));
		}
	}
}

static void gpiod_set_array_value_priv(bool raw, bool can_sleep,
				       unsigned int array_size,
				       struct gpio_desc **desc_array,
				       int *value_array)
{
	int i = 0;

	while (i < array_size) {
		struct gpio_chip *chip = desc_array[i]->chip;
		unsigned long mask[BITS_TO_LONGS(chip->ngpio)];
		unsigned long bits[BITS_TO_LONGS(chip->ngpio)];
		int count = 0;

		if (!can_sleep)
			WARN_ON(chip->can_sleep);

		memset(mask, 0, sizeof(mask));
		do {
			struct gpio_desc *desc = desc_array[i];
			int hwgpio = gpio_chip_hwgpio(desc);
			int value = value_array[i];

			if (!raw && test_bit(FLAG_ACTIVE_LOW, &desc->flags))
				value = !value;
			trace_gpio_value(desc_to_gpio(desc), 0, value);
			/*
			 * collect all normal outputs belonging to the same chip
			 * open drain and open source outputs are set individually
			   동일한 칩 오픈 드레인에 속하는 모든 일반 출력 수집 및 오픈 소스 출력은 개별적으로 설정됩니다.
			 */
			if (test_bit(FLAG_OPEN_DRAIN, &desc->flags)) {
				_gpio_set_open_drain_value(desc, value);
			} else if (test_bit(FLAG_OPEN_SOURCE, &desc->flags)) {
				_gpio_set_open_source_value(desc, value);
			} else {
				__set_bit(hwgpio, mask);
				if (value)
					__set_bit(hwgpio, bits);
				else
					__clear_bit(hwgpio, bits);
				count++;
			}
			i++;
		} while ((i < array_size) && (desc_array[i]->chip == chip));
		/* push collected bits to outputs
		   수집 된 비트를 출력 		  */
		if (count != 0)
			gpio_chip_set_multiple(chip, mask, bits);
	}
}

/**
 * gpiod_set_raw_value() - assign a gpio's raw value
 * @desc: gpio whose value will be assigned
 * @value: value to assign
 *
 * Set the raw value of the GPIO, i.e. the value of its physical line without
 * regard for its ACTIVE_LOW status.
 *
 * This function should be called from contexts where we cannot sleep, and will
 * complain if the GPIO chip functions potentially sleep.

  * gpiod_set_raw_value () - gpio의 원시 값을 지정합니다.
  * @desc : 값이 할당 될 gpio
  * @value : 할당 할 값
  * GPIO의 원시 값, 즉 ACTIVE_LOW 상태와 상관없이 물리적 라인의 값을 설정합니다.
  *이 함수는 잠들 수없는 컨텍스트에서 호출해야하며 GPIO 칩이 잠자기 상태 일 경우 불평 할 것입니다.
 */
void gpiod_set_raw_value(struct gpio_desc *desc, int value)
{
	if (!desc)
		return;
	/* Should be using gpio_set_value_cansleep()
	  gpio_set_value_cansleep ()을 사용해야합니다 	 */
	WARN_ON(desc->chip->can_sleep);
	_gpiod_set_raw_value(desc, value);
}
EXPORT_SYMBOL_GPL(gpiod_set_raw_value);

/**
 * gpiod_set_value() - assign a gpio's value
 * @desc: gpio whose value will be assigned
 * @value: value to assign
 *
 * Set the logical value of the GPIO, i.e. taking its ACTIVE_LOW status into
 * account
 *
 * This function should be called from contexts where we cannot sleep, and will
 * complain if the GPIO chip functions potentially sleep.

  * gpiod_set_value () - gpio의 값을 지정합니다.
  * @desc : 값이 할당 될 gpio
  * @value : 할당 할 값
  *
  * GPIO의 논리 값을 설정합니다. 즉, ACTIVE_LOW 상태를 고려합니다.
  *
  * 이 함수는 잠들 수없는 컨텍스트에서 호출해야하며 GPIO 칩이 잠자기 상태 일 경우 불평 할 것입니다.
 */
void gpiod_set_value(struct gpio_desc *desc, int value)
{
	if (!desc)
		return;
	/* Should be using gpio_set_value_cansleep()
	   gpio_set_value_cansleep ()를 사용해야합니다.		 */
	WARN_ON(desc->chip->can_sleep);
	if (test_bit(FLAG_ACTIVE_LOW, &desc->flags))
		value = !value;
	_gpiod_set_raw_value(desc, value);
}
EXPORT_SYMBOL_GPL(gpiod_set_value);

/**
 * gpiod_set_raw_array_value() - assign values to an array of GPIOs
 * @array_size: number of elements in the descriptor / value arrays
 * @desc_array: array of GPIO descriptors whose values will be assigned
 * @value_array: array of values to assign
 *
 * Set the raw values of the GPIOs, i.e. the values of the physical lines
 * without regard for their ACTIVE_LOW status.
 *
 * This function should be called from contexts where we cannot sleep, and will
 * complain if the GPIO chip functions potentially sleep.

  * gpiod_set_raw_array_value () - GPIO 배열에 값 지정
  * @array_size : 디스크립터 / 값 배열의 요소 수
  * @desc_array : 값이 할당 될 GPIO 디스크립터의 배열
  * @value_array : 할당 할 값의 배열
  *
  * GPIO의 원시 값, 즉 ACTIVE_LOW 상태에 상관없이 물리적 라인의 값을 설정합니다.
  *
  * 이 함수는 잠들 수없는 컨텍스트에서 호출해야하며 GPIO 칩이 잠자기 상태 일 경우 불평 할 것입니다.
 */
void gpiod_set_raw_array_value(unsigned int array_size,
			 struct gpio_desc **desc_array, int *value_array)
{
	if (!desc_array)
		return;
	gpiod_set_array_value_priv(true, false, array_size, desc_array,
				   value_array);
}
EXPORT_SYMBOL_GPL(gpiod_set_raw_array_value);

/**
 * gpiod_set_array_value() - assign values to an array of GPIOs
 * @array_size: number of elements in the descriptor / value arrays
 * @desc_array: array of GPIO descriptors whose values will be assigned
 * @value_array: array of values to assign
 *
 * Set the logical values of the GPIOs, i.e. taking their ACTIVE_LOW status
 * into account.
 *
 * This function should be called from contexts where we cannot sleep, and will
 * complain if the GPIO chip functions potentially sleep.

  * gpiod_set_array_value () - GPIO 배열에 값 지정
  * @array_size : 디스크립터 / 값 배열의 요소 수
  * @desc_array : 값이 할당 될 GPIO 디스크립터의 배열
  * @value_array : 할당 할 값의 배열
  *
  * GPIO의 논리 값을 설정합니다. 즉, ACTIVE_LOW 상태를 고려합니다.
  *
  * 이 함수는 잠들 수없는 컨텍스트에서 호출해야하며 GPIO 칩이 잠자기 상태 일 경우 불평 할 것입니다.
 */
void gpiod_set_array_value(unsigned int array_size,
			   struct gpio_desc **desc_array, int *value_array)
{
	if (!desc_array)
		return;
	gpiod_set_array_value_priv(false, false, array_size, desc_array,
				   value_array);
}
EXPORT_SYMBOL_GPL(gpiod_set_array_value);

/**
 * gpiod_cansleep() - report whether gpio value access may sleep
 * @desc: gpio to check

 * gpiod_cansleep () - gpio 값 액세스가 잠자기 상태 일 수 있는지 여부를보고합니다.
 * @desc : 확인할 gpio
 *
 */
int gpiod_cansleep(const struct gpio_desc *desc)
{
	if (!desc)
		return 0;
	return desc->chip->can_sleep;
}
EXPORT_SYMBOL_GPL(gpiod_cansleep);

/**
 * gpiod_to_irq() - return the IRQ corresponding to a GPIO
 * @desc: gpio whose IRQ will be returned (already requested)
 *
 * Return the IRQ corresponding to the passed GPIO, or an error code in case of
 * error.

  * gpiod_to_irq () - GPIO에 해당하는 IRQ를 반환합니다.
  * @desc : IRQ가 반환 될 gpio (이미 요청 됨)
  *
  * 전달 된 GPIO에 해당하는 IRQ를 반환하거나 오류가 발생하면 오류 코드를 반환합니다.
 */
int gpiod_to_irq(const struct gpio_desc *desc)
{
	struct gpio_chip	*chip;
	int			offset;

	if (!desc)
		return -EINVAL;
	chip = desc->chip;
	offset = gpio_chip_hwgpio(desc);
	return chip->to_irq ? chip->to_irq(chip, offset) : -ENXIO;
}
EXPORT_SYMBOL_GPL(gpiod_to_irq);

/**
 * gpiochip_lock_as_irq() - lock a GPIO to be used as IRQ
 * @chip: the chip the GPIO to lock belongs to
 * @offset: the offset of the GPIO to lock as IRQ
 *
 * This is used directly by GPIO drivers that want to lock down
 * a certain GPIO line to be used for IRQs.

  * gpiochip_lock_as_irq () - IRQ로 사용할 GPIO 잠금
  * @ 칩 : 잠글 GPIO가 속한 칩
  * @offset : IRQ로 잠글 GPIO의 오프셋
  *
  * 이것은 GPIO 드라이버가 직접 사용하여 특정 GPIO 라인을 잠궈 IRQ에 사용합니다.
 */
int gpiochip_lock_as_irq(struct gpio_chip *chip, unsigned int offset)
{
	if (offset >= chip->ngpio)
		return -EINVAL;

	if (test_bit(FLAG_IS_OUT, &chip->desc[offset].flags)) {
		chip_err(chip,
			  "%s: tried to flag a GPIO set as output for IRQ\n",
			  __func__);
		return -EIO;
	}

	set_bit(FLAG_USED_AS_IRQ, &chip->desc[offset].flags);
	return 0;
}
EXPORT_SYMBOL_GPL(gpiochip_lock_as_irq);

/**
 * gpiochip_unlock_as_irq() - unlock a GPIO used as IRQ
 * @chip: the chip the GPIO to lock belongs to
 * @offset: the offset of the GPIO to lock as IRQ
 *
 * This is used directly by GPIO drivers that want to indicate
 * that a certain GPIO is no longer used exclusively for IRQ.

  * gpiochip_unlock_as_irq () - IRQ로 사용되는 GPIO의 잠금을 해제합니다.
  * @ 칩 : 잠글 GPIO가 속한 칩
  * @offset : IRQ로 잠글 GPIO의 오프셋
  *
  * 이것은 특정 GPIO가 더 이상 IRQ에만 사용되지 않음을 나타내는 GPIO 드라이버에서 직접 사용됩니다.
 */
void gpiochip_unlock_as_irq(struct gpio_chip *chip, unsigned int offset)
{
	if (offset >= chip->ngpio)
		return;

	clear_bit(FLAG_USED_AS_IRQ, &chip->desc[offset].flags);
}
EXPORT_SYMBOL_GPL(gpiochip_unlock_as_irq);

/**
 * gpiod_get_raw_value_cansleep() - return a gpio's raw value
 * @desc: gpio whose value will be returned
 *
 * Return the GPIO's raw value, i.e. the value of the physical line disregarding
 * its ACTIVE_LOW status, or negative errno on failure.
 *
 * This function is to be called from contexts that can sleep.

  * gpiod_get_raw_value_cansleep () - GPIO의 원래 값을 반환합니다.
  * @desc : 값이 반환 될 gpio
  *
  * GPIO의 원시 값, 즉 ACTIVE_LOW 상태를 무시한 물리적 라인의 값 또는 실패시 음의 errno를 반환합니다.
  *
  *이 함수는 잠들 수있는 컨텍스트에서 호출됩니다.
 */
int gpiod_get_raw_value_cansleep(const struct gpio_desc *desc)
{
	might_sleep_if(extra_checks);
	if (!desc)
		return 0;
	return _gpiod_get_raw_value(desc);
}
EXPORT_SYMBOL_GPL(gpiod_get_raw_value_cansleep);

/**
 * gpiod_get_value_cansleep() - return a gpio's value
 * @desc: gpio whose value will be returned
 *
 * Return the GPIO's logical value, i.e. taking the ACTIVE_LOW status into
 * account, or negative errno on failure.
 *
 * This function is to be called from contexts that can sleep.

  * gpiod_get_value_cansleep () - GPIO의 값을 반환합니다.
  * @desc : 값이 반환 될 gpio
  *
  * GPIO의 논리 값을 반환합니다. 즉, ACTIVE_LOW 상태를 고려하거나 실패하면 음수 errno를 반환합니다.
  *
  * 이 함수는 잠들 수있는 컨텍스트에서 호출됩니다.
 */
int gpiod_get_value_cansleep(const struct gpio_desc *desc)
{
	int value;

	might_sleep_if(extra_checks);
	if (!desc)
		return 0;

	value = _gpiod_get_raw_value(desc);
	if (value < 0)
		return value;

	if (test_bit(FLAG_ACTIVE_LOW, &desc->flags))
		value = !value;

	return value;
}
EXPORT_SYMBOL_GPL(gpiod_get_value_cansleep);

/**
 * gpiod_set_raw_value_cansleep() - assign a gpio's raw value
 * @desc: gpio whose value will be assigned
 * @value: value to assign
 *
 * Set the raw value of the GPIO, i.e. the value of its physical line without
 * regard for its ACTIVE_LOW status.
 *
 * This function is to be called from contexts that can sleep.

  * gpiod_set_raw_value_cansleep () - gpio의 원시 값을 지정합니다.
  * @desc : 값이 할당 될 gpio
  * @value : 할당 할 값
  *
  * GPIO의 원시 값, 즉 ACTIVE_LOW 상태와 상관없이 물리적 라인의 값을 설정합니다.
  *
  * 이 함수는 잠들 수있는 컨텍스트에서 호출됩니다.
 */
void gpiod_set_raw_value_cansleep(struct gpio_desc *desc, int value)
{
	might_sleep_if(extra_checks);
	if (!desc)
		return;
	_gpiod_set_raw_value(desc, value);
}
EXPORT_SYMBOL_GPL(gpiod_set_raw_value_cansleep);

/**
 * gpiod_set_value_cansleep() - assign a gpio's value
 * @desc: gpio whose value will be assigned
 * @value: value to assign
 *
 * Set the logical value of the GPIO, i.e. taking its ACTIVE_LOW status into
 * account
 *
 * This function is to be called from contexts that can sleep.

  * gpiod_set_value_cansleep () - gpio의 값을 지정합니다.
  * @desc : 값이 할당 될 gpio
  * @value : 할당 할 값
  *
  * GPIO의 논리 값을 설정합니다. 즉, ACTIVE_LOW 상태를 고려합니다.
  *
  * 이 함수는 잠들 수있는 컨텍스트에서 호출됩니다.
 */
void gpiod_set_value_cansleep(struct gpio_desc *desc, int value)
{
	might_sleep_if(extra_checks);
	if (!desc)
		return;

	if (test_bit(FLAG_ACTIVE_LOW, &desc->flags))
		value = !value;
	_gpiod_set_raw_value(desc, value);
}
EXPORT_SYMBOL_GPL(gpiod_set_value_cansleep);

/**
 * gpiod_set_raw_array_value_cansleep() - assign values to an array of GPIOs
 * @array_size: number of elements in the descriptor / value arrays
 * @desc_array: array of GPIO descriptors whose values will be assigned
 * @value_array: array of values to assign
 *
 * Set the raw values of the GPIOs, i.e. the values of the physical lines
 * without regard for their ACTIVE_LOW status.
 *
 * This function is to be called from contexts that can sleep.

  * gpiod_set_raw_array_value_cansleep () - GPIO 배열에 값 지정
  * @array_size : 디스크립터 / 값 배열의 요소 수
  * @desc_array : 값이 할당 될 GPIO 디스크립터의 배열
  * @value_array : 할당 할 값의 배열
  *
  * GPIO의 원시 값, 즉 ACTIVE_LOW 상태에 상관없이 물리적 라인의 값을 설정합니다.
  *
  * 이 함수는 잠들 수있는 컨텍스트에서 호출됩니다.
 */
void gpiod_set_raw_array_value_cansleep(unsigned int array_size,
					struct gpio_desc **desc_array,
					int *value_array)
{
	might_sleep_if(extra_checks);
	if (!desc_array)
		return;
	gpiod_set_array_value_priv(true, true, array_size, desc_array,
				   value_array);
}
EXPORT_SYMBOL_GPL(gpiod_set_raw_array_value_cansleep);

/**
 * gpiod_set_array_value_cansleep() - assign values to an array of GPIOs
 * @array_size: number of elements in the descriptor / value arrays
 * @desc_array: array of GPIO descriptors whose values will be assigned
 * @value_array: array of values to assign
 *
 * Set the logical values of the GPIOs, i.e. taking their ACTIVE_LOW status
 * into account.
 *
 * This function is to be called from contexts that can sleep.

  * gpiod_set_array_value_cansleep () - GPIO 배열에 값 지정
  * @array_size : 디스크립터 / 값 배열의 요소 수
  * @desc_array : 값이 할당 될 GPIO 디스크립터의 배열
  * @value_array : 할당 할 값의 배열
  *
  * GPIO의 논리 값을 설정합니다. 즉, ACTIVE_LOW 상태를 고려합니다.
  *
  * 이 함수는 잠들 수있는 컨텍스트에서 호출됩니다.
 */
void gpiod_set_array_value_cansleep(unsigned int array_size,
				    struct gpio_desc **desc_array,
				    int *value_array)
{
	might_sleep_if(extra_checks);
	if (!desc_array)
		return;
	gpiod_set_array_value_priv(false, true, array_size, desc_array,
				   value_array);
}
EXPORT_SYMBOL_GPL(gpiod_set_array_value_cansleep);

/**
 * gpiod_add_lookup_table() - register GPIO device consumers
 * @table: table of consumers to register
 
  * gpiod_add_lookup_table () - GPIO 장치 소비자 등록
  * @ 테이블 : 등록 할 소비자 테이블
 */
void gpiod_add_lookup_table(struct gpiod_lookup_table *table)
{
	mutex_lock(&gpio_lookup_lock);

	list_add_tail(&table->list, &gpio_lookup_list);

	mutex_unlock(&gpio_lookup_lock);
}

/**
 * gpiod_remove_lookup_table() - unregister GPIO device consumers
 * @table: table of consumers to unregister
  * gpiod_remove_lookup_table () - GPIO 장치 소비자 등록 취소
  * @ 테이블 : 등록을 취소하려는 소비자의 테이블
 */
void gpiod_remove_lookup_table(struct gpiod_lookup_table *table)
{
	mutex_lock(&gpio_lookup_lock);

	list_del(&table->list);

	mutex_unlock(&gpio_lookup_lock);
}

static struct gpio_desc *of_find_gpio(struct device *dev, const char *con_id,
				      unsigned int idx,
				      enum gpio_lookup_flags *flags)
{
	char prop_name[32]; /* 32 is max size of property name
				32는 속성 이름의 최대 크기입니다.	 */
	enum of_gpio_flags of_flags;
	struct gpio_desc *desc;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(gpio_suffixes); i++) {
		if (con_id)
			snprintf(prop_name, sizeof(prop_name), "%s-%s", con_id,
				 gpio_suffixes[i]);
		else
			snprintf(prop_name, sizeof(prop_name), "%s",
				 gpio_suffixes[i]);

		desc = of_get_named_gpiod_flags(dev->of_node, prop_name, idx,
						&of_flags);
		if (!IS_ERR(desc) || (PTR_ERR(desc) == -EPROBE_DEFER))
			break;
	}

	if (IS_ERR(desc))
		return desc;

	if (of_flags & OF_GPIO_ACTIVE_LOW)
		*flags |= GPIO_ACTIVE_LOW;

	if (of_flags & OF_GPIO_SINGLE_ENDED) {
		if (of_flags & OF_GPIO_ACTIVE_LOW)
			*flags |= GPIO_OPEN_DRAIN;
		else
			*flags |= GPIO_OPEN_SOURCE;
	}

	return desc;
}

static struct gpio_desc *acpi_find_gpio(struct device *dev, const char *con_id,
					unsigned int idx,
					enum gpio_lookup_flags *flags)
{
	struct acpi_device *adev = ACPI_COMPANION(dev);
	struct acpi_gpio_info info;
	struct gpio_desc *desc;
	char propname[32];
	int i;

	/* Try first from _DSD */
	for (i = 0; i < ARRAY_SIZE(gpio_suffixes); i++) {
		if (con_id && strcmp(con_id, "gpios")) {
			snprintf(propname, sizeof(propname), "%s-%s",
				 con_id, gpio_suffixes[i]);
		} else {
			snprintf(propname, sizeof(propname), "%s",
				 gpio_suffixes[i]);
		}

		desc = acpi_get_gpiod_by_index(adev, propname, idx, &info);
		if (!IS_ERR(desc) || (PTR_ERR(desc) == -EPROBE_DEFER))
			break;
	}

	/* Then from plain _CRS GPIOs */
	if (IS_ERR(desc)) {
		desc = acpi_get_gpiod_by_index(adev, NULL, idx, &info);
		if (IS_ERR(desc))
			return desc;
	}

	if (info.active_low)
		*flags |= GPIO_ACTIVE_LOW;

	return desc;
}

static struct gpiod_lookup_table *gpiod_find_lookup_table(struct device *dev)
{
	const char *dev_id = dev ? dev_name(dev) : NULL;
	struct gpiod_lookup_table *table;

	mutex_lock(&gpio_lookup_lock);

	list_for_each_entry(table, &gpio_lookup_list, list) {
		if (table->dev_id && dev_id) {
			/*
			 * Valid strings on both ends, must be identical to have
			 * a match
			   양쪽 끝의 유효한 문자열은 다음과 같아야합니다.
			 */
			if (!strcmp(table->dev_id, dev_id))
				goto found;
		} else {
			/*
			 * One of the pointers is NULL, so both must be to have
			 * a match
			   포인터 중 하나가 NULL이므로 둘 다 가져야합니다.
			 */
			if (dev_id == table->dev_id)
				goto found;
		}
	}
	table = NULL;

found:
	mutex_unlock(&gpio_lookup_lock);
	return table;
}

static struct gpio_desc *gpiod_find(struct device *dev, const char *con_id,
				    unsigned int idx,
				    enum gpio_lookup_flags *flags)
{
	struct gpio_desc *desc = ERR_PTR(-ENOENT);
	struct gpiod_lookup_table *table;
	struct gpiod_lookup *p;

	table = gpiod_find_lookup_table(dev);
	if (!table)
		return desc;

	for (p = &table->table[0]; p->chip_label; p++) {
		struct gpio_chip *chip;

		/* idx must always match exactly 
		   idx는 항상 정확히 일치해야합니다.	*/
		if (p->idx != idx)
			continue;

		/* If the lookup entry has a con_id, require exact match
		   룩업 엔트리가 con_id를 가지고 있다면 정확한 일치를 요구		 */
		if (p->con_id && (!con_id || strcmp(p->con_id, con_id)))
			continue;

		chip = find_chip_by_name(p->chip_label);

		if (!chip) {
			dev_err(dev, "cannot find GPIO chip %s\n",
				p->chip_label);
			return ERR_PTR(-ENODEV);
		}

		if (chip->ngpio <= p->chip_hwnum) {
			dev_err(dev,
				"requested GPIO %d is out of range [0..%d] for chip %s\n",
				idx, chip->ngpio, chip->label);
			return ERR_PTR(-EINVAL);
		}

		desc = gpiochip_get_desc(chip, p->chip_hwnum);
		*flags = p->flags;

		return desc;
	}

	return desc;
}

static int dt_gpio_count(struct device *dev, const char *con_id)
{
	int ret;
	char propname[32];
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(gpio_suffixes); i++) {
		if (con_id)
			snprintf(propname, sizeof(propname), "%s-%s",
				 con_id, gpio_suffixes[i]);
		else
			snprintf(propname, sizeof(propname), "%s",
				 gpio_suffixes[i]);

		ret = of_gpio_named_count(dev->of_node, propname);
		if (ret >= 0)
			break;
	}
	return ret;
}

static int platform_gpio_count(struct device *dev, const char *con_id)
{
	struct gpiod_lookup_table *table;
	struct gpiod_lookup *p;
	unsigned int count = 0;

	table = gpiod_find_lookup_table(dev);
	if (!table)
		return -ENOENT;

	for (p = &table->table[0]; p->chip_label; p++) {
		if ((con_id && p->con_id && !strcmp(con_id, p->con_id)) ||
		    (!con_id && !p->con_id))
			count++;
	}
	if (!count)
		return -ENOENT;

	return count;
}

/**
 * gpiod_count - return the number of GPIOs associated with a device / function
 *		or -ENOENT if no GPIO has been assigned to the requested function
 * @dev:	GPIO consumer, can be NULL for system-global GPIOs
 * @con_id:	function within the GPIO consumer

 * gpiod_count - 장치 / 기능과 관련된 GPIO 수를 반환하거나 요청 된 기능에 GPIO가 할당되지 않은 경우 -ENOENT를 반환합니다.
 * @dev : GPIO 소비자는 시스템 전역 GPIO의 경우 NULL 일 수 있습니다.
 * @con_id : GPIO 소비자 내의 기능
 */
int gpiod_count(struct device *dev, const char *con_id)
{
	int count = -ENOENT;

	if (IS_ENABLED(CONFIG_OF) && dev && dev->of_node)
		count = dt_gpio_count(dev, con_id);
	else if (IS_ENABLED(CONFIG_ACPI) && dev && ACPI_HANDLE(dev))
		count = acpi_gpio_count(dev, con_id);

	if (count < 0)
		count = platform_gpio_count(dev, con_id);

	return count;
}
EXPORT_SYMBOL_GPL(gpiod_count);

/**
 * gpiod_get - obtain a GPIO for a given GPIO function
 * @dev:	GPIO consumer, can be NULL for system-global GPIOs
 * @con_id:	function within the GPIO consumer
 * @flags:	optional GPIO initialization flags
 *
 * Return the GPIO descriptor corresponding to the function con_id of device
 * dev, -ENOENT if no GPIO has been assigned to the requested function, or
 * another IS_ERR() code if an error occurred while trying to acquire the GPIO.


 * gpiod_get - 주어진 GPIO 함수에 대한 GPIO 얻기
 * @dev : GPIO 소비자는 시스템 전역 GPIO의 경우 NULL 일 수 있습니다.
 * @con_id : GPIO 소비자 내의 기능
 * @ 플래그 : 선택적 GPIO 초기화 플래그
 *
 * 장치 dev의 con_id 함수에 해당하는 GPIO 설명자를 반환합니다. 요청 된 함수에 GPIO가 할당되어 있지 않으면 -ENOENT를, GPIO를 가져 오는 동안 오류가 발생하면 다른 IS_ERR () 코드를 반환합니다.
 */
struct gpio_desc *__must_check gpiod_get(struct device *dev, const char *con_id,
					 enum gpiod_flags flags)
{
	return gpiod_get_index(dev, con_id, 0, flags);
}
EXPORT_SYMBOL_GPL(gpiod_get);

/**
 * gpiod_get_optional - obtain an optional GPIO for a given GPIO function
 * @dev: GPIO consumer, can be NULL for system-global GPIOs
 * @con_id: function within the GPIO consumer
 * @flags: optional GPIO initialization flags
 *
 * This is equivalent to gpiod_get(), except that when no GPIO was assigned to
 * the requested function it will return NULL. This is convenient for drivers
 * that need to handle optional GPIOs.

  * gpiod_get_optional - 주어진 GPIO 함수에 대한 선택적인 GPIO를 얻습니다.
  * @dev : GPIO 소비자는 시스템 전역 GPIO의 경우 NULL 일 수 있습니다.
  * @con_id : GPIO 소비자 내의 기능
  * @ 플래그 : 선택적 GPIO 초기화 플래그
  *
  * 요청 된 함수에 GPIO가 할당되지 않은 경우 NULL을 반환한다는 점을 제외하면 gpiod_get ()과 동일합니다. 이것은 옵션 GPIO를 처리해야하는 드라이버에 편리합니다.
 */
struct gpio_desc *__must_check gpiod_get_optional(struct device *dev,
						  const char *con_id,
						  enum gpiod_flags flags)
{
	return gpiod_get_index_optional(dev, con_id, 0, flags);
}
EXPORT_SYMBOL_GPL(gpiod_get_optional);


/**
 * gpiod_configure_flags - helper function to configure a given GPIO
 * @desc:	gpio whose value will be assigned
 * @con_id:	function within the GPIO consumer
 * @lflags:	gpio_lookup_flags - returned from of_find_gpio() or
 *		of_get_gpio_hog()
 * @dflags:	gpiod_flags - optional GPIO initialization flags
 *
 * Return 0 on success, -ENOENT if no GPIO has been assigned to the
 * requested function and/or index, or another IS_ERR() code if an error
 * occurred while trying to acquire the GPIO.
  * gpiod_configure_flags - 주어진 GPIO를 설정하는 헬퍼 함수
  * @desc : 값이 할당 될 gpio
  * @con_id : GPIO 소비자 내의 기능
  * @lflags : gpio_lookup_flags - of_find_gpio () 또는에서 반환되었습니다.
  * of_get_gpio_hog ()
  * @dflags : gpiod_flags - 선택적 GPIO 초기화 플래그
  *
  * 성공한 경우 0을 반환하고 요청 된 함수 및 / 또는 색인에 GPIO가 할당되어 있지 않으면 -ENOENT를, GPIO를 얻으려는 동안 오류가 발생하면 다른 IS_ERR () 코드를 반환합니다.
 */
static int gpiod_configure_flags(struct gpio_desc *desc, const char *con_id,
		unsigned long lflags, enum gpiod_flags dflags)
{
	int status;

	if (lflags & GPIO_ACTIVE_LOW)
		set_bit(FLAG_ACTIVE_LOW, &desc->flags);
	if (lflags & GPIO_OPEN_DRAIN)
		set_bit(FLAG_OPEN_DRAIN, &desc->flags);
	if (lflags & GPIO_OPEN_SOURCE)
		set_bit(FLAG_OPEN_SOURCE, &desc->flags);

	/* No particular flag request, return here... 
	   특정 플래그 요청이 없으면 여기로 돌아옵니다 ...	*/
	if (!(dflags & GPIOD_FLAGS_BIT_DIR_SET)) {
		pr_debug("no flags found for %s\n", con_id);
		return 0;
	}

	/* Process flags */
	if (dflags & GPIOD_FLAGS_BIT_DIR_OUT)
		status = gpiod_direction_output(desc,
					      dflags & GPIOD_FLAGS_BIT_DIR_VAL);
	else
		status = gpiod_direction_input(desc);

	return status;
}

/**
 * gpiod_get_index - obtain a GPIO from a multi-index GPIO function
 * @dev:	GPIO consumer, can be NULL for system-global GPIOs
 * @con_id:	function within the GPIO consumer
 * @idx:	index of the GPIO to obtain in the consumer
 * @flags:	optional GPIO initialization flags
 *
 * This variant of gpiod_get() allows to access GPIOs other than the first
 * defined one for functions that define several GPIOs.
 *
 * Return a valid GPIO descriptor, -ENOENT if no GPIO has been assigned to the
 * requested function and/or index, or another IS_ERR() code if an error
 * occurred while trying to acquire the GPIO.

  * gpiod_get_index - 다중 색인 GPIO 함수로부터 GPIO 얻기
  * @dev : GPIO 소비자는 시스템 전역 GPIO의 경우 NULL 일 수 있습니다.
  * @con_id : GPIO 소비자 내의 기능
  * @idx : 소비자에서 얻을 GPIO의 인덱스
  * @ 플래그 : 선택적 GPIO 초기화 플래그
  *
  * gpiod_get ()의이 변형은 여러 GPIO를 정의하는 함수에 대해 처음 정의 된 GPIO 이외의 다른 GPIO에 액세스 할 수있게합니다.
  *
  * 올바른 GPIO 디스크립터, GPIO가 요청 된 함수 및 / 또는 인덱스에 할당되지 않은 경우 -ENOENT를 반환하거나 GPIO를 획득하는 동안 오류가 발생하면 다른 IS_ERR () 코드를 반환합니다.
 */
struct gpio_desc *__must_check gpiod_get_index(struct device *dev,
					       const char *con_id,
					       unsigned int idx,
					       enum gpiod_flags flags)
{
	struct gpio_desc *desc = NULL;
	int status;
	enum gpio_lookup_flags lookupflags = 0;

	dev_dbg(dev, "GPIO lookup for consumer %s\n", con_id);

	if (dev) {
		/* Using device tree? */
		if (IS_ENABLED(CONFIG_OF) && dev->of_node) {
			dev_dbg(dev, "using device tree for GPIO lookup\n");
			desc = of_find_gpio(dev, con_id, idx, &lookupflags);
		} else if (ACPI_COMPANION(dev)) {
			dev_dbg(dev, "using ACPI for GPIO lookup\n");
			desc = acpi_find_gpio(dev, con_id, idx, &lookupflags);
		}
	}

	/*
	 * Either we are not using DT or ACPI, or their lookup did not return
	 * a result. In that case, use platform lookup as a fallback.
	   DT 또는 ACPI를 사용하지 않거나 조회에서 결과를 반환하지 않았습니다. 이 경우 대체품으로 플랫폼 조회를 사용하십시오.
	 */
	if (!desc || desc == ERR_PTR(-ENOENT)) {
		dev_dbg(dev, "using lookup tables for GPIO lookup\n");
		desc = gpiod_find(dev, con_id, idx, &lookupflags);
	}

	if (IS_ERR(desc)) {
		dev_dbg(dev, "lookup for GPIO %s failed\n", con_id);
		return desc;
	}

	status = gpiod_request(desc, con_id);
	if (status < 0)
		return ERR_PTR(status);

	status = gpiod_configure_flags(desc, con_id, lookupflags, flags);
	if (status < 0) {
		dev_dbg(dev, "setup of GPIO %s failed\n", con_id);
		gpiod_put(desc);
		return ERR_PTR(status);
	}

	return desc;
}
EXPORT_SYMBOL_GPL(gpiod_get_index);

/**
 * fwnode_get_named_gpiod - obtain a GPIO from firmware node
 * @fwnode:	handle of the firmware node
 * @propname:	name of the firmware property representing the GPIO
 *
 * This function can be used for drivers that get their configuration
 * from firmware.
 *
 * Function properly finds the corresponding GPIO using whatever is the
 * underlying firmware interface and then makes sure that the GPIO
 * descriptor is requested before it is returned to the caller.
 *
 * In case of error an ERR_PTR() is returned.

  * fwnode_get_named_gpiod - 펌웨어 노드에서 GPIO 얻기
  * @fwnode : 펌웨어 노드의 핸들
  * @propname : GPIO를 나타내는 펌웨어 속성의 이름
  *
  *이 기능은 펌웨어에서 구성을 가져 오는 드라이버에 사용할 수 있습니다.
  *
  * 함수는 기본 펌웨어 인터페이스를 사용하여 해당 GPIO를 올바르게 찾은 다음 호출자에게 반환되기 전에 GPIO 설명자가 요청되었는지 확인합니다.
  *
  * 오류가 발생하면 ERR_PTR ()이 리턴됩니다.
 */
struct gpio_desc *fwnode_get_named_gpiod(struct fwnode_handle *fwnode,
					 const char *propname)
{
	struct gpio_desc *desc = ERR_PTR(-ENODEV);
	bool active_low = false;
	bool single_ended = false;
	int ret;

	if (!fwnode)
		return ERR_PTR(-EINVAL);

	if (is_of_node(fwnode)) {
		enum of_gpio_flags flags;

		desc = of_get_named_gpiod_flags(to_of_node(fwnode), propname, 0,
						&flags);
		if (!IS_ERR(desc)) {
			active_low = flags & OF_GPIO_ACTIVE_LOW;
			single_ended = flags & OF_GPIO_SINGLE_ENDED;
		}
	} else if (is_acpi_node(fwnode)) {
		struct acpi_gpio_info info;

		desc = acpi_node_get_gpiod(fwnode, propname, 0, &info);
		if (!IS_ERR(desc))
			active_low = info.active_low;
	}

	if (IS_ERR(desc))
		return desc;

	ret = gpiod_request(desc, NULL);
	if (ret)
		return ERR_PTR(ret);

	if (active_low)
		set_bit(FLAG_ACTIVE_LOW, &desc->flags);

	if (single_ended) {
		if (active_low)
			set_bit(FLAG_OPEN_DRAIN, &desc->flags);
		else
			set_bit(FLAG_OPEN_SOURCE, &desc->flags);
	}

	return desc;
}
EXPORT_SYMBOL_GPL(fwnode_get_named_gpiod);

/**
 * gpiod_get_index_optional - obtain an optional GPIO from a multi-index GPIO
 *                            function
 * @dev: GPIO consumer, can be NULL for system-global GPIOs
 * @con_id: function within the GPIO consumer
 * @index: index of the GPIO to obtain in the consumer
 * @flags: optional GPIO initialization flags
 *
 * This is equivalent to gpiod_get_index(), except that when no GPIO with the
 * specified index was assigned to the requested function it will return NULL.
 * This is convenient for drivers that need to handle optional GPIOs.


  * 지정된 인덱스를 가진 GPIO가 요청 된 함수에 할당되지 않았 으면 NULL을 반환한다는 점을 제외하면 gpiod_get_index ()와 동일합니다.
  * 옵션 GPIO를 처리해야하는 드라이버에 편리합니다.
 */
struct gpio_desc *__must_check gpiod_get_index_optional(struct device *dev,
							const char *con_id,
							unsigned int index,
							enum gpiod_flags flags)
{
	struct gpio_desc *desc;

	desc = gpiod_get_index(dev, con_id, index, flags);
	if (IS_ERR(desc)) {
		if (PTR_ERR(desc) == -ENOENT)
			return NULL;
	}

	return desc;
}
EXPORT_SYMBOL_GPL(gpiod_get_index_optional);

/**
 * gpiod_hog - Hog the specified GPIO desc given the provided flags
 * @desc:	gpio whose value will be assigned
 * @name:	gpio line name
 * @lflags:	gpio_lookup_flags - returned from of_find_gpio() or
 *		of_get_gpio_hog()
 * @dflags:	gpiod_flags - optional GPIO initialization flags

  * gpiod_hog - 제공된 플래그가 주어지면 지정된 GPIO desc를 검색합니다.
  * @desc : 값이 할당 될 gpio
  * @name : gpio 행 이름
  * @lflags : gpio_lookup_flags - of_find_gpio () 또는 of_get_gpio_hog ()에서 반환되었습니다.
  * @dflags : gpiod_flags - 선택적 GPIO 초기화 플래그
 */
int gpiod_hog(struct gpio_desc *desc, const char *name,
	      unsigned long lflags, enum gpiod_flags dflags)
{
	struct gpio_chip *chip;
	struct gpio_desc *local_desc;
	int hwnum;
	int status;

	chip = gpiod_to_chip(desc);
	hwnum = gpio_chip_hwgpio(desc);

	local_desc = gpiochip_request_own_desc(chip, hwnum, name);
	if (IS_ERR(local_desc)) {
		pr_err("requesting hog GPIO %s (chip %s, offset %d) failed\n",
		       name, chip->label, hwnum);
		return PTR_ERR(local_desc);
	}

	status = gpiod_configure_flags(desc, name, lflags, dflags);
	if (status < 0) {
		pr_err("setup of hog GPIO %s (chip %s, offset %d) failed\n",
		       name, chip->label, hwnum);
		gpiochip_free_own_desc(desc);
		return status;
	}

	/* Mark GPIO as hogged so it can be identified and removed later 
	   나중에 GPIO를 식별하여 제거 할 수 있도록 GPIO를 표시			*/
	set_bit(FLAG_IS_HOGGED, &desc->flags);

	pr_info("GPIO line %d (%s) hogged as %s%s\n",
		desc_to_gpio(desc), name,
		(dflags&GPIOD_FLAGS_BIT_DIR_OUT) ? "output" : "input",
		(dflags&GPIOD_FLAGS_BIT_DIR_OUT) ?
		  (dflags&GPIOD_FLAGS_BIT_DIR_VAL) ? "/high" : "/low":"");

	return 0;
}

/**
 * gpiochip_free_hogs - Scan gpio-controller chip and release GPIO hog
 * @chip:	gpio chip to act on
 *
 * This is only used by of_gpiochip_remove to free hogged gpios


  * gpiochip_free_hogs - gpio-controller 칩을 스캔하고 GPIO 돼지를 출시하십시오.
  * @chip : gpio 칩이 작동합니다.
  *
  * 이것은 of_gpiochip_remove가 hogged gpios를 해제하는 데만 사용됩니다.
 */
static void gpiochip_free_hogs(struct gpio_chip *chip)
{
	int id;

	for (id = 0; id < chip->ngpio; id++) {
		if (test_bit(FLAG_IS_HOGGED, &chip->desc[id].flags))
			gpiochip_free_own_desc(&chip->desc[id]);
	}
}

/**
 * gpiod_get_array - obtain multiple GPIOs from a multi-index GPIO function
 * @dev:	GPIO consumer, can be NULL for system-global GPIOs
 * @con_id:	function within the GPIO consumer
 * @flags:	optional GPIO initialization flags
 *
 * This function acquires all the GPIOs defined under a given function.
 *
 * Return a struct gpio_descs containing an array of descriptors, -ENOENT if
 * no GPIO has been assigned to the requested function, or another IS_ERR()
 * code if an error occurred while trying to acquire the GPIOs.

  * gpiod_get_array - 다중 색인 GPIO 함수에서 여러 GPIO 가져 오기
  * @dev : GPIO 소비자는 시스템 전역 GPIO의 경우 NULL 일 수 있습니다.
  * @con_id : GPIO 소비자 내의 기능
  * @ 플래그 : 선택적 GPIO 초기화 플래그
  *
  * 이 함수는 주어진 함수 하에서 정의 된 모든 GPIO를 얻습니다.
  *
  * 설명자의 배열을 포함하는 구조체 gpio_descs, 요청 된 함수에 GPIO가 할당되지 않은 경우 -ENOENT 또는 GPIO를 획득하는 동안 오류가 발생한 경우 다른 IS_ERR () 코드를 반환합니다.
 */
struct gpio_descs *__must_check gpiod_get_array(struct device *dev,
						const char *con_id,
						enum gpiod_flags flags)
{
	struct gpio_desc *desc;
	struct gpio_descs *descs;
	int count;

	count = gpiod_count(dev, con_id);
	if (count < 0)
		return ERR_PTR(count);

	descs = kzalloc(sizeof(*descs) + sizeof(descs->desc[0]) * count,
			GFP_KERNEL);
	if (!descs)
		return ERR_PTR(-ENOMEM);

	for (descs->ndescs = 0; descs->ndescs < count; ) {
		desc = gpiod_get_index(dev, con_id, descs->ndescs, flags);
		if (IS_ERR(desc)) {
			gpiod_put_array(descs);
			return ERR_CAST(desc);
		}
		descs->desc[descs->ndescs] = desc;
		descs->ndescs++;
	}
	return descs;
}
EXPORT_SYMBOL_GPL(gpiod_get_array);

/**
 * gpiod_get_array_optional - obtain multiple GPIOs from a multi-index GPIO
 *                            function
 * @dev:	GPIO consumer, can be NULL for system-global GPIOs
 * @con_id:	function within the GPIO consumer
 * @flags:	optional GPIO initialization flags
 *
 * This is equivalent to gpiod_get_array(), except that when no GPIO was
 * assigned to the requested function it will return NULL.

  * gpiod_get_array_optional - 다중 색인 GPIO에서 여러 GPIO 가져 오기
  * 기능
  * @dev : GPIO 소비자는 시스템 전역 GPIO의 경우 NULL 일 수 있습니다.
  * @con_id : GPIO 소비자 내의 기능
  * @ 플래그 : 선택적 GPIO 초기화 플래그
  *
  * 요청 된 함수에 GPIO가 할당되지 않은 경우 NULL을 반환한다는 점을 제외하면 gpiod_get_array ()와 같습니다.
 */
struct gpio_descs *__must_check gpiod_get_array_optional(struct device *dev,
							const char *con_id,
							enum gpiod_flags flags)
{
	struct gpio_descs *descs;

	descs = gpiod_get_array(dev, con_id, flags);
	if (IS_ERR(descs) && (PTR_ERR(descs) == -ENOENT))
		return NULL;

	return descs;
}
EXPORT_SYMBOL_GPL(gpiod_get_array_optional);

/**
 * gpiod_put - dispose of a GPIO descriptor
 * @desc:	GPIO descriptor to dispose of
 *
 * No descriptor can be used after gpiod_put() has been called on it.

  * gpiod_put - GPIO 디스크립터를 처리합니다.
  * @desc : 처리 할 GPIO 설명자
  *
  * gpiod_put ()이 호출 된 후에는 설명자를 사용할 수 없습니다.
 */
void gpiod_put(struct gpio_desc *desc)
{
	gpiod_free(desc);
}
EXPORT_SYMBOL_GPL(gpiod_put);

/**
 * gpiod_put_array - dispose of multiple GPIO descriptors
 * @descs:	struct gpio_descs containing an array of descriptors

  * gpiod_put_array - 여러 개의 GPIO 디스크립터를 처리합니다.
  * @descs : 디스크립터 배열을 포함하는 gpio_descs 구조체
 */
void gpiod_put_array(struct gpio_descs *descs)
{
	unsigned int i;

	for (i = 0; i < descs->ndescs; i++)
		gpiod_put(descs->desc[i]);

	kfree(descs);
}
EXPORT_SYMBOL_GPL(gpiod_put_array);

#ifdef CONFIG_DEBUG_FS

static void gpiolib_dbg_show(struct seq_file *s, struct gpio_chip *chip)
{
	unsigned		i;
	unsigned		gpio = chip->base;
	struct gpio_desc	*gdesc = &chip->desc[0];
	int			is_out;
	int			is_irq;

	for (i = 0; i < chip->ngpio; i++, gpio++, gdesc++) {
		if (!test_bit(FLAG_REQUESTED, &gdesc->flags)) {
			if (gdesc->name) {
				seq_printf(s, " gpio-%-3d (%-20.20s)\n",
					   gpio, gdesc->name);
			}
			continue;
		}

		gpiod_get_direction(gdesc);
		is_out = test_bit(FLAG_IS_OUT, &gdesc->flags);
		is_irq = test_bit(FLAG_USED_AS_IRQ, &gdesc->flags);
		seq_printf(s, " gpio-%-3d (%-20.20s|%-20.20s) %s %s %s",
			gpio, gdesc->name ? gdesc->name : "", gdesc->label,
			is_out ? "out" : "in ",
			chip->get
				? (chip->get(chip, i) ? "hi" : "lo")
				: "?  ",
			is_irq ? "IRQ" : "   ");
		seq_printf(s, "\n");
	}
}

static void *gpiolib_seq_start(struct seq_file *s, loff_t *pos)
{
	unsigned long flags;
	struct gpio_chip *chip = NULL;
	loff_t index = *pos;

	s->private = "";

	spin_lock_irqsave(&gpio_lock, flags);
	list_for_each_entry(chip, &gpio_chips, list)
		if (index-- == 0) {
			spin_unlock_irqrestore(&gpio_lock, flags);
			return chip;
		}
	spin_unlock_irqrestore(&gpio_lock, flags);

	return NULL;
}

static void *gpiolib_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	unsigned long flags;
	struct gpio_chip *chip = v;
	void *ret = NULL;

	spin_lock_irqsave(&gpio_lock, flags);
	if (list_is_last(&chip->list, &gpio_chips))
		ret = NULL;
	else
		ret = list_entry(chip->list.next, struct gpio_chip, list);
	spin_unlock_irqrestore(&gpio_lock, flags);

	s->private = "\n";
	++*pos;

	return ret;
}

static void gpiolib_seq_stop(struct seq_file *s, void *v)
{
}

static int gpiolib_seq_show(struct seq_file *s, void *v)
{
	struct gpio_chip *chip = v;
	struct device *dev;

	seq_printf(s, "%sGPIOs %d-%d", (char *)s->private,
			chip->base, chip->base + chip->ngpio - 1);
	dev = chip->dev;
	if (dev)
		seq_printf(s, ", %s/%s", dev->bus ? dev->bus->name : "no-bus",
			dev_name(dev));
	if (chip->label)
		seq_printf(s, ", %s", chip->label);
	if (chip->can_sleep)
		seq_printf(s, ", can sleep");
	seq_printf(s, ":\n");

	if (chip->dbg_show)
		chip->dbg_show(s, chip);
	else
		gpiolib_dbg_show(s, chip);

	return 0;
}

static const struct seq_operations gpiolib_seq_ops = {
	.start = gpiolib_seq_start,
	.next = gpiolib_seq_next,
	.stop = gpiolib_seq_stop,
	.show = gpiolib_seq_show,
};

static int gpiolib_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &gpiolib_seq_ops);
}

static const struct file_operations gpiolib_operations = {
	.owner		= THIS_MODULE,
	.open		= gpiolib_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static int __init gpiolib_debugfs_init(void)
{
	/* /sys/kernel/debug/gpio */
	(void) debugfs_create_file("gpio", S_IFREG | S_IRUGO,
				NULL, NULL, &gpiolib_operations);
	return 0;
}
subsys_initcall(gpiolib_debugfs_init);

#endif	/* DEBUG_FS */
