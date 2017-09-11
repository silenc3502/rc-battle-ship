/*
 * I2C bus driver for the Cadence I2C controller.
 *
 * Copyright (C) 2009 - 2014 Xilinx, Inc.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any
 * later version.
 */

/*
 * Analysing Goal 2017-06-15 ~ 2017-06-16
 * Writing Rule :
 *  writer
 *  Meaning of this code
 *  Comment
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>

/*
 * 최준호
 * I2C 컨트롤러에서 사용하는 모든 레지스터들에 대한 것으로 그 각 레지스터의 시작 위치 상수 값들.
 * ug585 B.22 I2C Controller 참조
 */
/* Register offsets for the I2C device. */
#define CDNS_I2C_CR_OFFSET			0x00 /* Control Register, RW */
#define CDNS_I2C_SR_OFFSET			0x04 /* Status Register, RO */
#define CDNS_I2C_ADDR_OFFSET		0x08 /* I2C Address Register, RW */
#define CDNS_I2C_DATA_OFFSET		0x0C /* I2C Data Register, RW */
#define CDNS_I2C_ISR_OFFSET			0x10 /* IRQ Status Register, RW */
#define CDNS_I2C_XFER_SIZE_OFFSET	0x14 /* Transfer Size Register, RW */
#define CDNS_I2C_SLV_PAUSE_OFFSET	0x18 /* Transfer Size Register, RW */
#define CDNS_I2C_TIME_OUT_OFFSET	0x1C /* Time Out Register, RW */
#define CDNS_I2C_IER_OFFSET			0x24 /* IRQ Enable Register, WO */
#define CDNS_I2C_IDR_OFFSET			0x28 /* IRQ Disable Register, WO */

/*
 * 최준호
 * CDNS_I2C_CR_OFFSET로 시작하는 Control Register의 각 비트에 대한 마스크
 * 자세한 사항은 B.22 I2C Register (IIC) Control_reg0 참조
 */
/* Control Register Bit mask definitions */
#define CDNS_I2C_CR_SLVMON			BIT(5) /* Slave monitor mode bit */
#define CDNS_I2C_CR_HOLD			BIT(4) /* Hold Bus bit */
#define CDNS_I2C_CR_ACK_EN			BIT(3)
#define CDNS_I2C_CR_NEA				BIT(2)
#define CDNS_I2C_CR_MS				BIT(1)
/* Read or Write Master transfer 0 = Transmitter, 1 = Receiver */
#define CDNS_I2C_CR_RW				BIT(0)
/* 1 = Auto init FIFO to zeroes */
#define CDNS_I2C_CR_CLR_FIFO		BIT(6)
#define CDNS_I2C_CR_DIVA_SHIFT		14
#define CDNS_I2C_CR_DIVA_MASK		(3 << CDNS_I2C_CR_DIVA_SHIFT)
#define CDNS_I2C_CR_DIVB_SHIFT		8
#define CDNS_I2C_CR_DIVB_MASK		(0x3f << CDNS_I2C_CR_DIVB_SHIFT)

/*
 * 최준호
 * CDNS_I2C_SR_OFFSET로 시작하는 Status Register의 각 비트에 대한 마스크
 * 자세한 사항은 B.22 I2C Register (IIC) Register (IIC) Status_reg0 참조
 */
/* Status Register Bit mask definitions */
#define CDNS_I2C_SR_BA				BIT(8)
#define CDNS_I2C_SR_RXDV			BIT(5)

/*
 * 최준호
 * CDNS_I2C_SR_OFFSET로 시작하는 Address Register의 각 비트에 대한 마스크
 * 자세한 사항은 B.22 I2C Register (IIC) Register (IIC) address_reg0 참조
 */
/*
 * I2C Address Register Bit mask definitions
 * Normal addressing mode uses [6:0] bits. Extended addressing mode uses [9:0]
 * bits. A write access to this register always initiates a transfer if the I2C
 * is in master mode.
 */
#define CDNS_I2C_ADDR_MASK	0x000003FF /* I2C Address Mask */

/*
 * 최준호
 * CDNS_I2C_SR_OFFSET로 시작하는 Interrupt Register의 각 비트에 대한 마스크
 * 자세한 사항은 B.22 I2C Register (IIC) Register (IIC) Interrupt_status_reg0 참조
 */
/*
 * I2C Interrupt Registers Bit mask definitions
 * All the four interrupt registers (Status/Mask/Enable/Disable) have the same
 * bit definitions.
 */
#define CDNS_I2C_IXR_ARB_LOST		BIT(9)
#define CDNS_I2C_IXR_RX_UNF		BIT(7)
#define CDNS_I2C_IXR_TX_OVF		BIT(6)
#define CDNS_I2C_IXR_RX_OVF		BIT(5)
#define CDNS_I2C_IXR_SLV_RDY		BIT(4)
#define CDNS_I2C_IXR_TO			BIT(3)
#define CDNS_I2C_IXR_NACK		BIT(2)
#define CDNS_I2C_IXR_DATA		BIT(1)
#define CDNS_I2C_IXR_COMP		BIT(0)

/*
 * 최준호
 * 매크로 함수로서 특정 인터럽트 비트가 활성화 되어 있을 경우 true를 반환한다. 각
 * 자세한 사항은 B.22 I2C Register (IIC) Register (IIC) Interrupt_status_reg0 참조
 */
#define CDNS_I2C_IXR_ALL_INTR_MASK	(CDNS_I2C_IXR_ARB_LOST | \
					 CDNS_I2C_IXR_RX_UNF | \
					 CDNS_I2C_IXR_TX_OVF | \
					 CDNS_I2C_IXR_RX_OVF | \
					 CDNS_I2C_IXR_SLV_RDY | \
					 CDNS_I2C_IXR_TO | \
					 CDNS_I2C_IXR_NACK | \
					 CDNS_I2C_IXR_DATA | \
					 CDNS_I2C_IXR_COMP)

#define CDNS_I2C_IXR_ERR_INTR_MASK	(CDNS_I2C_IXR_ARB_LOST | \
					 CDNS_I2C_IXR_RX_UNF | \
					 CDNS_I2C_IXR_TX_OVF | \
					 CDNS_I2C_IXR_RX_OVF | \
					 CDNS_I2C_IXR_NACK)

#define CDNS_I2C_ENABLED_INTR_MASK	(CDNS_I2C_IXR_ARB_LOST | \
					 CDNS_I2C_IXR_RX_UNF | \
					 CDNS_I2C_IXR_TX_OVF | \
					 CDNS_I2C_IXR_RX_OVF | \
					 CDNS_I2C_IXR_NACK | \
					 CDNS_I2C_IXR_DATA | \
					 CDNS_I2C_IXR_COMP)

/*
 * 최준호
 * 타임아웃 시간을 설정 아래와 같이 msecs_to_jiffies(1000)는 1초의 시간을 타임아웃 시간으로 쓰겠다는 것
 * http://blog.naver.com/PostView.nhn?blogId=sysganda&logNo=30180958754&categoryNo=0&parentCategoryNo=1&viewDate=&currentPage=1&postListTopCurrentPage=1
 */
#define CDNS_I2C_TIMEOUT		msecs_to_jiffies(1000)

/*
 * 최준호
 *
 */
#define CDNS_I2C_FIFO_DEPTH		16
/* FIFO depth at which the DATA interrupt occurs */
#define CDNS_I2C_DATA_INTR_DEPTH	(CDNS_I2C_FIFO_DEPTH - 2)
#define CDNS_I2C_MAX_TRANSFER_SIZE	255
/* Transfer size in multiples of data interrupt depth */
#define CDNS_I2C_TRANSFER_SIZE	(CDNS_I2C_MAX_TRANSFER_SIZE - 3)

#define DRIVER_NAME		"cdns-i2c"

// 최대 속도 40만
#define CDNS_I2C_SPEED_MAX	400000
// 보통 속도 10만
#define CDNS_I2C_SPEED_DEFAULT	100000

// Table 20-1 참조
// divisor_a max는 0~3까지 4
#define CDNS_I2C_DIVA_MAX	4
// divisor_b max는 0~60까지 61
#define CDNS_I2C_DIVB_MAX	64

#define CDNS_I2C_TIMEOUT_MAX	0xFF

#define CDNS_I2C_BROKEN_HOLD_BIT	0x00000001

// i2c 레지스터의 기본 주소에 인자로 받은 각 레지스터 오프셋을 넘겨주고 더해서 해당 레지스터에 접
#define cdns_i2c_readreg(offset)       readl_relaxed(id->membase + offset)
#define cdns_i2c_writereg(val, offset) writel_relaxed(val, id->membase + offset)

/**
 * struct cdns_i2c - I2C device private data structure
 * @membase:		Base address of the I2C device
 * @adap:		I2C adapter instance
 * @p_msg:		Message pointer
 * @err_status:		Error status in Interrupt Status Register
 * @xfer_done:		Transfer complete status
 * @p_send_buf:		Pointer to transmit buffer
 * @p_recv_buf:		Pointer to receive buffer
 * @suspended:		Flag holding the device's PM status
 * @send_count:		Number of bytes still expected to send
 * @recv_count:		Number of bytes still expected to receive
 * @curr_recv_count:	Number of bytes to be received in current transfer
 * @irq:		IRQ number
 * @input_clk:		Input clock to I2C controller
 * @i2c_clk:		Maximum I2C clock speed
 * @bus_hold_flag:	Flag used in repeated start for clearing HOLD bit
 * @clk:		Pointer to struct clk
 * @clk_rate_change_nb:	Notifier block for clock rate changes
 * @quirks:		flag for broken hold bit usage in r1p10
 */
struct cdns_i2c {
	void __iomem *membase;
	struct i2c_adapter adap;
	struct i2c_msg *p_msg;
	int err_status;
	struct completion xfer_done;
	unsigned char *p_send_buf;
	unsigned char *p_recv_buf;
	u8 suspended;
	unsigned int send_count;
	unsigned int recv_count;
	unsigned int curr_recv_count;
	int irq;
	unsigned long input_clk;
	unsigned int i2c_clk;
	unsigned int bus_hold_flag;
	struct clk *clk;
	struct notifier_block clk_rate_change_nb;
	u32 quirks;
};

struct cdns_platform_data {
	u32 quirks;
};

// cast a member of a structure out to the containing structure
#define to_cdns_i2c(_nb)	container_of(_nb, struct cdns_i2c, \
					     clk_rate_change_nb)

/**
 * cdns_i2c_clear_bus_hold() - Clear bus hold bit
 * @id:	Pointer to driver data struct
 *
 * Helper to clear the controller's bus hold bit.
 */
static void cdns_i2c_clear_bus_hold(struct cdns_i2c *id)
{
//	Controll Register의 값을 읽어옴
	u32 reg = cdns_i2c_readreg(CDNS_I2C_CR_OFFSET);
//	읽어온 값과 HOLD와의 AND가 true이면
	if (reg & CDNS_I2C_CR_HOLD)
//		읽어온 값과 HOLD의 부정과 AND 연산한 값을 Controll Register에 씀(이렇게 되면 아마도 0을 쓰는 듯)
		cdns_i2c_writereg(reg & ~CDNS_I2C_CR_HOLD, CDNS_I2C_CR_OFFSET);
}

// 아마도 입력받은 값이 버퍼 최대값을 넘거나 꽉 찼을 때 신호를 주는 것인듯?
static inline bool cdns_is_holdquirk(struct cdns_i2c *id, bool hold_wrkaround)
{
//	wrkaround가 true이고 현재 전송 버퍼에 수신된 바이트 수(전송되는 등 처리되면 -1)와 버퍼 사이트 +1(초과)가 같으면 즉, 초과되면 true
	return (hold_wrkaround &&
		(id->curr_recv_count == CDNS_I2C_FIFO_DEPTH + 1));
}

/**
 * cdns_i2c_isr - Interrupt handler for the I2C device
 * @irq:	irq number for the I2C device
 * @ptr:	void pointer to cdns_i2c structure
 *
 * This function handles the data interrupt, transfer complete interrupt and
 * the error interrupts of the I2C device.
 *
 * Return: IRQ_HANDLED always
 */
static irqreturn_t cdns_i2c_isr(int irq, void *ptr)
{
//	isr_status : 인터럽트 상태 레지스터의 값,
	unsigned int isr_status, avail_bytes, updatetx;
	unsigned int bytes_to_send;
//	HOLD_QUIRK! quirk : flag for broken hold bit usage in r1p10(i2c cadence)
//	Register (IIC) Control_reg0의 4번 bit인데
//	전송 또는 수신될 것이 없으면 1로 된다. 1로 되면 hold bus가 low되면서 통신을 정지한다.
//	0이 되면 전송 및 수신을 시작한다.

//	전송 또는 수신을 더 이상 할 수 없는 상태(아마도 꽉 찬 상태?)이면 hold를 활성화 한다.
//	그 외의 경우 0
	bool hold_quirk;
	struct cdns_i2c *id = ptr;
	/* Signal completion only after everything is updated */
//	위의 주석으로 봐서 done_flag가 1이면 이 함수의 모든 사항이 업데이트 됐다는 뜻인듯
	int done_flag = 0;
//	인터럽트 상태를 인터럽트 없음으로 초기화
	irqreturn_t status = IRQ_NONE;

//	인터럽트 상태 레지스터의 값을 읽어와 isr_status에 담음
	isr_status = cdns_i2c_readreg(CDNS_I2C_ISR_OFFSET);
//	인터럽트 상태를 인터럽트 상태 레지스터에 씀
	cdns_i2c_writereg(isr_status, CDNS_I2C_ISR_OFFSET);

	/* Handling nack and arbitration lost interrupt */
//	nack은 Transfer not acknowledged의 약자로 전송이 승인되지 않은 상태임을 나타내고
//	1 = slave responds with a NACK or master terminates the transfer before all data is supplied

//	arbitration lost은 잘 모르겠음
//	1 = master loses bus ownership during a transfer due to ongoing arbitration
	if (isr_status & (CDNS_I2C_IXR_NACK | CDNS_I2C_IXR_ARB_LOST)) {
//		전송이 승인되지 않거나 arbitration을 잃어버렸으면 IRQ_HANDLED로 바꾸고 done_flag 1
		done_flag = 1;
		status = IRQ_HANDLED;
	}

	/*
	 * Check if transfer size register needs to be updated again for a
	 * large data receive operation.
	 */
//	큰 규모의 데이터를 받기 위해 transfer size register의 수정이 필요한지 확인하는 코드

//	초기화
	updatetx = 0;
//	앞으로 수신할 것으로 예상되는 바이트 수가 현재 전송 받은 바이트 수보다 크면
	if (id->recv_count > id->curr_recv_count)
//		전송 업데이트 플래그 1
		updatetx = 1;

//	i2c의 hold bit이 깨졌고(CDNS_I2C_BROKEN_HOLD_BIT는 1이다) 전송 업데이트 플래그가 1이면 hold_quirk도 1
//	아직 이게 의미하는 바가 정확히 무엇인지 모르겠다.
//	TODO hold_quirk과 quriks의 역할 파악
	hold_quirk = (id->quirks & CDNS_I2C_BROKEN_HOLD_BIT) && updatetx;

	/* When receiving, handle data interrupt and completion interrupt */
//	수신 버퍼가 있고, 전송이 완료 되었거나 전송 또는 수신 중인 데이터가 있으면 true
	if (id->p_recv_buf &&
	    ((isr_status & CDNS_I2C_IXR_COMP) ||
	     (isr_status & CDNS_I2C_IXR_DATA))) {
		/* Read data if receive data valid is set */
//		상태 레지스터가 Receiver Data Valid 상태일 때 true
//		1일때 valid, new data to be read from the interface.
		while (cdns_i2c_readreg(CDNS_I2C_SR_OFFSET) &
		       CDNS_I2C_SR_RXDV) {
			/*
			 * Clear hold bit that was set for FIFO control if
			 * RX data left is less than FIFO depth, unless
			 * repeated start is selected.
			 */
//			repeated start가 선택되어 있지 않고 RX 데이터의 여유 공간이 있으면
//			FIFO를 컨트롤 하는 hold bit을 비운다.

//			수신 받을 것으로 예상되는 바이트 수가 스택 크기보다 작고
//			Flag used in repeated start for clearing HOLD bit인 bus_hold_flag가 false이면
//			이 i2c에 대한 bus hold를 비운다.
//			repeated start가 true이면 알아서 bus hold가 반복적으로 비워지는듯
			if ((id->recv_count < CDNS_I2C_FIFO_DEPTH) &&
			    !id->bus_hold_flag)
				cdns_i2c_clear_bus_hold(id);

//			현재 수신 버퍼의 위치에 데이터 레지스터의 시작 주소를 넣고 수신 버퍼 위치 1 증가
			*(id->p_recv_buf)++ =
				cdns_i2c_readreg(CDNS_I2C_DATA_OFFSET);
//			수신될 것으로 예상되는 전체 바이트 수 1 감소
			id->recv_count--;
//			현재 전송 버퍼에 수신된 바이트 수 1 감소
			id->curr_recv_count--;

//			위와 같이 한 번 읽고 나서 버퍼가 꽉 찼다면 break
			if (cdns_is_holdquirk(id, hold_quirk))
				break;
		}

		/*
		 * The controller sends NACK to the slave when transfer size
		 * register reaches zero without considering the HOLD bit.
		 * This workaround is implemented for large data transfers to
		 * maintain transfer size non-zero while performing a large
		 * receive operation.
		 */
		if (cdns_is_holdquirk(id, hold_quirk)) {
			/* wait while fifo is full */
//			전송 크기를 나타내는 레지스터 != (현재 전송 버퍼에 받아진 데이터 바이트 수 - i2c 버퍼 크기)
//			readreg가 어떤 전송 버퍼가 꽉 찼을 때의 값을 보내주고 != (현재 수신된 바이트 수 - 버퍼 사이트 == ?)는 true
			while (cdns_i2c_readreg(CDNS_I2C_XFER_SIZE_OFFSET) != (id->curr_recv_count - CDNS_I2C_FIFO_DEPTH)) ;

			/*
			 * Check number of bytes to be received against maximum
			 * transfer size and update register accordingly.
			 */
//			여전히 받을 것으로 기대되는 바이트 수에서 버퍼 사이즈를 뺏더니 전송 버퍼 사이즈보다 크다면
			if (((int)(id->recv_count) - CDNS_I2C_FIFO_DEPTH) >
			    CDNS_I2C_TRANSFER_SIZE) {
//				CDNS_I2C_XFER_SIZE_OFFSET에 (CDNS_I2C_MAX_TRANSFER_SIZE - 3)를 쓴다. 즉 CDNS_I2C_XFER_SIZE_OFFSET에 252를 쓴다.
				cdns_i2c_writereg(CDNS_I2C_TRANSFER_SIZE,
						  CDNS_I2C_XFER_SIZE_OFFSET);
//				현재 전송 버퍼에 받아진 바이트 수를 252 + 버퍼 크기로 새로고침한다.
				id->curr_recv_count = CDNS_I2C_TRANSFER_SIZE +
						      CDNS_I2C_FIFO_DEPTH;
			} else {
//				CDNS_I2C_XFER_SIZE_OFFSET(전송 사이즈 버퍼 레지스터)에 계속 수신될 것으로 예상되는 수신 바이트 수에서 버퍼 크기를 뺀 값을 쓴다.
				cdns_i2c_writereg(id->recv_count -
						  CDNS_I2C_FIFO_DEPTH,
						  CDNS_I2C_XFER_SIZE_OFFSET);
//				현재 전송 버퍼에 수신된 크기 값을 수신될 것으로 예상되는 값과 같게 한다.
				id->curr_recv_count = id->recv_count;
			}
//			수신될 값이 있고 hold_quirk이 활성화가 아니면서 현재 수신된 값이 없으면
//			즉, 값을 받을 수 있는 상황이면?
		} else if (id->recv_count && !hold_quirk &&
						!id->curr_recv_count) {

			/* Set the slave address in address register*/
//			주소를 가지는 레지스터에 slave address 설정
//			즉, 슬레이브에게서 값을 받는 준비인 듯.
//			CDNS_I2C_ADDR_OFFSET에 현재 i2c 구조체의 slave msg의 주소를 masking하여 얻은 후 쓴다.
			cdns_i2c_writereg(id->p_msg->addr & CDNS_I2C_ADDR_MASK,
						CDNS_I2C_ADDR_OFFSET);

//			수신되야 할 값이 전송 사이즈보다 크면
			if (id->recv_count > CDNS_I2C_TRANSFER_SIZE) {
//				CDNS_I2C_XFER_SIZE_OFFSET 인터럽트를 일으킬 수 있으면서 최대 데이터 전송 가능한 크기를 주는듯?
				cdns_i2c_writereg(CDNS_I2C_TRANSFER_SIZE,
						CDNS_I2C_XFER_SIZE_OFFSET);
//				현재 수신된 바이트 수를 최대로 변경.
				id->curr_recv_count = CDNS_I2C_TRANSFER_SIZE;
			} else {
//				CDNS_I2C_XFER_SIZE_OFFSET에 수신될 바이트 수 씀.
				cdns_i2c_writereg(id->recv_count,
						CDNS_I2C_XFER_SIZE_OFFSET);
//				수신된 숫자 수신 할 숫자로 대입.
				id->curr_recv_count = id->recv_count;
			}
		}

		/* Clear hold (if not repeated start) and signal completion */
//		repeated start가 선택돼 있지 않으면 hold bit clear해서 작동하게 하고 신호 끝냄(status = IRQ_HANDLED)

//		인터럽트 상태 레지스터에서 읽어온 값을 저장하는 isr_status가 있고 & 인터럽트 상태 레지스터의 비트가 전송 완료이면서 수신될 값이 없으면
//		전송 완료이고 수신될 값이 없다. 즉, 전송 및 수신을 재개할 수 있으므로 hold를 클리어 한다.
		if ((isr_status & CDNS_I2C_IXR_COMP) && !id->recv_count) {

//			버스 홀드 플래그가 false이면
//			즉, 수신 및 송신을 받을 수 있는 상태이면
			if (!id->bus_hold_flag)
//			실제 i2c의 홀드를 해제한다.
				cdns_i2c_clear_bus_hold(id);
//			작업완료 했다는 상태로 업데이트
			done_flag = 1;
		}

//		인터럽트 처리됨을 알린다.
		status = IRQ_HANDLED;
	}

	/* When sending, handle transfer complete interrupt */
//	값을 보낼 때, 전송 완료 인터럽트를 처리(handle)하는 부분

//	인터럽트 상태가 있고 인터럽트 레지스터가 전송 완료이면서 수신 버퍼의 포인터가 없으면
//	즉 전송 완료 후 수신된 값이 없으면
	if ((isr_status & CDNS_I2C_IXR_COMP) && !id->p_recv_buf) {
		/*
		 * If there is more data to be sent, calculate the
		 * space available in FIFO and fill with that many bytes.
		 */
//		만약 저쪽에 보내질 데이터가 더 있으면, FIFO의 가용한 영역을 계산하고 그 데이터들로 채운다.

//		보내야 할 값이 있으면
		if (id->send_count) {
//			가용한 바이트 = 버퍼 사이즈 - 전송 사이즈
//			최대 버퍼 사이즈 16에서 값이 쓰여져 있는 것을 뺀다.
			avail_bytes = CDNS_I2C_FIFO_DEPTH -
			    cdns_i2c_readreg(CDNS_I2C_XFER_SIZE_OFFSET);
//			보내야 할 값이 가용한 값보다 크면
			if (id->send_count > avail_bytes)
//				보내야 할 값에 가용한 값을 쓴다.
//				TODO
				bytes_to_send = avail_bytes;
			else
//				보내야 할 값에 보내질 값을 쓴다.
				bytes_to_send = id->send_count;


			while (bytes_to_send--) {
				cdns_i2c_writereg(
					(*(id->p_send_buf)++),
					 CDNS_I2C_DATA_OFFSET);
				id->send_count--;
			}
		} else {
			/*
			 * Signal the completion of transaction and
			 * clear the hold bus bit if there are no
			 * further messages to be processed.
			 */
			done_flag = 1;
		}
		if (!id->send_count && !id->bus_hold_flag)
			cdns_i2c_clear_bus_hold(id);

		status = IRQ_HANDLED;
	}

	/* Handling Slave monitor mode interrupt */
	if (isr_status & CDNS_I2C_IXR_SLV_RDY) {
		unsigned int ctrl_reg;
		/* Read control register */
		ctrl_reg = cdns_i2c_readreg(CDNS_I2C_CR_OFFSET);

		/* Disable slave monitor mode */
		ctrl_reg &= ~CDNS_I2C_CR_SLVMON;
		cdns_i2c_writereg(ctrl_reg, CDNS_I2C_CR_OFFSET);

		/* Clear interrupt flag for slvmon mode */
		cdns_i2c_writereg(CDNS_I2C_IXR_SLV_RDY, CDNS_I2C_IDR_OFFSET);

		done_flag = 1;
		status = IRQ_HANDLED;
	}

	/* Update the status for errors */
	id->err_status = isr_status & CDNS_I2C_IXR_ERR_INTR_MASK;
	if (id->err_status)
		status = IRQ_HANDLED;

	if (done_flag)
		complete(&id->xfer_done);

	return status;
}

/**
 * cdns_i2c_mrecv - Prepare and start a master receive operation
 * @id:		pointer to the i2c device structure
 */
static void cdns_i2c_mrecv(struct cdns_i2c *id)
{
	unsigned int ctrl_reg;
	unsigned int isr_status;

	id->p_recv_buf = id->p_msg->buf;
	id->recv_count = id->p_msg->len;

	/* Put the controller in master receive mode and clear the FIFO */
	ctrl_reg = cdns_i2c_readreg(CDNS_I2C_CR_OFFSET);
	ctrl_reg |= CDNS_I2C_CR_RW | CDNS_I2C_CR_CLR_FIFO;

	if (id->p_msg->flags & I2C_M_RECV_LEN)
		id->recv_count = I2C_SMBUS_BLOCK_MAX + 1;

	id->curr_recv_count = id->recv_count;

	/*
	 * Check for the message size against FIFO depth and set the
	 * 'hold bus' bit if it is greater than FIFO depth.
	 */
	if (id->recv_count > CDNS_I2C_FIFO_DEPTH)
		ctrl_reg |= CDNS_I2C_CR_HOLD;

	cdns_i2c_writereg(ctrl_reg, CDNS_I2C_CR_OFFSET);

	/* Clear the interrupts in interrupt status register */
	isr_status = cdns_i2c_readreg(CDNS_I2C_ISR_OFFSET);
	cdns_i2c_writereg(isr_status, CDNS_I2C_ISR_OFFSET);

	/*
	 * The no. of bytes to receive is checked against the limit of
	 * max transfer size. Set transfer size register with no of bytes
	 * receive if it is less than transfer size and transfer size if
	 * it is more. Enable the interrupts.
	 */
	if (id->recv_count > CDNS_I2C_TRANSFER_SIZE) {
		cdns_i2c_writereg(CDNS_I2C_TRANSFER_SIZE,
				  CDNS_I2C_XFER_SIZE_OFFSET);
		id->curr_recv_count = CDNS_I2C_TRANSFER_SIZE;
	} else {
		cdns_i2c_writereg(id->recv_count, CDNS_I2C_XFER_SIZE_OFFSET);
	}

	/* Clear the bus hold flag if bytes to receive is less than FIFO size */
	if (!id->bus_hold_flag &&
		((id->p_msg->flags & I2C_M_RECV_LEN) != I2C_M_RECV_LEN) &&
		(id->recv_count <= CDNS_I2C_FIFO_DEPTH))
			cdns_i2c_clear_bus_hold(id);
	/* Set the slave address in address register - triggers operation */
	cdns_i2c_writereg(id->p_msg->addr & CDNS_I2C_ADDR_MASK,
						CDNS_I2C_ADDR_OFFSET);
	cdns_i2c_writereg(CDNS_I2C_ENABLED_INTR_MASK, CDNS_I2C_IER_OFFSET);
}

/**
 * cdns_i2c_msend - Prepare and start a master send operation
 * @id:		pointer to the i2c device
 */
static void cdns_i2c_msend(struct cdns_i2c *id)
{
	unsigned int avail_bytes;
	unsigned int bytes_to_send;
	unsigned int ctrl_reg;
	unsigned int isr_status;

	id->p_recv_buf = NULL;
	id->p_send_buf = id->p_msg->buf;
	id->send_count = id->p_msg->len;

	/* Set the controller in Master transmit mode and clear the FIFO. */
	ctrl_reg = cdns_i2c_readreg(CDNS_I2C_CR_OFFSET);
	ctrl_reg &= ~CDNS_I2C_CR_RW;
	ctrl_reg |= CDNS_I2C_CR_CLR_FIFO;

	/*
	 * Check for the message size against FIFO depth and set the
	 * 'hold bus' bit if it is greater than FIFO depth.
	 */
	if (id->send_count > CDNS_I2C_FIFO_DEPTH)
		ctrl_reg |= CDNS_I2C_CR_HOLD;
	cdns_i2c_writereg(ctrl_reg, CDNS_I2C_CR_OFFSET);

	/* Clear the interrupts in interrupt status register. */
	isr_status = cdns_i2c_readreg(CDNS_I2C_ISR_OFFSET);
	cdns_i2c_writereg(isr_status, CDNS_I2C_ISR_OFFSET);

	/*
	 * Calculate the space available in FIFO. Check the message length
	 * against the space available, and fill the FIFO accordingly.
	 * Enable the interrupts.
	 */
	avail_bytes = CDNS_I2C_FIFO_DEPTH -
				cdns_i2c_readreg(CDNS_I2C_XFER_SIZE_OFFSET);

	if (id->send_count > avail_bytes)
		bytes_to_send = avail_bytes;
	else
		bytes_to_send = id->send_count;

	while (bytes_to_send--) {
		cdns_i2c_writereg((*(id->p_send_buf)++), CDNS_I2C_DATA_OFFSET);
		id->send_count--;
	}

	/*
	 * Clear the bus hold flag if there is no more data
	 * and if it is the last message.
	 */
	if (!id->bus_hold_flag && !id->send_count)
		cdns_i2c_clear_bus_hold(id);
	/* Set the slave address in address register - triggers operation. */
	cdns_i2c_writereg(id->p_msg->addr & CDNS_I2C_ADDR_MASK,
						CDNS_I2C_ADDR_OFFSET);

	cdns_i2c_writereg(CDNS_I2C_ENABLED_INTR_MASK, CDNS_I2C_IER_OFFSET);
}

/**
 * cdns_i2c_slvmon - Handling Slav monitor mode feature
 * @id:		pointer to the i2c device
 */
static void cdns_i2c_slvmon(struct cdns_i2c *id)
{
	unsigned int ctrl_reg;
	unsigned int isr_status;

	id->p_recv_buf = NULL;
	id->p_send_buf = id->p_msg->buf;
	id->send_count = id->p_msg->len;

	/* Clear the interrupts in interrupt status register. */
	isr_status = cdns_i2c_readreg(CDNS_I2C_ISR_OFFSET);
	cdns_i2c_writereg(isr_status, CDNS_I2C_ISR_OFFSET);

	/* Enable slvmon control reg */
	ctrl_reg = cdns_i2c_readreg(CDNS_I2C_CR_OFFSET);
	ctrl_reg |=  CDNS_I2C_CR_MS | CDNS_I2C_CR_NEA | CDNS_I2C_CR_SLVMON
			| CDNS_I2C_CR_CLR_FIFO;
	ctrl_reg &= ~(CDNS_I2C_CR_RW);
	cdns_i2c_writereg(ctrl_reg, CDNS_I2C_CR_OFFSET);

	/* Initialize slvmon reg */
	cdns_i2c_writereg(0xF, CDNS_I2C_SLV_PAUSE_OFFSET);

	/* Set the slave address to start the slave address transmission */
	cdns_i2c_writereg(id->p_msg->addr, CDNS_I2C_ADDR_OFFSET);

	/* Setup slvmon interrupt flag */
	cdns_i2c_writereg(CDNS_I2C_IXR_SLV_RDY, CDNS_I2C_IER_OFFSET);
}

/**
 * cdns_i2c_master_reset - Reset the interface
 * @adap:	pointer to the i2c adapter driver instance
 *
 * This function cleanup the fifos, clear the hold bit and status
 * and disable the interrupts.
 */
static void cdns_i2c_master_reset(struct i2c_adapter *adap)
{
	struct cdns_i2c *id = adap->algo_data;
	u32 regval;

	/* Disable the interrupts */
	cdns_i2c_writereg(CDNS_I2C_IXR_ALL_INTR_MASK, CDNS_I2C_IDR_OFFSET);
	/* Clear the hold bit and fifos */
	regval = cdns_i2c_readreg(CDNS_I2C_CR_OFFSET);
	regval &= ~(CDNS_I2C_CR_HOLD | CDNS_I2C_CR_SLVMON);
	regval |= CDNS_I2C_CR_CLR_FIFO;
	cdns_i2c_writereg(regval, CDNS_I2C_CR_OFFSET);
	/* Update the transfercount register to zero */
	cdns_i2c_writereg(0, CDNS_I2C_XFER_SIZE_OFFSET);
	/* Clear the interupt status register */
	regval = cdns_i2c_readreg(CDNS_I2C_ISR_OFFSET);
	cdns_i2c_writereg(regval, CDNS_I2C_ISR_OFFSET);
	/* Clear the status register */
	regval = cdns_i2c_readreg(CDNS_I2C_SR_OFFSET);
	cdns_i2c_writereg(regval, CDNS_I2C_SR_OFFSET);
}

static int cdns_i2c_process_msg(struct cdns_i2c *id, struct i2c_msg *msg,
		struct i2c_adapter *adap)
{
	int ret;
	u32 reg;

	id->p_msg = msg;
	id->err_status = 0;
	reinit_completion(&id->xfer_done);

	/* Check for the TEN Bit mode on each msg */
	reg = cdns_i2c_readreg(CDNS_I2C_CR_OFFSET);
	if (msg->flags & I2C_M_TEN) {
		if (reg & CDNS_I2C_CR_NEA)
			cdns_i2c_writereg(reg & ~CDNS_I2C_CR_NEA,
					CDNS_I2C_CR_OFFSET);
	} else {
		if (!(reg & CDNS_I2C_CR_NEA))
			cdns_i2c_writereg(reg | CDNS_I2C_CR_NEA,
					CDNS_I2C_CR_OFFSET);
	}
	/* Check for zero lenght - Slave monitor mode */
	if (msg->len == 0)
		cdns_i2c_slvmon(id);
	 /* Check for the R/W flag on each msg */
	else if (msg->flags & I2C_M_RD)
		cdns_i2c_mrecv(id);
	else
		cdns_i2c_msend(id);

	/* Wait for the signal of completion */
	ret = wait_for_completion_timeout(&id->xfer_done, adap->timeout);
	if (!ret) {
		cdns_i2c_master_reset(adap);
		dev_err(id->adap.dev.parent,
				"timeout waiting on completion\n");
		return -ETIMEDOUT;
	}

	cdns_i2c_writereg(CDNS_I2C_IXR_ALL_INTR_MASK,
			  CDNS_I2C_IDR_OFFSET);

	/* If it is bus arbitration error, try again */
	if (id->err_status & CDNS_I2C_IXR_ARB_LOST)
		return -EAGAIN;

	return 0;
}

/**
 * cdns_i2c_master_xfer - The main i2c transfer function
 * @adap:	pointer to the i2c adapter driver instance
 * @msgs:	pointer to the i2c message structure
 * @num:	the number of messages to transfer
 *
 * Initiates the send/recv activity based on the transfer message received.
 *
 * Return: number of msgs processed on success, negative error otherwise
 */
static int cdns_i2c_master_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs,
				int num)
{
	int ret, count;
	u32 reg;
	struct cdns_i2c *id = adap->algo_data;
	bool hold_quirk;
	/* Check if the bus is free */
	if (msgs->len)
		if (cdns_i2c_readreg(CDNS_I2C_SR_OFFSET) & CDNS_I2C_SR_BA)
			return -EAGAIN;

	hold_quirk = !!(id->quirks & CDNS_I2C_BROKEN_HOLD_BIT);
	/*
	 * Set the flag to one when multiple messages are to be
	 * processed with a repeated start.
	 */
	if (num > 1) {
		/*
		 * This controller does not give completion interrupt after a
		 * master receive message if HOLD bit is set (repeated start),
		 * resulting in SW timeout. Hence, if a receive message is
		 * followed by any other message, an error is returned
		 * indicating that this sequence is not supported.
		 */
		for (count = 0; (count < num - 1 && hold_quirk); count++) {
			if (msgs[count].flags & I2C_M_RD) {
				dev_warn(adap->dev.parent,
					 "Can't do repeated start after a receive message\n");
				return -EOPNOTSUPP;
			}
		}
		id->bus_hold_flag = 1;
		reg = cdns_i2c_readreg(CDNS_I2C_CR_OFFSET);
		reg |= CDNS_I2C_CR_HOLD;
		cdns_i2c_writereg(reg, CDNS_I2C_CR_OFFSET);
	} else {
		id->bus_hold_flag = 0;
	}

	/* Process the msg one by one */
	for (count = 0; count < num; count++, msgs++) {
		if (count == (num - 1))
			id->bus_hold_flag = 0;

		ret = cdns_i2c_process_msg(id, msgs, adap);
		if (ret)
			return ret;

		/* Report the other error interrupts to application */
		if (id->err_status) {
			cdns_i2c_master_reset(adap);

			if (id->err_status & CDNS_I2C_IXR_NACK)
				return -ENXIO;

			return -EIO;
		}
	}

	return num;
}

/**
 * cdns_i2c_func - Returns the supported features of the I2C driver
 * @adap:	pointer to the i2c adapter structure
 *
 * Return: 32 bit value, each bit corresponding to a feature
 */
static u32 cdns_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_10BIT_ADDR |
		(I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK) |
		I2C_FUNC_SMBUS_BLOCK_DATA;
}

static const struct i2c_algorithm cdns_i2c_algo = {
	.master_xfer	= cdns_i2c_master_xfer,
	.functionality	= cdns_i2c_func,
};

/**
 * cdns_i2c_calc_divs - Calculate clock dividers
 * @f:		I2C clock frequency
 * @input_clk:	Input clock frequency
 * @a:		First divider (return value)
 * @b:		Second divider (return value)
 *
 * f is used as input and output variable. As input it is used as target I2C
 * frequency. On function exit f holds the actually resulting I2C frequency.
 *
 * Return: 0 on success, negative errno otherwise.
 */
static int cdns_i2c_calc_divs(unsigned long *f, unsigned long input_clk,
		unsigned int *a, unsigned int *b)
{
	unsigned long fscl = *f, best_fscl = *f, actual_fscl, temp;
	unsigned int div_a, div_b, calc_div_a = 0, calc_div_b = 0;
	unsigned int last_error, current_error;

	/* calculate (divisor_a+1) x (divisor_b+1) */
	temp = input_clk / (22 * fscl);

	/*
	 * If the calculated value is negative or 0, the fscl input is out of
	 * range. Return error.
	 */
	if (!temp || (temp > (CDNS_I2C_DIVA_MAX * CDNS_I2C_DIVB_MAX)))
		return -EINVAL;

	last_error = -1;
	for (div_a = 0; div_a < CDNS_I2C_DIVA_MAX; div_a++) {
		div_b = DIV_ROUND_UP(input_clk, 22 * fscl * (div_a + 1));

		if ((div_b < 1) || (div_b > CDNS_I2C_DIVB_MAX))
			continue;
		div_b--;

		actual_fscl = input_clk / (22 * (div_a + 1) * (div_b + 1));

		if (actual_fscl > fscl)
			continue;

		current_error = ((actual_fscl > fscl) ? (actual_fscl - fscl) :
							(fscl - actual_fscl));

		if (last_error > current_error) {
			calc_div_a = div_a;
			calc_div_b = div_b;
			best_fscl = actual_fscl;
			last_error = current_error;
		}
	}

	*a = calc_div_a;
	*b = calc_div_b;
	*f = best_fscl;

	return 0;
}

/**
 * cdns_i2c_setclk - This function sets the serial clock rate for the I2C device
 * @clk_in:	I2C clock input frequency in Hz
 * @id:		Pointer to the I2C device structure
 *
 * The device must be idle rather than busy transferring data before setting
 * these device options.
 * The data rate is set by values in the control register.
 * The formula for determining the correct register values is
 *	Fscl = Fpclk/(22 x (divisor_a+1) x (divisor_b+1))
 * See the hardware data sheet for a full explanation of setting the serial
 * clock rate. The clock can not be faster than the input clock divide by 22.
 * The two most common clock rates are 100KHz and 400KHz.
 *
 * Return: 0 on success, negative error otherwise
 */
static int cdns_i2c_setclk(unsigned long clk_in, struct cdns_i2c *id)
{
	unsigned int div_a, div_b;
	unsigned int ctrl_reg;
	int ret = 0;
	unsigned long fscl = id->i2c_clk;

	ret = cdns_i2c_calc_divs(&fscl, clk_in, &div_a, &div_b);
	if (ret)
		return ret;

	ctrl_reg = cdns_i2c_readreg(CDNS_I2C_CR_OFFSET);
	ctrl_reg &= ~(CDNS_I2C_CR_DIVA_MASK | CDNS_I2C_CR_DIVB_MASK);
	ctrl_reg |= ((div_a << CDNS_I2C_CR_DIVA_SHIFT) |
			(div_b << CDNS_I2C_CR_DIVB_SHIFT));
	cdns_i2c_writereg(ctrl_reg, CDNS_I2C_CR_OFFSET);

	return 0;
}

/**
 * cdns_i2c_clk_notifier_cb - Clock rate change callback
 * @nb:		Pointer to notifier block
 * @event:	Notification reason
 * @data:	Pointer to notification data object
 *
 * This function is called when the cdns_i2c input clock frequency changes.
 * The callback checks whether a valid bus frequency can be generated after the
 * change. If so, the change is acknowledged, otherwise the change is aborted.
 * New dividers are written to the HW in the pre- or post change notification
 * depending on the scaling direction.
 *
 * Return:	NOTIFY_STOP if the rate change should be aborted, NOTIFY_OK
 *		to acknowedge the change, NOTIFY_DONE if the notification is
 *		considered irrelevant.
 */
static int cdns_i2c_clk_notifier_cb(struct notifier_block *nb, unsigned long
		event, void *data)
{
	struct clk_notifier_data *ndata = data;
	struct cdns_i2c *id = to_cdns_i2c(nb);

	if (id->suspended)
		return NOTIFY_OK;

	switch (event) {
	case PRE_RATE_CHANGE:
	{
		unsigned long input_clk = ndata->new_rate;
		unsigned long fscl = id->i2c_clk;
		unsigned int div_a, div_b;
		int ret;

		ret = cdns_i2c_calc_divs(&fscl, input_clk, &div_a, &div_b);
		if (ret) {
			dev_warn(id->adap.dev.parent,
					"clock rate change rejected\n");
			return NOTIFY_STOP;
		}

		/* scale up */
		if (ndata->new_rate > ndata->old_rate)
			cdns_i2c_setclk(ndata->new_rate, id);

		return NOTIFY_OK;
	}
	case POST_RATE_CHANGE:
		id->input_clk = ndata->new_rate;
		/* scale down */
		if (ndata->new_rate < ndata->old_rate)
			cdns_i2c_setclk(ndata->new_rate, id);
		return NOTIFY_OK;
	case ABORT_RATE_CHANGE:
		/* scale up */
		if (ndata->new_rate > ndata->old_rate)
			cdns_i2c_setclk(ndata->old_rate, id);
		return NOTIFY_OK;
	default:
		return NOTIFY_DONE;
	}
}

/**
 * cdns_i2c_suspend - Suspend method for the driver
 * @_dev:	Address of the platform_device structure
 *
 * Put the driver into low power mode.
 *
 * Return: 0 always
 */
static int __maybe_unused cdns_i2c_suspend(struct device *_dev)
{
	struct platform_device *pdev = container_of(_dev,
			struct platform_device, dev);
	struct cdns_i2c *xi2c = platform_get_drvdata(pdev);

	clk_disable(xi2c->clk);
	xi2c->suspended = 1;

	return 0;
}

/**
 * cdns_i2c_resume - Resume from suspend
 * @_dev:	Address of the platform_device structure
 *
 * Resume operation after suspend.
 *
 * Return: 0 on success and error value on error
 */
static int __maybe_unused cdns_i2c_resume(struct device *_dev)
{
	struct platform_device *pdev = container_of(_dev,
			struct platform_device, dev);
	struct cdns_i2c *xi2c = platform_get_drvdata(pdev);
	int ret;

	ret = clk_enable(xi2c->clk);
	if (ret) {
		dev_err(_dev, "Cannot enable clock.\n");
		return ret;
	}

	xi2c->suspended = 0;

	return 0;
}

static SIMPLE_DEV_PM_OPS(cdns_i2c_dev_pm_ops, cdns_i2c_suspend,
			 cdns_i2c_resume);

static const struct cdns_platform_data r1p10_i2c_def = {
				.quirks = CDNS_I2C_BROKEN_HOLD_BIT, };

static const struct of_device_id cdns_i2c_of_match[] = {
	{ .compatible = "cdns,i2c-r1p10", .data = &r1p10_i2c_def },
	{ .compatible = "cdns,i2c-r1p14",},
	{ /* end of table */ }
};
MODULE_DEVICE_TABLE(of, cdns_i2c_of_match);

/**
 * cdns_i2c_probe - Platform registration call
 * @pdev:	Handle to the platform device structure
 *
 * This function does all the memory allocation and registration for the i2c
 * device. User can modify the address mode to 10 bit address mode using the
 * ioctl call with option I2C_TENBIT.
 *
 * Return: 0 on success, negative error otherwise
 */
static int cdns_i2c_probe(struct platform_device *pdev)
{
	struct resource *r_mem;
	struct cdns_i2c *id;
	int ret;
	const struct of_device_id *match;

	id = devm_kzalloc(&pdev->dev, sizeof(*id), GFP_KERNEL);
	if (!id)
		return -ENOMEM;

	platform_set_drvdata(pdev, id);

	match = of_match_node(cdns_i2c_of_match, pdev->dev.of_node);
	if (match && match->data) {
		const struct cdns_platform_data *data = match->data;
		id->quirks = data->quirks;
	}

	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	id->membase = devm_ioremap_resource(&pdev->dev, r_mem);
	if (IS_ERR(id->membase))
		return PTR_ERR(id->membase);

	id->irq = platform_get_irq(pdev, 0);

	id->adap.dev.of_node = pdev->dev.of_node;
	id->adap.algo = &cdns_i2c_algo;
	id->adap.timeout = CDNS_I2C_TIMEOUT;
	id->adap.retries = 3;		/* Default retry value. */
	id->adap.algo_data = id;
	id->adap.dev.parent = &pdev->dev;
	init_completion(&id->xfer_done);
	snprintf(id->adap.name, sizeof(id->adap.name),
		 "Cadence I2C at %08lx", (unsigned long)r_mem->start);

	id->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(id->clk)) {
		dev_err(&pdev->dev, "input clock not found.\n");
		return PTR_ERR(id->clk);
	}
	ret = clk_prepare_enable(id->clk);
	if (ret) {
		dev_err(&pdev->dev, "Unable to enable clock.\n");
		return ret;
	}
	id->clk_rate_change_nb.notifier_call = cdns_i2c_clk_notifier_cb;
	if (clk_notifier_register(id->clk, &id->clk_rate_change_nb))
		dev_warn(&pdev->dev, "Unable to register clock notifier.\n");
	id->input_clk = clk_get_rate(id->clk);

	ret = of_property_read_u32(pdev->dev.of_node, "clock-frequency",
			&id->i2c_clk);
	if (ret || (id->i2c_clk > CDNS_I2C_SPEED_MAX))
		id->i2c_clk = CDNS_I2C_SPEED_DEFAULT;

	cdns_i2c_writereg(CDNS_I2C_CR_ACK_EN | CDNS_I2C_CR_NEA | CDNS_I2C_CR_MS,
			  CDNS_I2C_CR_OFFSET);

	ret = cdns_i2c_setclk(id->input_clk, id);
	if (ret) {
		dev_err(&pdev->dev, "invalid SCL clock: %u Hz\n", id->i2c_clk);
		ret = -EINVAL;
		goto err_clk_dis;
	}

	ret = devm_request_irq(&pdev->dev, id->irq, cdns_i2c_isr, 0,
				 DRIVER_NAME, id);
	if (ret) {
		dev_err(&pdev->dev, "cannot get irq %d\n", id->irq);
		goto err_clk_dis;
	}

	ret = i2c_add_adapter(&id->adap);
	if (ret < 0) {
		dev_err(&pdev->dev, "reg adap failed: %d\n", ret);
		goto err_clk_dis;
	}

	/*
	 * Cadence I2C controller has a bug wherein it generates
	 * invalid read transaction after HW timeout in master receiver mode.
	 * HW timeout is not used by this driver and the interrupt is disabled.
	 * But the feature itself cannot be disabled. Hence maximum value
	 * is written to this register to reduce the chances of error.
	 */
	cdns_i2c_writereg(CDNS_I2C_TIMEOUT_MAX, CDNS_I2C_TIME_OUT_OFFSET);

	dev_info(&pdev->dev, "%u kHz mmio %08lx irq %d\n",
		 id->i2c_clk / 1000, (unsigned long)r_mem->start, id->irq);

	return 0;

err_clk_dis:
	clk_disable_unprepare(id->clk);
	return ret;
}

/**
 * cdns_i2c_remove - Unregister the device after releasing the resources
 * @pdev:	Handle to the platform device structure
 *
 * This function frees all the resources allocated to the device.
 *
 * Return: 0 always
 */
static int cdns_i2c_remove(struct platform_device *pdev)
{
	struct cdns_i2c *id = platform_get_drvdata(pdev);

	i2c_del_adapter(&id->adap);
	clk_notifier_unregister(id->clk, &id->clk_rate_change_nb);
	clk_disable_unprepare(id->clk);

	return 0;
}

static struct platform_driver cdns_i2c_drv = {
	.driver = {
		.name  = DRIVER_NAME,
		.of_match_table = cdns_i2c_of_match,
		.pm = &cdns_i2c_dev_pm_ops,
	},
	.probe  = cdns_i2c_probe,
	.remove = cdns_i2c_remove,
};

module_platform_driver(cdns_i2c_drv);

MODULE_AUTHOR("Xilinx Inc.");
MODULE_DESCRIPTION("Cadence I2C bus driver");
MODULE_LICENSE("GPL");
