/*	$NetBSD: ipaq_pcicreg.h,v 1.2.2.2 2001/08/03 04:11:32 lukem Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA (ichiro@ichiro.org).
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/* IRQ numbers */
#define IRQ_CD0		20
#define IRQ_CD1		21
#define IRQ_IRQ0	22
#define IRQ_IRQ1	23

/*
 * Linkup register of DUAL PCMCIA SLEEVE
 */
#define LINKUP_STATUS_REG0	0x1a000000
#define LINKUP_STATUS_REG1	0x19000000

/* Status Register */
#define LINKUP_S1		(1 << 0)
#define LINKUP_S2		(1 << 1)
#define LINKUP_S3		(1 << 2)
#define LINKUP_S4		(1 << 3)
#define LINKUP_BVD1		(1 << 4)
#define LINKUP_BVD2		(1 << 5)
#define LINKUP_VS1		(1 << 6)
#define LINKUP_VS2		(1 << 7)
#define LINKUP_RDY		(1 << 8)
#define LINKUP_CD1		(1 << 9)
#define LINKUP_CD2		(1 << 10)

/* Command Register */
#define LINKUP_CMD_S1		(1 << 0)
#define LINKUP_CMD_S2		(1 << 1)
#define LINKUP_CMD_S3		(1 << 2)
#define LINKUP_CMD_S4		(1 << 3)
#define LINKUP_CMD_RESET	(1 << 4)
#define LINKUP_CMD_AUTO_PWOFF	(1 << 5)
#define LINKUP_CMD_CFMODE	(1 << 6)
#define LINKUP_CMD_SIGNAL_OUT	(1 << 7)
#define LINKUP_CMD_SOCK_POLAR	(1 << 8)
