/*
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
Copyright (c) 2002-2007 Volkswagen Group 전자 연구 판권 소유.

 다음 조건이 충족되는 경우 수정 여부에 관계없이 원본 및 이진 형식의 재배포 및 사용이 허용됩니다.
 1. 소스 코드 재배포시 위의 저작권 표시,이 조건 목록 및 다음 책임 제한을 유지해야합니다.
 2. 바이너리 형식의 재배포는 위의 저작권 표시, 본 조건 목록 및 다음 면책 조항을 배포시 제공된 설명서 및 / 또는 기타 자료에 포함시켜야합니다.
 3. 사전 서면 허가 없이는이 소프트웨어에서 파생 된 제품을 보증하거나 홍보하기 위해 폭스 바겐 이름이나 그 기여자 이름을 사용할 수 없습니다.

 또는이 통지문이 전액 보존되어있는 경우,이 소프트웨어는 GNU 일반 공중 사용 허가서 (GPL) 버전 2의 조건에 따라 배포 될 수 있습니다.
 이 경우 GPL 규정은 위에 제시된 조항 대신 적용됩니다.

 이 코드의 제공된 데이터 구조와 외부 인터페이스는 GPL 호환 라이센스가있는 모듈에서 사용하도록 제한되지 않습니다.

 이 소프트웨어는 저작권 보유자 및 기여자가 제공합니다.
 "있는 그대로"제공되며 상품성 및 특정 목적에의 적합성에 대한 묵시적인 보증을 포함하여 (단, 이에 한하지 않음) 명시 적 또는 묵시적인 일체의 보증은 부인합니다. 
 저작권 소유자 또는 기여자는 직접, 간접적 인, 우발적 인, 우발적 인, 간접적 인 또는 필연적 인 손해 (대체 제품 또는 서비스의 조달, 사용 손실, 데이터 손실 또는 이익 손실을 포함하되 이에 국한되지 않음) 또는 비즈니스 중단)에 대한
 책임을지지 않습니다. 계약 위반, 엄격한 책임 또는 불법 행위 (과실 포함 또는 기타 포함)에 관계없이이 소프트웨어의 사용으로 인해 발생하는 모든 책임은 해당 손해의 가능성을 사전에 알고 있던 경우에도 마찬가지입니다.
 */

#ifndef AF_CAN_H
#define AF_CAN_H

#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/list.h>
#include <linux/rcupdate.h>
#include <linux/can.h>

/* af_can rx dispatcher structures 
	af_can rx 발송자 구조 */

struct receiver {
	struct hlist_node list;
	struct rcu_head rcu;
	canid_t can_id;
	canid_t mask;
	unsigned long matches;
	void (*func)(struct sk_buff *, void *);
	void *data;
	char *ident;
};

#define CAN_SFF_RCV_ARRAY_SZ (1 << CAN_SFF_ID_BITS)
#define CAN_EFF_RCV_HASH_BITS 10
#define CAN_EFF_RCV_ARRAY_SZ (1 << CAN_EFF_RCV_HASH_BITS)

enum { RX_ERR, RX_ALL, RX_FIL, RX_INV, RX_MAX };

/* per device receive filters linked at dev->ml_priv 
   장치 당 수신 필터는 dev-> ml_priv */
struct dev_rcv_lists {
	struct hlist_head rx[RX_MAX];
	struct hlist_head rx_sff[CAN_SFF_RCV_ARRAY_SZ];
	struct hlist_head rx_eff[CAN_EFF_RCV_ARRAY_SZ];
	int remove_on_zero_entries;
	int entries;
};

/* statistic structures */

/* can be reset e.g. by can_init_stats() */
struct s_stats {
	unsigned long jiffies_init;

	unsigned long rx_frames;
	unsigned long tx_frames;
	unsigned long matches;

	unsigned long total_rx_rate;
	unsigned long total_tx_rate;
	unsigned long total_rx_match_ratio;

	unsigned long current_rx_rate;
	unsigned long current_tx_rate;
	unsigned long current_rx_match_ratio;

	unsigned long max_rx_rate;
	unsigned long max_tx_rate;
	unsigned long max_rx_match_ratio;

	unsigned long rx_frames_delta;
	unsigned long tx_frames_delta;
	unsigned long matches_delta;
};

/* persistent statistics
	영구 통계 */
struct s_pstats {
	unsigned long stats_reset;
	unsigned long user_reset;
	unsigned long rcv_entries;
	unsigned long rcv_entries_max;
};

/* receive filters subscribed for 'all' CAN devices 
	'모든'CAN 장치에 가입 한 필터를 수신합니다. */
extern struct dev_rcv_lists can_rx_alldev_list;

/* function prototypes for the CAN networklayer procfs (proc.c) 
	CAN 네트워크 레이어를위한 함수 프로토 타입 procfs (proc.c) */
void can_init_proc(void);
void can_remove_proc(void);
void can_stat_update(unsigned long data);

/* structures and variables from af_can.c needed in proc.c for reading
	읽기를 위해 proc.c에 필요한 af_can.c의 구조와 변수  */
extern struct timer_list can_stattimer;    /* timer for statistics update 
						통계 업데이트를 위한 타이머  */
extern struct s_stats    can_stats;        /* packet statistics 
						패킷 통계  */
extern struct s_pstats   can_pstats;       /* receive list statistics 
						리스트 통계를 받습니다.  */
extern struct hlist_head can_rx_dev_list;  /* rx dispatcher structures 
						rx 디스패처 구조체  */

#endif /* AF_CAN_H */
