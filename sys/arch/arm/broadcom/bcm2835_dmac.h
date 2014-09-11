/* $NetBSD: bcm2835_dmac.h,v 1.1.2.2 2014/09/11 14:20:11 martin Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef BCM2835_DMAC_H
#define BCM2835_DMAC_H

#define DMAC_CS(n)		(0x00 + (0x100 * (n)))
#define  DMAC_CS_RESET		__BIT(31)
#define  DMAC_CS_ABORT		__BIT(30)
#define  DMAC_CS_DISDEBUG	__BIT(29)
#define  DMAC_CS_WAIT_FOR_OUTSTANDING_WRITES __BIT(28)
#define  DMAC_CS_PANIC_PRIORITY	__BITS(23,20)
#define  DMAC_CS_PRIORITY	__BITS(19,16)
#define  DMAC_CS_ERROR		__BIT(8)
#define  DMAC_CS_WAITING_FOR_OUTSTANDING_WRITES __BIT(6)
#define  DMAC_CS_DREQ_STOPS_DMA	__BIT(5)
#define  DMAC_CS_PAUSED		__BIT(4)
#define  DMAC_CS_DREQ		__BIT(3)
#define  DMAC_CS_INT		__BIT(2)
#define  DMAC_CS_END		__BIT(1)
#define  DMAC_CS_ACTIVE		__BIT(0)
#define  DMAC_CS_INTMASK	(DMAC_CS_INT|DMAC_CS_END)
#define DMAC_CONBLK_AD(n)	(0x04 + (0x100 * (n)))
#define DMAC_DEBUG(n)		(0x20 + (0x100 * (n)))
#define  DMAC_DEBUG_LITE	__BIT(28)
#define  DMAC_DEBUG_VERSION	__BITS(27,25)
#define  DMAC_DEBUG_DMA_STATE	__BITS(24,16)
#define  DMAC_DEBUG_DMA_ID	__BITS(15,8)
#define  DMAC_DEBUG_OUTSTANDING_WRITES __BITS(7,4)
#define  DMAC_DEBUG_READ_ERROR	__BIT(2)
#define  DMAC_DEBUG_FIFO_ERROR	__BIT(1)
#define  DMAC_DEBUG_READ_LAST_NOT_SET_ERROR __BIT(0)

struct bcm_dmac_conblk {
	uint32_t	cb_ti;
#define DMAC_TI_NO_WIDE_BURSTS	__BIT(26)
#define DMAC_TI_WAITS		__BITS(25,21)
#define DMAC_TI_PERMAP		__BITS(20,16)
#define DMAC_TI_BURST_LENGTH	__BITS(15,12)
#define DMAC_TI_SRC_IGNORE	__BIT(11)
#define DMAC_TI_SRC_DREQ	__BIT(10)
#define DMAC_TI_SRC_WIDTH	__BIT(9)
#define DMAC_TI_SRC_INC		__BIT(8)
#define DMAC_TI_DEST_IGNORE	__BIT(7)
#define DMAC_TI_DEST_DREQ	__BIT(6)
#define DMAC_TI_DEST_WIDTH	__BIT(5)
#define DMAC_TI_DEST_INC	__BIT(4)
#define DMAC_TI_WAIT_RESP	__BIT(3)
#define DMAC_TI_TDMODE		__BIT(1)
#define DMAC_TI_INTEN		__BIT(0)
	uint32_t	cb_source_ad;
	uint32_t	cb_dest_ad;
	uint32_t	cb_txfr_len;
#define DMAC_TXFR_LEN_YLENGTH	__BITS(29,16)
#define DMAC_TXFR_LEN_XLENGTH	__BITS(15,0)
	uint32_t	cb_stride;
#define DMAC_STRIDE_D_STRIDE	__BITS(31,16)
#define DMAC_STRIDE_S_STRIDE	__BITS(15,0)
	uint32_t	cb_nextconbk;
	uint32_t	cb_padding[2];
} __packed;

#define DMAC_INT_STATUS		0xfe0
#define DMAC_ENABLE		0xff0

enum bcm_dmac_type {
	BCM_DMAC_TYPE_NORMAL,
	BCM_DMAC_TYPE_LITE
};

struct bcm_dmac_channel;

struct bcm_dmac_channel *bcm_dmac_alloc(enum bcm_dmac_type,
					void (*)(void *), void *);
void bcm_dmac_free(struct bcm_dmac_channel *);
void bcm_dmac_set_conblk_addr(struct bcm_dmac_channel *, bus_addr_t);
int bcm_dmac_transfer(struct bcm_dmac_channel *);
void bcm_dmac_halt(struct bcm_dmac_channel *);


#endif /* !BCM2835_DMAC_H */
