/* 	$NetBSD */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#ifndef _VIRTEX_DEV_CDMACREG_H_
#define _VIRTEX_DEV_CDMACREG_H_

/*
 * CDMAC registers and data structures. Keep the names fully qualified, users
 * need at least CDMAC_STAT bits to manipulate descriptor queues. The offsets
 * are multiplied by four compared to Xilinx documentation -- so don't get
 * confused, bus space will deal.
 *
 * The number of channels and interrupt wiring differs per design.
 */

/* Status and control register block sizes. */
#define CDMAC_CTRL_SIZE 	0x0c
#define CDMAC_STAT_SIZE 	0x04

/* Status and control register offsets per channel. */
#define	CDMAC_STAT_BASE(n) 	(0x80 + (n) * 0x04)
#define	CDMAC_CTRL_BASE(n) 	(0x00 + (n) * 0x10)

/* Individual engine control registers. */
#define CDMAC_NEXT 		0x0000 	/* Next descriptor pointer,
					 * 32B aligned, 0x0 = stop */
#define CDMAC_CURADDR 		0x0004 	/* Address being transferred */
#define CDMAC_CURSIZE 		0x0008 	/* Remaining length */
#define CDMAC_CURDESC 		0x000c 	/* Current descriptor pointer */

#define CDMAC_CURSIZE_MASK 	0x00ffffff

/* Engine status reg bits. */
#define CDMAC_STAT_ERROR 	0x80000000 	/* EINVAL -> halt, interrupt */
#define CDMAC_STAT_INTR 	0x40000000 	/* Interrupt on end of descr */
#define CDMAC_STAT_STOP 	0x20000000 	/* Stop on end of descr */
#define CDMAC_STAT_DONE 	0x10000000 	/* Descriptor done */
#define CDMAC_STAT_SOP 		0x08000000 	/* Start of packet */
#define CDMAC_STAT_EOP 		0x04000000 	/* End of packet */
#define CDMAC_STAT_BUSY 	0x02000000 	/* Engine busy */
#define CDMAC_STAT_RESET 	0x01000000 	/* Channel reset */

#ifdef notdef
/*
 * DMA engine timers, 8bit. Rather useless -- we need "interrupt
 * if transfer stalls for N cycles", not "fire after N cycles".
 * If we ever use this, move definitions to design-specific code
 * like we do with STAT.
 */
#define CDMAC_TIMER_TX0 	0x00a0
#define CDMAC_TIMER_RX0 	0x00a4
#define CDMAC_TIMER_TX1 	0x00a8
#define CDMAC_TIMER_RX1 	0x00ac
#endif /* notdef */

/* Interrupt register (active-high level-sensitive intr). */
#define CDMAC_INTR 		0x00bc

#define CDMAC_INTR_MIE 		0x80000000 	/* Master interrupt enable */
#define CDMAC_INTR_TX0 		0x00000001 	/* Descriptor interrupts */
#define CDMAC_INTR_RX0 		0x00000002
#define CDMAC_INTR_TX1 		0x00000004
#define CDMAC_INTR_RX1 		0x00000008
#define CDMAC_TIMO_TX0 		0x01000000 	/* Timer interrupts */
#define CDMAC_TIMO_RX0 		0x02000000
#define CDMAC_TIMO_TX1 		0x04000000
#define CDMAC_TIMO_RX1 		0x08000000

#define CDMAC_CHAN_INTR(n) 	(1 << (n))

/*
 * Wire data structure of transfer descriptor (shared for Rx/Tx).
 */
struct cdmac_descr {
	uint32_t 	desc_next; 	/* Next descriptor */
	uint32_t 	desc_addr; 	/* Payload address */
	uint32_t 	desc_size; 	/* Payload size */

	/* Application defined fields, valid in 1st desc on Tx, last on Rx. */
	uint32_t 	desc_user0; 	/* See below */
#define desc_stat 	desc_user0

	uint32_t 	desc_user1;
	uint32_t 	desc_user2;
	uint32_t 	desc_user3;
	uint32_t 	desc_user4;
} __aligned(8) __packed;

#define DMAC_STAT_MASK 	0xff000000 	/* CDMAC portion of desc_user0 */
#define DMAC_USER_MASK 	0x00ffffff 	/* User defined part of desc_user0 */

#endif /*_VIRTEX_DEV_CDMACREG_H_*/
