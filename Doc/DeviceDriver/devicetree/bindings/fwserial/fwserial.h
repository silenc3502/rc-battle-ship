#ifndef _FIREWIRE_FWSERIAL_H
#define _FIREWIRE_FWSERIAL_H

#include <linux/kernel.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/list.h>
#include <linux/firewire.h>
#include <linux/firewire-constants.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>
#include <linux/mutex.h>
#include <linux/serial.h>
#include <linux/serial_reg.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>

#include "dma_fifo.h"

#ifdef FWTTY_PROFILING
#define DISTRIBUTION_MAX_SIZE     8192
#define DISTRIBUTION_MAX_INDEX    (ilog2(DISTRIBUTION_MAX_SIZE) + 1)
static inline void fwtty_profile_data(unsigned stat[], unsigned val)
{
	int n = (val) ? min(ilog2(val) + 1, DISTRIBUTION_MAX_INDEX) : 0;
	++stat[n];
}
#else
#define DISTRIBUTION_MAX_INDEX    0
#define fwtty_profile_data(st, n)
#endif

/* Parameters for both VIRT_CABLE_PLUG & VIRT_CABLE_PLUG_RSP mgmt codes
   VIRT_CABLE_PLUG & VIRT_CABLE_PLUG_RSP mgmt 코드의 매개 변수 */
struct virt_plug_params {
	__be32  status_hi;
	__be32  status_lo;
	__be32	fifo_hi;
	__be32	fifo_lo;
	__be32	fifo_len;
};

struct peer_work_params {
	union {
		struct virt_plug_params plug_req;
	};
};

/**
 * fwtty_peer: structure representing local & remote unit devices
 * @unit: unit child device of fw_device node
 * @serial: back pointer to associated fw_serial aggregate
 * @guid: unique 64-bit guid for this unit device
 * @generation: most recent bus generation
 * @node_id: most recent node_id
 * @speed: link speed of peer (0 = S100, 2 = S400, ... 5 = S3200)
 * @mgmt_addr: bus addr region to write mgmt packets to
 * @status_addr: bus addr register to write line status to
 * @fifo_addr: bus addr region to write serial output to
 * @fifo_len:  max length for single write to fifo_addr
 * @list: link for insertion into fw_serial's peer_list
 * @rcu: for deferring peer reclamation
 * @lock: spinlock to synchonize changes to state & port fields
 * @work: only one work item can be queued at any one time
 *        Note: pending work is canceled prior to removal, so this
 *        peer is valid for at least the lifetime of the work function
 * @work_params: parameter block for work functions
 * @timer: timer for resetting peer state if remote request times out
 * @state: current state
 * @connect: work item for auto-connecting
 * @connect_retries: # of connections already attempted
 * @port: associated tty_port (usable if state == FWSC_ATTACHED)

 * fwtty_peer : 로컬 및 원격 단위 장치를 나타내는 구조체
 * @unit : fw_device 노드의 자식 단위 장치
 * @serial : 연관된 fw_serial aggregate에 대한 포인터
 * @guid :이 장치 장치에 대한 고유 한 64 비트 GUID
 * @ 세대 : 가장 최근의 버스 생성
 * @node_id : 가장 최근의 node_id
 * @speed : 피어의 링크 속도 (0 = S100, 2 = S400, ... 5 = S3200)
 * @ mgmt_addr : mgmt 패킷을 기록 할 버스 주소 영역
 * @status_addr : 버스 상태를 기록하기위한 addr 레지스터
 * @fifo_addr : 직렬 출력을 쓸 버스 addr 영역
 * @fifo_len : fifo_addr에 대한 단일 쓰기의 최대 길이
 * @list : fw_serial의 peer_list에 삽입하기위한 링크
 * @rcu : 피어 재생 지연
 * @lock : 상태 및 포트 필드의 변경 사항을 동기화하는 스핀 록
 * @work : 한 번에 하나의 작업 항목 만 대기시킬 수 있습니다.
 * 참고 : 대기중인 작업은 제거하기 전에 취소되므로이 작업이 취소됩니다.
 * 피어는 최소한 업무 기능의 유효 기간 동안 유효합니다.
 * @work_params : 업무 기능을위한 매개 변수 블록
 * @timer : 원격 요청 시간이 초과 된 경우 피어 상태를 재설정하기위한 타이머
 * @state : 현재 상태
 * @connect : 자동 연결을위한 작업 항목
 * @connect_retries : 이미 시도한 연결 수
 * @port : 연관된 tty_port (상태 == FWSC_ATTACHED 인 경우 사용할 수 있음)
 */
struct fwtty_peer {
	struct fw_unit		*unit;
	struct fw_serial	*serial;
	u64			guid;
	int			generation;
	int			node_id;
	unsigned		speed;
	int			max_payload;
	u64			mgmt_addr;

	/* these are usable only if state == FWSC_ATTACHED
	   상태 == FWSC_ATTACHED 인 경우에만 사용할 수 있습니다. */
	u64			status_addr;
	u64			fifo_addr;
	int			fifo_len;

	struct list_head	list;
	struct rcu_head		rcu;

	spinlock_t		lock;
	work_func_t		workfn;
	struct work_struct	work;
	struct peer_work_params work_params;
	struct timer_list	timer;
	int			state;
	struct delayed_work	connect;
	int			connect_retries;

	struct fwtty_port	*port;
};

#define to_peer(ptr, field)	(container_of(ptr, struct fwtty_peer, field))

/* state values for fwtty_peer.state field
   fwtty_peer.state 필드의 상태 값 */
enum fwtty_peer_state {
	FWPS_GONE,
	FWPS_NOT_ATTACHED,
	FWPS_ATTACHED,
	FWPS_PLUG_PENDING,
	FWPS_PLUG_RESPONDING,
	FWPS_UNPLUG_PENDING,
	FWPS_UNPLUG_RESPONDING,

	FWPS_NO_MGMT_ADDR = -1,
};

#define CONNECT_RETRY_DELAY	HZ
#define MAX_CONNECT_RETRIES	10

/* must be holding peer lock for these state funclets
   이러한 상태 funclets에 대한 peer 잠금을 유지해야합니다. */
static inline void peer_set_state(struct fwtty_peer *peer, int new)
{
	peer->state = new;
}

static inline struct fwtty_port *peer_revert_state(struct fwtty_peer *peer)
{
	struct fwtty_port *port = peer->port;

	peer->port = NULL;
	peer_set_state(peer, FWPS_NOT_ATTACHED);
	return port;
}

struct fwserial_mgmt_pkt {
	struct {
		__be16		len;
		__be16		code;
	} hdr;
	union {
		struct virt_plug_params plug_req;
		struct virt_plug_params plug_rsp;
	};
} __packed;

/* fwserial_mgmt_packet codes */
#define FWSC_RSP_OK			0x0000
#define FWSC_RSP_NACK			0x8000
#define FWSC_CODE_MASK			0x0fff

#define FWSC_VIRT_CABLE_PLUG		1
#define FWSC_VIRT_CABLE_UNPLUG		2
#define FWSC_VIRT_CABLE_PLUG_RSP	3
#define FWSC_VIRT_CABLE_UNPLUG_RSP	4

/* 1 min. plug timeout -- suitable for userland authorization
   1 분. 플러그 타임 아웃 - 유저 랜드 인증에 적합 */
#define VIRT_CABLE_PLUG_TIMEOUT		(60 * HZ)

struct stats {
	unsigned	xchars;
	unsigned	dropped;
	unsigned	tx_stall;
	unsigned	fifo_errs;
	unsigned	sent;
	unsigned	lost;
	unsigned	throttled;
	unsigned	reads[DISTRIBUTION_MAX_INDEX + 1];
	unsigned	writes[DISTRIBUTION_MAX_INDEX + 1];
	unsigned	txns[DISTRIBUTION_MAX_INDEX + 1];
	unsigned	unthrottle[DISTRIBUTION_MAX_INDEX + 1];
};

struct fwconsole_ops {
	void (*notify)(int code, void *data);
	void (*stats)(struct stats *stats, void *data);
	void (*proc_show)(struct seq_file *m, void *data);
};

/* codes for console ops notify */
#define FWCON_NOTIFY_ATTACH		1
#define FWCON_NOTIFY_DETACH		2

/**
 * fwtty_port: structure used to track/represent underlying tty_port
 * @port: underlying tty_port
 * @device: tty device
 * @index: index into port_table for this particular port
 *    note: minor = index + minor_start assigned by tty_alloc_driver()
 * @serial: back pointer to the containing fw_serial
 * @rx_handler: bus address handler for unique addr region used by remotes
 *              to communicate with this port. Every port uses
 *		fwtty_port_handler() for per port transactions.
 * @fwcon_ops: ops for attached fw_console (if any)
 * @con_data: private data for fw_console
 * @wait_tx: waitqueue for sleeping until writer/drain completes tx
 * @emit_breaks: delayed work responsible for generating breaks when the
 *               break line status is active
 * @cps : characters per second computed from the termios settings
 * @break_last: timestamp in jiffies from last emit_breaks
 * @hangup: work responsible for HUPing when carrier is dropped/lost
 * @mstatus: loose virtualization of LSR/MSR
 *         bits 15..0  correspond to TIOCM_* bits
 *         bits 19..16 reserved for mctrl
 *         bit 20      OOB_TX_THROTTLE
 *	   bits 23..21 reserved
 *         bits 31..24 correspond to UART_LSR_* bits
 * @lock: spinlock for protecting concurrent access to fields below it
 * @mctrl: loose virtualization of MCR
 *         bits 15..0  correspond to TIOCM_* bits
 *         bit 16      OOB_RX_THROTTLE
 *         bits 19..17 reserved
 *	   bits 31..20 reserved for mstatus
 * @drain: delayed work scheduled to ensure that writes are flushed.
 *         The work can race with the writer but concurrent sending is
 *         prevented with the IN_TX flag. Scheduled under lock to
 *         limit scheduling when fifo has just been drained.
 * @tx_fifo: fifo used to store & block-up writes for dma to remote
 * @max_payload: max bytes transmissible per dma (based on peer's max_payload)
 * @status_mask: UART_LSR_* bitmask significant to rx (based on termios)
 * @ignore_mask: UART_LSR_* bitmask of states to ignore (also based on termios)
 * @break_ctl: if set, port is 'sending break' to remote
 * @write_only: self-explanatory
 * @overrun: previous rx was lost (partially or completely)
 * @loopback: if set, port is in loopback mode
 * @flags: atomic bit flags
 *         bit 0: IN_TX - gate to allow only one cpu to send from the dma fifo
 *                        at a time.
 *         bit 1: STOP_TX - force tx to exit while sending
 * @peer: rcu-pointer to associated fwtty_peer (if attached)
 *        NULL if no peer attached
 * @icount: predefined statistics reported by the TIOCGICOUNT ioctl
 * @stats: additional statistics reported in /proc/tty/driver/firewire_serial

 * fwtty_port : 밑에있는 tty_port를 추적 / 표시하는 데 사용되는 구조체
 * @port : 기본 tty_port
 * @device : tty 장치
 * @ 인덱스 :이 특정 포트에 대한 port_table에 대한 인덱스
 * 참고 : tty_alloc_driver ()에 의해 할당 된 minor = index + minor_start
 * @serial : fw_serial을 포함하는 포인터를 가리킨다.
 * @rx_handler : 리모트가이 포트와 통신하기 위해 사용하는 유일한 addr 영역을위한 버스 주소 핸들러.
                 모든 포트는 포트 트랜잭션 당 fwtty_port_handler ()를 사용합니다.
 * @fwcon_ops : 연결된 fw_console에 대한 작업 (있는 경우)
 * @con_data : fw_console의 개인 데이터
 * @wait_tx : writer / drain이 tx를 완료 할 때까지 대기하는 waitqueue
 * @emit_breaks : 브레이크 회선 상태가 활성화되었을 때 휴식 시간을 생성하는 지연된 작업
 * @cps : termios 설정에서 계산 된 초당 문자 수
 * @break_last : 마지막 emit_breaks의 jiffies에있는 타임 스탬프
 * @ hangup : 캐리어가 떨어 뜨렸거나 손실되었을 때 HUPing에 책임이있는 일
 * @mstatus : LSR / MSR의 느슨한 가상화
 * 비트 15..0은 TIOCM_ * 비트에 해당합니다.
 * 비트 19.16은 mctrl 용으로 예약되어 있습니다.
 * 비트 20 OOB_TX_THROTTLE
 * 비트 23..21 예약 됨
 * 비트 31..24는 UART_LSR_ * 비트에 해당합니다.
 * @lock : 아래의 필드에 대한 동시 액세스를 보호하기위한 스핀 록
 * @mctrl : MCR의 가상화가 느슨합니다.
 * 비트 15..0은 TIOCM_ * 비트에 해당합니다.
 * 비트 16 OOB_RX_THROTTLE
 * 비트 19..17 예약 됨
 * 비트 31..20은 mstatus 용으로 예약되어 있습니다.
 * @drain : 쓰기가 플러시되도록 예약 된 지연된 작업.
 * 작업은 작성자와 경쟁 할 수 있지만 동시 전송은 IN_TX 플래그로 방지됩니다.
   FIFO가 방금되었을 때 일정을 제한하기 위해 잠금 상태로 예약되었습니다.
 * @tx_fifo : FIFO를 저장하고 원격으로 쓰기위한 FIFO 사용
 * @max_payload : dma 당 전송 가능한 최대 바이트 수 (피어의 max_payload 기준)
 * @status_mask : UART_LSR_ * 중요한 비트를 rx로 (termios 기반)
 * @ignore_mask : UART_LSR_ * 무시할 상태 비트 마스크 (또한 termios 기반)
 * @break_ctl : 설정하면 포트가 원격지로 '휴식을 보내고'있습니다.
 * @write_only : 자명 한
 * @overrun : 이전 rx가 손실되었습니다 (부분적으로 또는 완전히).
 * @ 루프백 : 설정된 경우 포트가 루프백 모드입니다.
 * @ 플래그 : 원자 비트 플래그
 * 비트 0 : IN_TX - 오직 하나의 CPU가 dma fifo에서 보내는 것을 허용하는 게이트
 * 한 번에.
 * 비트 1 : STOP_TX - 보내는 동안 tx가 종료되도록합니다.
 * @peer : 연결된 fwtty_peer에 대한 rcu 포인터 (연결된 경우) 피어가 연결되지 않은 경우 NULL
 * @icount : TIOCGICOUNT ioctl에 의해보고 된 미리 정의 된 통계
 * @stats : / proc / tty / driver / firewire_serial에 추가 통계가보고되었습니다.
 */
struct fwtty_port {
	struct tty_port		   port;
	struct device		   *device;
	unsigned		   index;
	struct fw_serial	   *serial;
	struct fw_address_handler  rx_handler;

	struct fwconsole_ops	   *fwcon_ops;
	void			   *con_data;

	wait_queue_head_t	   wait_tx;
	struct delayed_work	   emit_breaks;
	unsigned		   cps;
	unsigned long		   break_last;

	struct work_struct	   hangup;

	unsigned		   mstatus;

	spinlock_t		   lock;
	unsigned		   mctrl;
	struct delayed_work	   drain;
	struct dma_fifo		   tx_fifo;
	int			   max_payload;
	unsigned		   status_mask;
	unsigned		   ignore_mask;
	unsigned		   break_ctl:1,
				   write_only:1,
				   overrun:1,
				   loopback:1;
	unsigned long		   flags;

	struct fwtty_peer __rcu	   *peer;

	struct async_icount	   icount;
	struct stats		   stats;
};

#define to_port(ptr, field)	(container_of(ptr, struct fwtty_port, field))

/* bit #s for flags field 
    플래그 필드의 비트 수*/
#define IN_TX                      0
#define STOP_TX                    1

/* bitmasks for special mctrl/mstatus bits
   특별 mctrl / mstatus 비트를위한 bitmasks  */
#define OOB_RX_THROTTLE   0x00010000
#define MCTRL_RSRVD       0x000e0000
#define OOB_TX_THROTTLE   0x00100000
#define MSTATUS_RSRVD     0x00e00000

#define MCTRL_MASK        (TIOCM_DTR | TIOCM_RTS | TIOCM_OUT1 | TIOCM_OUT2 | \
			   TIOCM_LOOP | OOB_RX_THROTTLE | MCTRL_RSRVD)

/* XXX even every 1/50th secs. may be unnecessarily accurate */
/* delay in jiffies between brk emits */
/* 1 / 50 초마다 심지어 XXX. 불필요하게 정확할 수 있습니다 */
/* brk 방출 사이의 jiffies 지연 */
#define FREQ_BREAKS        (HZ / 50)

/* Ports are allocated in blocks of num_ports for each fw_card
   포트는 각 fw_card에 대해 num_ports 블록으로 할당됩니다 */
#define MAX_CARD_PORTS           CONFIG_FWTTY_MAX_CARD_PORTS
#define MAX_TOTAL_PORTS          CONFIG_FWTTY_MAX_TOTAL_PORTS

/* tuning parameters
   튜닝 파라미터 */
#define FWTTY_PORT_TXFIFO_LEN	4096
#define FWTTY_PORT_MAX_PEND_DMA    8    /* costs a cache line per pend 
	                      	 	   보류 상태의 캐시 라인 비용	*/
#define DRAIN_THRESHOLD         1024
#define MAX_ASYNC_PAYLOAD       4096    /* ohci-defined limit          
					   ohci 정의 한계		*/
#define WRITER_MINIMUM           128
/* TODO: how to set watermark to AR context size? see fwtty_rx() 
   TODO: 워터 마크를 AR 컨텍스트 크기로 설정하는 방법은 무엇입니까? fwtty_rx () 참조 */
#define HIGH_WATERMARK         32768	/* AR context is 32K	       */

/*
 * Size of bus addr region above 4GB used per port as the recv addr
 * - must be at least as big as the MAX_ASYNC_PAYLOAD
   recv 주소로 사용되는 포트 당 4GB 이상의 버스 주소 영역 크기
   - MAX_ASYNC_PAYLOAD만큼 커야합니다.
 */
#define FWTTY_PORT_RXFIFO_LEN	MAX_ASYNC_PAYLOAD

/**
 * fw_serial: aggregate used to associate tty ports with specific fw_card
 * @card: fw_card associated with this fw_serial device (1:1 association)
 * @kref: reference-counted multi-port management allows delayed destroy
 * @self: local unit device as 'peer'. Not valid until local unit device
 *         is enumerated.
 * @list: link for insertion into fwserial_list
 * @peer_list: list of local & remote unit devices attached to this card
 * @ports: fixed array of tty_ports provided by this serial device
   fw_serial : tty 포트를 특정 fw_card와 연결하는 데 사용되는 집계
   @card :이 fw_serial 장치와 연관된 fw_card (1 : 1 연관)
   @kref : 참조 카운트 된 다중 포트 관리로 인해 지연된 삭제가 가능합니다.
   @self : 로컬 장치를 'peer'로 지정합니다. 로컬 장치가 열거 될 때까지는 유효하지 않습니다.
   @list : fwserial_list에 삽입하기위한 링크
   @peer_list :이 카드에 연결된 로컬 및 원격 유닛 장치 목록
   @ports :이 직렬 장치에서 제공하는 tty_ports의 고정 배열
 */
struct fw_serial {
	struct fw_card	  *card;
	struct kref	  kref;

	struct dentry	  *debugfs;
	struct fwtty_peer *self;

	struct list_head  list;
	struct list_head  peer_list;

	struct fwtty_port *ports[MAX_CARD_PORTS];
};

#define to_serial(ptr, field)	(container_of(ptr, struct fw_serial, field))

#define TTY_DEV_NAME		    "fwtty"	/* ttyFW was taken           */
static const char tty_dev_name[] =  TTY_DEV_NAME;
static const char loop_dev_name[] = "fwloop";

extern struct tty_driver *fwtty_driver;

struct fwtty_port *fwtty_port_get(unsigned index);
void fwtty_port_put(struct fwtty_port *port);

static inline void fwtty_bind_console(struct fwtty_port *port,
				      struct fwconsole_ops *fwcon_ops,
				      void *data)
{
	port->con_data = data;
	port->fwcon_ops = fwcon_ops;
}

/*
 * Returns the max send async payload size in bytes based on the unit device
 * link speed. Self-limiting asynchronous bandwidth (via reducing the payload)
 * is not necessary and does not work, because
 *   1) asynchronous traffic will absorb all available bandwidth (less that
 *	being used for isochronous traffic)
 *   2) isochronous arbitration always wins.
   단위 전송 장치 링크 속도를 기준으로 최대 보내기 비동기 페이로드 크기를 바이트 단위로 반환합니다.
   자체 제한 비동기 대역폭 (페이로드 감소를 통한)은 필요하지 않으며 작동하지 않습니다.
   1) 비동기 트래픽은 사용 가능한 모든 대역폭을 흡수합니다 (등시성 트래픽에 사용되는 대역폭보다 적음)
   2) 등시성 중재가 항상 승리합니다.
 */
static inline int link_speed_to_max_payload(unsigned speed)
{
	/* Max async payload is 4096 - see IEEE 1394-2008 tables 6-4, 16-18 
	   최대 비동기 페이로드는 4096입니다 (IEEE 1394-2008 표 6-4, 16-18 참조). */
	return min(512 << speed, 4096);
}

#endif /* _FIREWIRE_FWSERIAL_H */
