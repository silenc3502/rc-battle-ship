/*
 * DMA-able FIFO implementation
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

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/bug.h>

#include "dma_fifo.h"

#ifdef DEBUG_TRACING
#define df_trace(s, args...) pr_debug(s, ##args)
#else
#define df_trace(s, args...)
#endif

#define FAIL(fifo, condition, format...) ({				\
	fifo->corrupt = !!(condition);					\
	WARN(fifo->corrupt, format);					\
})

/*
 * private helper fn to determine if check is in open interval (lo,hi)
   개인 도우미 fn이 수표가 열린 간격 (lo, hi)인지 확인합니다.
 */
static bool addr_check(unsigned check, unsigned lo, unsigned hi)
{
	return check - (lo + 1) < (hi - 1) - lo;
}

/**
 * dma_fifo_init: initialize the fifo to a valid but inoperative state
 * @fifo: address of in-place "struct dma_fifo" object
dma_fifo_init : FIFO를 유효하지만 작동하지 않는 상태로 초기화합니다. 
@fifo : 내부 "struct dma_fifo"객체의 주소
 */
void dma_fifo_init(struct dma_fifo *fifo)
{
	memset(fifo, 0, sizeof(*fifo));
	INIT_LIST_HEAD(&fifo->pending);
}

/**
 * dma_fifo_alloc - initialize and allocate dma_fifo
 * @fifo: address of in-place "struct dma_fifo" object
 * @size: 'apparent' size, in bytes, of fifo
 * @align: dma alignment to maintain (should be at least cpu cache alignment),
 *         must be power of 2
 * @tx_limit: maximum # of bytes transmissible per dma (rounded down to
 *            multiple of alignment, but at least align size)
 * @open_limit: maximum # of outstanding dma transactions allowed
 * @gfp_mask: get_free_pages mask, passed to kmalloc()
 *
 * The 'apparent' size will be rounded up to next greater aligned size.
 * Returns 0 if no error, otherwise an error code
 dma_fifo_alloc - dma_fifo 초기화 및 할당
 @fifo : 내부 "struct dma_fifo"개체의 주소 @size : FIFO의 '명백한'크기 (바이트 단위) 
 @align : 유지 관리 할 dma 정렬 (최소 CPU 캐시 정렬이어야 함)이며 2의 제곱 수이어야합니다
 @ tx_limit : dma 당 전송할 수있는 최대 바이트 수 (정렬의 배수로 반올림되었지만 최소 크기는 반올림 됨) 
 @open_limit : 허용 된 미해결 dma 트랜잭션의 최대 수 @gfp_mask : kmalloc ()에 전달 된 get_free_pages 마스크
 
 '명백한'크기는 다음으로 크게 정렬 된 크기로 반올림됩니다.
  오류가 없으면 0을 반환하고, 그렇지 않으면 오류 코드를 반환합니다.
 */
int dma_fifo_alloc(struct dma_fifo *fifo, int size, unsigned align,
		   int tx_limit, int open_limit, gfp_t gfp_mask)
{
	int capacity;

	if (!is_power_of_2(align) || size < 0)
		return -EINVAL;

	size = round_up(size, align);
	capacity = size + align * open_limit + align * DMA_FIFO_GUARD;
	fifo->data = kmalloc(capacity, gfp_mask);
	if (!fifo->data)
		return -ENOMEM;

	fifo->in = 0;
	fifo->out = 0;
	fifo->done = 0;
	fifo->size = size;
	fifo->avail = size;
	fifo->align = align;
	fifo->tx_limit = max_t(int, round_down(tx_limit, align), align);
	fifo->open = 0;
	fifo->open_limit = open_limit;
	fifo->guard = size + align * open_limit;
	fifo->capacity = capacity;
	fifo->corrupt = 0;

	return 0;
}

/**
 * dma_fifo_free - frees the fifo
 * @fifo: address of in-place "struct dma_fifo" to free
 *
 * Also reinits the fifo to a valid but inoperative state. This
 * allows the fifo to be reused with a different target requiring
 * different fifo parameters.
   dma_fifo_free - FIFO를 해제합니다.
   @fifo : 내부 주소 "struct dma_fifo"를 무료로 만듭니다.
  
   또한 fifo를 유효하지만 작동 불능 상태로 다시 설정합니다. 
   이렇게하면 fifo가 다른 fifo 매개 변수를 요구하는 다른 대상과 재사용 될 수 있습니다.
 */
void dma_fifo_free(struct dma_fifo *fifo)
{
	struct dma_pending *pending, *next;

	if (fifo->data == NULL)
		return;

	list_for_each_entry_safe(pending, next, &fifo->pending, link)
		list_del_init(&pending->link);
	kfree(fifo->data);
	fifo->data = NULL;
}

/**
 * dma_fifo_reset - dumps the fifo contents and reinits for reuse
 * @fifo: address of in-place "struct dma_fifo" to reset
   dma_fifo_reset - FIFO 컨텐츠를 덤프하고 다시 사용하기 위해 다시 삽입합니다.
   @fifo : 리셋 할 내부 주소 "struct dma_fifo"의 주소
 */
void dma_fifo_reset(struct dma_fifo *fifo)
{
	struct dma_pending *pending, *next;

	if (fifo->data == NULL)
		return;

	list_for_each_entry_safe(pending, next, &fifo->pending, link)
		list_del_init(&pending->link);
	fifo->in = 0;
	fifo->out = 0;
	fifo->done = 0;
	fifo->avail = fifo->size;
	fifo->open = 0;
	fifo->corrupt = 0;
}

/**
 * dma_fifo_in - copies data into the fifo
 * @fifo: address of in-place "struct dma_fifo" to write to
 * @src: buffer to copy from
 * @n: # of bytes to copy
 *
 * Returns the # of bytes actually copied, which can be less than requested if
 * the fifo becomes full. If < 0, return is error code.
   dma_fifo_in - 데이터를 FIFO에 복사합니다.
   @fifo : @src에 쓰는 "struct dma_fifo"의 내부 주소 : @n에서 복사 할 버퍼 : 복사 할 바이트 수
   실제 복사 된 바이트 수를 반환합니다. FIFO가 가득 차게되면 요청 된 바이트 수보다 적을 수 있습니다.
   <0이면 오류 코드가 반환됩니다.
 */
int dma_fifo_in(struct dma_fifo *fifo, const void *src, int n)
{
	int ofs, l;

	if (fifo->data == NULL)
		return -ENOENT;
	if (fifo->corrupt)
		return -ENXIO;

	if (n > fifo->avail)
		n = fifo->avail;
	if (n <= 0)
		return 0;

	ofs = fifo->in % fifo->capacity;
	l = min(n, fifo->capacity - ofs);
	memcpy(fifo->data + ofs, src, l);
	memcpy(fifo->data, src + l, n - l);

	if (FAIL(fifo, addr_check(fifo->done, fifo->in, fifo->in + n) ||
		 fifo->avail < n,
		 "fifo corrupt: in:%u out:%u done:%u n:%d avail:%d",
		 fifo->in, fifo->out, fifo->done, n, fifo->avail))
		return -ENXIO;

	fifo->in += n;
	fifo->avail -= n;

	df_trace("in:%u out:%u done:%u n:%d avail:%d", fifo->in, fifo->out,
		 fifo->done, n, fifo->avail);

	return n;
}

/**
 * dma_fifo_out_pend - gets address/len of next avail read and marks as pended
 * @fifo: address of in-place "struct dma_fifo" to read from
 * @pended: address of structure to fill with read address/len
 *          The data/len fields will be NULL/0 if no dma is pended.
 *
 * Returns the # of used bytes remaining in fifo (ie, if > 0, more data
 * remains in the fifo that was not pended). If < 0, return is error code.
   dma_fifo_out_pend - 다음 제공 목록의 주소 / len을 가져오고 pended로 표시합니다. 
   @fifo : 읽기 주소 / len으로 채울 구조체의 주소 인 @pended : read에서 해당 위치의 "struct dma_fifo"
   dma가 보류되지 않으면 data / len 필드는 NULL / 0이됩니다.
   fifo에 남아있는 사용 된 바이트 수를 반환합니다 (예 : 0보다 크면 보류되지 않은 FIFO에 더 많은 데이터가 남아 있음).
   <0이면 오류 코드가 반환됩니다.
  * /
 */
int dma_fifo_out_pend(struct dma_fifo *fifo, struct dma_pending *pended)
{
	unsigned len, n, ofs, l, limit;

	if (fifo->data == NULL)
		return -ENOENT;
	if (fifo->corrupt)
		return -ENXIO;

	pended->len = 0;
	pended->data = NULL;
	pended->out = fifo->out;

	len = fifo->in - fifo->out;
	if (!len)
		return -ENODATA;
	if (fifo->open == fifo->open_limit)
		return -EAGAIN;

	n = len;
	ofs = fifo->out % fifo->capacity;
	l = fifo->capacity - ofs;
	limit = min_t(unsigned, l, fifo->tx_limit);
	if (n > limit) {
		n = limit;
		fifo->out += limit;
	} else if (ofs + n > fifo->guard) {
		fifo->out += l;
		fifo->in = fifo->out;
	} else {
		fifo->out += round_up(n, fifo->align);
		fifo->in = fifo->out;
	}

	df_trace("in: %u out: %u done: %u n: %d len: %u avail: %d", fifo->in,
		 fifo->out, fifo->done, n, len, fifo->avail);

	pended->len = n;
	pended->data = fifo->data + ofs;
	pended->next = fifo->out;
	list_add_tail(&pended->link, &fifo->pending);
	++fifo->open;

	if (FAIL(fifo, fifo->open > fifo->open_limit,
		 "past open limit:%d (limit:%d)",
		 fifo->open, fifo->open_limit))
		return -ENXIO;
	if (FAIL(fifo, fifo->out & (fifo->align - 1),
		 "fifo out unaligned:%u (align:%u)",
		 fifo->out, fifo->align))
		return -ENXIO;

	return len - n;
}

/**
 * dma_fifo_out_complete - marks pended dma as completed
 * @fifo: address of in-place "struct dma_fifo" which was read from
 * @complete: address of structure for previously pended dma to mark completed
   dma_fifo_out_complete - 완성 된 dma를 완료로 표시합니다. 
   @fifo : 완료된 dma에 대한 구조체의 @complete : 주소에서 읽은 "struct dma_fifo"의 내부 주소가 완료되었습니다.
 */
int dma_fifo_out_complete(struct dma_fifo *fifo, struct dma_pending *complete)
{
	struct dma_pending *pending, *next, *tmp;

	if (fifo->data == NULL)
		return -ENOENT;
	if (fifo->corrupt)
		return -ENXIO;
	if (list_empty(&fifo->pending) && fifo->open == 0)
		return -EINVAL;

	if (FAIL(fifo, list_empty(&fifo->pending) != (fifo->open == 0),
		 "pending list disagrees with open count:%d",
		 fifo->open))
		return -ENXIO;

	tmp = complete->data;
	*tmp = *complete;
	list_replace(&complete->link, &tmp->link);
	dp_mark_completed(tmp);

	/* Only update the fifo in the original pended order
	   원래 보류 된 주문으로 만 FIFO를 업데이트하십시오. */
	list_for_each_entry_safe(pending, next, &fifo->pending, link) {
		if (!dp_is_completed(pending)) {
			df_trace("still pending: saved out: %u len: %d",
				 pending->out, pending->len);
			break;
		}

		if (FAIL(fifo, pending->out != fifo->done ||
			 addr_check(fifo->in, fifo->done, pending->next),
			 "in:%u out:%u done:%u saved:%u next:%u",
			 fifo->in, fifo->out, fifo->done, pending->out,
			 pending->next))
			return -ENXIO;

		list_del_init(&pending->link);
		fifo->done = pending->next;
		fifo->avail += pending->len;
		--fifo->open;

		df_trace("in: %u out: %u done: %u len: %u avail: %d", fifo->in,
			 fifo->out, fifo->done, pending->len, fifo->avail);
	}

	if (FAIL(fifo, fifo->open < 0, "open dma:%d < 0", fifo->open))
		return -ENXIO;
	if (FAIL(fifo, fifo->avail > fifo->size, "fifo avail:%d > size:%d",
		 fifo->avail, fifo->size))
		return -ENXIO;

	return 0;
}
