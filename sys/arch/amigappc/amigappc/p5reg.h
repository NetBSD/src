/*	$NetBSD: p5reg.h,v 1.1 2009/07/21 09:49:15 phx Exp $ */

/*
 * Copyright (C) 2000 Adam Ciarcinski.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Adam Ciarcinski for
 *	the NetBSD project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _P5REG_H_
#define _P5REG_H_

#define	P5BASE	0xf60000

/* registers */
#define	P5_REG_RESET		0x00
#define	P5_REG_ENABLE		0x08
#define	P5_REG_WAITSTATE	0x10
#define P5_BPPC_MAGIC		0x13
#define	P5_REG_SHADOW		0x18
#define	P5_REG_LOCK		0x20
#define	P5_REG_INT		0x28
#define	P5_IPL_EMU		0x30
#define	P5_INT_LVL		0x38

/* bit definitions */
#define	P5_SET_CLEAR	0x80

/* REQ_RESET */
#define	P5_PPC_RESET	0x10
#define	P5_M68K_RESET	0x08
#define	P5_AMIGA_RESET	0x04
#define	P5_AUX_RESET	0x02
#define	P5_SCSI_RESET	0x01

/* REG_WAITSTATE */
#define	P5_PPC_WRITE	0x08
#define	P5_PPC_READ	0x04
#define	P5_M68K_WRITE	0x02
#define	P5_M68K_READ	0x01

/* REG_SHADOW */
#define	P5_SELF_RESET	0x40
#define	P5_SHADOW	0x01

/* REG_LOCK */
#define	P5_MAGIC1	0x40
#define	P5_MAGIC2	0x20
#define	P5_MAGIC3	0x10

/* REG_INT */
#define	P5_ENABLE_IPL	0x02
#define	P5_INT_MASTER	0x01

/* IPL_EMU */
#define	P5_DISABLE_INT	0x40
#define	P5_M68K_IPL2	0x20
#define	P5_M68K_IPL1	0x10
#define	P5_M68K_IPL0	0x08
#define	P5_PPC_IPL2	0x04
#define	P5_PPC_IPL1	0x02
#define	P5_PPC_IPL0	0x01

/* INT_LVL */
#define	P5_LVL7		0x40
#define	P5_LVL6		0x20
#define	P5_LVL5		0x10
#define	P5_LVL4		0x08
#define	P5_LVL3		0x04
#define	P5_LVL2		0x02
#define	P5_LVL1		0x01

/* macros to read and write P5 registers */
#define P5read(reg, val)						\
	do {								\
		(val) = *(volatile unsigned char *)(P5BASE + (reg));	\
		__asm volatile("sync");					\
	} while (0);

#define P5write(reg, val)						\
	do {								\
		*(volatile unsigned char *)(P5BASE + (reg)) = (val);	\
		__asm volatile("sync");					\
	} while (0);

#endif /* _P5REG_H_ */
