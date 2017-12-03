/*	$NetBSD: ingenic_coreregs.h,v 1.1.10.2 2017/12/03 11:36:28 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 Michael Lorenz
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

#ifndef INGENIC_COREREGS_H
#define INGENIC_COREREGS_H

#ifdef _LOCORE
#define	_(n)	__CONCAT($,n)
#else
#define	_(n)	n
#endif

/* cores status, 12 select 3 */
#define CP0_CORE_CTRL	_(12), 2	/* select 2 */
#define  CC_SW_RST0	__BIT(0)	/* reset core 0 */
#define  CC_SW_RST1	__BIT(1)	/* reset core 1 */
#define  CC_RPC0	__BIT(8)	/* dedicated reset entry core 0 */
#define  CC_RPC1	__BIT(9)	/* -- || -- core 1 */
#define  CC_SLEEP0M	__BIT(16)	/* mask sleep core 0 */
#define  CC_SLEEP1M	__BIT(17)	/* mask sleep core 1 */

/* cores status, 12 select 3 */
#define CP0_CORE_STATUS	_(12), 3
#define  CS_MIRQ0_P	__BIT(0)	/* mailbox IRQ for 0 pending */
#define  CS_MIRQ1_P	__BIT(1)	/* || core 1 */
#define  CS_IRQ0_P	__BIT(8)	/* peripheral IRQ for core 0 */
#define  CS_IRQ1_P	__BIT(9)	/* || core 1 */
#define  CS_SLEEP0	__BIT(16)	/* core 0 sleeping */
#define  CS_SLEEP1	__BIT(17)	/* core 1 sleeping */

/* cores reset entry & IRQ masks - 12 select 4 */
#define CP0_CORE_REIM	_(12), 4
#define  REIM_MIRQ0_M	__BIT(0)	/* allow mailbox IRQ for core 0 */
#define  REIM_MIRQ1_M	__BIT(1)	/* allow mailbox IRQ for core 1 */
#define  REIM_IRQ0_M	__BIT(8)	/* allow peripheral IRQ for core 0 */
#define  REIM_IRQ1_M	__BIT(9)	/* allow peripheral IRQ for core 1 */
#define  REIM_ENTRY_M	__BITS(31,16)	/* reset exception entry if RPCn=1 */

#define CP0_SPINLOCK	_(12), 5
#define CP0_SPINATOMIC	_(12), 6

#define CP0_CORE0_MBOX	_(20), 0
#define CP0_CORE1_MBOX	_(20), 1

#endif /* INGENIC_COREREGS_H */
