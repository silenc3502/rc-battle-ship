/*
 * DMA-able FIFO interface
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

#ifndef _DMA_FIFO_H_
#define _DMA_FIFO_H_

/**
 * The design basis for the DMA FIFO is to provide an output side that
 * complies with the streaming DMA API design that can be DMA'd from directly
 * (without additional copying), coupled with an input side that maintains a
 * logically consistent 'apparent' size (ie, bytes in + bytes avail is static
 * for the lifetime of the FIFO).
 *
 * DMA output transactions originate on a cache line boundary and can be
 * variably-sized. DMA output transactions can be retired out-of-order but
 * the FIFO will only advance the output in the original input sequence.
 * This means the FIFO will eventually stall if a transaction is never retired.
 *
 * Chunking the output side into cache line multiples means that some FIFO
 * memory is unused. For example, if all the avail input has been pended out,
 * then the in and out markers are re-aligned to the next cache line.
 * The maximum possible waste is
 *     (cache line alignment - 1) * (max outstanding dma transactions)
 * This potential waste requires additional hidden capacity within the FIFO
 * to be able to accept input while the 'apparent' size has not been reached.
 *
 * Additional cache lines (ie, guard area) are used to minimize DMA
 * fragmentation when wrapping at the end of the FIFO. Input is allowed into the
 * guard area, but the in and out FIFO markers are wrapped when DMA is pended.
 */
/*DMA FIFO의 설계 기준은 논리적으로 일관된 '명백한'크기를 유지하는 입력면과 함께 직접적으로 (추가 복사없이) DMA 할 수있는 스트리밍 DMA API 설계를 준수하는 출력면을 제공하는 것입니다 즉, + 바이트 avail의 바이트는 FIFO의 수명 동안 정적입니다.

 DMA 출력 트랜잭션은 캐시 라인 경계에서 시작되며 가변적 인 크기 일 수 있습니다. DMA 출력 트랜잭션은 비 순차적으로 폐기 될 수 있지만 FIFO는 원래 입력 순서로만 출력을 진행합니다. 이것은 트랜잭션이 결코 폐기되지 않으면 FIFO가 결국 정지하게됨을 의미합니다.

 출력 라인을 캐시 라인 배수로 청크하는 것은 일부 FIFO 메모리가 사용되지 않는다는 것을 의미합니다. 예를 들어, 모든 avail 입력이 pending out 된 경우, in 및 out 마커는 다음 캐시 라인으로 다시 정렬됩니다. 가능한 최대 낭비는 (캐시 라인 정렬 - 1) * (최대 미해결 dma 트랜잭션)입니다.이 잠재적 낭비는 '명백한'크기에 도달하지 않은 동안 입력을 수용 할 수 있도록 FIFO 내에 추가 숨겨진 용량을 필요로합니다.

 추가 캐시 라인 (즉, 가드 영역)은 FIFO 끝에 래핑 할 때 DMA 단편화를 최소화하는 데 사용됩니다. 가드 영역에는 입력이 허용되지만 DMA가 보류 될 때 입력 및 출력 FIFO 마커가 래핑됩니다.   */

#define DMA_FIFO_GUARD 3   /* # of cache lines to reserve for the guard area 
				가드 영역을 위해 예약 할 캐시 라인의 수  */


struct dma_fifo {
	unsigned	 in;
	unsigned	 out;		/* updated when dma is pended         
					   dma가 보류되었을 때 업데이트 됨  */
	unsigned	 done;		/* updated upon dma completion       
					    DMA 완료시 업데이트           */
	struct {
		unsigned corrupt:1;
	};
	int		 size;		/* 'apparent' size of fifo	      
					     fifo의 '명백한'크기   */
	int		 guard;		/* ofs of guard area		     
					     of 가드 영역         */
	int		 capacity;	/* size + reserved                    */
	int		 avail;		/* # of unused bytes in fifo         
					    fifo에서 사용되지 않는 바이트 수    */
	unsigned	 align;		/* must be power of 2                
					    반드시 2의 제곱이어야합니다.        */
	int		 tx_limit;	/* max # of bytes per dma transaction 
					    dma 트랜잭션 당 최대 바이트 수     */
	int		 open_limit;	/* max # of outstanding allowed       
					    허용 된 최대 허용 개수             */
	int		 open;		/* # of outstanding dma transactions 
					    뛰어난 dma 트랜잭션 수             */
	struct list_head pending;	/* fifo markers for outstanding dma   
					    뛰어난 dma에 대한 * fifo 마커      */
	void		 *data;
};

struct dma_pending {
	struct list_head link;
	void		 *data;
	unsigned	 len;
	unsigned         next;
	unsigned         out;
};

static inline void dp_mark_completed(struct dma_pending *dp)
{
	dp->data += 1;
}

static inline bool dp_is_completed(struct dma_pending *dp)
{
	return (unsigned long)dp->data & 1UL;
}

void dma_fifo_init(struct dma_fifo *fifo);
int dma_fifo_alloc(struct dma_fifo *fifo, int size, unsigned align,
		   int tx_limit, int open_limit, gfp_t gfp_mask);
void dma_fifo_free(struct dma_fifo *fifo);
void dma_fifo_reset(struct dma_fifo *fifo);
int dma_fifo_in(struct dma_fifo *fifo, const void *src, int n);
int dma_fifo_out_pend(struct dma_fifo *fifo, struct dma_pending *pended);
int dma_fifo_out_complete(struct dma_fifo *fifo,
			  struct dma_pending *complete);

/* returns the # of used bytes in the fifo 
   FIFO에 사용 된 바이트 수를 반환합니다.*/
static inline int dma_fifo_level(struct dma_fifo *fifo)
{
	return fifo->size - fifo->avail;
}

/* returns the # of bytes ready for output in the fifo 
    FIFO에 출력 할 준비가 된 바이트 수를 반환   */
static inline int dma_fifo_out_level(struct dma_fifo *fifo)
{
	return fifo->in - fifo->out;
}

/* returns the # of unused bytes in the fifo 
   FIFO에서 사용되지 않은 바이트 수를 반환합니다.  */
static inline int dma_fifo_avail(struct dma_fifo *fifo)
{
	return fifo->avail;
}

/* returns true if fifo has max # of outstanding dmas 
   fifo에 미해결 dmas가있는 경우 true를 반환합니다. */
static inline bool dma_fifo_busy(struct dma_fifo *fifo)
{
	return fifo->open == fifo->open_limit;
}

/* changes the max size of dma returned from dma_fifo_out_pend()
   dma_fifo_out_pend ()에서 반환 된 dma의 최대 크기를 변경합니다. */
static inline int dma_fifo_change_tx_limit(struct dma_fifo *fifo, int tx_limit)
{
	tx_limit = round_down(tx_limit, fifo->align);
	fifo->tx_limit = max_t(int, tx_limit, fifo->align);
	return 0;
}

#endif /* _DMA_FIFO_H_ */
