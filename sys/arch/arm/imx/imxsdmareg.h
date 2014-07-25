/*	$NetBSD: imxsdmareg.h,v 1.2 2014/07/25 07:49:56 hkenken Exp $	*/

/*
 * Copyright (c) 2009  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi and Hiroyuki Bessho for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_IMXSDMAREG_H
#define	_IMXSDMAREG_H

#include "opt_imx.h"

#define	SDMA_SIZE	0x2c0

/* SDMA Controller */
#define	SDMA_N_CHANNELS	32
#define	SDMA_PRIORITY_MIN	1
#define	SDMA_PRIORITY_MAX	7

#define	SDMA_N_EVENTS	32	/* DMA events from periperals */

#define	SDMA_MC0PTR	0x0000
#define	SDMA_INTR	0x0004
#define	SDMA_STOP_STAT	0x0008
#define	SDMA_HSTART	0x000C
#define	SDMA_EVTOVR	0x0010
#define	SDMA_DSPOVR	0x0014
#define	SDMA_HOSTOVR	0x0018
#define	SDMA_EVTPEND	0x001C
#define	SDMA_RESET	0x0024
#define	 SDMA_RESET_RESCHED	__BIT(1)
#define	 SDMA_RESET_RESET	__BIT(0)
#define	SDMA_EVTERR	0x0028
#define	SDMA_INTRMASK	0x002C
#define	SDMA_PSW	0x0030
#define	SDMA_EVTERRDBG	0x0034
#define	SDMA_CONFIG	0x0038
#define	SDMA_ONCE_ENB	0x0040
#define	SDMA_ONCE_DATA	0x0044
#define	SDMA_ONCE_INSTR	0x0048
#define	SDMA_ONCE_STAT	0x004c
#define	 ONCE_STAT_PST_SHIFT         	12
#define	 ONCE_STAT_PST_MASK     	(0xf<<ONCE_STAT_PST_SHIFT)
#define	 ONCE_STAT_PST_PROGRAM     	(0<<ONCE_STAT_PST_SHIFT)
#define	 ONCE_STAT_PST_DATA     	(1<<ONCE_STAT_PST_SHIFT)
#define	 ONCE_STAT_PST_CHGFLOW   	(2<<ONCE_STAT_PST_SHIFT)
#define	 ONCE_STAT_PST_CHGFLOW_IN_LOOP	(3<<ONCE_STAT_PST_SHIFT)
#define	 ONCE_STAT_PST_DEBUG     	(4<<ONCE_STAT_PST_SHIFT)
#define	 ONCE_STAT_PST_FUNCUNIT   	(5<<ONCE_STAT_PST_SHIFT)
#define	 ONCE_STAT_PST_SLEEP     	(6<<ONCE_STAT_PST_SHIFT)
#define	 ONCE_STAT_PST_SAVE     	(7<<ONCE_STAT_PST_SHIFT)
#define	 ONCE_STAT_PST_PROGRAM_IN_SLEEP	(8<<ONCE_STAT_PST_SHIFT)
#define	 ONCE_STAT_PST_DATA_IN_SLEEP	(9<<ONCE_STAT_PST_SHIFT)
#define	 ONCE_STAT_PST_CHGFLOW_IN_SLEEP	(10<<ONCE_STAT_PST_SHIFT)
#define	 ONCE_STAT_PST_CHGFLOW_IN_LOOP_IN_SLEEP	(10<<ONCE_STAT_PST_SHIFT)
#define	 ONCE_STAT_PST_DEBUG_IN_SLEEP	(12<<ONCE_STAT_PST_SHIFT)
#define	 ONCE_STAT_PST_FUNCUNIT_IN_SLEEP  (13<<ONCE_STAT_PST_SHIFT)
#define	 ONCE_STAT_PST_SLEEP_AFTER_RESET  (14<<ONCE_STAT_PST_SHIFT)
#define	 ONCE_STAT_PST_RESTORE    	(15<<ONCE_STAT_PST_SHIFT)

#define	 ONCE_STAT_RCV	__BIT(11)
#define	 ONCE_STAT_EDR	__BIT(10)
#define	 ONCE_STAT_ODR	__BIT(9)
#define	 ONCE_STAT_SWB	__BIT(8)
#define	 ONCE_STAT_MST	__BIT(7)
#define	 ONCE_STAT_ECDR	0X07

#define	SDMA_ONCE_CMD	0x0050
#define	 ONCE_RSTATUS		0
#define	 ONCE_DMOV		1
#define	 ONCE_EXEC_ONCE		2
#define	 ONCE_RUN		3
#define	 ONCE_EXEC_CORE		4
#define	 ONCE_DEBUG_RQST	5
#define	 ONCE_RBUFFER		6
#define	SDMA_EVT_MIRROR	0x0054
#define	SDMA_ILLINSTADDR  0x0058	/* Illegal Instruction Trap Address */
#define	SDMA_CHN0ADDR	0x005c	/* Channel 0 Boot address */
#define	SDMA_XTRIG_CONF1  0x0070	/* Cross-Triger Evennts Config */
#define	SDMA_XTRIG_CONF2  0x0074
#if defined(IMX31)
#define	SDMA_CHNENBL(n)	(0x80+(n)*4)	/* Channel Enable RAM */
#elif defined(IMX51)
#define	SDMA_OTB	0x0078
#define	SDMA_PRF_CNT(n) (0x07c+(n)*4)
#define	SDMA_CHNENBL(n)	(0x200+(n)*4)	/* Channel Enable RAM */
#endif
#define	SDMA_CHNPRI(n)	(0x100+(n)*4)	/* Channel Priority */


/*
 * Memory of SDMA Risc Core
 */

#define	SDMACORE_ROM_BASE	0x0000
#define	SDMACORE_ROM_SIZE	0x0400	/* in 32-bit word. 4K bytes */
#define	SDMACORE_RAM_BASE	0x0800
#define	SDMACORE_RAM_SIZE	0x0800	/* in 32-bit word. 8K byte */

#define	SDMACORE_CONTEXT_BASE	0x0800
#define	SDMACORE_CONTEXT_SIZE	32		/* XXX: or 24 */
#define	SDMACORE_CONTEXT_ADDR(ch)  (SDMACORE_CONTEXT_BASE+ \
				   (SDMACORE_CONTEXT_SIZE * (ch)))
#define	SDMACORE_CONTEXT_END	SDMACORE_CONTEXT_ADDR(SDMA_N_CHANNELS)


#endif	/* _IMXSDMAREG_H */
