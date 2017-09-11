/*
 * bcm.c - Broadcast Manager to filter/send (cyclic) CAN content
 *
 * Copyright (c) 2002-2007 Volkswagen Group Electronic Research
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Volkswagen nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * Alternatively, provided that this notice is retained in full, this
 * software may be distributed under the terms of the GNU General
 * Public License ("GPL") version 2, in which case the provisions of the
 * GPL apply INSTEAD OF those given above.
 *
 * The provided data structures and external interfaces from this code
 * are not restricted to be used by modules with a GPL compatible license.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uio.h>
#include <linux/net.h>
#include <linux/netdevice.h>
#include <linux/socket.h>
#include <linux/if_arp.h>
#include <linux/skbuff.h>
#include <linux/can.h>
#include <linux/can/core.h>
#include <linux/can/skb.h>
#include <linux/can/bcm.h>
#include <linux/slab.h>
#include <net/sock.h>
#include <net/net_namespace.h>

/*
 * To send multiple CAN frame content within TX_SETUP or to filter
 * CAN messages with multiplex index within RX_SETUP, the number of
 * different filters is limited to 256 due to the one byte index value.
  TX_SETUP 내에서 여러 CAN 프레임 내용을 보내거나 RX_SETUP 내에서 다중 인덱스가있는 CAN 메시지를 필터링하려면 1 바이트 인덱스 값으로 인해 다른 필터 수가 256 개로 제한됩니다.
 */
#define MAX_NFRAMES 256

/* use of last_frames[index].can_dlc */
#define RX_RECV    0x40 /* received data for this element 	
			   이 요소에 대한 데이터를 받았다. */		
#define RX_THR     0x80 /* element not been sent due to throttle feature 
			   throttle 기능으로 인해 요소가 전송되지 않았습니다. */
#define BCM_CAN_DLC_MASK 0x0F /* clean private flags in can_dlc by masking 
				masking을 통해 can_dic의 비공개 플래그를 정리 */

/* get best masking value for can_rx_register() for a given single can_id
  주어진 단일 can_id에 대해 can_rx_register ()에 대한 최상의 마스킹 값을 얻습니다. */
#define REGMASK(id) ((id & CAN_EFF_FLAG) ? \
		     (CAN_EFF_MASK | CAN_EFF_FLAG | CAN_RTR_FLAG) : \
		     (CAN_SFF_MASK | CAN_EFF_FLAG | CAN_RTR_FLAG))

#define CAN_BCM_VERSION CAN_VERSION

MODULE_DESCRIPTION("PF_CAN broadcast manager protocol");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Oliver Hartkopp <oliver.hartkopp@volkswagen.de>");
MODULE_ALIAS("can-proto-2");

/* easy access to can_frame payload */
static inline u64 GET_U64(const struct can_frame *cp)
{
	return *(u64 *)cp->data;
}

struct bcm_op {
	struct list_head list;
	int ifindex;
	canid_t can_id;
	u32 flags;
	unsigned long frames_abs, frames_filtered;
	struct bcm_timeval ival1, ival2;
	struct hrtimer timer, thrtimer;
	struct tasklet_struct tsklet, thrtsklet;
	ktime_t rx_stamp, kt_ival1, kt_ival2, kt_lastmsg;
	int rx_ifindex;
	u32 count;
	u32 nframes;
	u32 currframe;
	struct can_frame *frames;
	struct can_frame *last_frames;
	struct can_frame sframe;
	struct can_frame last_sframe;
	struct sock *sk;
	struct net_device *rx_reg_dev;
};

static struct proc_dir_entry *proc_dir;

struct bcm_sock {
	struct sock sk;
	int bound;
	int ifindex;
	struct notifier_block notifier;
	struct list_head rx_ops;
	struct list_head tx_ops;
	unsigned long dropped_usr_msgs;
	struct proc_dir_entry *bcm_proc_read;
	char procname [32]; /* inode number in decimal with \0 */
};

static inline struct bcm_sock *bcm_sk(const struct sock *sk)
{
	return (struct bcm_sock *)sk;
}

static inline ktime_t bcm_timeval_to_ktime(struct bcm_timeval tv)
{
	return ktime_set(tv.tv_sec, tv.tv_usec * NSEC_PER_USEC);
}

#define CFSIZ sizeof(struct can_frame)
#define OPSIZ sizeof(struct bcm_op)
#define MHSIZ sizeof(struct bcm_msg_head)

/*
 * procfs functions
 */
static char *bcm_proc_getifname(char *result, int ifindex)
{
	struct net_device *dev;

	if (!ifindex)
		return "any";

	rcu_read_lock();
	dev = dev_get_by_index_rcu(&init_net, ifindex);
	if (dev)
		strcpy(result, dev->name);
	else
		strcpy(result, "???");
	rcu_read_unlock();

	return result;
}

static int bcm_proc_show(struct seq_file *m, void *v)
{
	char ifname[IFNAMSIZ];
	struct sock *sk = (struct sock *)m->private;
	struct bcm_sock *bo = bcm_sk(sk);
	struct bcm_op *op;

	seq_printf(m, ">>> socket %pK", sk->sk_socket);
	seq_printf(m, " / sk %pK", sk);
	seq_printf(m, " / bo %pK", bo);
	seq_printf(m, " / dropped %lu", bo->dropped_usr_msgs);
	seq_printf(m, " / bound %s", bcm_proc_getifname(ifname, bo->ifindex));
	seq_printf(m, " <<<\n");

	list_for_each_entry(op, &bo->rx_ops, list) {

		unsigned long reduction;

		/* print only active entries & prevent division by zero 
		   활성 항목 만 인쇄하고 0으로 나눈 것을 방지합니다. */
		if (!op->frames_abs)
			continue;

		seq_printf(m, "rx_op: %03X %-5s ",
				op->can_id, bcm_proc_getifname(ifname, op->ifindex));
		seq_printf(m, "[%u]%c ", op->nframes,
				(op->flags & RX_CHECK_DLC)?'d':' ');
		if (op->kt_ival1.tv64)
			seq_printf(m, "timeo=%lld ",
					(long long)
					ktime_to_us(op->kt_ival1));

		if (op->kt_ival2.tv64)
			seq_printf(m, "thr=%lld ",
					(long long)
					ktime_to_us(op->kt_ival2));

		seq_printf(m, "# recv %ld (%ld) => reduction: ",
				op->frames_filtered, op->frames_abs);

		reduction = 100 - (op->frames_filtered * 100) / op->frames_abs;

		seq_printf(m, "%s%ld%%\n",
				(reduction == 100)?"near ":"", reduction);
	}

	list_for_each_entry(op, &bo->tx_ops, list) {

		seq_printf(m, "tx_op: %03X %s [%u] ",
				op->can_id,
				bcm_proc_getifname(ifname, op->ifindex),
				op->nframes);

		if (op->kt_ival1.tv64)
			seq_printf(m, "t1=%lld ",
					(long long) ktime_to_us(op->kt_ival1));

		if (op->kt_ival2.tv64)
			seq_printf(m, "t2=%lld ",
					(long long) ktime_to_us(op->kt_ival2));

		seq_printf(m, "# sent %ld\n", op->frames_abs);
	}
	seq_putc(m, '\n');
	return 0;
}

static int bcm_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, bcm_proc_show, PDE_DATA(inode));
}

static const struct file_operations bcm_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= bcm_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

/*
 * bcm_can_tx - send the (next) CAN frame to the appropriate CAN interface
 *              of the given bcm tx op
 * bcm_can_tx - (다음) CAN 프레임을 주어진 bcm tx op의 적절한 CAN 인터페이스로 보냅니다.
 */
static void bcm_can_tx(struct bcm_op *op)
{
	struct sk_buff *skb;
	struct net_device *dev;
	struct can_frame *cf = &op->frames[op->currframe];

	/* no target device? => exit */
	if (!op->ifindex)
		return;

	dev = dev_get_by_index(&init_net, op->ifindex);
	if (!dev) {
		/* RFC: should this bcm_op remove itself here?
			이 bcm_op가 여기에서 제거되어야합니까? */
		return;
	}

	skb = alloc_skb(CFSIZ + sizeof(struct can_skb_priv), gfp_any());
	if (!skb)
		goto out;

	can_skb_reserve(skb);
	can_skb_prv(skb)->ifindex = dev->ifindex;
	can_skb_prv(skb)->skbcnt = 0;

	memcpy(skb_put(skb, CFSIZ), cf, CFSIZ);

	/* send with loopback */
	skb->dev = dev;
	can_skb_set_owner(skb, op->sk);
	can_send(skb, 1);

	/* update statistics */
	op->currframe++;
	op->frames_abs++;

	/* reached last frame? */
	if (op->currframe >= op->nframes)
		op->currframe = 0;
 out:
	dev_put(dev);
}

/*
 * bcm_send_to_user - send a BCM message to the userspace 사용자 공간에 BCM 메시지 보내기
 *                    (consisting of bcm_msg_head + x CAN frames)
 */
static void bcm_send_to_user(struct bcm_op *op, struct bcm_msg_head *head,
			     struct can_frame *frames, int has_timestamp)
{
	struct sk_buff *skb;
	struct can_frame *firstframe;
	struct sockaddr_can *addr;
	struct sock *sk = op->sk;
	unsigned int datalen = head->nframes * CFSIZ;
	int err;

	skb = alloc_skb(sizeof(*head) + datalen, gfp_any());
	if (!skb)
		return;

	memcpy(skb_put(skb, sizeof(*head)), head, sizeof(*head));

	if (head->nframes) {
		/* can_frames starting here */
		firstframe = (struct can_frame *)skb_tail_pointer(skb);

		memcpy(skb_put(skb, datalen), frames, datalen);

		/*
		 * the BCM uses the can_dlc-element of the can_frame
		 * structure for internal purposes. This is only
		 * relevant for updates that are generated by the
		 * BCM, where nframes is 1
		 BCM은 내부 목적을 위해 can_frame 구조체의 can_dlc 요소를 사용합니다.
		 이것은 nframes가 1 인 BCM에 의해 생성 된 업데이트에만 관련이 있습니다.
		 */
		if (head->nframes == 1)
			firstframe->can_dlc &= BCM_CAN_DLC_MASK;
	}

	if (has_timestamp) {
		/* restore rx timestamp */
		skb->tstamp = op->rx_stamp;
	}

	/*
	 *  Put the datagram to the queue so that bcm_recvmsg() can
	 *  get it from there.  We need to pass the interface index to
	 *  bcm_recvmsg().  We pass a whole struct sockaddr_can in skb->cb
	 *  containing the interface index.
	   bcm_recvmsg ()가 거기에서 그것을 얻을 수 있도록 대기열에 데이터 그램을 넣으십시오.
	   bcm_recvmsg ()에 인터페이스 색인을 전달해야합니다. 
	   인터페이스 인덱스를 포함하는 skb-> cb에서 전체 struct sockaddr_can을 전달합니다.
	 */

	sock_skb_cb_check_size(sizeof(struct sockaddr_can));
	addr = (struct sockaddr_can *)skb->cb;
	memset(addr, 0, sizeof(*addr));
	addr->can_family  = AF_CAN;
	addr->can_ifindex = op->rx_ifindex;

	err = sock_queue_rcv_skb(sk, skb);
	if (err < 0) {
		struct bcm_sock *bo = bcm_sk(sk);

		kfree_skb(skb);
		/* don't care about overflows in this statistic 
		   이 통계에서 오버플로는 신경 쓰지 마십시오. */
		
		bo->dropped_usr_msgs++;
	}
}

static void bcm_tx_start_timer(struct bcm_op *op)
{
	if (op->kt_ival1.tv64 && op->count)
		hrtimer_start(&op->timer,
			      ktime_add(ktime_get(), op->kt_ival1),
			      HRTIMER_MODE_ABS);
	else if (op->kt_ival2.tv64)
		hrtimer_start(&op->timer,
			      ktime_add(ktime_get(), op->kt_ival2),
			      HRTIMER_MODE_ABS);
}

static void bcm_tx_timeout_tsklet(unsigned long data)
{
	struct bcm_op *op = (struct bcm_op *)data;
	struct bcm_msg_head msg_head;

	if (op->kt_ival1.tv64 && (op->count > 0)) {

		op->count--;
		if (!op->count && (op->flags & TX_COUNTEVT)) {

			/* create notification to user */
			msg_head.opcode  = TX_EXPIRED;
			msg_head.flags   = op->flags;
			msg_head.count   = op->count;
			msg_head.ival1   = op->ival1;
			msg_head.ival2   = op->ival2;
			msg_head.can_id  = op->can_id;
			msg_head.nframes = 0;

			bcm_send_to_user(op, &msg_head, NULL, 0);
		}
		bcm_can_tx(op);

	} else if (op->kt_ival2.tv64)
		bcm_can_tx(op);

	bcm_tx_start_timer(op);
}

/*
 * bcm_tx_timeout_handler - performs cyclic CAN frame transmissions 순환 CAN 프레임 전송을 수행합니다.
 */
static enum hrtimer_restart bcm_tx_timeout_handler(struct hrtimer *hrtimer)
{
	struct bcm_op *op = container_of(hrtimer, struct bcm_op, timer);

	tasklet_schedule(&op->tsklet);

	return HRTIMER_NORESTART;
}

/*
 * bcm_rx_changed - create a RX_CHANGED notification due to changed content 변경된 내용으로 인해 RX_CHANGED 알림을 생성합니다.
 */
static void bcm_rx_changed(struct bcm_op *op, struct can_frame *data)
{
	struct bcm_msg_head head;

	/* update statistics */
	op->frames_filtered++;

	/* prevent statistics overflow */
	if (op->frames_filtered > ULONG_MAX/100)
		op->frames_filtered = op->frames_abs = 0;

	/* this element is not throttled anymore
	   이 요소는 더이상 throttle 되지 않습니다. */
	data->can_dlc &= (BCM_CAN_DLC_MASK|RX_RECV);

	head.opcode  = RX_CHANGED;
	head.flags   = op->flags;
	head.count   = op->count;
	head.ival1   = op->ival1;
	head.ival2   = op->ival2;
	head.can_id  = op->can_id;
	head.nframes = 1;

	bcm_send_to_user(op, &head, data, 1);
}

/*
 * bcm_rx_update_and_send - process a detected relevant receive content change
 *                          1. update the last received data
 *                          2. send a notification to the user (if possible)
	검출 된 관련 수신 콘텐츠 변경을 처리한다.
	1. 마지막 수신 데이터 업데이트
	2. 가능한 경우 사용자에게 알림을 보냅니다.
 */
static void bcm_rx_update_and_send(struct bcm_op *op,
				   struct can_frame *lastdata,
				   const struct can_frame *rxdata)
{
	memcpy(lastdata, rxdata, CFSIZ);

	/* mark as used and throttled by default
 	   기본적으로 사용 및 조절 됨으로 표시 */
	lastdata->can_dlc |= (RX_RECV|RX_THR);

	/* throttling mode inactive ?
	   throttling 모드 비활성 ? */
	if (!op->kt_ival2.tv64) {
		/* send RX_CHANGED to the user immediately 
		   사용자에게 즉시 RX_CHANGED 종료  */
		bcm_rx_changed(op, lastdata);
		return;
	}

	/* with active throttling timer we are just done here
	   능동적 인 스로틀 타이머가있어서 우리는 여기서 막 끝납니다. */
	if (hrtimer_active(&op->thrtimer))
		return;

	/* first reception with enabled throttling mode
	   스로틀 모드가 활성화 된 첫 번째 수신  */
	if (!op->kt_lastmsg.tv64)
		goto rx_changed_settime;

	/* got a second frame inside a potential throttle period?
	   잠재적 인 스로틀 기간 내에서 두 번째 프레임을 얻었습니까? */
	if (ktime_us_delta(ktime_get(), op->kt_lastmsg) <
	    ktime_to_us(op->kt_ival2)) {
		/* do not send the saved data - only start throttle timer
		   저장된 데이터 만 송신하지 마십시오 - 스로틀 타이머를 시작하십시오. */
		hrtimer_start(&op->thrtimer,
			      ktime_add(op->kt_lastmsg, op->kt_ival2),
			      HRTIMER_MODE_ABS);
		return;
	}

	/* the gap was that big, that throttling was not needed here
	   그 격차가 너무 커서 조절은 필요하지 않았다. */
rx_changed_settime:
	bcm_rx_changed(op, lastdata);
	op->kt_lastmsg = ktime_get();
}

/*
 * bcm_rx_cmp_to_index - (bit)compares the currently received data to formerly
 *                       received data stored in op->last_frames[]
   현재 수신 된 데이터를 op-> last_frames []에 저장된 이전에 수신 된 데이터와 비교합니다
 */
static void bcm_rx_cmp_to_index(struct bcm_op *op, unsigned int index,
				const struct can_frame *rxdata)
{
	/*
	 * no one uses the MSBs of can_dlc for comparison,
	 * so we use it here to detect the first time of reception
          아무도 비교를 위해 can_dlc의 MSB를 사용하지 않으므로 여기에서 첫 수신 시간을 감지합니다
	 */

	if (!(op->last_frames[index].can_dlc & RX_RECV)) {
		/* received data for the first time => send update to user
		   처음으로받은 데이터 => 사용자에게 업데이트 보내기 */
		bcm_rx_update_and_send(op, &op->last_frames[index], rxdata);
		return;
	}

	/* do a real check in can_frame data section
	  can_frame 데이터 섹션에서 실제 점검을하십시오. */

	if ((GET_U64(&op->frames[index]) & GET_U64(rxdata)) !=
	    (GET_U64(&op->frames[index]) & GET_U64(&op->last_frames[index]))) {
		bcm_rx_update_and_send(op, &op->last_frames[index], rxdata);
		return;
	}

	if (op->flags & RX_CHECK_DLC) {
		/* do a real check in can_frame dlc 
		   can_frame dlc에서 실제 검사를 수행하십시오. */
		if (rxdata->can_dlc != (op->last_frames[index].can_dlc &
					BCM_CAN_DLC_MASK)) {
			bcm_rx_update_and_send(op, &op->last_frames[index],
					       rxdata);
			return;
		}
	}
}

/*
 * bcm_rx_starttimer - enable timeout monitoring for CAN frame reception
			CAN 프레임 수신을위한 타임 아웃 모니터링을 가능하게한다.
 */
static void bcm_rx_starttimer(struct bcm_op *op)
{
	if (op->flags & RX_NO_AUTOTIMER)
		return;

	if (op->kt_ival1.tv64)
		hrtimer_start(&op->timer, op->kt_ival1, HRTIMER_MODE_REL);
}

static void bcm_rx_timeout_tsklet(unsigned long data)
{
	struct bcm_op *op = (struct bcm_op *)data;
	struct bcm_msg_head msg_head;

	/* create notification to user
	  사용자에게 알림을 작성하십시오. */
	msg_head.opcode  = RX_TIMEOUT;
	msg_head.flags   = op->flags;
	msg_head.count   = op->count;
	msg_head.ival1   = op->ival1;
	msg_head.ival2   = op->ival2;
	msg_head.can_id  = op->can_id;
	msg_head.nframes = 0;

	bcm_send_to_user(op, &msg_head, NULL, 0);
}

/*
 * bcm_rx_timeout_handler - when the (cyclic) CAN frame reception timed out
			    (주기적) CAN 프레임 수신 시간 초과
 */
static enum hrtimer_restart bcm_rx_timeout_handler(struct hrtimer *hrtimer)
{
	struct bcm_op *op = container_of(hrtimer, struct bcm_op, timer);

	/* schedule before NET_RX_SOFTIRQ */
	tasklet_hi_schedule(&op->tsklet);

	/* no restart of the timer is done here!
	   여기서는 타이머를 다시 시작하지 않아도됩니다! */

	/* if user wants to be informed, when cyclic CAN-Messages come back */
	if ((op->flags & RX_ANNOUNCE_RESUME) && op->last_frames) {
		/* clear received can_frames to indicate 'nothing received' */
		memset(op->last_frames, 0, op->nframes * CFSIZ);
	}

	return HRTIMER_NORESTART;
}

/*
 * bcm_rx_do_flush - helper for bcm_rx_thr_flush
 */
static inline int bcm_rx_do_flush(struct bcm_op *op, int update,
				  unsigned int index)
{
	if ((op->last_frames) && (op->last_frames[index].can_dlc & RX_THR)) {
		if (update)
			bcm_rx_changed(op, &op->last_frames[index]);
		return 1;
	}
	return 0;
}

/*
 * bcm_rx_thr_flush - Check for throttled data and send it to the userspace
 *
 * update == 0 : just check if throttled data is available  (any irq context)
 * update == 1 : check and send throttled data to userspace (soft_irq context)
  bcm_rx_thr_flush - 스로틀 된 데이터를 확인하여 사용자 공간으로 보냅니다.
  
  * 업데이트 == 0 : 스로틀 된 데이터를 사용할 수 있는지 확인하십시오 (irq 컨텍스트)
  * 업데이트 == 1 : 사용자 공간에 스로틀 링 된 데이터 확인 및 전송 (soft_irq 컨텍스트)
  
 */
static int bcm_rx_thr_flush(struct bcm_op *op, int update)
{
	int updated = 0;

	if (op->nframes > 1) {
		unsigned int i;

		/* for MUX filter we start at index 1 */
		for (i = 1; i < op->nframes; i++)
			updated += bcm_rx_do_flush(op, update, i);

	} else {
		/* for RX_FILTER_ID and simple filter */
		updated += bcm_rx_do_flush(op, update, 0);
	}

	return updated;
}

static void bcm_rx_thr_tsklet(unsigned long data)
{
	struct bcm_op *op = (struct bcm_op *)data;

	/* push the changed data to the userspace */
	bcm_rx_thr_flush(op, 1);
}

/*
 * bcm_rx_thr_handler - the time for blocked content updates is over now:
 *                      Check for throttled data and send it to the userspace
   차단 된 콘텐츠 업데이트 시간이 끝났습니다.
   스로틀 된 데이터를 확인하여 사용자 공간으로 보냅니다.
 */
static enum hrtimer_restart bcm_rx_thr_handler(struct hrtimer *hrtimer)
{
	struct bcm_op *op = container_of(hrtimer, struct bcm_op, thrtimer);

	tasklet_schedule(&op->thrtsklet);

	if (bcm_rx_thr_flush(op, 0)) {
		hrtimer_forward(hrtimer, ktime_get(), op->kt_ival2);
		return HRTIMER_RESTART;
	} else {
		/* rearm throttle handling */
		op->kt_lastmsg = ktime_set(0, 0);
		return HRTIMER_NORESTART;
	}
}

/*
 * bcm_rx_handler - handle a CAN frame reception
   bcm_rx_handler - CAN 프레임 수신 처리
 */
static void bcm_rx_handler(struct sk_buff *skb, void *data)
{
	struct bcm_op *op = (struct bcm_op *)data;
	const struct can_frame *rxframe = (struct can_frame *)skb->data;
	unsigned int i;

	/* disable timeout */
	hrtimer_cancel(&op->timer);

	if (op->can_id != rxframe->can_id)
		return;

	/* save rx timestamp */
	op->rx_stamp = skb->tstamp;
	/* save originator for recvfrom() */
	op->rx_ifindex = skb->dev->ifindex;
	/* update statistics */
	op->frames_abs++;

	if (op->flags & RX_RTR_FRAME) {
		/* send reply for RTR-request (placed in op->frames[0]) */
		bcm_can_tx(op);
		return;
	}

	if (op->flags & RX_FILTER_ID) {
		/* the easiest case */
		bcm_rx_update_and_send(op, &op->last_frames[0], rxframe);
		goto rx_starttimer;
	}

	if (op->nframes == 1) {
		/* simple compare with index 0 */
		bcm_rx_cmp_to_index(op, 0, rxframe);
		goto rx_starttimer;
	}

	if (op->nframes > 1) {
		/*
		 * multiplex compare
		 *
		 * find the first multiplex mask that fits.
		 * Remark: The MUX-mask is stored in index 0
		 * 다중 비교
		 *
		 * 맞는 첫 번째 다중 마스크를 찾으십시오.
		 * 참고 : MUX- 마스크는 인덱스 0에 저장됩니다.
		 */

		for (i = 1; i < op->nframes; i++) {
			if ((GET_U64(&op->frames[0]) & GET_U64(rxframe)) ==
			    (GET_U64(&op->frames[0]) &
			     GET_U64(&op->frames[i]))) {
				bcm_rx_cmp_to_index(op, i, rxframe);
				break;
			}
		}
	}

rx_starttimer:
	bcm_rx_starttimer(op);
}

/*
 * helpers for bcm_op handling: find & delete bcm [rx|tx] op elements
 */
static struct bcm_op *bcm_find_op(struct list_head *ops, canid_t can_id,
				  int ifindex)
{
	struct bcm_op *op;

	list_for_each_entry(op, ops, list) {
		if ((op->can_id == can_id) && (op->ifindex == ifindex))
			return op;
	}

	return NULL;
}

static void bcm_remove_op(struct bcm_op *op)
{
	hrtimer_cancel(&op->timer);
	hrtimer_cancel(&op->thrtimer);

	if (op->tsklet.func)
		tasklet_kill(&op->tsklet);

	if (op->thrtsklet.func)
		tasklet_kill(&op->thrtsklet);

	if ((op->frames) && (op->frames != &op->sframe))
		kfree(op->frames);

	if ((op->last_frames) && (op->last_frames != &op->last_sframe))
		kfree(op->last_frames);

	kfree(op);
}

static void bcm_rx_unreg(struct net_device *dev, struct bcm_op *op)
{
	if (op->rx_reg_dev == dev) {
		can_rx_unregister(dev, op->can_id, REGMASK(op->can_id),
				  bcm_rx_handler, op);

		/* mark as removed subscription */
		op->rx_reg_dev = NULL;
	} else
		printk(KERN_ERR "can-bcm: bcm_rx_unreg: registered device "
		       "mismatch %p %p\n", op->rx_reg_dev, dev);
}

/*
 * bcm_delete_rx_op - find and remove a rx op (returns number of removed ops)
 */
static int bcm_delete_rx_op(struct list_head *ops, canid_t can_id, int ifindex)
{
	struct bcm_op *op, *n;

	list_for_each_entry_safe(op, n, ops, list) {
		if ((op->can_id == can_id) && (op->ifindex == ifindex)) {

			/*
			 * Don't care if we're bound or not (due to netdev
			 * problems) can_rx_unregister() is always a save
			 * thing to do here.
			우리가 묶인 지 아닌지 신경 쓰지 마라. 
			(netdev 문제 때문에) can_rx_unregister ()는 항상 여기에서 할일을 남겨둔다.
			 */
			if (op->ifindex) {
				/*
				 * Only remove subscriptions that had not
				 * been removed due to NETDEV_UNREGISTER
				 * in bcm_notifier()
				 bcm_notifier ()에서 NETDEV_UNREGISTER로 인해 제거되지 않은 구독 만 제거
				 */
				if (op->rx_reg_dev) {
					struct net_device *dev;

					dev = dev_get_by_index(&init_net,
							       op->ifindex);
					if (dev) {
						bcm_rx_unreg(dev, op);
						dev_put(dev);
					}
				}
			} else
				can_rx_unregister(NULL, op->can_id,
						  REGMASK(op->can_id),
						  bcm_rx_handler, op);

			list_del(&op->list);
			bcm_remove_op(op);
			return 1; /* done */
		}
	}

	return 0; /* not found */
}

/*
 * bcm_delete_tx_op - find and remove a tx op (returns number of removed ops)
 */
static int bcm_delete_tx_op(struct list_head *ops, canid_t can_id, int ifindex)
{
	struct bcm_op *op, *n;

	list_for_each_entry_safe(op, n, ops, list) {
		if ((op->can_id == can_id) && (op->ifindex == ifindex)) {
			list_del(&op->list);
			bcm_remove_op(op);
			return 1; /* done */
		}
	}

	return 0; /* not found */
}

/*
 * bcm_read_op - read out a bcm_op and send it to the user (for bcm_sendmsg)
   bcm_read_op - bcm_op를 읽어 사용자에게 보냅니다 (bcm_sendmsg의 경우).
 */
static int bcm_read_op(struct list_head *ops, struct bcm_msg_head *msg_head,
		       int ifindex)
{
	struct bcm_op *op = bcm_find_op(ops, msg_head->can_id, ifindex);

	if (!op)
		return -EINVAL;

	/* put current values into msg_head */
	msg_head->flags   = op->flags;
	msg_head->count   = op->count;
	msg_head->ival1   = op->ival1;
	msg_head->ival2   = op->ival2;
	msg_head->nframes = op->nframes;

	bcm_send_to_user(op, msg_head, op->frames, 0);

	return MHSIZ;
}

/*
 * bcm_tx_setup - create or update a bcm tx op (for bcm_sendmsg)
 */
static int bcm_tx_setup(struct bcm_msg_head *msg_head, struct msghdr *msg,
			int ifindex, struct sock *sk)
{
	struct bcm_sock *bo = bcm_sk(sk);
	struct bcm_op *op;
	unsigned int i;
	int err;

	/* we need a real device to send frames */
	if (!ifindex)
		return -ENODEV;

	/* check nframes boundaries - we need at least one can_frame */
	if (msg_head->nframes < 1 || msg_head->nframes > MAX_NFRAMES)
		return -EINVAL;

	/* check the given can_id */
	op = bcm_find_op(&bo->tx_ops, msg_head->can_id, ifindex);

	if (op) {
		/* update existing BCM operation */

		/*
		 * Do we need more space for the can_frames than currently
		 * allocated? -> This is a _really_ unusual use-case and
		 * therefore (complexity / locking) it is not supported.
		   can_frames에 현재 할당 된 공간보다 더 많은 공간이 필요합니까? -> 이것은 특별한 _ 유례가없는 사용 사례이므로 지원되지 않습니다 (복잡성 / 잠금).
		 */
		if (msg_head->nframes > op->nframes)
			return -E2BIG;

		/* update can_frames content */
		for (i = 0; i < msg_head->nframes; i++) {
			err = memcpy_from_msg((u8 *)&op->frames[i], msg, CFSIZ);

			if (op->frames[i].can_dlc > 8)
				err = -EINVAL;

			if (err < 0)
				return err;

			if (msg_head->flags & TX_CP_CAN_ID) {
				/* copy can_id into frame */
				op->frames[i].can_id = msg_head->can_id;
			}
		}

	} else {
		/* insert new BCM operation for the given can_id 
		   주어진 can_id에 대해 새로운 BCM 연산을 삽입한다. */

		op = kzalloc(OPSIZ, GFP_KERNEL);
		if (!op)
			return -ENOMEM;

		op->can_id    = msg_head->can_id;

		/* create array for can_frames and copy the data
		   can_frames에 대한 배열을 만들고 데이터 복사   */
		if (msg_head->nframes > 1) {
			op->frames = kmalloc(msg_head->nframes * CFSIZ,
					     GFP_KERNEL);
			if (!op->frames) {
				kfree(op);
				return -ENOMEM;
			}
		} else
			op->frames = &op->sframe;

		for (i = 0; i < msg_head->nframes; i++) {
			err = memcpy_from_msg((u8 *)&op->frames[i], msg, CFSIZ);

			if (op->frames[i].can_dlc > 8)
				err = -EINVAL;

			if (err < 0) {
				if (op->frames != &op->sframe)
					kfree(op->frames);
				kfree(op);
				return err;
			}

			if (msg_head->flags & TX_CP_CAN_ID) {
				/* copy can_id into frame */
				op->frames[i].can_id = msg_head->can_id;
			}
		}

		/* tx_ops never compare with previous received messages 
		   tx_ops는 이전에받은 메시지와 절대 비교하지 않습니다 */
		op->last_frames = NULL;

		/* bcm_can_tx / bcm_tx_timeout_handler needs this 
		   bcm_can_tx / bcm_tx_timeout_handler가 필요합니다 */
		op->sk = sk;
		op->ifindex = ifindex;

		/* initialize uninitialized (kzalloc) structure 
		   초기화되지 않은 (kzalloc) 구조 초기화 */
		hrtimer_init(&op->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		op->timer.function = bcm_tx_timeout_handler;

		/* initialize tasklet for tx countevent notification 
		   tx countevent 알림을위한 작업 표 초기화 */
		tasklet_init(&op->tsklet, bcm_tx_timeout_tsklet,
			     (unsigned long) op);

		/* currently unused in tx_ops 
		   tx_ops에서 현재 사용되지 않음 */
		hrtimer_init(&op->thrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

		/* add this bcm_op to the list of the tx_ops
		   이 bcm_op을 tx_ops 목록에 추가하십시오.  */
		list_add(&op->list, &bo->tx_ops);

	} /* if ((op = bcm_find_op(&bo->tx_ops, msg_head->can_id, ifindex))) */

	if (op->nframes != msg_head->nframes) {
		op->nframes   = msg_head->nframes;
		/* start multiple frame transmission with index 0
		   이 bcm_op을 tx_ops의 인덱스 0t를 사용하여 lis start 다중 프레임 전송에 추가하십시오.  */
		op->currframe = 0;
	}

	/* check flags */

	op->flags = msg_head->flags;

	if (op->flags & TX_RESET_MULTI_IDX) {
		/* start multiple frame transmission with index 0
		   인덱스 0으로 다중 프레임 전송을 시작한다. */
		op->currframe = 0;
	}

	if (op->flags & SETTIMER) {
		/* set timer values */
		op->count = msg_head->count;
		op->ival1 = msg_head->ival1;
		op->ival2 = msg_head->ival2;
		op->kt_ival1 = bcm_timeval_to_ktime(msg_head->ival1);
		op->kt_ival2 = bcm_timeval_to_ktime(msg_head->ival2);

		/* disable an active timer due to zero values?
		   0 값으로 인해 활성 타이머를 비활성화 하시겠습니까? */
		if (!op->kt_ival1.tv64 && !op->kt_ival2.tv64)
			hrtimer_cancel(&op->timer);
	}

	if (op->flags & STARTTIMER) {
		hrtimer_cancel(&op->timer);
		/* spec: send can_frame when starting timer
		   사양 : 타이머를 시작할 때 can_frame을 보냅니다. */
		op->flags |= TX_ANNOUNCE;
	}

	if (op->flags & TX_ANNOUNCE) {
		bcm_can_tx(op);
		if (op->count)
			op->count--;
	}

	if (op->flags & STARTTIMER)
		bcm_tx_start_timer(op);

	return msg_head->nframes * CFSIZ + MHSIZ;
}

/*
 * bcm_rx_setup - create or update a bcm rx op (for bcm_sendmsg)
 */
static int bcm_rx_setup(struct bcm_msg_head *msg_head, struct msghdr *msg,
			int ifindex, struct sock *sk)
{
	struct bcm_sock *bo = bcm_sk(sk);
	struct bcm_op *op;
	int do_rx_register;
	int err = 0;

	if ((msg_head->flags & RX_FILTER_ID) || (!(msg_head->nframes))) {
		/* be robust against wrong usage ... 
		   잘못된 사용법에 강하다. */
		msg_head->flags |= RX_FILTER_ID;
		/* ignore trailing garbage
		   후행 쓰레기 무시 */
		msg_head->nframes = 0;
	}

	/* the first element contains the mux-mask => MAX_NFRAMES + 1 
	  첫 번째 요소는 mux-mask => MAX_NFRAMES + 1을 포함한다.   */
	if (msg_head->nframes > MAX_NFRAMES + 1)
		return -EINVAL;

	if ((msg_head->flags & RX_RTR_FRAME) &&
	    ((msg_head->nframes != 1) ||
	     (!(msg_head->can_id & CAN_RTR_FLAG))))
		return -EINVAL;

	/* check the given can_id */
	op = bcm_find_op(&bo->rx_ops, msg_head->can_id, ifindex);
	if (op) {
		/* update existing BCM operation
		   기존 BCM 작업 업데이트 */

		/*
		 * Do we need more space for the can_frames than currently
		 * allocated? -> This is a _really_ unusual use-case and
		 * therefore (complexity / locking) it is not supported.
		  현재보다 can_frames에 더 많은 공간이 필요합니까?
		  할당? -> 이것은 특별한 _ 유례가없는 사용 사례이므로 지원되지 않습니다 (복잡성 / 잠금).
		 */
		if (msg_head->nframes > op->nframes)
			return -E2BIG;

		if (msg_head->nframes) {
			/* update can_frames content */
			err = memcpy_from_msg((u8 *)op->frames, msg,
					      msg_head->nframes * CFSIZ);
			if (err < 0)
				return err;

			/* clear last_frames to indicate 'nothing received' 
			   last_frames를 지워서 '아무것도받지 못함'을 나타냅니다. */
			memset(op->last_frames, 0, msg_head->nframes * CFSIZ);
		}

		op->nframes = msg_head->nframes;

		/* Only an update -> do not call can_rx_register() */
		do_rx_register = 0;

	} else {
		/* insert new BCM operation for the given can_id */
		op = kzalloc(OPSIZ, GFP_KERNEL);
		if (!op)
			return -ENOMEM;

		op->can_id    = msg_head->can_id;
		op->nframes   = msg_head->nframes;

		if (msg_head->nframes > 1) {
			/* create array for can_frames and copy the data */
			op->frames = kmalloc(msg_head->nframes * CFSIZ,
					     GFP_KERNEL);
			if (!op->frames) {
				kfree(op);
				return -ENOMEM;
			}

			/* create and init array for received can_frames */
			op->last_frames = kzalloc(msg_head->nframes * CFSIZ,
						  GFP_KERNEL);
			if (!op->last_frames) {
				kfree(op->frames);
				kfree(op);
				return -ENOMEM;
			}

		} else {
			op->frames = &op->sframe;
			op->last_frames = &op->last_sframe;
		}

		if (msg_head->nframes) {
			err = memcpy_from_msg((u8 *)op->frames, msg,
					      msg_head->nframes * CFSIZ);
			if (err < 0) {
				if (op->frames != &op->sframe)
					kfree(op->frames);
				if (op->last_frames != &op->last_sframe)
					kfree(op->last_frames);
				kfree(op);
				return err;
			}
		}

		/* bcm_can_tx / bcm_tx_timeout_handler needs this */
		op->sk = sk;
		op->ifindex = ifindex;

		/* ifindex for timeout events w/o previous frame reception 
		   이전 프레임 수신이없는 시간 초과 이벤트의 경우 ifindex  */
		op->rx_ifindex = ifindex;

		/* initialize uninitialized (kzalloc) structure 
		    초기화되지 않은 (kzalloc) 구조 초기화  */
		hrtimer_init(&op->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		op->timer.function = bcm_rx_timeout_handler;

		/* initialize tasklet for rx timeout notification 
		     rx 시간 초과 알림을위한 작업 표 초기화 */
		tasklet_init(&op->tsklet, bcm_rx_timeout_tsklet,
			     (unsigned long) op);

		hrtimer_init(&op->thrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		op->thrtimer.function = bcm_rx_thr_handler;

		/* initialize tasklet for rx throttle handling
 		   rx 스로틀 핸들링을위한 tasklet 초기화   */
		tasklet_init(&op->thrtsklet, bcm_rx_thr_tsklet,
			     (unsigned long) op);

		/* add this bcm_op to the list of the rx_ops 
		   이 bcm_op을 rx_ops의 목록에 추가하십시오  */
		list_add(&op->list, &bo->rx_ops);

		/* call can_rx_register() */
		do_rx_register = 1;

	} /* if ((op = bcm_find_op(&bo->rx_ops, msg_head->can_id, ifindex))) */

	/* check flags */
	op->flags = msg_head->flags;

	if (op->flags & RX_RTR_FRAME) {

		/* no timers in RTR-mode */
		hrtimer_cancel(&op->thrtimer);
		hrtimer_cancel(&op->timer);

		/*
		 * funny feature in RX(!)_SETUP only for RTR-mode:
		 * copy can_id into frame BUT without RTR-flag to
		 * prevent a full-load-loopback-test ... ;-]
		RTR 모드에 대해서만 RX (!) SETUP의 재미있는 기능 : 
		전체로드 루프백 테스트를 방지하기 위해 RTR 플래그없이 프레임에 can_id 복사 ...; -]
		 */
		if ((op->flags & TX_CP_CAN_ID) ||
		    (op->frames[0].can_id == op->can_id))
			op->frames[0].can_id = op->can_id & ~CAN_RTR_FLAG;

	} else {
		if (op->flags & SETTIMER) {

			/* set timer value */
			op->ival1 = msg_head->ival1;
			op->ival2 = msg_head->ival2;
			op->kt_ival1 = bcm_timeval_to_ktime(msg_head->ival1);
			op->kt_ival2 = bcm_timeval_to_ktime(msg_head->ival2);

			/* disable an active timer due to zero value?
			   0 값으로 인해 활성 타이머를 비활성화 하시겠습니까? */
			if (!op->kt_ival1.tv64)
				hrtimer_cancel(&op->timer);

			/*
			 * In any case cancel the throttle timer, flush
			 * potentially blocked msgs and reset throttle handling
			   어떤 경우에는 스로틀 타이머를 취소하고 잠재적으로 차단 된 메시지를 플러시하고 스로틀 처리를 재설정하십시오
			 */
			op->kt_lastmsg = ktime_set(0, 0);
			hrtimer_cancel(&op->thrtimer);
			bcm_rx_thr_flush(op, 1);
		}

		if ((op->flags & STARTTIMER) && op->kt_ival1.tv64)
			hrtimer_start(&op->timer, op->kt_ival1,
				      HRTIMER_MODE_REL);
	}

	/* now we can register for can_ids, if we added a new bcm_op
	   새로운 bcm_op을 추가하면 이제 can_ids에 등록 할 수 있습니다. */
	if (do_rx_register) {
		if (ifindex) {
			struct net_device *dev;

			dev = dev_get_by_index(&init_net, ifindex);
			if (dev) {
				err = can_rx_register(dev, op->can_id,
						      REGMASK(op->can_id),
						      bcm_rx_handler, op,
						      "bcm");

				op->rx_reg_dev = dev;
				dev_put(dev);
			}

		} else
			err = can_rx_register(NULL, op->can_id,
					      REGMASK(op->can_id),
					      bcm_rx_handler, op, "bcm");
		if (err) {
			/* this bcm rx op is broken -> remove it */
			list_del(&op->list);
			bcm_remove_op(op);
			return err;
		}
	}

	return msg_head->nframes * CFSIZ + MHSIZ;
}

/*
 * bcm_tx_send - send a single CAN frame to the CAN interface (for bcm_sendmsg)
		 단일 CAN 프레임을 CAN 인터페이스로 보내십시오 (bcm_sendmsg 용).
 */
static int bcm_tx_send(struct msghdr *msg, int ifindex, struct sock *sk)
{
	struct sk_buff *skb;
	struct net_device *dev;
	int err;

	/* we need a real device to send frames */
	if (!ifindex)
		return -ENODEV;

	skb = alloc_skb(CFSIZ + sizeof(struct can_skb_priv), GFP_KERNEL);
	if (!skb)
		return -ENOMEM;

	can_skb_reserve(skb);

	err = memcpy_from_msg(skb_put(skb, CFSIZ), msg, CFSIZ);
	if (err < 0) {
		kfree_skb(skb);
		return err;
	}

	dev = dev_get_by_index(&init_net, ifindex);
	if (!dev) {
		kfree_skb(skb);
		return -ENODEV;
	}

	can_skb_prv(skb)->ifindex = dev->ifindex;
	can_skb_prv(skb)->skbcnt = 0;
	skb->dev = dev;
	can_skb_set_owner(skb, sk);
	err = can_send(skb, 1); /* send with loopback */
	dev_put(dev);

	if (err)
		return err;

	return CFSIZ + MHSIZ;
}

/*
 * bcm_sendmsg - process BCM commands (opcodes) from the userspace
		 사용자 공간에서 BCM 명령 (opcode)을 처리합니다.
 */
static int bcm_sendmsg(struct socket *sock, struct msghdr *msg, size_t size)
{
	struct sock *sk = sock->sk;
	struct bcm_sock *bo = bcm_sk(sk);
	int ifindex = bo->ifindex; /* default ifindex for this bcm_op  
				      이것에 대한 ifindex 디폴트 bcm_op */
	struct bcm_msg_head msg_head;
	int ret; /* read bytes or error codes as return value 
		    바이트 또는 오류 코드를 반환 값으로 읽습니다. */

	if (!bo->bound)
		return -ENOTCONN;

	/* check for valid message length from userspace 
	   사용자 공간에서 유효한 메시지 길이를 확인  */
	if (size < MHSIZ || (size - MHSIZ) % CFSIZ)
		return -EINVAL;

	/* check for alternative ifindex for this bcm_op 
	   이것에 대한 대체 ifindex를 확인하십시오 bcm_op  */

	if (!ifindex && msg->msg_name) {
		/* no bound device as default => check msg_name 
		   기본값으로 바인딩 된 장치 없음 => check msg_name */
		DECLARE_SOCKADDR(struct sockaddr_can *, addr, msg->msg_name);

		if (msg->msg_namelen < sizeof(*addr))
			return -EINVAL;

		if (addr->can_family != AF_CAN)
			return -EINVAL;

		/* ifindex from sendto() */
		ifindex = addr->can_ifindex;

		if (ifindex) {
			struct net_device *dev;

			dev = dev_get_by_index(&init_net, ifindex);
			if (!dev)
				return -ENODEV;

			if (dev->type != ARPHRD_CAN) {
				dev_put(dev);
				return -ENODEV;
			}

			dev_put(dev);
		}
	}

	/* read message head information */

	ret = memcpy_from_msg((u8 *)&msg_head, msg, MHSIZ);
	if (ret < 0)
		return ret;

	lock_sock(sk);

	switch (msg_head.opcode) {

	case TX_SETUP:
		ret = bcm_tx_setup(&msg_head, msg, ifindex, sk);
		break;

	case RX_SETUP:
		ret = bcm_rx_setup(&msg_head, msg, ifindex, sk);
		break;

	case TX_DELETE:
		if (bcm_delete_tx_op(&bo->tx_ops, msg_head.can_id, ifindex))
			ret = MHSIZ;
		else
			ret = -EINVAL;
		break;

	case RX_DELETE:
		if (bcm_delete_rx_op(&bo->rx_ops, msg_head.can_id, ifindex))
			ret = MHSIZ;
		else
			ret = -EINVAL;
		break;

	case TX_READ:
		/* reuse msg_head for the reply to TX_READ */
		msg_head.opcode  = TX_STATUS;
		ret = bcm_read_op(&bo->tx_ops, &msg_head, ifindex);
		break;

	case RX_READ:
		/* reuse msg_head for the reply to RX_READ */
		msg_head.opcode  = RX_STATUS;
		ret = bcm_read_op(&bo->rx_ops, &msg_head, ifindex);
		break;

	case TX_SEND:
		/* we need exactly one can_frame behind the msg head */
		if ((msg_head.nframes != 1) || (size != CFSIZ + MHSIZ))
			ret = -EINVAL;
		else
			ret = bcm_tx_send(msg, ifindex, sk);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	release_sock(sk);

	return ret;
}

/*
 * notification handler for netdevice status changes
   netdevice 상태 변경에 대한 알림 처리기
 */
static int bcm_notifier(struct notifier_block *nb, unsigned long msg,
			void *ptr)
{
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);
	struct bcm_sock *bo = container_of(nb, struct bcm_sock, notifier);
	struct sock *sk = &bo->sk;
	struct bcm_op *op;
	int notify_enodev = 0;

	if (!net_eq(dev_net(dev), &init_net))
		return NOTIFY_DONE;

	if (dev->type != ARPHRD_CAN)
		return NOTIFY_DONE;

	switch (msg) {

	case NETDEV_UNREGISTER:
		lock_sock(sk);

		/* remove device specific receive entries 
		   장치 관련 수신 항목 제거 */
		list_for_each_entry(op, &bo->rx_ops, list)
			if (op->rx_reg_dev == dev)
				bcm_rx_unreg(dev, op);

		/* remove device reference, if this is our bound device
		   장치가 바인딩 된 장치 인 경우 장치 참조를 제거하십시오.  */
		if (bo->bound && bo->ifindex == dev->ifindex) {
			bo->bound   = 0;
			bo->ifindex = 0;
			notify_enodev = 1;
		}

		release_sock(sk);

		if (notify_enodev) {
			sk->sk_err = ENODEV;
			if (!sock_flag(sk, SOCK_DEAD))
				sk->sk_error_report(sk);
		}
		break;

	case NETDEV_DOWN:
		if (bo->bound && bo->ifindex == dev->ifindex) {
			sk->sk_err = ENETDOWN;
			if (!sock_flag(sk, SOCK_DEAD))
				sk->sk_error_report(sk);
		}
	}

	return NOTIFY_DONE;
}

/*
 * initial settings for all BCM sockets to be set at socket creation time
   모든 BCM 소켓이 소켓 생성시 설정되도록 초기 설정
 */
static int bcm_init(struct sock *sk)
{
	struct bcm_sock *bo = bcm_sk(sk);

	bo->bound            = 0;
	bo->ifindex          = 0;
	bo->dropped_usr_msgs = 0;
	bo->bcm_proc_read    = NULL;

	INIT_LIST_HEAD(&bo->tx_ops);
	INIT_LIST_HEAD(&bo->rx_ops);

	/* set notifier */
	bo->notifier.notifier_call = bcm_notifier;

	register_netdevice_notifier(&bo->notifier);

	return 0;
}

/*
 * standard socket functions
 */
static int bcm_release(struct socket *sock)
{
	struct sock *sk = sock->sk;
	struct bcm_sock *bo;
	struct bcm_op *op, *next;

	if (sk == NULL)
		return 0;

	bo = bcm_sk(sk);

	/* remove bcm_ops, timer, rx_unregister(), etc. */

	unregister_netdevice_notifier(&bo->notifier);

	lock_sock(sk);

	list_for_each_entry_safe(op, next, &bo->tx_ops, list)
		bcm_remove_op(op);

	list_for_each_entry_safe(op, next, &bo->rx_ops, list) {
		/*
		 * Don't care if we're bound or not (due to netdev problems)
		 * can_rx_unregister() is always a save thing to do here.
		   우리가 묶인 지 아닌지 신경 쓰지 마라. 
		   (netdev 문제 때문에) can_rx_unregister ()는 항상 여기에서 할일을 남겨둔다.
		 */
		if (op->ifindex) {
			/*
			 * Only remove subscriptions that had not
			 * been removed due to NETDEV_UNREGISTER
			 * in bcm_notifier()
			   NETDEV_UNREGISTER로 인해 제거되지 않은 구독 만 제거
			   in bcm_notifier ()
			 */
			if (op->rx_reg_dev) {
				struct net_device *dev;

				dev = dev_get_by_index(&init_net, op->ifindex);
				if (dev) {
					bcm_rx_unreg(dev, op);
					dev_put(dev);
				}
			}
		} else
			can_rx_unregister(NULL, op->can_id,
					  REGMASK(op->can_id),
					  bcm_rx_handler, op);

		bcm_remove_op(op);
	}

	/* remove procfs entry */
	if (proc_dir && bo->bcm_proc_read)
		remove_proc_entry(bo->procname, proc_dir);

	/* remove device reference */
	if (bo->bound) {
		bo->bound   = 0;
		bo->ifindex = 0;
	}

	sock_orphan(sk);
	sock->sk = NULL;

	release_sock(sk);
	sock_put(sk);

	return 0;
}

static int bcm_connect(struct socket *sock, struct sockaddr *uaddr, int len,
		       int flags)
{
	struct sockaddr_can *addr = (struct sockaddr_can *)uaddr;
	struct sock *sk = sock->sk;
	struct bcm_sock *bo = bcm_sk(sk);

	if (len < sizeof(*addr))
		return -EINVAL;

	if (bo->bound)
		return -EISCONN;

	/* bind a device to this socket */
	if (addr->can_ifindex) {
		struct net_device *dev;

		dev = dev_get_by_index(&init_net, addr->can_ifindex);
		if (!dev)
			return -ENODEV;

		if (dev->type != ARPHRD_CAN) {
			dev_put(dev);
			return -ENODEV;
		}

		bo->ifindex = dev->ifindex;
		dev_put(dev);

	} else {
		/* no interface reference for ifindex = 0 ('any' CAN device) */
		bo->ifindex = 0;
	}

	bo->bound = 1;

	if (proc_dir) {
		/* unique socket address as filename */
		sprintf(bo->procname, "%lu", sock_i_ino(sk));
		bo->bcm_proc_read = proc_create_data(bo->procname, 0644,
						     proc_dir,
						     &bcm_proc_fops, sk);
	}

	return 0;
}

static int bcm_recvmsg(struct socket *sock, struct msghdr *msg, size_t size,
		       int flags)
{
	struct sock *sk = sock->sk;
	struct sk_buff *skb;
	int error = 0;
	int noblock;
	int err;

	noblock =  flags & MSG_DONTWAIT;
	flags   &= ~MSG_DONTWAIT;
	skb = skb_recv_datagram(sk, flags, noblock, &error);
	if (!skb)
		return error;

	if (skb->len < size)
		size = skb->len;

	err = memcpy_to_msg(msg, skb->data, size);
	if (err < 0) {
		skb_free_datagram(sk, skb);
		return err;
	}

	sock_recv_ts_and_drops(msg, sk, skb);

	if (msg->msg_name) {
		__sockaddr_check_size(sizeof(struct sockaddr_can));
		msg->msg_namelen = sizeof(struct sockaddr_can);
		memcpy(msg->msg_name, skb->cb, msg->msg_namelen);
	}

	skb_free_datagram(sk, skb);

	return size;
}

static const struct proto_ops bcm_ops = {
	.family        = PF_CAN,
	.release       = bcm_release,
	.bind          = sock_no_bind,
	.connect       = bcm_connect,
	.socketpair    = sock_no_socketpair,
	.accept        = sock_no_accept,
	.getname       = sock_no_getname,
	.poll          = datagram_poll,
	.ioctl         = can_ioctl,	/* use can_ioctl() from af_can.c */
	.listen        = sock_no_listen,
	.shutdown      = sock_no_shutdown,
	.setsockopt    = sock_no_setsockopt,
	.getsockopt    = sock_no_getsockopt,
	.sendmsg       = bcm_sendmsg,
	.recvmsg       = bcm_recvmsg,
	.mmap          = sock_no_mmap,
	.sendpage      = sock_no_sendpage,
};

static struct proto bcm_proto __read_mostly = {
	.name       = "CAN_BCM",
	.owner      = THIS_MODULE,
	.obj_size   = sizeof(struct bcm_sock),
	.init       = bcm_init,
};

static const struct can_proto bcm_can_proto = {
	.type       = SOCK_DGRAM,
	.protocol   = CAN_BCM,
	.ops        = &bcm_ops,
	.prot       = &bcm_proto,
};

static int __init bcm_module_init(void)
{
	int err;

	pr_info("can: broadcast manager protocol (rev " CAN_BCM_VERSION " t)\n");

	err = can_proto_register(&bcm_can_proto);
	if (err < 0) {
		printk(KERN_ERR "can: registration of bcm protocol failed\n");
		return err;
	}

	/* create /proc/net/can-bcm directory */
	proc_dir = proc_mkdir("can-bcm", init_net.proc_net);
	return 0;
}

static void __exit bcm_module_exit(void)
{
	can_proto_unregister(&bcm_can_proto);

	if (proc_dir)
		remove_proc_entry("can-bcm", init_net.proc_net);
}

module_init(bcm_module_init);
module_exit(bcm_module_exit);
