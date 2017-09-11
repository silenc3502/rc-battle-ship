/*
 * FireWire Serial driver
 *
 * Copyright (C) 2012 Peter Hurley <peter@hurleysoftware.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/mod_devicetable.h>
#include <linux/rculist.h>
#include <linux/workqueue.h>
#include <linux/ratelimit.h>
#include <linux/bug.h>
#include <linux/uaccess.h>

#include "fwserial.h"

#define be32_to_u64(hi, lo)  ((u64)be32_to_cpu(hi) << 32 | be32_to_cpu(lo))

#define LINUX_VENDOR_ID   0xd00d1eU  /* same id used in card root directory   */
/* 동일한 ID가 카드 루트 디렉토리에 사용됨 */
#define FWSERIAL_VERSION  0x00e81cU  /* must be unique within LINUX_VENDOR_ID */
/* 는 LINUX_VENDOR_ID 내에서 고유해야합니다.*/

/* configurable options */
static int num_ttys = 4;	    /* # of std ttys to create per fw_card    */
					/* fw_card 당 생성 할 표준 ttys */
				    /* - doubles as loopback port index       */
					/* 루프백 포트 인덱스의 두 배가됩니다*/
static bool auto_connect = true;    /* try to VIRT_CABLE to every peer        */
					/* 모든 동료에게 VIRT_CABLE 시도 */
static bool create_loop_dev = true; /* create a loopback device for each card */
					/* 각 카드에 대해 루프백 장치를 만듭니다 */

module_param_named(ttys, num_ttys, int, S_IRUGO | S_IWUSR);
module_param_named(auto, auto_connect, bool, S_IRUGO | S_IWUSR);
module_param_named(loop, create_loop_dev, bool, S_IRUGO | S_IWUSR);

/*
 * Threshold below which the tty is woken for writing
 * - should be equal to WAKEUP_CHARS in drivers/tty/n_tty.c because
 *   even if the writer is woken, n_tty_poll() won't set POLLOUT until
 *   our fifo is below this level
쓰기를 위해 tty가 깨어나는 임계 값은 drivers / tty / n_tty.c의 WAKEUP_CHARS와 같아야합니다.
왜냐하면 작성기가 깨어 있어도 FIFO가이 레벨보다 낮을 때까지 n_tty_poll ()이 POLLOUT을 설정하지 않기 때문입니다
 */
#define WAKEUP_CHARS             256

/**
 * fwserial_list: list of every fw_serial created for each fw_card
 * See discussion in fwserial_probe.
 * fwserial_list : 각 fw_card에 대해 생성 된 모든 fw_serial의 목록
 * fwserial_probe의 토론을 참조하십시오.
 */
static LIST_HEAD(fwserial_list);
static DEFINE_MUTEX(fwserial_list_mutex);

/**
 * port_table: array of tty ports allocated to each fw_card
 *
 * tty ports are allocated during probe when an fw_serial is first
 * created for a given fw_card. Ports are allocated in a contiguous block,
 * each block consisting of 'num_ports' ports.
port_table : 각 fw_card에 할당 된 tty 포트의 배열 tty 포트는 주어진 fw_card에 대해 fw_serial이 처음 생성 될 때 검사 중에 할당됩니다. 포트는 'num_ports'포트로 구성된 연속 블록에 할당됩니다.
 */
static struct fwtty_port *port_table[MAX_TOTAL_PORTS];
static DEFINE_MUTEX(port_table_lock);
static bool port_table_corrupt;
#define FWTTY_INVALID_INDEX  MAX_TOTAL_PORTS

#define loop_idx(port)	(((port)->index) / num_ports)
#define table_idx(loop)	((loop) * num_ports + num_ttys)

/* total # of tty ports created per fw_card 
   fw_card 당 생성 된 총 tty 포트 수 */
static int num_ports;

/* slab used as pool for struct fwtty_transactions
   구조체 fwtty_transactions의 풀로 사용 된 slab */
static struct kmem_cache *fwtty_txn_cache;

struct tty_driver *fwtty_driver;
static struct tty_driver *fwloop_driver;

static struct dentry *fwserial_debugfs;

struct fwtty_transaction;
typedef void (*fwtty_transaction_cb)(struct fw_card *card, int rcode,
				     void *data, size_t length,
				     struct fwtty_transaction *txn);

struct fwtty_transaction {
	struct fw_transaction      fw_txn;
	fwtty_transaction_cb       callback;
	struct fwtty_port	   *port;
	union {
		struct dma_pending dma_pended;
	};
};

#define to_device(a, b)			(a->b)
#define fwtty_err(p, fmt, ...)						\
	dev_err(to_device(p, device), fmt, ##__VA_ARGS__)
#define fwtty_info(p, fmt, ...)						\
	dev_info(to_device(p, device), fmt, ##__VA_ARGS__)
#define fwtty_notice(p, fmt, ...)					\
	dev_notice(to_device(p, device), fmt, ##__VA_ARGS__)
#define fwtty_dbg(p, fmt, ...)						\
	dev_dbg(to_device(p, device), "%s: " fmt, __func__, ##__VA_ARGS__)
#define fwtty_err_ratelimited(p, fmt, ...)				\
	dev_err_ratelimited(to_device(p, device), fmt, ##__VA_ARGS__)

#ifdef DEBUG
static inline void debug_short_write(struct fwtty_port *port, int c, int n)
{
	int avail;

	if (n < c) {
		spin_lock_bh(&port->lock);
		avail = dma_fifo_avail(&port->tx_fifo);
		spin_unlock_bh(&port->lock);
		fwtty_dbg(port, "short write: avail:%d req:%d wrote:%d\n",
			  avail, c, n);
	}
}
#else
#define debug_short_write(port, c, n)
#endif

static struct fwtty_peer *__fwserial_peer_by_node_id(struct fw_card *card,
						     int generation, int id);

#ifdef FWTTY_PROFILING

static void fwtty_profile_fifo(struct fwtty_port *port, unsigned *stat)
{
	spin_lock_bh(&port->lock);
	fwtty_profile_data(stat, dma_fifo_avail(&port->tx_fifo));
	spin_unlock_bh(&port->lock);
}

static void fwtty_dump_profile(struct seq_file *m, struct stats *stats)
{
	/* for each stat, print sum of 0 to 2^k, then individually
	   각 통계에 대해 0에서 2 ^ k까지의 인쇄 합계, 개별적으로      */
	int k = 4;
	unsigned sum;
	int j;
	char t[10];

	snprintf(t, 10, "< %d", 1 << k);
	seq_printf(m, "\n%14s  %6s", " ", t);
	for (j = k + 1; j < DISTRIBUTION_MAX_INDEX; ++j)
		seq_printf(m, "%6d", 1 << j);

	++k;
	for (j = 0, sum = 0; j <= k; ++j)
		sum += stats->reads[j];
	seq_printf(m, "\n%14s: %6d", "reads", sum);
	for (j = k + 1; j <= DISTRIBUTION_MAX_INDEX; ++j)
		seq_printf(m, "%6d", stats->reads[j]);

	for (j = 0, sum = 0; j <= k; ++j)
		sum += stats->writes[j];
	seq_printf(m, "\n%14s: %6d", "writes", sum);
	for (j = k + 1; j <= DISTRIBUTION_MAX_INDEX; ++j)
		seq_printf(m, "%6d", stats->writes[j]);

	for (j = 0, sum = 0; j <= k; ++j)
		sum += stats->txns[j];
	seq_printf(m, "\n%14s: %6d", "txns", sum);
	for (j = k + 1; j <= DISTRIBUTION_MAX_INDEX; ++j)
		seq_printf(m, "%6d", stats->txns[j]);

	for (j = 0, sum = 0; j <= k; ++j)
		sum += stats->unthrottle[j];
	seq_printf(m, "\n%14s: %6d", "avail @ unthr", sum);
	for (j = k + 1; j <= DISTRIBUTION_MAX_INDEX; ++j)
		seq_printf(m, "%6d", stats->unthrottle[j]);
}

#else
#define fwtty_profile_fifo(port, stat)
#define fwtty_dump_profile(m, stats)
#endif

/*
 * Returns the max receive packet size for the given node
 * Devices which are OHCI v1.0/ v1.1/ v1.2-draft or RFC 2734 compliant
 * are required by specification to support max_rec of 8 (512 bytes) or more.
  주어진 노드에 대한 최대 수신 패킷 크기를 반환합니다. 
  OHCI v1.0 / v1.1 / v1.2 초안 또는 RFC 2734 준수 장치는 max_rec 8 (512 바이트) 이상을 지원하는 사양이 필요합니다.
 */
static inline int device_max_receive(struct fw_device *fw_device)
{
	/* see IEEE 1394-2008 table 8-8 */
	return min(2 << fw_device->max_rec, 4096);
}

static void fwtty_log_tx_error(struct fwtty_port *port, int rcode)
{
	switch (rcode) {
	case RCODE_SEND_ERROR:
		fwtty_err_ratelimited(port, "card busy\n");
		break;
	case RCODE_ADDRESS_ERROR:
		fwtty_err_ratelimited(port, "bad unit addr or write length\n");
		break;
	case RCODE_DATA_ERROR:
		fwtty_err_ratelimited(port, "failed rx\n");
		break;
	case RCODE_NO_ACK:
		fwtty_err_ratelimited(port, "missing ack\n");
		break;
	case RCODE_BUSY:
		fwtty_err_ratelimited(port, "remote busy\n");
		break;
	default:
		fwtty_err_ratelimited(port, "failed tx: %d\n", rcode);
	}
}

static void fwtty_txn_constructor(void *this)
{
	struct fwtty_transaction *txn = this;

	init_timer(&txn->fw_txn.split_timeout_timer);
}

static void fwtty_common_callback(struct fw_card *card, int rcode,
				  void *payload, size_t len, void *cb_data)
{
	struct fwtty_transaction *txn = cb_data;
	struct fwtty_port *port = txn->port;

	if (port && rcode != RCODE_COMPLETE)
		fwtty_log_tx_error(port, rcode);
	if (txn->callback)
		txn->callback(card, rcode, payload, len, txn);
	kmem_cache_free(fwtty_txn_cache, txn);
}

static int fwtty_send_data_async(struct fwtty_peer *peer, int tcode,
				 unsigned long long addr, void *payload,
				 size_t len, fwtty_transaction_cb callback,
				 struct fwtty_port *port)
{
	struct fwtty_transaction *txn;
	int generation;

	txn = kmem_cache_alloc(fwtty_txn_cache, GFP_ATOMIC);
	if (!txn)
		return -ENOMEM;

	txn->callback = callback;
	txn->port = port;

	generation = peer->generation;
	smp_rmb();
	fw_send_request(peer->serial->card, &txn->fw_txn, tcode,
			peer->node_id, generation, peer->speed, addr, payload,
			len, fwtty_common_callback, txn);
	return 0;
}

static void fwtty_send_txn_async(struct fwtty_peer *peer,
				 struct fwtty_transaction *txn, int tcode,
				 unsigned long long addr, void *payload,
				 size_t len, fwtty_transaction_cb callback,
				 struct fwtty_port *port)
{
	int generation;

	txn->callback = callback;
	txn->port = port;

	generation = peer->generation;
	smp_rmb();
	fw_send_request(peer->serial->card, &txn->fw_txn, tcode,
			peer->node_id, generation, peer->speed, addr, payload,
			len, fwtty_common_callback, txn);
}

static void __fwtty_restart_tx(struct fwtty_port *port)
{
	int len, avail;

	len = dma_fifo_out_level(&port->tx_fifo);
	if (len)
		schedule_delayed_work(&port->drain, 0);
	avail = dma_fifo_avail(&port->tx_fifo);

	fwtty_dbg(port, "fifo len: %d avail: %d\n", len, avail);
}

static void fwtty_restart_tx(struct fwtty_port *port)
{
	spin_lock_bh(&port->lock);
	__fwtty_restart_tx(port);
	spin_unlock_bh(&port->lock);
}

/**
 * fwtty_update_port_status - decodes & dispatches line status changes
 *
 * Note: in loopback, the port->lock is being held. Only use functions that
 * don't attempt to reclaim the port->lock.
  fwtty_update_port_status - 회선 상태 변경을 디코딩하고 처리합니다. 
  참고 : 루프백에서 port-> lock이 유지됩니다. port-> lock을 재 시도하지 않는 함수 만 사용하십시오.
 */
static void fwtty_update_port_status(struct fwtty_port *port, unsigned status)
{
	unsigned delta;
	struct tty_struct *tty;

	/* simulated LSR/MSR status from remote
	   원격에서 시뮬레이트 된 LSR / MSR 상태  */
	status &= ~MCTRL_MASK;
	delta = (port->mstatus ^ status) & ~MCTRL_MASK;
	delta &= ~(status & TIOCM_RNG);
	port->mstatus = status;

	if (delta & TIOCM_RNG)
		++port->icount.rng;
	if (delta & TIOCM_DSR)
		++port->icount.dsr;
	if (delta & TIOCM_CAR)
		++port->icount.dcd;
	if (delta & TIOCM_CTS)
		++port->icount.cts;

	fwtty_dbg(port, "status: %x delta: %x\n", status, delta);

	if (delta & TIOCM_CAR) {
		tty = tty_port_tty_get(&port->port);
		if (tty && !C_CLOCAL(tty)) {
			if (status & TIOCM_CAR)
				wake_up_interruptible(&port->port.open_wait);
			else
				schedule_work(&port->hangup);
		}
		tty_kref_put(tty);
	}

	if (delta & TIOCM_CTS) {
		tty = tty_port_tty_get(&port->port);
		if (tty && C_CRTSCTS(tty)) {
			if (tty->hw_stopped) {
				if (status & TIOCM_CTS) {
					tty->hw_stopped = 0;
					if (port->loopback)
						__fwtty_restart_tx(port);
					else
						fwtty_restart_tx(port);
				}
			} else {
				if (~status & TIOCM_CTS)
					tty->hw_stopped = 1;
			}
		}
		tty_kref_put(tty);

	} else if (delta & OOB_TX_THROTTLE) {
		tty = tty_port_tty_get(&port->port);
		if (tty) {
			if (tty->hw_stopped) {
				if (~status & OOB_TX_THROTTLE) {
					tty->hw_stopped = 0;
					if (port->loopback)
						__fwtty_restart_tx(port);
					else
						fwtty_restart_tx(port);
				}
			} else {
				if (status & OOB_TX_THROTTLE)
					tty->hw_stopped = 1;
			}
		}
		tty_kref_put(tty);
	}

	if (delta & (UART_LSR_BI << 24)) {
		if (status & (UART_LSR_BI << 24)) {
			port->break_last = jiffies;
			schedule_delayed_work(&port->emit_breaks, 0);
		} else {
			/* run emit_breaks one last time (if pending)
			   마지막으로 emit_breaks를 실행합니다 (보류중인 경우). */
			mod_delayed_work(system_wq, &port->emit_breaks, 0);
		}
	}

	if (delta & (TIOCM_DSR | TIOCM_CAR | TIOCM_CTS | TIOCM_RNG))
		wake_up_interruptible(&port->port.delta_msr_wait);
}

/**
 * __fwtty_port_line_status - generate 'line status' for indicated port
 *
 * This function returns a remote 'MSR' state based on the local 'MCR' state,
 * as if a null modem cable was attached. The actual status is a mangling
 * of TIOCM_* bits suitable for sending to a peer's status_addr.
   __fwtty_port_line_status - 지정된 포트의 '회선 상태'생성이 함수는 널 모뎀 케이블이 연결되어있는 것처럼 
   로컬 'MCR'상태를 기반으로 원격 'MSR'상태를 반환합니다. 
   실제 상태는 피어의 status_addr에 보내기에 적합한 TIOCM_ * 비트의 맹 글링입니다.
   참고 : 발신자는 포트 잠금 장치를 보유해야합니다.
 *
 * Note: caller must be holding port lock
 */
static unsigned __fwtty_port_line_status(struct fwtty_port *port)
{
	unsigned status = 0;

	/* TODO: add module param to tie RNG to DTR as well 
		 모듈 param을 추가하여 RNG와 DTR을 연결합니다.  */

	if (port->mctrl & TIOCM_DTR)
		status |= TIOCM_DSR | TIOCM_CAR;
	if (port->mctrl & TIOCM_RTS)
		status |= TIOCM_CTS;
	if (port->mctrl & OOB_RX_THROTTLE)
		status |= OOB_TX_THROTTLE;
	/* emulate BRK as add'l line status */
	if (port->break_ctl)
		status |= UART_LSR_BI << 24;

	return status;
}

/**
 * __fwtty_write_port_status - send the port line status to peer
	                       피어에게 포트 라인 상태를 보낸다.
 *
 * Note: caller must be holding the port lock.
         발신자가 포트 잠금 장치를 잡고 있어야합니다.
 */
static int __fwtty_write_port_status(struct fwtty_port *port)
{
	struct fwtty_peer *peer;
	int err = -ENOENT;
	unsigned status = __fwtty_port_line_status(port);

	rcu_read_lock();
	peer = rcu_dereference(port->peer);
	if (peer) {
		err = fwtty_send_data_async(peer, TCODE_WRITE_QUADLET_REQUEST,
					    peer->status_addr, &status,
					    sizeof(status), NULL, port);
	}
	rcu_read_unlock();

	return err;
}

/**
 * fwtty_write_port_status - same as above but locked by port lock
	                     위와 동일 하나 포트 잠금 장치로 잠김
 */
static int fwtty_write_port_status(struct fwtty_port *port)
{
	int err;

	spin_lock_bh(&port->lock);
	err = __fwtty_write_port_status(port);
	spin_unlock_bh(&port->lock);
	return err;
}

static void fwtty_throttle_port(struct fwtty_port *port)
{
	struct tty_struct *tty;
	unsigned old;

	tty = tty_port_tty_get(&port->port);
	if (!tty)
		return;

	spin_lock_bh(&port->lock);

	old = port->mctrl;
	port->mctrl |= OOB_RX_THROTTLE;
	if (C_CRTSCTS(tty))
		port->mctrl &= ~TIOCM_RTS;
	if (~old & OOB_RX_THROTTLE)
		__fwtty_write_port_status(port);

	spin_unlock_bh(&port->lock);

	tty_kref_put(tty);
}

/**
 * fwtty_do_hangup - wait for ldisc to deliver all pending rx; only then hangup
 *
 * When the remote has finished tx, and all in-flight rx has been received and
 * and pushed to the flip buffer, the remote may close its device. This will
 * drop DTR on the remote which will drop carrier here. Typically, the tty is
 * hung up when carrier is dropped or lost.
 *
 * However, there is a race between the hang up and the line discipline
 * delivering its data to the reader. A hangup will cause the ldisc to flush
 * (ie., clear) the read buffer and flip buffer. Because of firewire's
 * relatively high throughput, the ldisc frequently lags well behind the driver,
 * resulting in lost data (which has already been received and written to
 * the flip buffer) when the remote closes its end.
 *
 * Unfortunately, since the flip buffer offers no direct method for determining
 * if it holds data, ensuring the ldisc has delivered all data is problematic.
 * fwtty_do_hangup - ldisc가 대기중인 모든 rx를 전달할 때까지 기다립니다. 
   리모콘이 tx를 끝내고 모든 기내 rx가 수신되어 플립 버퍼로 푸시 될 때 원격 장치가 해당 장치를 닫을 수 있습니다. 
   그러면 DTR이 리모컨에 드롭되어 여기에 통신 사업자가 드롭됩니다. 
   일반적으로 운반 대를 떨어 뜨리거나 잃어 버리면 tty가 매달리게됩니다.
  
   그러나 전화를 끊으면 리더에게 데이터를 전달하는 회선 사이에는 경쟁이 있습니다. 
   hangup을 실행하면 ldisc가 읽기 버퍼와 플립 버퍼를 플러시 (즉, 지우기)합니다. 
   Firewire의 상대적으로 높은 처리량으로 인해 ldisc는 드라이버 뒤에서 빈번하게 뒤쳐져 원격이 끝날 때 손실 된 데이터 (이미 플립 버퍼에 수신되고 기록 된 데이터)가 발생합니다.
  
   불행히도, 플립 버퍼는 데이터를 보유하는지 여부를 결정하는 직접적인 방법을 제공하지 않으므로 ldisc가 모든 데이터를 전달했는지 확인하는 것이 문제가됩니다.
 */

/* FIXME: drop this workaround when __tty_hangup waits for ldisc completion 
	  __tty_hangup이 ldisc 완료를 기다릴 때이 해결 방법을 사용하지 마십시오.  */
static void fwtty_do_hangup(struct work_struct *work)
{
	struct fwtty_port *port = to_port(work, hangup);
	struct tty_struct *tty;

	schedule_timeout_uninterruptible(msecs_to_jiffies(50));

	tty = tty_port_tty_get(&port->port);
	if (tty)
		tty_vhangup(tty);
	tty_kref_put(tty);
}

static void fwtty_emit_breaks(struct work_struct *work)
{
	struct fwtty_port *port = to_port(to_delayed_work(work), emit_breaks);
	static const char buf[16];
	unsigned long now = jiffies;
	unsigned long elapsed = now - port->break_last;
	int n, t, c, brk = 0;

	/* generate breaks at the line rate (but at least 1) 
 	   회선 속도로 중단을 생성합니다 (단 최소 1 개).*/
	n = (elapsed * port->cps) / HZ + 1;
	port->break_last = now;

	fwtty_dbg(port, "sending %d brks\n", n);

	while (n) {
		t = min(n, 16);
		c = tty_insert_flip_string_fixed_flag(&port->port, buf,
						      TTY_BREAK, t);
		n -= c;
		brk += c;
		if (c < t)
			break;
	}
	tty_flip_buffer_push(&port->port);

	if (port->mstatus & (UART_LSR_BI << 24))
		schedule_delayed_work(&port->emit_breaks, FREQ_BREAKS);
	port->icount.brk += brk;
}

static int fwtty_rx(struct fwtty_port *port, unsigned char *data, size_t len)
{
	int c, n = len;
	unsigned lsr;
	int err = 0;

	fwtty_dbg(port, "%d\n", n);
	fwtty_profile_data(port->stats.reads, n);

	if (port->write_only) {
		n = 0;
		goto out;
	}

	/* disregard break status; breaks are generated by emit_breaks work
	   break 상태 무시; emit_breaks 작업으로 나누기가 생성됩니다. */
	lsr = (port->mstatus >> 24) & ~UART_LSR_BI;

	if (port->overrun)
		lsr |= UART_LSR_OE;

	if (lsr & UART_LSR_OE)
		++port->icount.overrun;

	lsr &= port->status_mask;
	if (lsr & ~port->ignore_mask & UART_LSR_OE) {
		if (!tty_insert_flip_char(&port->port, 0, TTY_OVERRUN)) {
			err = -EIO;
			goto out;
		}
	}
	port->overrun = false;

	if (lsr & port->ignore_mask & ~UART_LSR_OE) {
		/* TODO: don't drop SAK and Magic SysRq here
	        	 SAK와 Magic SysRq를 여기에 놓지 마십시오. */
		n = 0;
		goto out;
	}

	c = tty_insert_flip_string_fixed_flag(&port->port, data, TTY_NORMAL, n);
	if (c > 0)
		tty_flip_buffer_push(&port->port);
	n -= c;

	if (n) {
		port->overrun = true;
		err = -EIO;
		fwtty_err_ratelimited(port, "flip buffer overrun\n");

	} else {
		/* throttle the sender if remaining flip buffer space has
		 * reached high watermark to avoid losing data which may be
		 * in-flight. Since the AR request context is 32k, that much
		 * data may have _already_ been acked.
		   기내에있을 수있는 데이터 손실을 피하기 위해 플립 버퍼 공간이 높은 워터 마크에 도달하면 발신자를
                   제한합니다. AR 요청 문맥이 32k이므로, 많은 양의 데이터가 이미 _ 도착할 수 있습니다.
		 */
		if (tty_buffer_space_avail(&port->port) < HIGH_WATERMARK)
			fwtty_throttle_port(port);
	}

out:
	port->icount.rx += len;
	port->stats.lost += n;
	return err;
}

/**
 * fwtty_port_handler - bus address handler for port reads/writes
 * @parameters: fw_address_callback_t as specified by firewire core interface
 *
 * This handler is responsible for handling inbound read/write dma from remotes.
   fwtty_port_handler - 포트 읽기 / 쓰기를위한 버스 주소 핸들러
   @parameters : firewire 코어 인터페이스에 의해 지정된 fw_address_callback_t이 핸들러는 리모트에서 인바운드 읽기 / 쓰기 dma를 처리합니다.
 */
static void fwtty_port_handler(struct fw_card *card,
			       struct fw_request *request,
			       int tcode, int destination, int source,
			       int generation,
			       unsigned long long addr,
			       void *data, size_t len,
			       void *callback_data)
{
	struct fwtty_port *port = callback_data;
	struct fwtty_peer *peer;
	int err;
	int rcode;

	/* Only accept rx from the peer virtual-cabled to this port
	   피어 - 투 - 피어에서이 포트로 rx 만 수락 */
	rcu_read_lock();
	peer = __fwserial_peer_by_node_id(card, generation, source);
	rcu_read_unlock();
	if (!peer || peer != rcu_access_pointer(port->peer)) {
		rcode = RCODE_ADDRESS_ERROR;
		fwtty_err_ratelimited(port, "ignoring unauthenticated data\n");
		goto respond;
	}

	switch (tcode) {
	case TCODE_WRITE_QUADLET_REQUEST:
		if (addr != port->rx_handler.offset || len != 4) {
			rcode = RCODE_ADDRESS_ERROR;
		} else {
			fwtty_update_port_status(port, *(unsigned *)data);
			rcode = RCODE_COMPLETE;
		}
		break;

	case TCODE_WRITE_BLOCK_REQUEST:
		if (addr != port->rx_handler.offset + 4 ||
		    len > port->rx_handler.length - 4) {
			rcode = RCODE_ADDRESS_ERROR;
		} else {
			err = fwtty_rx(port, data, len);
			switch (err) {
			case 0:
				rcode = RCODE_COMPLETE;
				break;
			case -EIO:
				rcode = RCODE_DATA_ERROR;
				break;
			default:
				rcode = RCODE_CONFLICT_ERROR;
				break;
			}
		}
		break;

	default:
		rcode = RCODE_TYPE_ERROR;
	}

respond:
	fw_send_response(card, request, rcode);
}

/**
 * fwtty_tx_complete - callback for tx dma
 * @data: ignored, has no meaning for write txns
 * @length: ignored, has no meaning for write txns
 *
 * The writer must be woken here if the fifo has been emptied because it
 * may have slept if chars_in_buffer was != 0
   fwtty_tx_complete - tx dma에 대한 콜백 
   @data : 무시, 쓰기에는 의미가 없습니다 
   txns @length : 무시, txns 쓰기에는 의미가 없습니다.
   chars_in_buffer가! = 0 인 경우에 잠자기되었을 수 있기 때문에 FIFO가 비어 있다면 작가가 깨우쳐야 합니다.
 */
static void fwtty_tx_complete(struct fw_card *card, int rcode,
			      void *data, size_t length,
			      struct fwtty_transaction *txn)
{
	struct fwtty_port *port = txn->port;
	int len;

	fwtty_dbg(port, "rcode: %d\n", rcode);

	switch (rcode) {
	case RCODE_COMPLETE:
		spin_lock_bh(&port->lock);
		dma_fifo_out_complete(&port->tx_fifo, &txn->dma_pended);
		len = dma_fifo_level(&port->tx_fifo);
		spin_unlock_bh(&port->lock);

		port->icount.tx += txn->dma_pended.len;
		break;

	default:
		/* TODO: implement retries
		   	 재시도 구현 */
		spin_lock_bh(&port->lock);
		dma_fifo_out_complete(&port->tx_fifo, &txn->dma_pended);
		len = dma_fifo_level(&port->tx_fifo);
		spin_unlock_bh(&port->lock);

		port->stats.dropped += txn->dma_pended.len;
	}

	if (len < WAKEUP_CHARS)
		tty_port_tty_wakeup(&port->port);
}

static int fwtty_tx(struct fwtty_port *port, bool drain)
{
	struct fwtty_peer *peer;
	struct fwtty_transaction *txn;
	struct tty_struct *tty;
	int n, len;

	tty = tty_port_tty_get(&port->port);
	if (!tty)
		return -ENOENT;

	rcu_read_lock();
	peer = rcu_dereference(port->peer);
	if (!peer) {
		n = -EIO;
		goto out;
	}

	if (test_and_set_bit(IN_TX, &port->flags)) {
		n = -EALREADY;
		goto out;
	}

	/* try to write as many dma transactions out as possible
	   최대한 많은 dma 트랜잭션을 작성하려고 시도하십시오. */
	n = -EAGAIN;
	while (!tty->stopped && !tty->hw_stopped &&
	       !test_bit(STOP_TX, &port->flags)) {
		txn = kmem_cache_alloc(fwtty_txn_cache, GFP_ATOMIC);
		if (!txn) {
			n = -ENOMEM;
			break;
		}

		spin_lock_bh(&port->lock);
		n = dma_fifo_out_pend(&port->tx_fifo, &txn->dma_pended);
		spin_unlock_bh(&port->lock);

		fwtty_dbg(port, "out: %u rem: %d\n", txn->dma_pended.len, n);

		if (n < 0) {
			kmem_cache_free(fwtty_txn_cache, txn);
			if (n == -EAGAIN) {
				++port->stats.tx_stall;
			} else if (n == -ENODATA) {
				fwtty_profile_data(port->stats.txns, 0);
			} else {
				++port->stats.fifo_errs;
				fwtty_err_ratelimited(port, "fifo err: %d\n",
						      n);
			}
			break;
		}

		fwtty_profile_data(port->stats.txns, txn->dma_pended.len);

		fwtty_send_txn_async(peer, txn, TCODE_WRITE_BLOCK_REQUEST,
				     peer->fifo_addr, txn->dma_pended.data,
				     txn->dma_pended.len, fwtty_tx_complete,
				     port);
		++port->stats.sent;

		/*
		 * Stop tx if the 'last view' of the fifo is empty or if
		 * this is the writer and there's not enough data to bother
		   FIFO의 '마지막보기'가 비어 있거나 이것이 작가이고 괴롭히는 데 충분한 데이터가없는 경우 tx를 중지하십시오.
		 */
		if (n == 0 || (!drain && n < WRITER_MINIMUM))
			break;
	}

	if (n >= 0 || n == -EAGAIN || n == -ENOMEM || n == -ENODATA) {
		spin_lock_bh(&port->lock);
		len = dma_fifo_out_level(&port->tx_fifo);
		if (len) {
			unsigned long delay = (n == -ENOMEM) ? HZ : 1;

			schedule_delayed_work(&port->drain, delay);
		}
		len = dma_fifo_level(&port->tx_fifo);
		spin_unlock_bh(&port->lock);

		/* wakeup the writer */
		if (drain && len < WAKEUP_CHARS)
			tty_wakeup(tty);
	}

	clear_bit(IN_TX, &port->flags);
	wake_up_interruptible(&port->wait_tx);

out:
	rcu_read_unlock();
	tty_kref_put(tty);
	return n;
}

static void fwtty_drain_tx(struct work_struct *work)
{
	struct fwtty_port *port = to_port(to_delayed_work(work), drain);

	fwtty_tx(port, true);
}

static void fwtty_write_xchar(struct fwtty_port *port, char ch)
{
	struct fwtty_peer *peer;

	++port->stats.xchars;

	fwtty_dbg(port, "%02x\n", ch);

	rcu_read_lock();
	peer = rcu_dereference(port->peer);
	if (peer) {
		fwtty_send_data_async(peer, TCODE_WRITE_BLOCK_REQUEST,
				      peer->fifo_addr, &ch, sizeof(ch),
				      NULL, port);
	}
	rcu_read_unlock();
}

struct fwtty_port *fwtty_port_get(unsigned index)
{
	struct fwtty_port *port;

	if (index >= MAX_TOTAL_PORTS)
		return NULL;

	mutex_lock(&port_table_lock);
	port = port_table[index];
	if (port)
		kref_get(&port->serial->kref);
	mutex_unlock(&port_table_lock);
	return port;
}
EXPORT_SYMBOL(fwtty_port_get);

static int fwtty_ports_add(struct fw_serial *serial)
{
	int err = -EBUSY;
	int i, j;

	if (port_table_corrupt)
		return err;

	mutex_lock(&port_table_lock);
	for (i = 0; i + num_ports <= MAX_TOTAL_PORTS; i += num_ports) {
		if (!port_table[i]) {
			for (j = 0; j < num_ports; ++i, ++j) {
				serial->ports[j]->index = i;
				port_table[i] = serial->ports[j];
			}
			err = 0;
			break;
		}
	}
	mutex_unlock(&port_table_lock);
	return err;
}

static void fwserial_destroy(struct kref *kref)
{
	struct fw_serial *serial = to_serial(kref, kref);
	struct fwtty_port **ports = serial->ports;
	int j, i = ports[0]->index;

	synchronize_rcu();

	mutex_lock(&port_table_lock);
	for (j = 0; j < num_ports; ++i, ++j) {
		port_table_corrupt |= port_table[i] != ports[j];
		WARN_ONCE(port_table_corrupt, "port_table[%d]: %p != ports[%d]: %p",
			  i, port_table[i], j, ports[j]);

		port_table[i] = NULL;
	}
	mutex_unlock(&port_table_lock);

	for (j = 0; j < num_ports; ++j) {
		fw_core_remove_address_handler(&ports[j]->rx_handler);
		tty_port_destroy(&ports[j]->port);
		kfree(ports[j]);
	}
	kfree(serial);
}

void fwtty_port_put(struct fwtty_port *port)
{
	kref_put(&port->serial->kref, fwserial_destroy);
}
EXPORT_SYMBOL(fwtty_port_put);

static void fwtty_port_dtr_rts(struct tty_port *tty_port, int on)
{
	struct fwtty_port *port = to_port(tty_port, port);

	fwtty_dbg(port, "on/off: %d\n", on);

	spin_lock_bh(&port->lock);
	/* Don't change carrier state if this is a console
  	   이것이 콘솔 인 경우 이동 통신사 상태를 변경하지 마십시오. */
	if (!port->port.console) {
		if (on)
			port->mctrl |= TIOCM_DTR | TIOCM_RTS;
		else
			port->mctrl &= ~(TIOCM_DTR | TIOCM_RTS);
	}

	__fwtty_write_port_status(port);
	spin_unlock_bh(&port->lock);
}

/**
 * fwtty_port_carrier_raised: required tty_port operation
 *
 * This port operation is polled after a tty has been opened and is waiting for
 * carrier detect -- see drivers/tty/tty_port:tty_port_block_til_ready().
   fwtty_port_carrier_raised : 필수 tty_port 작업 tty가 열리고 캐리어 감지를 기다리는 중 폴링됩니다. 
   drivers / tty / tty_port : tty_port_block_til_ready ()를 참조하십시오.
 */
static int fwtty_port_carrier_raised(struct tty_port *tty_port)
{
	struct fwtty_port *port = to_port(tty_port, port);
	int rc;

	rc = (port->mstatus & TIOCM_CAR);

	fwtty_dbg(port, "%d\n", rc);

	return rc;
}

static unsigned set_termios(struct fwtty_port *port, struct tty_struct *tty)
{
	unsigned baud, frame;

	baud = tty_termios_baud_rate(&tty->termios);
	tty_termios_encode_baud_rate(&tty->termios, baud, baud);

	/* compute bit count of 2 frames 
	   2 프레임의 비트 수 계산  */
	frame = 12 + ((C_CSTOPB(tty)) ? 4 : 2) + ((C_PARENB(tty)) ? 2 : 0);

	switch (C_CSIZE(tty)) {
	case CS5:
		frame -= (C_CSTOPB(tty)) ? 1 : 0;
		break;
	case CS6:
		frame += 2;
		break;
	case CS7:
		frame += 4;
		break;
	case CS8:
		frame += 6;
		break;
	}

	port->cps = (baud << 1) / frame;

	port->status_mask = UART_LSR_OE;
	if (_I_FLAG(tty, BRKINT | PARMRK))
		port->status_mask |= UART_LSR_BI;

	port->ignore_mask = 0;
	if (I_IGNBRK(tty)) {
		port->ignore_mask |= UART_LSR_BI;
		if (I_IGNPAR(tty))
			port->ignore_mask |= UART_LSR_OE;
	}

	port->write_only = !C_CREAD(tty);

	/* turn off echo and newline xlat if loopback
	   루프백이면 에코와 개행 문자를 끄라.   */
	if (port->loopback) {
		tty->termios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHOKE |
					  ECHONL | ECHOPRT | ECHOCTL);
		tty->termios.c_oflag &= ~ONLCR;
	}

	return baud;
}

static int fwtty_port_activate(struct tty_port *tty_port,
			       struct tty_struct *tty)
{
	struct fwtty_port *port = to_port(tty_port, port);
	unsigned baud;
	int err;

	set_bit(TTY_IO_ERROR, &tty->flags);

	err = dma_fifo_alloc(&port->tx_fifo, FWTTY_PORT_TXFIFO_LEN,
			     cache_line_size(),
			     port->max_payload,
			     FWTTY_PORT_MAX_PEND_DMA,
			     GFP_KERNEL);
	if (err)
		return err;

	spin_lock_bh(&port->lock);

	baud = set_termios(port, tty);

	/* if console, don't change carrier state
	   콘솔 인 경우 이동 통신사 상태를 변경하지 마십시오. */
	if (!port->port.console) {
		port->mctrl = 0;
		if (baud != 0)
			port->mctrl = TIOCM_DTR | TIOCM_RTS;
	}

	if (C_CRTSCTS(tty) && ~port->mstatus & TIOCM_CTS)
		tty->hw_stopped = 1;

	__fwtty_write_port_status(port);
	spin_unlock_bh(&port->lock);

	clear_bit(TTY_IO_ERROR, &tty->flags);

	return 0;
}

/**
 * fwtty_port_shutdown
 *
 * Note: the tty port core ensures this is not the console and
 * manages TTY_IO_ERROR properly
   참고 : tty 포트 코어는 콘솔이 아니며 TTY_IO_ERROR를 제대로 관리하도록합니다.
 */
static void fwtty_port_shutdown(struct tty_port *tty_port)
{
	struct fwtty_port *port = to_port(tty_port, port);

	/* TODO: cancel outstanding transactions
		 미결 거래 취소 */

	cancel_delayed_work_sync(&port->emit_breaks);
	cancel_delayed_work_sync(&port->drain);

	spin_lock_bh(&port->lock);
	port->flags = 0;
	port->break_ctl = 0;
	port->overrun = 0;
	__fwtty_write_port_status(port);
	dma_fifo_free(&port->tx_fifo);
	spin_unlock_bh(&port->lock);
}

static int fwtty_open(struct tty_struct *tty, struct file *fp)
{
	struct fwtty_port *port = tty->driver_data;

	return tty_port_open(&port->port, tty, fp);
}

static void fwtty_close(struct tty_struct *tty, struct file *fp)
{
	struct fwtty_port *port = tty->driver_data;

	tty_port_close(&port->port, tty, fp);
}

static void fwtty_hangup(struct tty_struct *tty)
{
	struct fwtty_port *port = tty->driver_data;

	tty_port_hangup(&port->port);
}

static void fwtty_cleanup(struct tty_struct *tty)
{
	struct fwtty_port *port = tty->driver_data;

	tty->driver_data = NULL;
	fwtty_port_put(port);
}

static int fwtty_install(struct tty_driver *driver, struct tty_struct *tty)
{
	struct fwtty_port *port = fwtty_port_get(tty->index);
	int err;

	err = tty_standard_install(driver, tty);
	if (!err)
		tty->driver_data = port;
	else
		fwtty_port_put(port);
	return err;
}

static int fwloop_install(struct tty_driver *driver, struct tty_struct *tty)
{
	struct fwtty_port *port = fwtty_port_get(table_idx(tty->index));
	int err;

	err = tty_standard_install(driver, tty);
	if (!err)
		tty->driver_data = port;
	else
		fwtty_port_put(port);
	return err;
}

static int fwtty_write(struct tty_struct *tty, const unsigned char *buf, int c)
{
	struct fwtty_port *port = tty->driver_data;
	int n, len;

	fwtty_dbg(port, "%d\n", c);
	fwtty_profile_data(port->stats.writes, c);

	spin_lock_bh(&port->lock);
	n = dma_fifo_in(&port->tx_fifo, buf, c);
	len = dma_fifo_out_level(&port->tx_fifo);
	if (len < DRAIN_THRESHOLD)
		schedule_delayed_work(&port->drain, 1);
	spin_unlock_bh(&port->lock);

	if (len >= DRAIN_THRESHOLD)
		fwtty_tx(port, false);

	debug_short_write(port, c, n);

	return (n < 0) ? 0 : n;
}

static int fwtty_write_room(struct tty_struct *tty)
{
	struct fwtty_port *port = tty->driver_data;
	int n;

	spin_lock_bh(&port->lock);
	n = dma_fifo_avail(&port->tx_fifo);
	spin_unlock_bh(&port->lock);

	fwtty_dbg(port, "%d\n", n);

	return n;
}

static int fwtty_chars_in_buffer(struct tty_struct *tty)
{
	struct fwtty_port *port = tty->driver_data;
	int n;

	spin_lock_bh(&port->lock);
	n = dma_fifo_level(&port->tx_fifo);
	spin_unlock_bh(&port->lock);

	fwtty_dbg(port, "%d\n", n);

	return n;
}

static void fwtty_send_xchar(struct tty_struct *tty, char ch)
{
	struct fwtty_port *port = tty->driver_data;

	fwtty_dbg(port, "%02x\n", ch);

	fwtty_write_xchar(port, ch);
}

static void fwtty_throttle(struct tty_struct *tty)
{
	struct fwtty_port *port = tty->driver_data;

	/*
	 * Ignore throttling (but not unthrottling).
	 * It only makes sense to throttle when data will no longer be
	 * accepted by the tty flip buffer. For example, it is
	 * possible for received data to overflow the tty buffer long
	 * before the line discipline ever has a chance to throttle the driver.
	 * Additionally, the driver may have already completed the I/O
	 * but the tty buffer is still emptying, so the line discipline is
	 * throttling and unthrottling nothing.
 	   조절을 무시합니다 (조절은 조절하지 않음).
	   tty 플립 버퍼가 더 이상 데이터를 받아들이지 않을 때만 조절할 수 있습니다. 
	   예를 들어, 수신 된 데이터가 회선 제어가 드라이버를 스로틀 (throttle) 할 수있는 기회를 갖기 오래 전에 tty 버퍼를 오버플로하는 것이 가능합니다.
	   또한 드라이버가 이미 I / O를 완료했을 수 있지만 tty 버퍼가 여전히 비어있어 회선 제어가 제한되고 아무런 문제가 없습니다.
	 */

	++port->stats.throttled;
}

static void fwtty_unthrottle(struct tty_struct *tty)
{
	struct fwtty_port *port = tty->driver_data;

	fwtty_dbg(port, "CRTSCTS: %d\n", C_CRTSCTS(tty) != 0);

	fwtty_profile_fifo(port, port->stats.unthrottle);

	spin_lock_bh(&port->lock);
	port->mctrl &= ~OOB_RX_THROTTLE;
	if (C_CRTSCTS(tty))
		port->mctrl |= TIOCM_RTS;
	__fwtty_write_port_status(port);
	spin_unlock_bh(&port->lock);
}

static int check_msr_delta(struct fwtty_port *port, unsigned long mask,
			   struct async_icount *prev)
{
	struct async_icount now;
	int delta;

	now = port->icount;

	delta = ((mask & TIOCM_RNG && prev->rng != now.rng) ||
		 (mask & TIOCM_DSR && prev->dsr != now.dsr) ||
		 (mask & TIOCM_CAR && prev->dcd != now.dcd) ||
		 (mask & TIOCM_CTS && prev->cts != now.cts));

	*prev = now;

	return delta;
}

static int wait_msr_change(struct fwtty_port *port, unsigned long mask)
{
	struct async_icount prev;

	prev = port->icount;

	return wait_event_interruptible(port->port.delta_msr_wait,
					check_msr_delta(port, mask, &prev));
}

static int get_serial_info(struct fwtty_port *port,
			   struct serial_struct __user *info)
{
	struct serial_struct tmp;

	memset(&tmp, 0, sizeof(tmp));

	tmp.type =  PORT_UNKNOWN;
	tmp.line =  port->port.tty->index;
	tmp.flags = port->port.flags;
	tmp.xmit_fifo_size = FWTTY_PORT_TXFIFO_LEN;
	tmp.baud_base = 400000000;
	tmp.close_delay = port->port.close_delay;

	return (copy_to_user(info, &tmp, sizeof(*info))) ? -EFAULT : 0;
}

static int set_serial_info(struct fwtty_port *port,
			   struct serial_struct __user *info)
{
	struct serial_struct tmp;

	if (copy_from_user(&tmp, info, sizeof(tmp)))
		return -EFAULT;

	if (tmp.irq != 0 || tmp.port != 0 || tmp.custom_divisor != 0 ||
	    tmp.baud_base != 400000000)
		return -EPERM;

	if (!capable(CAP_SYS_ADMIN)) {
		if (((tmp.flags & ~ASYNC_USR_MASK) !=
		     (port->port.flags & ~ASYNC_USR_MASK)))
			return -EPERM;
	} else {
		port->port.close_delay = tmp.close_delay * HZ / 100;
	}

	return 0;
}

static int fwtty_ioctl(struct tty_struct *tty, unsigned cmd,
		       unsigned long arg)
{
	struct fwtty_port *port = tty->driver_data;
	int err;

	switch (cmd) {
	case TIOCGSERIAL:
		mutex_lock(&port->port.mutex);
		err = get_serial_info(port, (void __user *)arg);
		mutex_unlock(&port->port.mutex);
		break;

	case TIOCSSERIAL:
		mutex_lock(&port->port.mutex);
		err = set_serial_info(port, (void __user *)arg);
		mutex_unlock(&port->port.mutex);
		break;

	case TIOCMIWAIT:
		err = wait_msr_change(port, arg);
		break;

	default:
		err = -ENOIOCTLCMD;
	}

	return err;
}

static void fwtty_set_termios(struct tty_struct *tty, struct ktermios *old)
{
	struct fwtty_port *port = tty->driver_data;
	unsigned baud;

	spin_lock_bh(&port->lock);
	baud = set_termios(port, tty);

	if ((baud == 0) && (old->c_cflag & CBAUD)) {
		port->mctrl &= ~(TIOCM_DTR | TIOCM_RTS);
	} else if ((baud != 0) && !(old->c_cflag & CBAUD)) {
		if (C_CRTSCTS(tty) || !test_bit(TTY_THROTTLED, &tty->flags))
			port->mctrl |= TIOCM_DTR | TIOCM_RTS;
		else
			port->mctrl |= TIOCM_DTR;
	}
	__fwtty_write_port_status(port);
	spin_unlock_bh(&port->lock);

	if (old->c_cflag & CRTSCTS) {
		if (!C_CRTSCTS(tty)) {
			tty->hw_stopped = 0;
			fwtty_restart_tx(port);
		}
	} else if (C_CRTSCTS(tty) && ~port->mstatus & TIOCM_CTS) {
		tty->hw_stopped = 1;
	}
}

/**
 * fwtty_break_ctl - start/stop sending breaks
 *
 * Signals the remote to start or stop generating simulated breaks.
 * First, stop dequeueing from the fifo and wait for writer/drain to leave tx
 * before signalling the break line status. This guarantees any pending rx will
 * be queued to the line discipline before break is simulated on the remote.
 * Conversely, turning off break_ctl requires signalling the line status change,
 * then enabling tx.
   fwtty_break_ctl - 중단 전송 시작 / 중지 원격으로 시작하여 시뮬레이션 중단 생성을 중지하도록 신호를 보냅니다.
   우선, FIFO에서 큐를 빼고 중단 라인 상태를 알리기 전에 작가 / 배수구가 tx를 기다릴 때까지 기다리십시오. 
   이렇게하면 보류중인 모든 rx가 원격에서 휴식 시간을 시뮬레이션하기 전에 회선 규칙에 대기하게됩니다.
   반대로, break_ctl을 끄려면 회선 상태 변경 신호가 필요합니다.
   그런 다음 tx를 사용합니다.
  * /	
 */
static int fwtty_break_ctl(struct tty_struct *tty, int state)
{
	struct fwtty_port *port = tty->driver_data;
	long ret;

	fwtty_dbg(port, "%d\n", state);

	if (state == -1) {
		set_bit(STOP_TX, &port->flags);
		ret = wait_event_interruptible_timeout(port->wait_tx,
					       !test_bit(IN_TX, &port->flags),
					       10);
		if (ret == 0 || ret == -ERESTARTSYS) {
			clear_bit(STOP_TX, &port->flags);
			fwtty_restart_tx(port);
			return -EINTR;
		}
	}

	spin_lock_bh(&port->lock);
	port->break_ctl = (state == -1);
	__fwtty_write_port_status(port);
	spin_unlock_bh(&port->lock);

	if (state == 0) {
		spin_lock_bh(&port->lock);
		dma_fifo_reset(&port->tx_fifo);
		clear_bit(STOP_TX, &port->flags);
		spin_unlock_bh(&port->lock);
	}
	return 0;
}

static int fwtty_tiocmget(struct tty_struct *tty)
{
	struct fwtty_port *port = tty->driver_data;
	unsigned tiocm;

	spin_lock_bh(&port->lock);
	tiocm = (port->mctrl & MCTRL_MASK) | (port->mstatus & ~MCTRL_MASK);
	spin_unlock_bh(&port->lock);

	fwtty_dbg(port, "%x\n", tiocm);

	return tiocm;
}

static int fwtty_tiocmset(struct tty_struct *tty, unsigned set, unsigned clear)
{
	struct fwtty_port *port = tty->driver_data;

	fwtty_dbg(port, "set: %x clear: %x\n", set, clear);

	/* TODO: simulate loopback if TIOCM_LOOP set
	         TIOCM_LOOP가 설정된 경우 루프백을 시뮬레이트합니다. */

	spin_lock_bh(&port->lock);
	port->mctrl &= ~(clear & MCTRL_MASK & 0xffff);
	port->mctrl |= set & MCTRL_MASK & 0xffff;
	__fwtty_write_port_status(port);
	spin_unlock_bh(&port->lock);
	return 0;
}

static int fwtty_get_icount(struct tty_struct *tty,
			    struct serial_icounter_struct *icount)
{
	struct fwtty_port *port = tty->driver_data;
	struct stats stats;

	memcpy(&stats, &port->stats, sizeof(stats));
	if (port->port.console)
		(*port->fwcon_ops->stats)(&stats, port->con_data);

	icount->cts = port->icount.cts;
	icount->dsr = port->icount.dsr;
	icount->rng = port->icount.rng;
	icount->dcd = port->icount.dcd;
	icount->rx  = port->icount.rx;
	icount->tx  = port->icount.tx + stats.xchars;
	icount->frame   = port->icount.frame;
	icount->overrun = port->icount.overrun;
	icount->parity  = port->icount.parity;
	icount->brk     = port->icount.brk;
	icount->buf_overrun = port->icount.overrun;
	return 0;
}

static void fwtty_proc_show_port(struct seq_file *m, struct fwtty_port *port)
{
	struct stats stats;

	memcpy(&stats, &port->stats, sizeof(stats));
	if (port->port.console)
		(*port->fwcon_ops->stats)(&stats, port->con_data);

	seq_printf(m, " addr:%012llx tx:%d rx:%d", port->rx_handler.offset,
		   port->icount.tx + stats.xchars, port->icount.rx);
	seq_printf(m, " cts:%d dsr:%d rng:%d dcd:%d", port->icount.cts,
		   port->icount.dsr, port->icount.rng, port->icount.dcd);
	seq_printf(m, " fe:%d oe:%d pe:%d brk:%d", port->icount.frame,
		   port->icount.overrun, port->icount.parity, port->icount.brk);
}

static void fwtty_debugfs_show_port(struct seq_file *m, struct fwtty_port *port)
{
	struct stats stats;

	memcpy(&stats, &port->stats, sizeof(stats));
	if (port->port.console)
		(*port->fwcon_ops->stats)(&stats, port->con_data);

	seq_printf(m, " dr:%d st:%d err:%d lost:%d", stats.dropped,
		   stats.tx_stall, stats.fifo_errs, stats.lost);
	seq_printf(m, " pkts:%d thr:%d", stats.sent, stats.throttled);

	if (port->port.console) {
		seq_puts(m, "\n    ");
		(*port->fwcon_ops->proc_show)(m, port->con_data);
	}

	fwtty_dump_profile(m, &port->stats);
}

static void fwtty_debugfs_show_peer(struct seq_file *m, struct fwtty_peer *peer)
{
	int generation = peer->generation;

	smp_rmb();
	seq_printf(m, " %s:", dev_name(&peer->unit->device));
	seq_printf(m, " node:%04x gen:%d", peer->node_id, generation);
	seq_printf(m, " sp:%d max:%d guid:%016llx", peer->speed,
		   peer->max_payload, (unsigned long long) peer->guid);
	seq_printf(m, " mgmt:%012llx", (unsigned long long) peer->mgmt_addr);
	seq_printf(m, " addr:%012llx", (unsigned long long) peer->status_addr);
	seq_putc(m, '\n');
}

static int fwtty_proc_show(struct seq_file *m, void *v)
{
	struct fwtty_port *port;
	int i;

	seq_puts(m, "fwserinfo: 1.0 driver: 1.0\n");
	for (i = 0; i < MAX_TOTAL_PORTS && (port = fwtty_port_get(i)); ++i) {
		seq_printf(m, "%2d:", i);
		if (capable(CAP_SYS_ADMIN))
			fwtty_proc_show_port(m, port);
		fwtty_port_put(port);
		seq_puts(m, "\n");
	}
	return 0;
}

static int fwtty_debugfs_stats_show(struct seq_file *m, void *v)
{
	struct fw_serial *serial = m->private;
	struct fwtty_port *port;
	int i;

	for (i = 0; i < num_ports; ++i) {
		port = fwtty_port_get(serial->ports[i]->index);
		if (port) {
			seq_printf(m, "%2d:", port->index);
			fwtty_proc_show_port(m, port);
			fwtty_debugfs_show_port(m, port);
			fwtty_port_put(port);
			seq_puts(m, "\n");
		}
	}
	return 0;
}

static int fwtty_debugfs_peers_show(struct seq_file *m, void *v)
{
	struct fw_serial *serial = m->private;
	struct fwtty_peer *peer;

	rcu_read_lock();
	seq_printf(m, "card: %s  guid: %016llx\n",
		   dev_name(serial->card->device),
		   (unsigned long long) serial->card->guid);
	list_for_each_entry_rcu(peer, &serial->peer_list, list)
		fwtty_debugfs_show_peer(m, peer);
	rcu_read_unlock();
	return 0;
}

static int fwtty_proc_open(struct inode *inode, struct file *fp)
{
	return single_open(fp, fwtty_proc_show, NULL);
}

static int fwtty_stats_open(struct inode *inode, struct file *fp)
{
	return single_open(fp, fwtty_debugfs_stats_show, inode->i_private);
}

static int fwtty_peers_open(struct inode *inode, struct file *fp)
{
	return single_open(fp, fwtty_debugfs_peers_show, inode->i_private);
}

static const struct file_operations fwtty_stats_fops = {
	.owner =	THIS_MODULE,
	.open =		fwtty_stats_open,
	.read =		seq_read,
	.llseek =	seq_lseek,
	.release =	single_release,
};

static const struct file_operations fwtty_peers_fops = {
	.owner =	THIS_MODULE,
	.open =		fwtty_peers_open,
	.read =		seq_read,
	.llseek =	seq_lseek,
	.release =	single_release,
};

static const struct file_operations fwtty_proc_fops = {
	.owner =        THIS_MODULE,
	.open =         fwtty_proc_open,
	.read =         seq_read,
	.llseek =       seq_lseek,
	.release =      single_release,
};

static const struct tty_port_operations fwtty_port_ops = {
	.dtr_rts =		fwtty_port_dtr_rts,
	.carrier_raised =	fwtty_port_carrier_raised,
	.shutdown =		fwtty_port_shutdown,
	.activate =		fwtty_port_activate,
};

static const struct tty_operations fwtty_ops = {
	.open =			fwtty_open,
	.close =		fwtty_close,
	.hangup =		fwtty_hangup,
	.cleanup =		fwtty_cleanup,
	.install =		fwtty_install,
	.write =		fwtty_write,
	.write_room =		fwtty_write_room,
	.chars_in_buffer =	fwtty_chars_in_buffer,
	.send_xchar =           fwtty_send_xchar,
	.throttle =             fwtty_throttle,
	.unthrottle =           fwtty_unthrottle,
	.ioctl =		fwtty_ioctl,
	.set_termios =		fwtty_set_termios,
	.break_ctl =		fwtty_break_ctl,
	.tiocmget =		fwtty_tiocmget,
	.tiocmset =		fwtty_tiocmset,
	.get_icount =		fwtty_get_icount,
	.proc_fops =		&fwtty_proc_fops,
};

static const struct tty_operations fwloop_ops = {
	.open =			fwtty_open,
	.close =		fwtty_close,
	.hangup =		fwtty_hangup,
	.cleanup =		fwtty_cleanup,
	.install =		fwloop_install,
	.write =		fwtty_write,
	.write_room =		fwtty_write_room,
	.chars_in_buffer =	fwtty_chars_in_buffer,
	.send_xchar =           fwtty_send_xchar,
	.throttle =             fwtty_throttle,
	.unthrottle =           fwtty_unthrottle,
	.ioctl =		fwtty_ioctl,
	.set_termios =		fwtty_set_termios,
	.break_ctl =		fwtty_break_ctl,
	.tiocmget =		fwtty_tiocmget,
	.tiocmset =		fwtty_tiocmset,
	.get_icount =		fwtty_get_icount,
};

static inline int mgmt_pkt_expected_len(__be16 code)
{
	static const struct fwserial_mgmt_pkt pkt;

	switch (be16_to_cpu(code)) {
	case FWSC_VIRT_CABLE_PLUG:
		return sizeof(pkt.hdr) + sizeof(pkt.plug_req);

	case FWSC_VIRT_CABLE_PLUG_RSP:  /* | FWSC_RSP_OK */
		return sizeof(pkt.hdr) + sizeof(pkt.plug_rsp);

	case FWSC_VIRT_CABLE_UNPLUG:
	case FWSC_VIRT_CABLE_UNPLUG_RSP:
	case FWSC_VIRT_CABLE_PLUG_RSP | FWSC_RSP_NACK:
	case FWSC_VIRT_CABLE_UNPLUG_RSP | FWSC_RSP_NACK:
		return sizeof(pkt.hdr);

	default:
		return -1;
	}
}

static inline void fill_plug_params(struct virt_plug_params *params,
				    struct fwtty_port *port)
{
	u64 status_addr = port->rx_handler.offset;
	u64 fifo_addr = port->rx_handler.offset + 4;
	size_t fifo_len = port->rx_handler.length - 4;

	params->status_hi = cpu_to_be32(status_addr >> 32);
	params->status_lo = cpu_to_be32(status_addr);
	params->fifo_hi = cpu_to_be32(fifo_addr >> 32);
	params->fifo_lo = cpu_to_be32(fifo_addr);
	params->fifo_len = cpu_to_be32(fifo_len);
}

static inline void fill_plug_req(struct fwserial_mgmt_pkt *pkt,
				 struct fwtty_port *port)
{
	pkt->hdr.code = cpu_to_be16(FWSC_VIRT_CABLE_PLUG);
	pkt->hdr.len = cpu_to_be16(mgmt_pkt_expected_len(pkt->hdr.code));
	fill_plug_params(&pkt->plug_req, port);
}

static inline void fill_plug_rsp_ok(struct fwserial_mgmt_pkt *pkt,
				    struct fwtty_port *port)
{
	pkt->hdr.code = cpu_to_be16(FWSC_VIRT_CABLE_PLUG_RSP);
	pkt->hdr.len = cpu_to_be16(mgmt_pkt_expected_len(pkt->hdr.code));
	fill_plug_params(&pkt->plug_rsp, port);
}

static inline void fill_plug_rsp_nack(struct fwserial_mgmt_pkt *pkt)
{
	pkt->hdr.code = cpu_to_be16(FWSC_VIRT_CABLE_PLUG_RSP | FWSC_RSP_NACK);
	pkt->hdr.len = cpu_to_be16(mgmt_pkt_expected_len(pkt->hdr.code));
}

static inline void fill_unplug_req(struct fwserial_mgmt_pkt *pkt)
{
	pkt->hdr.code = cpu_to_be16(FWSC_VIRT_CABLE_UNPLUG);
	pkt->hdr.len = cpu_to_be16(mgmt_pkt_expected_len(pkt->hdr.code));
}

static inline void fill_unplug_rsp_nack(struct fwserial_mgmt_pkt *pkt)
{
	pkt->hdr.code = cpu_to_be16(FWSC_VIRT_CABLE_UNPLUG_RSP | FWSC_RSP_NACK);
	pkt->hdr.len = cpu_to_be16(mgmt_pkt_expected_len(pkt->hdr.code));
}

static inline void fill_unplug_rsp_ok(struct fwserial_mgmt_pkt *pkt)
{
	pkt->hdr.code = cpu_to_be16(FWSC_VIRT_CABLE_UNPLUG_RSP);
	pkt->hdr.len = cpu_to_be16(mgmt_pkt_expected_len(pkt->hdr.code));
}

static void fwserial_virt_plug_complete(struct fwtty_peer *peer,
					struct virt_plug_params *params)
{
	struct fwtty_port *port = peer->port;

	peer->status_addr = be32_to_u64(params->status_hi, params->status_lo);
	peer->fifo_addr = be32_to_u64(params->fifo_hi, params->fifo_lo);
	peer->fifo_len = be32_to_cpu(params->fifo_len);
	peer_set_state(peer, FWPS_ATTACHED);

	/* reconfigure tx_fifo optimally for this peer
	   이 peer에 대해 tx_fifo를 최적으로 재구성하십시오. */
	spin_lock_bh(&port->lock);
	port->max_payload = min(peer->max_payload, peer->fifo_len);
	dma_fifo_change_tx_limit(&port->tx_fifo, port->max_payload);
	spin_unlock_bh(&peer->port->lock);

	if (port->port.console && port->fwcon_ops->notify != NULL)
		(*port->fwcon_ops->notify)(FWCON_NOTIFY_ATTACH, port->con_data);

	fwtty_info(&peer->unit, "peer (guid:%016llx) connected on %s\n",
		   (unsigned long long)peer->guid, dev_name(port->device));
}

static inline int fwserial_send_mgmt_sync(struct fwtty_peer *peer,
					  struct fwserial_mgmt_pkt *pkt)
{
	int generation;
	int rcode, tries = 5;

	do {
		generation = peer->generation;
		smp_rmb();

		rcode = fw_run_transaction(peer->serial->card,
					   TCODE_WRITE_BLOCK_REQUEST,
					   peer->node_id,
					   generation, peer->speed,
					   peer->mgmt_addr,
					   pkt, be16_to_cpu(pkt->hdr.len));
		if (rcode == RCODE_BUSY || rcode == RCODE_SEND_ERROR ||
		    rcode == RCODE_GENERATION) {
			fwtty_dbg(&peer->unit, "mgmt write error: %d\n", rcode);
			continue;
		} else {
			break;
		}
	} while (--tries > 0);
	return rcode;
}

/**
 * fwserial_claim_port - attempt to claim port @ index for peer
 *
 * Returns ptr to claimed port or error code (as ERR_PTR())
 * Can sleep - must be called from process context
   fwserial_claim_port - peer에 대해 port @ index를 청구하려고 시도 함. 
   청구 된 포트 또는 오류 코드 (ERR_PTR ()과 같은)로 ptr을 반환합니다. 
   sleep 가능 - 프로세스 컨텍스트에서 호출해야 함
 */
static struct fwtty_port *fwserial_claim_port(struct fwtty_peer *peer,
					      int index)
{
	struct fwtty_port *port;

	if (index < 0 || index >= num_ports)
		return ERR_PTR(-EINVAL);

	/* must guarantee that previous port releases have completed
	   이전 포트 릴리스가 완료되었음을 보장해야합니다. */
	synchronize_rcu();

	port = peer->serial->ports[index];
	spin_lock_bh(&port->lock);
	if (!rcu_access_pointer(port->peer))
		rcu_assign_pointer(port->peer, peer);
	else
		port = ERR_PTR(-EBUSY);
	spin_unlock_bh(&port->lock);

	return port;
}

/**
 * fwserial_find_port - find avail port and claim for peer
 *
 * Returns ptr to claimed port or NULL if none avail
 * Can sleep - must be called from process context
   fwserial_find_port - 피어에 대한 avail 포트 및 클레임을 찾습니다.
   요청 된 포트에 ptr을 반환하거나 사용하지 않을 경우 NULL을 반환합니다.
   잠자기 가능 - 프로세스 컨텍스트에서 호출해야 함
 */
static struct fwtty_port *fwserial_find_port(struct fwtty_peer *peer)
{
	struct fwtty_port **ports = peer->serial->ports;
	int i;

	/* must guarantee that previous port releases have completed 
	   이전 포트 릴리스가 완료되었음을 보증해야합니다.  */
	synchronize_rcu();

	/* TODO: implement optional GUID-to-specific port # matching 
  	   TODO : 선택적 GUID와 특정 포트 일치 # 구현 */

	/* find an unattached port (but not the loopback port, if present)
	   연결되지 않은 포트를 찾습니다 (루프백 포트가있는 경우 해당 포트가없는 경우).  */
	for (i = 0; i < num_ttys; ++i) {
		spin_lock_bh(&ports[i]->lock);
		if (!ports[i]->peer) {
			/* claim port */
			rcu_assign_pointer(ports[i]->peer, peer);
			spin_unlock_bh(&ports[i]->lock);
			return ports[i];
		}
		spin_unlock_bh(&ports[i]->lock);
	}
	return NULL;
}

static void fwserial_release_port(struct fwtty_port *port, bool reset)
{
	/* drop carrier (and all other line status)
	   드롭 캐리어 (및 기타 모든 회선 상태) */
	if (reset)
		fwtty_update_port_status(port, 0);

	spin_lock_bh(&port->lock);

	/* reset dma fifo max transmission size back to S100
	   dma fifo 최대 전송 크기를 다시 S100으로 재설정하십시오. */
	port->max_payload = link_speed_to_max_payload(SCODE_100);
	dma_fifo_change_tx_limit(&port->tx_fifo, port->max_payload);

	RCU_INIT_POINTER(port->peer, NULL);
	spin_unlock_bh(&port->lock);

	if (port->port.console && port->fwcon_ops->notify != NULL)
		(*port->fwcon_ops->notify)(FWCON_NOTIFY_DETACH, port->con_data);
}

static void fwserial_plug_timeout(unsigned long data)
{
	struct fwtty_peer *peer = (struct fwtty_peer *)data;
	struct fwtty_port *port;

	spin_lock_bh(&peer->lock);
	if (peer->state != FWPS_PLUG_PENDING) {
		spin_unlock_bh(&peer->lock);
		return;
	}

	port = peer_revert_state(peer);
	spin_unlock_bh(&peer->lock);

	if (port)
		fwserial_release_port(port, false);
}

/**
 * fwserial_connect_peer - initiate virtual cable with peer
 *
 * Returns 0 if VIRT_CABLE_PLUG request was successfully sent,
 * otherwise error code.  Must be called from process context.
   fwserial_connect_peer - 피어와 함께 가상 케이블 시작
  
   VIRT_CABLE_PLUG 요청이 성공적으로 전송 된 경우 0을 반환하고, 그렇지 않으면 오류 코드를 반환합니다. 
   프로세스 컨텍스트에서 호출되어야합니다.
 */
static int fwserial_connect_peer(struct fwtty_peer *peer)
{
	struct fwtty_port *port;
	struct fwserial_mgmt_pkt *pkt;
	int err, rcode;

	pkt = kmalloc(sizeof(*pkt), GFP_KERNEL);
	if (!pkt)
		return -ENOMEM;

	port = fwserial_find_port(peer);
	if (!port) {
		fwtty_err(&peer->unit, "avail ports in use\n");
		err = -EBUSY;
		goto free_pkt;
	}

	spin_lock_bh(&peer->lock);

	/* only initiate VIRT_CABLE_PLUG if peer is currently not attached 
	   피어가 현재 연결되어 있지 않으면 VIRT_CABLE_PLUG 만 시작합니다.*/
	if (peer->state != FWPS_NOT_ATTACHED) {
		err = -EBUSY;
		goto release_port;
	}

	peer->port = port;
	peer_set_state(peer, FWPS_PLUG_PENDING);

	fill_plug_req(pkt, peer->port);

	setup_timer(&peer->timer, fwserial_plug_timeout, (unsigned long)peer);
	mod_timer(&peer->timer, jiffies + VIRT_CABLE_PLUG_TIMEOUT);
	spin_unlock_bh(&peer->lock);

	rcode = fwserial_send_mgmt_sync(peer, pkt);

	spin_lock_bh(&peer->lock);
	if (peer->state == FWPS_PLUG_PENDING && rcode != RCODE_COMPLETE) {
		if (rcode == RCODE_CONFLICT_ERROR)
			err = -EAGAIN;
		else
			err = -EIO;
		goto cancel_timer;
	}
	spin_unlock_bh(&peer->lock);

	kfree(pkt);
	return 0;

cancel_timer:
	del_timer(&peer->timer);
	peer_revert_state(peer);
release_port:
	spin_unlock_bh(&peer->lock);
	fwserial_release_port(port, false);
free_pkt:
	kfree(pkt);
	return err;
}

/**
 * fwserial_close_port -
 * HUP the tty (if the tty exists) and unregister the tty device.
 * Only used by the unit driver upon unit removal to disconnect and
 * cleanup all attached ports
 *
 * The port reference is put by fwtty_cleanup (if a reference was
 * ever taken).
   fwserial_close_port -
   HUP는 tty (tty가있는 경우) 및 tty 장치의 등록을 취소합니다.
   장치를 제거 할 때 장치 드라이버가 사용하여 연결된 모든 포트의 연결을 끊고 정리할 수 있습니다 
   포트 참조는 fwtty_cleanup에 의해 참조됩니다 (참조가 수행 된 경우).
 */
static void fwserial_close_port(struct tty_driver *driver,
				struct fwtty_port *port)
{
	struct tty_struct *tty;

	mutex_lock(&port->port.mutex);
	tty = tty_port_tty_get(&port->port);
	if (tty) {
		tty_vhangup(tty);
		tty_kref_put(tty);
	}
	mutex_unlock(&port->port.mutex);

	if (driver == fwloop_driver)
		tty_unregister_device(driver, loop_idx(port));
	else
		tty_unregister_device(driver, port->index);
}

/**
 * fwserial_lookup - finds first fw_serial associated with card
 * @card: fw_card to match
 *
 * NB: caller must be holding fwserial_list_mutex
   fwserial_lookup - 카드와 관련된 첫 번째 fw_serial을 찾습니다. 
   @card : 일치시킬 fw_card
  
   주의 : 호출자는 fwserial_list_mutex를 보유해야합니다.
 */
static struct fw_serial *fwserial_lookup(struct fw_card *card)
{
	struct fw_serial *serial;

	list_for_each_entry(serial, &fwserial_list, list) {
		if (card == serial->card)
			return serial;
	}

	return NULL;
}

/**
 * __fwserial_lookup_rcu - finds first fw_serial associated with card
 * @card: fw_card to match
 *
 * NB: caller must be inside rcu_read_lock() section
   __fwserial_lookup_rcu - 카드와 관련된 첫 번째 fw_serial을 찾습니다.
   @card : 일치시킬 fw_card
  
   주의 : 호출자는 rcu_read_lock () 섹션 안에 있어야합니다.
 */
static struct fw_serial *__fwserial_lookup_rcu(struct fw_card *card)
{
	struct fw_serial *serial;

	list_for_each_entry_rcu(serial, &fwserial_list, list) {
		if (card == serial->card)
			return serial;
	}

	return NULL;
}

/**
 * __fwserial_peer_by_node_id - finds a peer matching the given generation + id
 *
 * If a matching peer could not be found for the specified generation/node id,
 * this could be because:
 * a) the generation has changed and one of the nodes hasn't updated yet
 * b) the remote node has created its remote unit device before this
 *    local node has created its corresponding remote unit device
 * In either case, the remote node should retry
 *
 * Note: caller must be in rcu_read_lock() section
  __fwserial_peer_by_node_id - 주어진 생성 + id와 일치하는 피어를 찾습니다.
 
  일치하는 피어가 지정된 생성 / 노드 ID에 대해 찾을 수없는 경우 다음이 원인 일 수 있습니다.
   a) 생성이 변경되었고 노드 중 하나가 아직 업데이트되지 않았습니다.
   b)이 로컬 노드가 해당 원격 장치를 생성하기 전에 원격 장치가 원격 장치를 생성했습니다. 
     두 경우 모두 원격 노드가 재 시도해야합니다
  
  참고 : 호출자는 rcu_read_lock () 섹션에 있어야합니다.
 */
static struct fwtty_peer *__fwserial_peer_by_node_id(struct fw_card *card,
						     int generation, int id)
{
	struct fw_serial *serial;
	struct fwtty_peer *peer;

	serial = __fwserial_lookup_rcu(card);
	if (!serial) {
		/*
		 * Something is very wrong - there should be a matching
		 * fw_serial structure for every fw_card. Maybe the remote node
		 * has created its remote unit device before this driver has
		 * been probed for any unit devices...
		   뭔가 잘못되었습니다 - 모든 fw_card에 대해 일치하는 fw_serial 구조가 있어야합니다.
		   이 드라이버가 모든 장치 장치에 대해 검색되기 전에 원격 노드가 원격 장치 장치를 생성했을 수 있습니다.  */
		fwtty_err(card, "unknown card (guid %016llx)\n",
			  (unsigned long long) card->guid);
		return NULL;
	}

	list_for_each_entry_rcu(peer, &serial->peer_list, list) {
		int g = peer->generation;

		smp_rmb();
		if (generation == g && id == peer->node_id)
			return peer;
	}

	return NULL;
}

#ifdef DEBUG
static void __dump_peer_list(struct fw_card *card)
{
	struct fw_serial *serial;
	struct fwtty_peer *peer;

	serial = __fwserial_lookup_rcu(card);
	if (!serial)
		return;

	list_for_each_entry_rcu(peer, &serial->peer_list, list) {
		int g = peer->generation;

		smp_rmb();
		fwtty_dbg(card, "peer(%d:%x) guid: %016llx\n",
			  g, peer->node_id, (unsigned long long) peer->guid);
	}
}
#else
#define __dump_peer_list(s)
#endif

static void fwserial_auto_connect(struct work_struct *work)
{
	struct fwtty_peer *peer = to_peer(to_delayed_work(work), connect);
	int err;

	err = fwserial_connect_peer(peer);
	if (err == -EAGAIN && ++peer->connect_retries < MAX_CONNECT_RETRIES)
		schedule_delayed_work(&peer->connect, CONNECT_RETRY_DELAY);
}

static void fwserial_peer_workfn(struct work_struct *work)
{
	struct fwtty_peer *peer = to_peer(work, work);

	peer->workfn(work);
}

/**
 * fwserial_add_peer - add a newly probed 'serial' unit device as a 'peer'
 * @serial: aggregate representing the specific fw_card to add the peer to
 * @unit: 'peer' to create and add to peer_list of serial
 *
 * Adds a 'peer' (ie, a local or remote 'serial' unit device) to the list of
 * peers for a specific fw_card. Optionally, auto-attach this peer to an
 * available tty port. This function is called either directly or indirectly
 * as a result of a 'serial' unit device being created & probed.
 *
 * Note: this function is serialized with fwserial_remove_peer() by the
 * fwserial_list_mutex held in fwserial_probe().
 *
 * A 1:1 correspondence between an fw_unit and an fwtty_peer is maintained
 * via the dev_set_drvdata() for the device of the fw_unit.

   fwserial_add_peer - 새로 조사 된 'serial'단위 장치를 'peer'로 추가합니다. 
   @serial : 특정 fw_card를 나타내는 집합체를 @unit에 추가합니다. 'peer'는 serial의 peer_list를 만들고 추가합니다.
  
   특정 fw_card에 대해 피어 목록 (즉, 로컬 또는 원격 '직렬'장치)을 피어 목록에 추가합니다. 
   선택적으로이 피어를 사용 가능한 tty 포트에 자동 연결하십시오. 
   이 함수는 '직렬'단위 장치가 생성되고 검사 된 결과로 직접 또는 간접적으로 호출됩니다.
  
   참고 :이 함수는 fwserial_probe ()에있는 fwserial_list_mutex에 의해 fwserial_remove_peer ()와 직렬화됩니다.
  
   fw_unit과 fwtty_peer 간의 1 : 1 대응은 fw_unit의 장치에 대한 dev_set_drvdata ()를 통해 유지됩니다.
 */
static int fwserial_add_peer(struct fw_serial *serial, struct fw_unit *unit)
{
	struct device *dev = &unit->device;
	struct fw_device  *parent = fw_parent_device(unit);
	struct fwtty_peer *peer;
	struct fw_csr_iterator ci;
	int key, val;
	int generation;

	peer = kzalloc(sizeof(*peer), GFP_KERNEL);
	if (!peer)
		return -ENOMEM;

	peer_set_state(peer, FWPS_NOT_ATTACHED);

	dev_set_drvdata(dev, peer);
	peer->unit = unit;
	peer->guid = (u64)parent->config_rom[3] << 32 | parent->config_rom[4];
	peer->speed = parent->max_speed;
	peer->max_payload = min(device_max_receive(parent),
				link_speed_to_max_payload(peer->speed));

	generation = parent->generation;
	smp_rmb();
	peer->node_id = parent->node_id;
	smp_wmb();
	peer->generation = generation;

	/* retrieve the mgmt bus addr from the unit directory 
	   단위 디렉토리에서 mgmt bus addr을 검색한다. */
	fw_csr_iterator_init(&ci, unit->directory);
	while (fw_csr_iterator_next(&ci, &key, &val)) {
		if (key == (CSR_OFFSET | CSR_DEPENDENT_INFO)) {
			peer->mgmt_addr = CSR_REGISTER_BASE + 4 * val;
			break;
		}
	}
	if (peer->mgmt_addr == 0ULL) {
		/*
		 * No mgmt address effectively disables VIRT_CABLE_PLUG -
		 * this peer will not be able to attach to a remote
	    	   어떤 mgmt 주소도 VIRT_CABLE_PLUG를 효과적으로 비활성화하지 않습니다 -
		   이 피어는 리모컨에 연결할 수 없습니다.	
		 */
		peer_set_state(peer, FWPS_NO_MGMT_ADDR);
	}

	spin_lock_init(&peer->lock);
	peer->port = NULL;

	init_timer(&peer->timer);
	INIT_WORK(&peer->work, fwserial_peer_workfn);
	INIT_DELAYED_WORK(&peer->connect, fwserial_auto_connect);

	/* associate peer with specific fw_card 
	   특정 fw_card와 피어를 연결 */
	peer->serial = serial;
	list_add_rcu(&peer->list, &serial->peer_list);

	fwtty_info(&peer->unit, "peer added (guid:%016llx)\n",
		   (unsigned long long)peer->guid);

	/* identify the local unit & virt cable to loopback port 
	   로컬 장치 및 루프백 포트에 대한 virt 케이블 식별  */
	if (parent->is_local) {
		serial->self = peer;
		if (create_loop_dev) {
			struct fwtty_port *port;

			port = fwserial_claim_port(peer, num_ttys);
			if (!IS_ERR(port)) {
				struct virt_plug_params params;

				spin_lock_bh(&peer->lock);
				peer->port = port;
				fill_plug_params(&params, port);
				fwserial_virt_plug_complete(peer, &params);
				spin_unlock_bh(&peer->lock);

				fwtty_write_port_status(port);
			}
		}

	} else if (auto_connect) {
		/* auto-attach to remote units only (if policy allows)
	    	   원격 장치에만 자동 연결 (정책이 허용하는 경우) */
		schedule_delayed_work(&peer->connect, 1);
	}

	return 0;
}

/**
 * fwserial_remove_peer - remove a 'serial' unit device as a 'peer'
 *
 * Remove a 'peer' from its list of peers. This function is only
 * called by fwserial_remove() on bus removal of the unit device.
 *
 * Note: this function is serialized with fwserial_add_peer() by the
 * fwserial_list_mutex held in fwserial_remove().
   fwserial_remove_peer - '직렬'단위 장치를 'peer'로 제거합니다.
  
   동료 목록에서 '동료'를 삭제하십시오. 이 함수는 유닛 장치의 버스 제거시 fwserial_remove ()에 의해서만 호출됩니다.
  
   참고 :이 함수는 fwserial_remove ()에있는 fwserial_list_mutex에 의해 fwserial_add_peer ()로 직렬화됩니다.
 */
static void fwserial_remove_peer(struct fwtty_peer *peer)
{
	struct fwtty_port *port;

	spin_lock_bh(&peer->lock);
	peer_set_state(peer, FWPS_GONE);
	spin_unlock_bh(&peer->lock);

	cancel_delayed_work_sync(&peer->connect);
	cancel_work_sync(&peer->work);

	spin_lock_bh(&peer->lock);
	/* if this unit is the local unit, clear link 
	   이 장치가 로컬 장치 인 경우 링크를 지우십시오  */
	if (peer == peer->serial->self)
		peer->serial->self = NULL;

	/* cancel the request timeout timer (if running)
	   요청 시간 초과 타이머를 취소합니다 (실행중인 경우) */
	del_timer(&peer->timer);

	port = peer->port;
	peer->port = NULL;

	list_del_rcu(&peer->list);

	fwtty_info(&peer->unit, "peer removed (guid:%016llx)\n",
		   (unsigned long long)peer->guid);

	spin_unlock_bh(&peer->lock);

	if (port)
		fwserial_release_port(port, true);

	synchronize_rcu();
	kfree(peer);
}

/**
 * fwserial_create - init everything to create TTYs for a specific fw_card
 * @unit: fw_unit for first 'serial' unit device probed for this fw_card
 *
 * This function inits the aggregate structure (an fw_serial instance)
 * used to manage the TTY ports registered by a specific fw_card. Also, the
 * unit device is added as the first 'peer'.
 *
 * This unit device may represent a local unit device (as specified by the
 * config ROM unit directory) or it may represent a remote unit device
 * (as specified by the reading of the remote node's config ROM).
 *
 * Returns 0 to indicate "ownership" of the unit device, or a negative errno
 * value to indicate which error.
   fwserial_create - 특정 fw_card에 대해 TTY를 생성하기위한 모든 것을 초기화하십시오. 
   @unit :이 fw_card에 대해 탐색 된 첫 번째 '직렬'단위 장치에 대한 fw_unit
   이 함수는 특정 fw_card에 의해 등록 된 TTY 포트를 관리하는 데 사용되는 집계 구조 (fw_serial 인스턴스)를 나타냅니다. 
   또한 단위 장치가 첫 번째 '피어'로 추가됩니다.
  
  이 장치 장치는 로컬 장치 장치 (config ROM 장치 디렉토리에 지정된대로)를 나타낼 수도 있고 원격 장치 장치를 나타낼 수도 있습니다 (원격 노드의 구성 ROM 읽기로 지정).
  
   단위 장치의 "소유권"을 나타내려면 0을 반환하고 어떤 오류를 나타내려면 음수의 errno 값을 반환합니다.
 */
static int fwserial_create(struct fw_unit *unit)
{
	struct fw_device *parent = fw_parent_device(unit);
	struct fw_card *card = parent->card;
	struct fw_serial *serial;
	struct fwtty_port *port;
	struct device *tty_dev;
	int i, j;
	int err;

	serial = kzalloc(sizeof(*serial), GFP_KERNEL);
	if (!serial)
		return -ENOMEM;

	kref_init(&serial->kref);
	serial->card = card;
	INIT_LIST_HEAD(&serial->peer_list);

	for (i = 0; i < num_ports; ++i) {
		port = kzalloc(sizeof(*port), GFP_KERNEL);
		if (!port) {
			err = -ENOMEM;
			goto free_ports;
		}
		tty_port_init(&port->port);
		port->index = FWTTY_INVALID_INDEX;
		port->port.ops = &fwtty_port_ops;
		port->serial = serial;
		tty_buffer_set_limit(&port->port, 128 * 1024);

		spin_lock_init(&port->lock);
		INIT_DELAYED_WORK(&port->drain, fwtty_drain_tx);
		INIT_DELAYED_WORK(&port->emit_breaks, fwtty_emit_breaks);
		INIT_WORK(&port->hangup, fwtty_do_hangup);
		init_waitqueue_head(&port->wait_tx);
		port->max_payload = link_speed_to_max_payload(SCODE_100);
		dma_fifo_init(&port->tx_fifo);

		RCU_INIT_POINTER(port->peer, NULL);
		serial->ports[i] = port;

		/* get unique bus addr region for port's status & recv fifo 
		   포트 상태 & recv fifo를위한 유일한 버스 addr 지역을 얻는다  */
		port->rx_handler.length = FWTTY_PORT_RXFIFO_LEN + 4;
		port->rx_handler.address_callback = fwtty_port_handler;
		port->rx_handler.callback_data = port;
		/*
		 * XXX: use custom memory region above cpu physical memory addrs
		 * this will ease porting to 64-bit firewire adapters
		   cpu 실제 메모리 addrs 위의 사용자 정의 메모리 영역 사용
  		   이렇게하면 64 비트 Firewire 어댑터로 쉽게 이식 할 수 있습니다.
		 */
		err = fw_core_add_address_handler(&port->rx_handler,
						  &fw_high_memory_region);
		if (err) {
			kfree(port);
			goto free_ports;
		}
	}
	/* preserve i for error cleanup
	   오류 정리를 위해 i를 보존  	 */

	err = fwtty_ports_add(serial);
	if (err) {
		fwtty_err(&unit, "no space in port table\n");
		goto free_ports;
	}

	for (j = 0; j < num_ttys; ++j) {
		tty_dev = tty_port_register_device(&serial->ports[j]->port,
						   fwtty_driver,
						   serial->ports[j]->index,
						   card->device);
		if (IS_ERR(tty_dev)) {
			err = PTR_ERR(tty_dev);
			fwtty_err(&unit, "register tty device error (%d)\n",
				  err);
			goto unregister_ttys;
		}

		serial->ports[j]->device = tty_dev;
	}
	/* preserve j for error cleanup 
	   오류 정리를 위해 j를 보존	 */

	if (create_loop_dev) {
		struct device *loop_dev;

		loop_dev = tty_port_register_device(&serial->ports[j]->port,
						    fwloop_driver,
						    loop_idx(serial->ports[j]),
						    card->device);
		if (IS_ERR(loop_dev)) {
			err = PTR_ERR(loop_dev);
			fwtty_err(&unit, "create loop device failed (%d)\n",
				  err);
			goto unregister_ttys;
		}
		serial->ports[j]->device = loop_dev;
		serial->ports[j]->loopback = true;
	}

	if (!IS_ERR_OR_NULL(fwserial_debugfs)) {
		serial->debugfs = debugfs_create_dir(dev_name(&unit->device),
						     fwserial_debugfs);
		if (!IS_ERR_OR_NULL(serial->debugfs)) {
			debugfs_create_file("peers", 0444, serial->debugfs,
					    serial, &fwtty_peers_fops);
			debugfs_create_file("stats", 0444, serial->debugfs,
					    serial, &fwtty_stats_fops);
		}
	}

	list_add_rcu(&serial->list, &fwserial_list);

	fwtty_notice(&unit, "TTY over FireWire on device %s (guid %016llx)\n",
		     dev_name(card->device), (unsigned long long) card->guid);

	err = fwserial_add_peer(serial, unit);
	if (!err)
		return 0;

	fwtty_err(&unit, "unable to add peer unit device (%d)\n", err);

	/* fall-through to error processing 
	   오류 처리로의 전환			*/
	debugfs_remove_recursive(serial->debugfs);

	list_del_rcu(&serial->list);
	if (create_loop_dev)
		tty_unregister_device(fwloop_driver,
				      loop_idx(serial->ports[j]));
unregister_ttys:
	for (--j; j >= 0; --j)
		tty_unregister_device(fwtty_driver, serial->ports[j]->index);
	kref_put(&serial->kref, fwserial_destroy);
	return err;

free_ports:
	for (--i; i >= 0; --i) {
		tty_port_destroy(&serial->ports[i]->port);
		kfree(serial->ports[i]);
	}
	kfree(serial);
	return err;
}

/**
 * fwserial_probe: bus probe function for firewire 'serial' unit devices
 *
 * A 'serial' unit device is created and probed as a result of:
 * - declaring a ieee1394 bus id table for 'devices' matching a fabricated
 *   'serial' unit specifier id
 * - adding a unit directory to the config ROM(s) for a 'serial' unit
 *
 * The firewire core registers unit devices by enumerating unit directories
 * of a node's config ROM after reading the config ROM when a new node is
 * added to the bus topology after a bus reset.
 *
 * The practical implications of this are:
 * - this probe is called for both local and remote nodes that have a 'serial'
 *   unit directory in their config ROM (that matches the specifiers in
 *   fwserial_id_table).
 * - no specific order is enforced for local vs. remote unit devices
 *
 * This unit driver copes with the lack of specific order in the same way the
 * firewire net driver does -- each probe, for either a local or remote unit
 * device, is treated as a 'peer' (has a struct fwtty_peer instance) and the
 * first peer created for a given fw_card (tracked by the global fwserial_list)
 * creates the underlying TTYs (aggregated in a fw_serial instance).
 *
 * NB: an early attempt to differentiate local & remote unit devices by creating
 *     peers only for remote units and fw_serial instances (with their
 *     associated TTY devices) only for local units was discarded. Managing
 *     the peer lifetimes on device removal proved too complicated.
 *
 * fwserial_probe/fwserial_remove are effectively serialized by the
 * fwserial_list_mutex. This is necessary because the addition of the first peer
 * for a given fw_card will trigger the creation of the fw_serial for that
 * fw_card, which must not simultaneously contend with the removal of the
 * last peer for a given fw_card triggering the destruction of the same
 * fw_serial for the same fw_card.

   fwserial_probe : firewire 'serial'단위 장치에 대한 버스 프로브 기능 'serial'단위 장치는 다음과 같은 결과로 생성되고 검사됩니다.
  - 가공 된 'serial'단위 지정자 ID와 일치하는 'devices'에 대한 ieee1394 버스 ID 테이블 선언
  - '직렬'장치에 대한 구성 ROM에 단위 디렉토리 추가
 
   FireWire 코어는 버스 리셋 후에 새로운 노드가 버스 토폴로지에 추가 될 때 구성 ROM을 읽은 후 노드의 구성 ROM의 단위 디렉토리를 열거하여 장치를 장치 단위로 등록합니다.
 
  실용적인 의미는 다음과 같습니다.
  -이 프로브는 구성 ROM에 'serial'단위 디렉토리가있는 로컬 및 원격 노드 모두에 대해 호출됩니다 (fwserial_id_table의 지정자와 일치 함).
  - 로컬 장치와 원격 장치 장치에 특정한 순서가 적용되지 않습니다.
 
  이 유닛 드라이버는 firewire net 드라이버가하는 것과 같은 방식으로 특정 순서의 부족을 해결합니다. 로컬 또는 원격 유닛 장치에 대한 각 프로브는 '피어'(fwtty_peer 인스턴스가있는 구조체)와 주어진 fw_card (글로벌 fwserial_list에 의해 추적 됨)에 대해 생성 된 첫 번째 피어는 기본 TTY를 생성합니다 (fw_serial 인스턴스에서 집계 됨).
 
  NB : 로컬 유닛에 대해서만 원격 유닛과 fw_serial 인스턴스 (관련 TTY 디바이스와 함께)에 대해 피어를 작성하여 로컬 및 원격 유닛 장치를 차별화하려는 초기 시도가 삭제되었습니다. 장치 제거시 피어 수명을 관리하는 것은 너무 복잡했습니다.
 
  fwserial_probe / fwserial_remove는 fwserial_list_mutex에 의해 효과적으로 직렬화됩니다. 주어진 fw_card에 대한 첫 번째 피어의 추가가 해당 fw_card에 대한 fw_serial의 생성을 트리거하기 때문에 필요합니다. fw_card에 대한 동일한 fw_serial의 삭제를 트리거하는 주어진 fw_card에 대한 마지막 피어의 제거와 동시에 경쟁해서는 안됩니다. 같은 fw_card.
 */
static int fwserial_probe(struct fw_unit *unit,
			  const struct ieee1394_device_id *id)
{
	struct fw_serial *serial;
	int err;

	mutex_lock(&fwserial_list_mutex);
	serial = fwserial_lookup(fw_parent_device(unit)->card);
	if (!serial)
		err = fwserial_create(unit);
	else
		err = fwserial_add_peer(serial, unit);
	mutex_unlock(&fwserial_list_mutex);
	return err;
}

/**
 * fwserial_remove: bus removal function for firewire 'serial' unit devices
 *
 * The corresponding 'peer' for this unit device is removed from the list of
 * peers for the associated fw_serial (which has a 1:1 correspondence with a
 * specific fw_card). If this is the last peer being removed, then trigger
 * the destruction of the underlying TTYs.
   fwserial_remove : firewire 'serial'단위 장치에 대한 버스 제거 기능
   이 장치 장치에 해당하는 '피어'는 연관된 fw_serial (특정 fw_card와 1 : 1 대응)에 대한 피어 목록에서 제거됩니다. 
   이것이 제거 된 마지막 피어라면 TTY의 파괴를 유발하십시오.
 
   */
static void fwserial_remove(struct fw_unit *unit)
{
	struct fwtty_peer *peer = dev_get_drvdata(&unit->device);
	struct fw_serial *serial = peer->serial;
	int i;

	mutex_lock(&fwserial_list_mutex);
	fwserial_remove_peer(peer);

	if (list_empty(&serial->peer_list)) {
		/* unlink from the fwserial_list here */
		list_del_rcu(&serial->list);

		debugfs_remove_recursive(serial->debugfs);

		for (i = 0; i < num_ttys; ++i)
			fwserial_close_port(fwtty_driver, serial->ports[i]);
		if (create_loop_dev)
			fwserial_close_port(fwloop_driver, serial->ports[i]);
		kref_put(&serial->kref, fwserial_destroy);
	}
	mutex_unlock(&fwserial_list_mutex);
}

/**
 * fwserial_update: bus update function for 'firewire' serial unit devices
 *
 * Updates the new node_id and bus generation for this peer. Note that locking
 * is unnecessary; but careful memory barrier usage is important to enforce the
 * load and store order of generation & node_id.
 *
 * The fw-core orders the write of node_id before generation in the parent
 * fw_device to ensure that a stale node_id cannot be used with a current
 * bus generation. So the generation value must be read before the node_id.
 *
 * In turn, this orders the write of node_id before generation in the peer to
 * also ensure a stale node_id cannot be used with a current bus generation.
  
   fwserial_update : 'firewire'시리얼 장치의 버스 업데이트 기능
  
  이 피어에 대한 새로운 node_id 및 버스 생성을 갱신합니다. 잠금은 불필요합니다. 그러나 메모리 barrier를 세 심하게 사용하면 generation & node_id의로드 및 저장 순서를 강제하는 것이 중요합니다.
  
   fw-core는 부모 fw_device에서 생성되기 전에 node_id의 쓰기를 명령하여 부실한 node_id를 현재 버스 생성과 함께 사용할 수 없도록합니다. 따라서 생성 값은 node_id보다 먼저 읽어야합니다.
  
   차례로 이것은 피어에서 생성되기 전에 node_id의 쓰기를 명령하여 부실한 node_id가 현재 버스 생성과 함께 사용될 수 없도록합니다.
 */
static void fwserial_update(struct fw_unit *unit)
{
	struct fw_device *parent = fw_parent_device(unit);
	struct fwtty_peer *peer = dev_get_drvdata(&unit->device);
	int generation;

	generation = parent->generation;
	smp_rmb();
	peer->node_id = parent->node_id;
	smp_wmb();
	peer->generation = generation;
}

static const struct ieee1394_device_id fwserial_id_table[] = {
	{
		.match_flags  = IEEE1394_MATCH_SPECIFIER_ID |
				IEEE1394_MATCH_VERSION,
		.specifier_id = LINUX_VENDOR_ID,
		.version      = FWSERIAL_VERSION,
	},
	{ }
};

static struct fw_driver fwserial_driver = {
	.driver = {
		.owner  = THIS_MODULE,
		.name   = KBUILD_MODNAME,
		.bus    = &fw_bus_type,
	},
	.probe    = fwserial_probe,
	.update   = fwserial_update,
	.remove   = fwserial_remove,
	.id_table = fwserial_id_table,
};

#define FW_UNIT_SPECIFIER(id)	((CSR_SPECIFIER_ID << 24) | (id))
#define FW_UNIT_VERSION(ver)	((CSR_VERSION << 24) | (ver))
#define FW_UNIT_ADDRESS(ofs)	(((CSR_OFFSET | CSR_DEPENDENT_INFO) << 24)  \
				 | (((ofs) - CSR_REGISTER_BASE) >> 2))
/* XXX: config ROM definitons could be improved with semi-automated offset
 * and length calculation
   XXX: 구성 ROM 정의는 반자동 오프셋 및 길이 계산으로 향상 될 수 있습니다.

 */
#define FW_ROM_LEN(quads)	((quads) << 16)
#define FW_ROM_DESCRIPTOR(ofs)	(((CSR_LEAF | CSR_DESCRIPTOR) << 24) | (ofs))

struct fwserial_unit_directory_data {
	u32	len_crc;
	u32	unit_specifier;
	u32	unit_sw_version;
	u32	unit_addr_offset;
	u32	desc1_ofs;
	u32	desc1_len_crc;
	u32	desc1_data[5];
} __packed;

static struct fwserial_unit_directory_data fwserial_unit_directory_data = {
	.len_crc = FW_ROM_LEN(4),
	.unit_specifier = FW_UNIT_SPECIFIER(LINUX_VENDOR_ID),
	.unit_sw_version = FW_UNIT_VERSION(FWSERIAL_VERSION),
	.desc1_ofs = FW_ROM_DESCRIPTOR(1),
	.desc1_len_crc = FW_ROM_LEN(5),
	.desc1_data = {
		0x00000000,			/*   type = text            */
		0x00000000,			/*   enc = ASCII, lang EN   */
		0x4c696e75,			/* 'Linux TTY'              */
		0x78205454,
		0x59000000,
	},
};

static struct fw_descriptor fwserial_unit_directory = {
	.length = sizeof(fwserial_unit_directory_data) / sizeof(u32),
	.key    = (CSR_DIRECTORY | CSR_UNIT) << 24,
	.data   = (u32 *)&fwserial_unit_directory_data,
};

/*
 * The management address is in the unit space region but above other known
 * address users (to keep wild writes from causing havoc)
   관리 주소는 단위 공간 영역에 있지만 다른 알려진 주소 사용자보다 큽니다 (야생 쓰기가 혼란을 야기하지 않도록)
 */
static const struct fw_address_region fwserial_mgmt_addr_region = {
	.start = CSR_REGISTER_BASE + 0x1e0000ULL,
	.end = 0x1000000000000ULL,
};

static struct fw_address_handler fwserial_mgmt_addr_handler;

/**
 * fwserial_handle_plug_req - handle VIRT_CABLE_PLUG request work
 * @work: ptr to peer->work
 *
 * Attempts to complete the VIRT_CABLE_PLUG handshake sequence for this peer.
 *
 * This checks for a collided request-- ie, that a VIRT_CABLE_PLUG request was
 * already sent to this peer. If so, the collision is resolved by comparing
 * guid values; the loser sends the plug response.
 *
 * Note: if an error prevents a response, don't do anything -- the
 * remote will timeout its request.
  
  fwserial_handle_plug_req - VIRT_CABLE_PLUG 요청 작업 처리 @work : ptr to peer-> work
  
  이 피어에 대한 VIRT_CABLE_PLUG 핸드 셰이크 시퀀스를 완료하려고 시도합니다.
 
  충돌 한 요청 (즉, VIRT_CABLE_PLUG 요청이이 피어에 이미 전송 되었음)을 확인합니다. 그렇다면 충돌은 guid 값을 비교하여 해결됩니다. 패자는 플러그 응답을 보낸다.
 
  참고 : 오류로 인해 응답이 차단되면 아무 것도하지 마십시오. 리모컨이 요청을 시간 초과합니다.
 */
static void fwserial_handle_plug_req(struct work_struct *work)
{
	struct fwtty_peer *peer = to_peer(work, work);
	struct virt_plug_params *plug_req = &peer->work_params.plug_req;
	struct fwtty_port *port;
	struct fwserial_mgmt_pkt *pkt;
	int rcode;

	pkt = kmalloc(sizeof(*pkt), GFP_KERNEL);
	if (!pkt)
		return;

	port = fwserial_find_port(peer);

	spin_lock_bh(&peer->lock);

	switch (peer->state) {
	case FWPS_NOT_ATTACHED:
		if (!port) {
			fwtty_err(&peer->unit, "no more ports avail\n");
			fill_plug_rsp_nack(pkt);
		} else {
			peer->port = port;
			fill_plug_rsp_ok(pkt, peer->port);
			peer_set_state(peer, FWPS_PLUG_RESPONDING);
			/* don't release claimed port */
			port = NULL;
		}
		break;

	case FWPS_PLUG_PENDING:
		if (peer->serial->card->guid > peer->guid)
			goto cleanup;

		/* We lost - hijack the already-claimed port and send ok
			     이미 청구 된 포트를 납치하고 확인을 보냅니다. */
		del_timer(&peer->timer);
		fill_plug_rsp_ok(pkt, peer->port);
		peer_set_state(peer, FWPS_PLUG_RESPONDING);
		break;

	default:
		fill_plug_rsp_nack(pkt);
	}

	spin_unlock_bh(&peer->lock);
	if (port)
		fwserial_release_port(port, false);

	rcode = fwserial_send_mgmt_sync(peer, pkt);

	spin_lock_bh(&peer->lock);
	if (peer->state == FWPS_PLUG_RESPONDING) {
		if (rcode == RCODE_COMPLETE) {
			struct fwtty_port *tmp = peer->port;

			fwserial_virt_plug_complete(peer, plug_req);
			spin_unlock_bh(&peer->lock);

			fwtty_write_port_status(tmp);
			spin_lock_bh(&peer->lock);
		} else {
			fwtty_err(&peer->unit, "PLUG_RSP error (%d)\n", rcode);
			port = peer_revert_state(peer);
		}
	}
cleanup:
	spin_unlock_bh(&peer->lock);
	if (port)
		fwserial_release_port(port, false);
	kfree(pkt);
}

static void fwserial_handle_unplug_req(struct work_struct *work)
{
	struct fwtty_peer *peer = to_peer(work, work);
	struct fwtty_port *port = NULL;
	struct fwserial_mgmt_pkt *pkt;
	int rcode;

	pkt = kmalloc(sizeof(*pkt), GFP_KERNEL);
	if (!pkt)
		return;

	spin_lock_bh(&peer->lock);

	switch (peer->state) {
	case FWPS_ATTACHED:
		fill_unplug_rsp_ok(pkt);
		peer_set_state(peer, FWPS_UNPLUG_RESPONDING);
		break;

	case FWPS_UNPLUG_PENDING:
		if (peer->serial->card->guid > peer->guid)
			goto cleanup;

		/* We lost - send unplug rsp */
		del_timer(&peer->timer);
		fill_unplug_rsp_ok(pkt);
		peer_set_state(peer, FWPS_UNPLUG_RESPONDING);
		break;

	default:
		fill_unplug_rsp_nack(pkt);
	}

	spin_unlock_bh(&peer->lock);

	rcode = fwserial_send_mgmt_sync(peer, pkt);

	spin_lock_bh(&peer->lock);
	if (peer->state == FWPS_UNPLUG_RESPONDING) {
		if (rcode != RCODE_COMPLETE)
			fwtty_err(&peer->unit, "UNPLUG_RSP error (%d)\n",
				  rcode);
		port = peer_revert_state(peer);
	}
cleanup:
	spin_unlock_bh(&peer->lock);
	if (port)
		fwserial_release_port(port, true);
	kfree(pkt);
}

static int fwserial_parse_mgmt_write(struct fwtty_peer *peer,
				     struct fwserial_mgmt_pkt *pkt,
				     unsigned long long addr,
				     size_t len)
{
	struct fwtty_port *port = NULL;
	bool reset = false;
	int rcode;

	if (addr != fwserial_mgmt_addr_handler.offset || len < sizeof(pkt->hdr))
		return RCODE_ADDRESS_ERROR;

	if (len != be16_to_cpu(pkt->hdr.len) ||
	    len != mgmt_pkt_expected_len(pkt->hdr.code))
		return RCODE_DATA_ERROR;

	spin_lock_bh(&peer->lock);
	if (peer->state == FWPS_GONE) {
		/*
		 * This should never happen - it would mean that the
		 * remote unit that just wrote this transaction was
		 * already removed from the bus -- and the removal was
		 * processed before we rec'd this transaction
		   이것은 결코 일어나서는 안됩니다. 
		   즉,이 트랜잭션을 작성한 원격 장치가 이미 버스에서 제거되었음을 의미하며,
                   이 트랜잭션을 회수하기 전에 제거가 처리되었습니다
		 */
		fwtty_err(&peer->unit, "peer already removed\n");
		spin_unlock_bh(&peer->lock);
		return RCODE_ADDRESS_ERROR;
	}

	rcode = RCODE_COMPLETE;

	fwtty_dbg(&peer->unit, "mgmt: hdr.code: %04hx\n", pkt->hdr.code);

	switch (be16_to_cpu(pkt->hdr.code) & FWSC_CODE_MASK) {
	case FWSC_VIRT_CABLE_PLUG:
		if (work_pending(&peer->work)) {
			fwtty_err(&peer->unit, "plug req: busy\n");
			rcode = RCODE_CONFLICT_ERROR;

		} else {
			peer->work_params.plug_req = pkt->plug_req;
			peer->workfn = fwserial_handle_plug_req;
			queue_work(system_unbound_wq, &peer->work);
		}
		break;

	case FWSC_VIRT_CABLE_PLUG_RSP:
		if (peer->state != FWPS_PLUG_PENDING) {
			rcode = RCODE_CONFLICT_ERROR;

		} else if (be16_to_cpu(pkt->hdr.code) & FWSC_RSP_NACK) {
			fwtty_notice(&peer->unit, "NACK plug rsp\n");
			port = peer_revert_state(peer);

		} else {
			struct fwtty_port *tmp = peer->port;

			fwserial_virt_plug_complete(peer, &pkt->plug_rsp);
			spin_unlock_bh(&peer->lock);

			fwtty_write_port_status(tmp);
			spin_lock_bh(&peer->lock);
		}
		break;

	case FWSC_VIRT_CABLE_UNPLUG:
		if (work_pending(&peer->work)) {
			fwtty_err(&peer->unit, "unplug req: busy\n");
			rcode = RCODE_CONFLICT_ERROR;
		} else {
			peer->workfn = fwserial_handle_unplug_req;
			queue_work(system_unbound_wq, &peer->work);
		}
		break;

	case FWSC_VIRT_CABLE_UNPLUG_RSP:
		if (peer->state != FWPS_UNPLUG_PENDING) {
			rcode = RCODE_CONFLICT_ERROR;
		} else {
			if (be16_to_cpu(pkt->hdr.code) & FWSC_RSP_NACK)
				fwtty_notice(&peer->unit, "NACK unplug?\n");
			port = peer_revert_state(peer);
			reset = true;
		}
		break;

	default:
		fwtty_err(&peer->unit, "unknown mgmt code %d\n",
			  be16_to_cpu(pkt->hdr.code));
		rcode = RCODE_DATA_ERROR;
	}
	spin_unlock_bh(&peer->lock);

	if (port)
		fwserial_release_port(port, reset);

	return rcode;
}

/**
 * fwserial_mgmt_handler: bus address handler for mgmt requests
 * @parameters: fw_address_callback_t as specified by firewire core interface
 *
 * This handler is responsible for handling virtual cable requests from remotes
 * for all cards.
   fwserial_mgmt_handler : mgmt 요청을위한 버스 주소 핸들러 
   @parameters : firewire 코어 인터페이스에 지정된 fw_address_callback_t
   이 핸들러는 모든 카드에 대한 리모컨의 가상 케이블 요청 처리를 담당합니다.
 */
static void fwserial_mgmt_handler(struct fw_card *card,
				  struct fw_request *request,
				  int tcode, int destination, int source,
				  int generation,
				  unsigned long long addr,
				  void *data, size_t len,
				  void *callback_data)
{
	struct fwserial_mgmt_pkt *pkt = data;
	struct fwtty_peer *peer;
	int rcode;

	rcu_read_lock();
	peer = __fwserial_peer_by_node_id(card, generation, source);
	if (!peer) {
		fwtty_dbg(card, "peer(%d:%x) not found\n", generation, source);
		__dump_peer_list(card);
		rcode = RCODE_CONFLICT_ERROR;

	} else {
		switch (tcode) {
		case TCODE_WRITE_BLOCK_REQUEST:
			rcode = fwserial_parse_mgmt_write(peer, pkt, addr, len);
			break;

		default:
			rcode = RCODE_TYPE_ERROR;
		}
	}

	rcu_read_unlock();
	fw_send_response(card, request, rcode);
}

static int __init fwserial_init(void)
{
	int err, num_loops = !!(create_loop_dev);

	/* XXX: placeholder for a "firewire" debugfs node 
	   XXX : "firewire"debugfs 노드의 자리 표시 자 */
	fwserial_debugfs = debugfs_create_dir(KBUILD_MODNAME, NULL);

	/* num_ttys/num_ports must not be set above the static alloc avail
	   num_ttys / num_ports는 정적 할당량 avail 	 */
	if (num_ttys + num_loops > MAX_CARD_PORTS)
		num_ttys = MAX_CARD_PORTS - num_loops;

	num_ports = num_ttys + num_loops;

	fwtty_driver = tty_alloc_driver(MAX_TOTAL_PORTS, TTY_DRIVER_REAL_RAW
					| TTY_DRIVER_DYNAMIC_DEV);
	if (IS_ERR(fwtty_driver)) {
		err = PTR_ERR(fwtty_driver);
		goto remove_debugfs;
	}

	fwtty_driver->driver_name	= KBUILD_MODNAME;
	fwtty_driver->name		= tty_dev_name;
	fwtty_driver->major		= 0;
	fwtty_driver->minor_start	= 0;
	fwtty_driver->type		= TTY_DRIVER_TYPE_SERIAL;
	fwtty_driver->subtype		= SERIAL_TYPE_NORMAL;
	fwtty_driver->init_termios	    = tty_std_termios;
	fwtty_driver->init_termios.c_cflag  |= CLOCAL;
	tty_set_operations(fwtty_driver, &fwtty_ops);

	err = tty_register_driver(fwtty_driver);
	if (err) {
		pr_err("register tty driver failed (%d)\n", err);
		goto put_tty;
	}

	if (create_loop_dev) {
		fwloop_driver = tty_alloc_driver(MAX_TOTAL_PORTS / num_ports,
						 TTY_DRIVER_REAL_RAW
						 | TTY_DRIVER_DYNAMIC_DEV);
		if (IS_ERR(fwloop_driver)) {
			err = PTR_ERR(fwloop_driver);
			goto unregister_driver;
		}

		fwloop_driver->driver_name	= KBUILD_MODNAME "_loop";
		fwloop_driver->name		= loop_dev_name;
		fwloop_driver->major		= 0;
		fwloop_driver->minor_start	= 0;
		fwloop_driver->type		= TTY_DRIVER_TYPE_SERIAL;
		fwloop_driver->subtype		= SERIAL_TYPE_NORMAL;
		fwloop_driver->init_termios	    = tty_std_termios;
		fwloop_driver->init_termios.c_cflag  |= CLOCAL;
		tty_set_operations(fwloop_driver, &fwloop_ops);

		err = tty_register_driver(fwloop_driver);
		if (err) {
			pr_err("register loop driver failed (%d)\n", err);
			goto put_loop;
		}
	}

	fwtty_txn_cache = kmem_cache_create("fwtty_txn_cache",
					    sizeof(struct fwtty_transaction),
					    0, 0, fwtty_txn_constructor);
	if (!fwtty_txn_cache) {
		err = -ENOMEM;
		goto unregister_loop;
	}

	/*
	 * Ideally, this address handler would be registered per local node
	 * (rather than the same handler for all local nodes). However,
	 * since the firewire core requires the config rom descriptor *before*
	 * the local unit device(s) are created, a single management handler
	 * must suffice for all local serial units.
	   이상적으로이 주소 처리기는 모든 로컬 노드에 대해 동일한 처리기가 아니라 로컬 노드별로 등록됩니다.
	   그러나 파이어 와이어 코어는 로컬 유닛 장치가 생성되기 전에 config rom descriptor *를 필요로하므로
           모든 로컬 시리얼 장치에 대해 단일 관리 핸들러만으로 충분합니다.
	 */
	fwserial_mgmt_addr_handler.length = sizeof(struct fwserial_mgmt_pkt);
	fwserial_mgmt_addr_handler.address_callback = fwserial_mgmt_handler;

	err = fw_core_add_address_handler(&fwserial_mgmt_addr_handler,
					  &fwserial_mgmt_addr_region);
	if (err) {
		pr_err("add management handler failed (%d)\n", err);
		goto destroy_cache;
	}

	fwserial_unit_directory_data.unit_addr_offset =
		FW_UNIT_ADDRESS(fwserial_mgmt_addr_handler.offset);
	err = fw_core_add_descriptor(&fwserial_unit_directory);
	if (err) {
		pr_err("add unit descriptor failed (%d)\n", err);
		goto remove_handler;
	}

	err = driver_register(&fwserial_driver.driver);
	if (err) {
		pr_err("register fwserial driver failed (%d)\n", err);
		goto remove_descriptor;
	}

	return 0;

remove_descriptor:
	fw_core_remove_descriptor(&fwserial_unit_directory);
remove_handler:
	fw_core_remove_address_handler(&fwserial_mgmt_addr_handler);
destroy_cache:
	kmem_cache_destroy(fwtty_txn_cache);
unregister_loop:
	if (create_loop_dev)
		tty_unregister_driver(fwloop_driver);
put_loop:
	if (create_loop_dev)
		put_tty_driver(fwloop_driver);
unregister_driver:
	tty_unregister_driver(fwtty_driver);
put_tty:
	put_tty_driver(fwtty_driver);
remove_debugfs:
	debugfs_remove_recursive(fwserial_debugfs);

	return err;
}

static void __exit fwserial_exit(void)
{
	driver_unregister(&fwserial_driver.driver);
	fw_core_remove_descriptor(&fwserial_unit_directory);
	fw_core_remove_address_handler(&fwserial_mgmt_addr_handler);
	kmem_cache_destroy(fwtty_txn_cache);
	if (create_loop_dev) {
		tty_unregister_driver(fwloop_driver);
		put_tty_driver(fwloop_driver);
	}
	tty_unregister_driver(fwtty_driver);
	put_tty_driver(fwtty_driver);
	debugfs_remove_recursive(fwserial_debugfs);
}

module_init(fwserial_init);
module_exit(fwserial_exit);

MODULE_AUTHOR("Peter Hurley (peter@hurleysoftware.com)");
MODULE_DESCRIPTION("FireWire Serial TTY Driver");
MODULE_LICENSE("GPL");
MODULE_DEVICE_TABLE(ieee1394, fwserial_id_table);
MODULE_PARM_DESC(ttys, "Number of ttys to create for each local firewire node");
MODULE_PARM_DESC(auto, "Auto-connect a tty to each firewire node discovered");
MODULE_PARM_DESC(loop, "Create a loopback device, fwloop<n>, with ttys");
