#ifndef _LINUX_IRQRETURN_H
#define _LINUX_IRQRETURN_H

/**
 * enum irqreturn
 * @IRQ_NONE		interrupt was not from this device
 * @IRQ_HANDLED		interrupt was handled by this device
 * @IRQ_WAKE_THREAD	handler requests to wake the handler thread
 */
enum irqreturn {
	IRQ_NONE		= (0 << 0),		// 인터럽트 없었음									0
	IRQ_HANDLED		= (1 << 0),		// 인터럽트 처리됐음									1
	IRQ_WAKE_THREAD		= (1 << 1),	// 인터럽트 핸들러가 핸들러 스레드에게 요청해서 스레드 깨움	2
};

typedef enum irqreturn irqreturn_t;
#define IRQ_RETVAL(x)	((x) ? IRQ_HANDLED : IRQ_NONE)

#endif
