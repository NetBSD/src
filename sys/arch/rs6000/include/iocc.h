/*	$NetBSD: iocc.h,v 1.1.8.2 2008/01/21 09:38:44 yamt Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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

#ifndef _RS6000_IOCC_H_
#define _RS6000_IOCC_H_

/*
 * Ports and defines used for control of the IOCC
 */

/* MCA register addresses for PPC */

#define	IOCC_BASE_PPC		0x10000
#define IOCC_POSBASE_PPC	0x400000

#define IOCC_BASE		IOCC_BASE_PPC
#define IOCC_POSBASE		IOCC_POSBASE_PPC

#define IOCC_CFG_REG		(IOCC_BASE + 0x80)	/* config reg */
#define IOCC_PERS		(IOCC_BASE + 0x88)	/* personalization */
#define IOCC_TCE_ADDR_HIGH	(IOCC_BASE + 0x98)	/* MSW of tceaddr */
#define IOCC_TCE_ADDR_LOW	(IOCC_BASE + 0x9C)	/* LSW of tceaddr */
#define IOCC_CRESET		(IOCC_BASE + 0xA0)	/* comp reset reg */
#define IOCC_BUS_MAP		(IOCC_BASE + 0xA8)	/* bus mapping reg */
#define IOCC_IEE		(IOCC_BASE + 0x180) /* intr enable */
#define IOCC_IRR		(IOCC_BASE + 0x188) /* intr request reg */
#define IOCC_MIR		(IOCC_BASE + 0x190) /* misc intr reg */
#define IOCC_CPU_AVAIL(proc) \
	(IOCC_BASE + 0x1C0 + (proc * 4))		/* available procs */
#define IOCC_XIVR(intr) \
	(IOCC_BASE + 0x200 + (intr * 4))		/* extr intr vector */
#define IOCC_DMA_SLAVE_CTRL(slave) \
	(IOCC_BASE + 0x380 + (slave * 4))		/* slave control regs */
#define IOCC_CHAN_STAT(chan) \
	(IOCC_BASE + 0x400 + (chan * 4))		/* channel stat regs */
#define IOCC_EOI(intr) \
	(IOCC_BASE + 0x480 + (intr * 4))		/* ext intr vec */
#define IOCC_ARBLEVEL(chan) \
	(IOCC_BASE + 0x500 + (chan * 4))	/* chan arb level en/disable */

#define MAX_IOCC		2

#endif /* _RS6000_IOCC_H_ */
