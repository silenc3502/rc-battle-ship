/*
 * af_can.c - Protocol family CAN core module
 *            (used by different CAN protocol modules)
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
///
/*
 * af_can.c - 프로토콜 제품군 CAN 코어 모듈
 * (다른 CAN 프로토콜 모듈에서 사용됨)
 *
 * Copyright (c) 2002-2007 Volkswagen Group 전자 연구 판권 소유.
 *
 * 원본 및 바이너리 형식의 재배포 및 사용은 수정 여부와 상관없이 다음 조건이 충족되는 경우 허용됩니다.
 * 1. 소스 코드 재배포시 위의 저작권 공지, 본 조건 목록 및 다음 면책 조항을 포함해야합니다.
 * 2. 바이너리 형식으로 재배포하는 경우 위의 저작권 표시, 본 조건 목록 및 다음 면책 조항을 배포시 제공된 설명서 및 / 또는 기타 자료에 포함시켜야합니다.
 * 3. 사전 서면 허가없이이 소프트웨어에서 파생 된 제품을 보증하거나 홍보하기 위해 폭스 바겐 이름이나 공헌자 이름을 사용할 수 없습니다.
 *
 * 또는이 통지문이 완전히 보존되어있는 경우,이 소프트웨어는 GNU 일반 공중 라이선스 ( "GPL") 버전 2의 조건에 따라 배포 될 수 있습니다.이 경우 GPL 조항은 위에 제공된 것 대신 적용됩니다.
 *
 * 이 코드의 제공된 데이터 구조 및 외부 인터페이스는 GPL 호환 라이센스가있는 모듈에서 사용하도록 제한되지 않습니다.
 *
 * 이 소프트웨어는 저작권 보유자 및 기여자가 제공합니다.
 * 상품성 및 특정 목적에의 적합성에 대한 묵시적인 보증을 포함하여 (단, 이에 한하지 않음) 명시 적 또는 묵시적인 일체의 보증없이 "있는 그대로"제공합니다. 
 *저작권 소유자 또는 기여자는 직접, 간접적 인, 우발적 인, 우발적 인, 간접적 인 또는 필연적 인 손해 
 *(대체 제품 또는 서비스의 조달, 사용 손실, 데이터 손실 또는 이익 손실을 포함하되 이에 국한되지 않음) 또는 비즈니스 중단)에 대한 책임을지지 않습니다.
 * 계약 위반, 엄격한 책임 또는 불법 행위 (과실 포함 또는 기타 포함)에 관계없이이 소프트웨어의 사용으로 인해 발생하는 모든 책임은 해당 손해의 가능성을 사전에 알고 있던 경우에도 마찬가지입니다.
 *
 */

#include <linux/module.h>
#include <linux/stddef.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>
#include <linux/uaccess.h>
#include <linux/net.h>
#include <linux/netdevice.h>
#include <linux/socket.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/skbuff.h>
#include <linux/can.h>
#include <linux/can/core.h>
#include <linux/can/skb.h>
#include <linux/ratelimit.h>
#include <net/net_namespace.h>
#include <net/sock.h>

#include "af_can.h"

MODULE_DESCRIPTION("Controller Area Network PF_CAN core");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Urs Thuermann <urs.thuermann@volkswagen.de>, "
	      "Oliver Hartkopp <oliver.hartkopp@volkswagen.de>");

MODULE_ALIAS_NETPROTO(PF_CAN);

static int stats_timer __read_mostly = 1;
module_param(stats_timer, int, S_IRUGO);
MODULE_PARM_DESC(stats_timer, "enable timer for statistics (default:on)");

/* receive filters subscribed for 'all' CAN devices */ /* '모든'CAN 장치에 가입 한 필터 수신 */
struct dev_rcv_lists can_rx_alldev_list;
static DEFINE_SPINLOCK(can_rcvlists_lock);

static struct kmem_cache *rcv_cache __read_mostly;

/* table of registered CAN protocols */
static const struct can_proto *proto_tab[CAN_NPROTO] __read_mostly;
static DEFINE_MUTEX(proto_tab_lock);

struct timer_list can_stattimer;   /* timer for statistics update */ /*통계 업데이트를 위한 타이머*/
struct s_stats    can_stats;       /* packet statistics */ /* 패킷 통계 */
struct s_pstats   can_pstats;      /* receive list statistics */ /* 리스트 통계를 받습니다.*/

static atomic_t skbcounter = ATOMIC_INIT(0);

/*
 * af_can socket functions
 */

int can_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg)
{
	struct sock *sk = sock->sk;

	switch (cmd) {

	case SIOCGSTAMP:
		return sock_get_timestamp(sk, (struct timeval __user *)arg);

	default:
		return -ENOIOCTLCMD;
	}
}
EXPORT_SYMBOL(can_ioctl);

static void can_sock_destruct(struct sock *sk)
{
	skb_queue_purge(&sk->sk_receive_queue);
}

static const struct can_proto *can_get_proto(int protocol)
{
	const struct can_proto *cp;

	rcu_read_lock();
	cp = rcu_dereference(proto_tab[protocol]);
	if (cp && !try_module_get(cp->prot->owner))
		cp = NULL;
	rcu_read_unlock();

	return cp;
}

static inline void can_put_proto(const struct can_proto *cp)
{
	module_put(cp->prot->owner);
}

static int can_create(struct net *net, struct socket *sock, int protocol,
		      int kern)
{
	struct sock *sk;
	const struct can_proto *cp;
	int err = 0;

	sock->state = SS_UNCONNECTED;

	if (protocol < 0 || protocol >= CAN_NPROTO)
		return -EINVAL;

	if (!net_eq(net, &init_net))
		return -EAFNOSUPPORT;

	cp = can_get_proto(protocol);

#ifdef CONFIG_MODULES
	if (!cp) {
		/* try to load protocol module if kernel is modular */ /* 커널이 모듈화 되어 있다면 프로토콜 모듈을 로드하려고 시도한다. */

		err = request_module("can-proto-%d", protocol);

		/*
		 * In case of error we only print a message but don't  오류가 발생하는 경우에만 메세지를 인쇄하지만 인쇄하지 마십시오.
		 * return the error code immediately.  Below we will	즉시 오류 코드를 반환하십시오. 
		 * return -EPROTONOSUPPORT
		 */
		if (err)
			printk_ratelimited(KERN_ERR "can: request_module "
			       "(can-proto-%d) failed.\n", protocol);

		cp = can_get_proto(protocol);
	}
#endif

	/* check for available protocol and correct usage */

	if (!cp)
		return -EPROTONOSUPPORT;

	if (cp->type != sock->type) {
		err = -EPROTOTYPE;
		goto errout;
	}

	sock->ops = cp->ops;

	sk = sk_alloc(net, PF_CAN, GFP_KERNEL, cp->prot, kern);
	if (!sk) {
		err = -ENOMEM;
		goto errout;
	}

	sock_init_data(sock, sk);
	sk->sk_destruct = can_sock_destruct;

	if (sk->sk_prot->init)
		err = sk->sk_prot->init(sk);

	if (err) {
		/* release sk on errors */
		sock_orphan(sk);
		sock_put(sk);
	}

 errout:
	can_put_proto(cp);
	return err;
}

/*
 * af_can tx path
 */

/**
 * can_send - transmit a CAN frame (optional with local loopback)
 * @skb: pointer to socket buffer with CAN frame in data section
 * @loop: loopback for listeners on local CAN sockets (recommended default!)
 *
 * Due to the loopback this routine must not be called from hardirq context.
/* can_send - CAN 프레임 전송 (로컬 루프백은 옵션) @skb : 데이터 섹션의 CAN 프레임을 사용하는 소켓 버퍼 포인터
 @loop : 로컬 CAN 소켓의 리스너를위한 루프백 (권장 기본값!) 
루프백으로 인해이 루틴은 hardirq 컨텍스트에서 호출됩니다.
*/
/*
 *
 * Return:
 *  0 on success
 *  -ENETDOWN when the selected interface is down
 *  -ENOBUFS on full driver queue (see net_xmit_errno())
 *  -ENOMEM when local loopback failed at calling skb_clone()
 *  -EPERM when trying to send on a non-CAN interface
 *  -EMSGSIZE CAN frame size is bigger than CAN interface MTU
 *  -EINVAL when the skb->data does not contain a valid CAN frame
 */
/*선택한 인터페이스가 작동하지 않을 때 * -ENETDOWN
  전체 드라이버 대기열에서 * -ENOBUFS (net_xmit_errno () 참조)
  skb_clone () 호출시 로컬 루프백에 실패했을 때 * -ENOMEM
  * -EPERM : 비 CAN 인터페이스에서 보내려고 할 때
  * -EMSGSIZE CAN 프레임 크기가 CAN 인터페이스 MTU보다 큽니다.
  * -INBAL : skb-> data에 유효한 CAN 프레임이 포함되어 있지 않은 경우
  */
int can_send(struct sk_buff *skb, int loop)
{
	struct sk_buff *newskb = NULL;
	struct canfd_frame *cfd = (struct canfd_frame *)skb->data;
	int err = -EINVAL;

	if (skb->len == CAN_MTU) {
		skb->protocol = htons(ETH_P_CAN);
		if (unlikely(cfd->len > CAN_MAX_DLEN))
			goto inval_skb;
	} else if (skb->len == CANFD_MTU) {
		skb->protocol = htons(ETH_P_CANFD);
		if (unlikely(cfd->len > CANFD_MAX_DLEN))
			goto inval_skb;
	} else
		goto inval_skb;

	/*
	 * Make sure the CAN frame can pass the selected CAN netdevice.
	 * As structs can_frame and canfd_frame are similar, we can provide
	 * CAN FD frames to legacy CAN drivers as long as the length is <= 8
	CAN 프레임이 선택된 CAN netdevice를 통과 할 수 있는지 확인하십시오.
	구조체 can_frame과 canfd_frame이 비슷하기 때문에 길이가 8보다 길면 CAN FD 프레임을 레거시 CAN 드라이버에 제공 할 수 있습니다.
	 */
	if (unlikely(skb->len > skb->dev->mtu && cfd->len > CAN_MAX_DLEN)) {
		err = -EMSGSIZE;
		goto inval_skb;
	}

	if (unlikely(skb->dev->type != ARPHRD_CAN)) {
		err = -EPERM;
		goto inval_skb;
	}

	if (unlikely(!(skb->dev->flags & IFF_UP))) {
		err = -ENETDOWN;
		goto inval_skb;
	}

	skb->ip_summed = CHECKSUM_UNNECESSARY;

	skb_reset_mac_header(skb);
	skb_reset_network_header(skb);
	skb_reset_transport_header(skb);

	if (loop) {
		/* local loopback of sent CAN frames */

		/* indication for the CAN driver: do loopback */
		skb->pkt_type = PACKET_LOOPBACK;

		/*
		 * The reference to the originating sock may be required
		 * by the receiving socket to check whether the frame is
		 * its own. Example: can_raw sockopt CAN_RAW_RECV_OWN_MSGS
		 * Therefore we have to ensure that skb->sk remains the
		 * reference to the originating sock by restoring skb->sk
		 * after each skb_clone() or skb_orphan() usage.
		원래 sock에 대한 언급이 필요할 수 있습니다.
		수신 소켓에 의해 그 프레임이 자신의 것인지를 검사한다. 
		예 : can_raw sockopt CAN_RAW_RECV_OWN_MSGS 따라서 skb-> sk는 각 skb_clone () 또는 skb_orphan () 사용 후에 skb-> sk를 복원하여 원래 양말에 대한 참조로 남아 있어야합니다.
		 */

		if (!(skb->dev->flags & IFF_ECHO)) {
			/*
			 * If the interface is not capable to do loopback
			 * itself, we do it here.
			 */
			newskb = skb_clone(skb, GFP_ATOMIC);
			if (!newskb) {
				kfree_skb(skb);
				return -ENOMEM;
			}

			can_skb_set_owner(newskb, skb->sk);
			newskb->ip_summed = CHECKSUM_UNNECESSARY;
			newskb->pkt_type = PACKET_BROADCAST;
		}
	} else {
		/* indication for the CAN driver: no loopback required */
		skb->pkt_type = PACKET_HOST;
	}

	/* send to netdevice */
	err = dev_queue_xmit(skb);
	if (err > 0)
		err = net_xmit_errno(err);

	if (err) {
		kfree_skb(newskb);
		return err;
	}

	if (newskb)
		netif_rx_ni(newskb);

	/* update statistics */
	can_stats.tx_frames++;
	can_stats.tx_frames_delta++;

	return 0;

inval_skb:
	kfree_skb(skb);
	return err;
}
EXPORT_SYMBOL(can_send);

/*
 * af_can rx path
 */

static struct dev_rcv_lists *find_dev_rcv_lists(struct net_device *dev)
{
	if (!dev)
		return &can_rx_alldev_list;
	else
		return (struct dev_rcv_lists *)dev->ml_priv;
}

/**
 * effhash - hash function for 29 bit CAN identifier reduction
 * @can_id: 29 bit CAN identifier
 *
 * Description:
 *  To reduce the linear traversal in one linked list of _single_ EFF CAN
 *  frame subscriptions the 29 bit identifier is mapped to 10 bits.
 *  (see CAN_EFF_RCV_HASH_BITS definition)
 *
 * Return:
 *  Hash value from 0x000 - 0x3FF ( enforced by CAN_EFF_RCV_HASH_BITS mask )
 effhash - 29 비트 CAN 식별자 감소를위한 해시 함수 @can_id : 29 비트 CAN 식별자
 설명 :
 _single_ EFF CAN 프레임 가입의 링크 된 목록에서 선형 순회를 줄이기 위해 29 비트 식별자는 10 비트로 매핑됩니다.
 (CAN_EFF_RCV_HASH_BITS 정의 참조)
 0x000 - 0x3FF의 해시 값 (마스크가 적용되는 CAN_EFF_RCV_HASH_BITS 개)
 */
static unsigned int effhash(canid_t can_id)
{
	unsigned int hash;

	hash = can_id;
	hash ^= can_id >> CAN_EFF_RCV_HASH_BITS;
	hash ^= can_id >> (2 * CAN_EFF_RCV_HASH_BITS);

	return hash & ((1 << CAN_EFF_RCV_HASH_BITS) - 1);
}

/**
 * find_rcv_list - determine optimal filterlist inside device filter struct
 * @can_id: pointer to CAN identifier of a given can_filter
 * @mask: pointer to CAN mask of a given can_filter
 * @d: pointer to the device filter struct
 *
 * Description:
 *  Returns the optimal filterlist to reduce the filter handling in the
 *  receive path. This function is called by service functions that need
 *  to register or unregister a can_filter in the filter lists.
 *
 *  A filter matches in general, when
 *
 *          <received_can_id> & mask == can_id & mask
 *
 *  so every bit set in the mask (even CAN_EFF_FLAG, CAN_RTR_FLAG) describe
 *  relevant bits for the filter.
 *
 *  The filter can be inverted (CAN_INV_FILTER bit set in can_id) or it can
 *  filter for error messages (CAN_ERR_FLAG bit set in mask). For error msg
 *  frames there is a special filterlist and a special rx path filter handling.
 *
 * Return:
 *  Pointer to optimal filterlist for the given can_id/mask pair.
 *  Constistency checked mask.
 *  Reduced can_id to have a preprocessed filter compare value.

 find_rcv_list - 장치 필터 구조체 내에서 최적의 필터 목록 결정
 @can_id : 주어진 can_filter의 CAN 식별자에 대한 포인터
 @mask : 주어진 can_filter의 CAN 마스크에 대한 포인터
 @d : 장치 필터 구조체에 대한 포인터
 설명 :
 수신 경로에서 필터 처리를 줄이기 위해 최적의 필터 목록을 반환합니다. 이 함수는 필터 목록에서 can_filter를 등록 또는 등록 해제해야하는 서비스 함수에 의해 호출됩니다.
 일반적으로 필터가 일치합니다.
 
 <received_can_id> & 마스크 == can_id & 마스크
 
 마스크에 설정된 모든 비트 (심지어 CAN_EFF_FLAG, CAN_RTR_FLAG)는 필터 관련 비트를 설명합니다.
 
 필터는 반전되거나 (CAN_INV_FILTER 비트는 can_id에 설정 됨) 오류 메시지 (CAN_ERR_FLAG 비트는 마스크에서 설정 됨)를 필터링 할 수 있습니다. 오류 msg 프레임의 경우 특수 필터 목록과 특수한 rx 경로 필터 처리가 있습니다.
 
 
 주어진 can_id / 마스크 쌍에 대한 최적의 필터 목록을 가리키는 포인터.
 일관성 검사 마스크.
 사전 처리 된 필터 비교 값을 갖도록 can_id가 감소되었습니다.
 */
static struct hlist_head *find_rcv_list(canid_t *can_id, canid_t *mask,
					struct dev_rcv_lists *d)
{
	canid_t inv = *can_id & CAN_INV_FILTER; /* save flag before masking */

	/* filter for error message frames in extra filterlist */
	if (*mask & CAN_ERR_FLAG) {
		/* clear CAN_ERR_FLAG in filter entry */
		*mask &= CAN_ERR_MASK;
		return &d->rx[RX_ERR];
	}

	/* with cleared CAN_ERR_FLAG we have a simple mask/value filterpair */

#define CAN_EFF_RTR_FLAGS (CAN_EFF_FLAG | CAN_RTR_FLAG)

	/* ensure valid values in can_mask for 'SFF only' frame filtering */
	if ((*mask & CAN_EFF_FLAG) && !(*can_id & CAN_EFF_FLAG))
		*mask &= (CAN_SFF_MASK | CAN_EFF_RTR_FLAGS);

	/* reduce condition testing at receive time */
	*can_id &= *mask;

	/* inverse can_id/can_mask filter */
	if (inv)
		return &d->rx[RX_INV];

	/* mask == 0 => no condition testing at receive time */
	if (!(*mask))
		return &d->rx[RX_ALL];

	/* extra filterlists for the subscription of a single non-RTR can_id */
	if (((*mask & CAN_EFF_RTR_FLAGS) == CAN_EFF_RTR_FLAGS) &&
	    !(*can_id & CAN_RTR_FLAG)) {

		if (*can_id & CAN_EFF_FLAG) {
			if (*mask == (CAN_EFF_MASK | CAN_EFF_RTR_FLAGS))
				return &d->rx_eff[effhash(*can_id)];
		} else {
			if (*mask == (CAN_SFF_MASK | CAN_EFF_RTR_FLAGS))
				return &d->rx_sff[*can_id];
		}
	}

	/* default: filter via can_id/can_mask */
	return &d->rx[RX_FIL];
}

/**
 * can_rx_register - subscribe CAN frames from a specific interface
 * @dev: pointer to netdevice (NULL => subcribe from 'all' CAN devices list)
 * @can_id: CAN identifier (see description)
 * @mask: CAN mask (see description)
 * @func: callback function on filter match
 * @data: returned parameter for callback function
 * @ident: string for calling module identification
 *
 * Description:
 *  Invokes the callback function with the received sk_buff and the given
 *  parameter 'data' on a matching receive filter. A filter matches, when
 *
 *          <received_can_id> & mask == can_id & mask
 *
 *  The filter can be inverted (CAN_INV_FILTER bit set in can_id) or it can
 *  filter for error message frames (CAN_ERR_FLAG bit set in mask).
 *
 *  The provided pointer to the sk_buff is guaranteed to be valid as long as
 *  the callback function is running. The callback function must *not* free
 *  the given sk_buff while processing it's task. When the given sk_buff is
 *  needed after the end of the callback function it must be cloned inside
 *  the callback function with skb_clone().
 *
 * Return:
 *  0 on success
 *  -ENOMEM on missing cache mem to create subscription entry
 *  -ENODEV unknown device
 * can_rx_register - 특정 인터페이스에서 CAN 프레임을 구독합니다.
 * @dev : netdevice에 대한 포인터 (NULL => '모든'CAN 장치 목록에서 작성)
 * @can_id : CAN 식별자 (설명 참조)
 * @mask : CAN 마스크 (설명 참조)
 * @func : 필터 일치에 대한 콜백 함수
 * @ 데이터 : 콜백 함수에 대한 반환 된 매개 변수
 * @ident : 호출 모듈 식별을위한 문자열
 * 설명 :
 * 일치하는 수신 필터에서 수신 된 sk_buff 및 주어진 매개 변수 'data'를 사용하여 콜백 함수를 호출합니다. 		<received_can_id> & mask == can_id & mask 일 때 필터가 일치합니다.
 *
 * 필터는 반전되거나 (CAN_INV_FILTER 비트가 can_id에 설정 됨) 오류 메시지 프레임 (CAN_ERR_FLAG 비트가 마스크로 설정된)을 필터링 할 수 있습니다.
 *
 * sk_buff에 제공된 포인터는 콜백 함수가 실행되는 한 유효한 것으로 보장됩니다. 콜백 함수는 작업을 처리하는 동안 주어진 sk_buff를 해제하지 않아야합니다. 주어진 sk_buff가 콜백 함수의 끝 후에 필요하면 skb_clone ()을 사용하여 콜백 함수 내에서 복제되어야합니다.
 *
 * 귀국 :
 * 성공시 0
 * 누락 된 캐시 mem에 대한 -ENOMEM을 사용하여 가입 항목을 작성하십시오.
 * -ENODEV 알 수없는 장치
 *
 */
int can_rx_register(struct net_device *dev, canid_t can_id, canid_t mask,
		    void (*func)(struct sk_buff *, void *), void *data,
		    char *ident)
{
	struct receiver *r;
	struct hlist_head *rl;
	struct dev_rcv_lists *d;
	int err = 0;

	/* insert new receiver  (dev,canid,mask) -> (func,data) */

	if (dev && dev->type != ARPHRD_CAN)
		return -ENODEV;

	r = kmem_cache_alloc(rcv_cache, GFP_KERNEL);
	if (!r)
		return -ENOMEM;

	spin_lock(&can_rcvlists_lock);

	d = find_dev_rcv_lists(dev);
	if (d) {
		rl = find_rcv_list(&can_id, &mask, d);

		r->can_id  = can_id;
		r->mask    = mask;
		r->matches = 0;
		r->func    = func;
		r->data    = data;
		r->ident   = ident;

		hlist_add_head_rcu(&r->list, rl);
		d->entries++;

		can_pstats.rcv_entries++;
		if (can_pstats.rcv_entries_max < can_pstats.rcv_entries)
			can_pstats.rcv_entries_max = can_pstats.rcv_entries;
	} else {
		kmem_cache_free(rcv_cache, r);
		err = -ENODEV;
	}

	spin_unlock(&can_rcvlists_lock);

	return err;
}
EXPORT_SYMBOL(can_rx_register);

/*
 * can_rx_delete_receiver - rcu callback for single receiver entry removal
   can_rx_delete_receiver - 단일 수신자 항목 제거를위한 rcu 콜백
 */
static void can_rx_delete_receiver(struct rcu_head *rp)
{
	struct receiver *r = container_of(rp, struct receiver, rcu);

	kmem_cache_free(rcv_cache, r);
}

/**
 * can_rx_unregister - unsubscribe CAN frames from a specific interface
 * @dev: pointer to netdevice (NULL => unsubscribe from 'all' CAN devices list)
 * @can_id: CAN identifier
 * @mask: CAN mask
 * @func: callback function on filter match
 * @data: returned parameter for callback function
 *
 * Description:
 *  Removes subscription entry depending on given (subscription) values.
  * can_rx_unregister - 특정 인터페이스에서 CAN 프레임을 수신 거부합니다.
  * @dev : netdevice에 대한 포인터 (NULL => '모든'CAN 장치 목록에서 수신 거부)
  * @can_id : CAN 식별자
  * @ mask : CAN 마스크
  * @func : 필터 일치에 대한 콜백 함수
  * @ 데이터 : 콜백 함수에 대한 반환 된 매개 변수
  *
  * 설명 :
  * 주어진 (구독) 값에 따라 구독 항목을 제거합니다.
 */
void can_rx_unregister(struct net_device *dev, canid_t can_id, canid_t mask,
		       void (*func)(struct sk_buff *, void *), void *data)
{
	struct receiver *r = NULL;
	struct hlist_head *rl;
	struct dev_rcv_lists *d;

	if (dev && dev->type != ARPHRD_CAN)
		return;

	spin_lock(&can_rcvlists_lock);

	d = find_dev_rcv_lists(dev);
	if (!d) {
		pr_err("BUG: receive list not found for "
		       "dev %s, id %03X, mask %03X\n",
		       DNAME(dev), can_id, mask);
		goto out;
	}

	rl = find_rcv_list(&can_id, &mask, d);

	/*
	 * Search the receiver list for the item to delete.  This should
	 * exist, since no receiver may be unregistered that hasn't
	 * been registered before.
	수신자 목록에서 삭제할 항목을 검색하십시오. 
	이전에 등록되지 않은 수신자가 등록 취소 될 수 없기 때문에 이것은 존재해야합니다.
	 */

	hlist_for_each_entry_rcu(r, rl, list) {
		if (r->can_id == can_id && r->mask == mask &&
		    r->func == func && r->data == data)
			break;
	}

	/*
	 * Check for bugs in CAN protocol implementations using af_can.c:
	 * 'r' will be NULL if no matching list item was found for removal.
	af_can.c를 사용하여 CAN 프로토콜 구현에서 버그를 확인하십시오. 일치하는 목록 항목이 제거되지 않으면 'r'이 NULL입니다.
	 */

	if (!r) {
		WARN(1, "BUG: receive list entry not found for dev %s, "
		     "id %03X, mask %03X\n", DNAME(dev), can_id, mask);
		goto out;
	}

	hlist_del_rcu(&r->list);
	d->entries--;

	if (can_pstats.rcv_entries > 0)
		can_pstats.rcv_entries--;

	/* remove device structure requested by NETDEV_UNREGISTER */
	if (d->remove_on_zero_entries && !d->entries) {
		kfree(d);
		dev->ml_priv = NULL;
	}

 out:
	spin_unlock(&can_rcvlists_lock);

	/* schedule the receiver item for deletion */
	if (r)
		call_rcu(&r->rcu, can_rx_delete_receiver);
}
EXPORT_SYMBOL(can_rx_unregister);

static inline void deliver(struct sk_buff *skb, struct receiver *r)
{
	r->func(skb, r->data);
	r->matches++;
}

static int can_rcv_filter(struct dev_rcv_lists *d, struct sk_buff *skb)
{
	struct receiver *r;
	int matches = 0;
	struct can_frame *cf = (struct can_frame *)skb->data;
	canid_t can_id = cf->can_id;

	if (d->entries == 0)
		return 0;

	if (can_id & CAN_ERR_FLAG) {
		/* check for error message frame entries only */
		hlist_for_each_entry_rcu(r, &d->rx[RX_ERR], list) {
			if (can_id & r->mask) {
				deliver(skb, r);
				matches++;
			}
		}
		return matches;
	}

	/* check for unfiltered entries */
	hlist_for_each_entry_rcu(r, &d->rx[RX_ALL], list) {
		deliver(skb, r);
		matches++;
	}

	/* check for can_id/mask entries */
	hlist_for_each_entry_rcu(r, &d->rx[RX_FIL], list) {
		if ((can_id & r->mask) == r->can_id) {
			deliver(skb, r);
			matches++;
		}
	}

	/* check for inverted can_id/mask entries */
	hlist_for_each_entry_rcu(r, &d->rx[RX_INV], list) {
		if ((can_id & r->mask) != r->can_id) {
			deliver(skb, r);
			matches++;
		}
	}

	/* check filterlists for single non-RTR can_ids */
	if (can_id & CAN_RTR_FLAG)
		return matches;

	if (can_id & CAN_EFF_FLAG) {
		hlist_for_each_entry_rcu(r, &d->rx_eff[effhash(can_id)], list) {
			if (r->can_id == can_id) {
				deliver(skb, r);
				matches++;
			}
		}
	} else {
		can_id &= CAN_SFF_MASK;
		hlist_for_each_entry_rcu(r, &d->rx_sff[can_id], list) {
			deliver(skb, r);
			matches++;
		}
	}

	return matches;
}

static void can_receive(struct sk_buff *skb, struct net_device *dev)
{
	struct dev_rcv_lists *d;
	int matches;

	/* update statistics */
	can_stats.rx_frames++;
	can_stats.rx_frames_delta++;

	/* create non-zero unique skb identifier together with *skb */
	while (!(can_skb_prv(skb)->skbcnt))
		can_skb_prv(skb)->skbcnt = atomic_inc_return(&skbcounter);

	rcu_read_lock();

	/* deliver the packet to sockets listening on all devices */
	matches = can_rcv_filter(&can_rx_alldev_list, skb);

	/* find receive list for this device */
	d = find_dev_rcv_lists(dev);
	if (d)
		matches += can_rcv_filter(d, skb);

	rcu_read_unlock();

	/* consume the skbuff allocated by the netdevice driver */
	consume_skb(skb);

	if (matches > 0) {
		can_stats.matches++;
		can_stats.matches_delta++;
	}
}

static int can_rcv(struct sk_buff *skb, struct net_device *dev,
		   struct packet_type *pt, struct net_device *orig_dev)
{
	struct canfd_frame *cfd = (struct canfd_frame *)skb->data;

	if (unlikely(!net_eq(dev_net(dev), &init_net)))
		goto drop;

	if (WARN_ONCE(dev->type != ARPHRD_CAN ||
		      skb->len != CAN_MTU ||
		      cfd->len > CAN_MAX_DLEN,
		      "PF_CAN: dropped non conform CAN skbuf: "
		      "dev type %d, len %d, datalen %d\n",
		      dev->type, skb->len, cfd->len))
		goto drop;

	can_receive(skb, dev);
	return NET_RX_SUCCESS;

drop:
	kfree_skb(skb);
	return NET_RX_DROP;
}

static int canfd_rcv(struct sk_buff *skb, struct net_device *dev,
		   struct packet_type *pt, struct net_device *orig_dev)
{
	struct canfd_frame *cfd = (struct canfd_frame *)skb->data;

	if (unlikely(!net_eq(dev_net(dev), &init_net)))
		goto drop;

	if (WARN_ONCE(dev->type != ARPHRD_CAN ||
		      skb->len != CANFD_MTU ||
		      cfd->len > CANFD_MAX_DLEN,
		      "PF_CAN: dropped non conform CAN FD skbuf: "
		      "dev type %d, len %d, datalen %d\n",
		      dev->type, skb->len, cfd->len))
		goto drop;

	can_receive(skb, dev);
	return NET_RX_SUCCESS;

drop:
	kfree_skb(skb);
	return NET_RX_DROP;
}

/*
 * af_can protocol functions
 */

/**
 * can_proto_register - register CAN transport protocol
 * @cp: pointer to CAN protocol structure
 *
 * Return:
 *  0 on success
 *  -EINVAL invalid (out of range) protocol number
 *  -EBUSY  protocol already in use
 *  -ENOBUF if proto_register() fails
  * can_proto_register - CAN 전송 프로토콜 등록
  * @cp : CAN 프로토콜 구조에 대한 포인터
  *
  * Return :
  * 성공시 0
  * -INVAL 유효하지 않은 (범위를 벗어난) 프로토콜 번호
  * -EBUSY 프로토콜이 이미 사용 중입니다.
  * proto_register ()가 실패하면  -ENOBUF
 */
int can_proto_register(const struct can_proto *cp)
{
	int proto = cp->protocol;
	int err = 0;

	if (proto < 0 || proto >= CAN_NPROTO) {
		pr_err("can: protocol number %d out of range\n", proto);
		return -EINVAL;
	}

	err = proto_register(cp->prot, 0);
	if (err < 0)
		return err;

	mutex_lock(&proto_tab_lock);

	if (proto_tab[proto]) {
		pr_err("can: protocol %d already registered\n", proto);
		err = -EBUSY;
	} else
		RCU_INIT_POINTER(proto_tab[proto], cp);

	mutex_unlock(&proto_tab_lock);

	if (err < 0)
		proto_unregister(cp->prot);

	return err;
}
EXPORT_SYMBOL(can_proto_register);

/**
 * can_proto_unregister - unregister CAN transport protocol
 * @cp: pointer to CAN protocol structure
  can_proto_unregister - CAN 전송 프로토콜의 등록을 취소합니다.
  @cp : CAN 프로토콜 구조체에 대한 포인터
 */
void can_proto_unregister(const struct can_proto *cp)
{
	int proto = cp->protocol;

	mutex_lock(&proto_tab_lock);
	BUG_ON(proto_tab[proto] != cp);
	RCU_INIT_POINTER(proto_tab[proto], NULL);
	mutex_unlock(&proto_tab_lock);

	synchronize_rcu();

	proto_unregister(cp->prot);
}
EXPORT_SYMBOL(can_proto_unregister);

/*
 * af_can notifier to create/remove CAN netdevice specific structs
   af_can 알리미는 CAN netdevice 특정 구조체를 생성 / 제거합니다.
 */
static int can_notifier(struct notifier_block *nb, unsigned long msg,
			void *ptr)
{
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);
	struct dev_rcv_lists *d;

	if (!net_eq(dev_net(dev), &init_net))
		return NOTIFY_DONE;

	if (dev->type != ARPHRD_CAN)
		return NOTIFY_DONE;

	switch (msg) {

	case NETDEV_REGISTER:

		/* create new dev_rcv_lists for this device */
		d = kzalloc(sizeof(*d), GFP_KERNEL);
		if (!d)
			return NOTIFY_DONE;
		BUG_ON(dev->ml_priv);
		dev->ml_priv = d;

		break;

	case NETDEV_UNREGISTER:
		spin_lock(&can_rcvlists_lock);

		d = dev->ml_priv;
		if (d) {
			if (d->entries)
				d->remove_on_zero_entries = 1;
			else {
				kfree(d);
				dev->ml_priv = NULL;
			}
		} else
			pr_err("can: notifier: receive list not found for dev "
			       "%s\n", dev->name);

		spin_unlock(&can_rcvlists_lock);

		break;
	}

	return NOTIFY_DONE;
}

/*
 * af_can module init/exit functions
 */

static struct packet_type can_packet __read_mostly = {
	.type = cpu_to_be16(ETH_P_CAN),
	.func = can_rcv,
};

static struct packet_type canfd_packet __read_mostly = {
	.type = cpu_to_be16(ETH_P_CANFD),
	.func = canfd_rcv,
};

static const struct net_proto_family can_family_ops = {
	.family = PF_CAN,
	.create = can_create,
	.owner  = THIS_MODULE,
};

/* notifier block for netdevice event */
static struct notifier_block can_netdev_notifier __read_mostly = {
	.notifier_call = can_notifier,
};

static __init int can_init(void)
{
	/* check for correct padding to be able to use the structs similarly
        	마찬가지로 스트럿을 사용할 수 있도록 올바른 패딩을 확인하십시오. */
	BUILD_BUG_ON(offsetof(struct can_frame, can_dlc) !=
		     offsetof(struct canfd_frame, len) ||
		     offsetof(struct can_frame, data) !=
		     offsetof(struct canfd_frame, data));

	pr_info("can: controller area network core (" CAN_VERSION_STRING ")\n");

	memset(&can_rx_alldev_list, 0, sizeof(can_rx_alldev_list));

	rcv_cache = kmem_cache_create("can_receiver", sizeof(struct receiver),
				      0, 0, NULL);
	if (!rcv_cache)
		return -ENOMEM;

	if (stats_timer) {
		/* the statistics are updated every second (timer triggered) 
			통계는 매 초마다 업데이트됩니다 (타이머가 트리거 됨)*/
		setup_timer(&can_stattimer, can_stat_update, 0);
		mod_timer(&can_stattimer, round_jiffies(jiffies + HZ));
	} else
		can_stattimer.function = NULL;

	can_init_proc();

	/* protocol register */
	sock_register(&can_family_ops);
	register_netdevice_notifier(&can_netdev_notifier);
	dev_add_pack(&can_packet);
	dev_add_pack(&canfd_packet);

	return 0;
}

static __exit void can_exit(void)
{
	struct net_device *dev;

	if (stats_timer)
		del_timer_sync(&can_stattimer);

	can_remove_proc();

	/* protocol unregister */
	dev_remove_pack(&canfd_packet);
	dev_remove_pack(&can_packet);
	unregister_netdevice_notifier(&can_netdev_notifier);
	sock_unregister(PF_CAN);

	/* remove created dev_rcv_lists from still registered CAN devices
		아직 등록 된 CAN 장치에서 생성 된 dev_rcv_lists를 제거합니다. */
	rcu_read_lock();
	for_each_netdev_rcu(&init_net, dev) {
		if (dev->type == ARPHRD_CAN && dev->ml_priv) {

			struct dev_rcv_lists *d = dev->ml_priv;

			BUG_ON(d->entries);
			kfree(d);
			dev->ml_priv = NULL;
		}
	}
	rcu_read_unlock();

	rcu_barrier(); /* Wait for completion of call_rcu()'s
			call_rcu ()의 완료를 기다립니다. */

	kmem_cache_destroy(rcv_cache);
}

module_init(can_init);
module_exit(can_exit);
