/*	$NetBSD: mainboard.h,v 1.3 2001/03/31 00:08:34 wdk Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *
 * Define all hardware address map.
 */

#ifndef _MACHINE_MAINBOARD_H_
#define	_MACHINE_MAINBOARD_H_	1

/*----------------------------------------------------------------------
 */
#define TOD_BASE        0xbd000000
#define	RTC_PORT	    0x1fe3
#define	DATA_PORT	    0x1fe7

#define RAMBO_BASE      0xbc000000 /* Base address for RAMBO DMA */

#define RAMBO_TCOUNT    (RAMBO_BASE+0xc00) /* Timer count register */
#define RAMBO_TBREAK    (RAMBO_BASE+0xd00) /* Timner break register */
#define RAMBO_ERREG	(RAMBO_BASE+0xe00) /* Machine error register */
#define RAMBO_CTL       (RAMBO_BASE+0xf00) /* Machine control register */

#define	LANCE_PORT	0xba000000
#define	ETHER_ID	0xbd000000

#define ZS0_ADDR        0xbb000000

#define INTREG_0        0xb9800003         /* Interrupt 0 status register  */
#define    INT_CEB      0x80	           /* Modem call indicator */
#define    INT_DSRB     0x40               /* Data Set Ready */
#define    INT_DRSInB   0x20               /* Data Rate Select (in) */
#define    INT_Lance    0x10               /* Lance Ethernet */
#define    INT_NCR      0x08               /* NCR 53c94 SCSI */
#define    INT_SCC      0x04               /* Z8530 SCC */
#define    INT_Kbd      0x02               /* Keyboard controller */
#define    INT_ExpSlot  0x01               /* Expansion Slot */

#define PIZAZZ_ISA_IOBASE	0x10000000 /* ISA Bus I/O */
#define PIZAZZ_ISA_IOSIZE	0x00040000 /* 64k -> 256k */
#define PIZAZZ_ISA_MEMBASE	0x14000000 /* ISA Bus Memory */
#define PIZAZZ_ISA_MEMSIZE	0x00100000 /* 16MB -> 64MB */
#define PIZAZZ_ISA_INTRLATCH	0x10400000 /* Interrupt Latch */

#endif /* _MACHINE_MAINBOARD_H_ */
