/* $NetBSD: pi1ppcreg.h,v 1.1 2005/12/28 08:31:09 kurahone Exp $ */

/*-
 * Copyright (c) 2001 Alcove - Nicolas Souchu
 * Copyright (c) 2005 Joe Britt <britt@danger.com> - SGI PI1 version
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * FreeBSD: src/sys/isa/ppcreg.h,v 1.10.2.4 2001/10/02 05:21:45 nsouch Exp
 *
 */

#ifndef __PI1PPCREG_H
#define __PI1PPCREG_H

/* see iocreg.h for data/ctl/status reg offsets */
	
/* SPP mode control register bit positions. */
#define STROBE		0x01
#define AUTOFEED	0x02
#define nINIT		0x04
#define SELECTIN	0x08

/* we emulate this bit */ 
#define IRQENABLE	0x10

/* data dir in PS/2 mode */
#define PCD             0x20

/* SPP status register bit positions. */
/* #define TIMEOUT	0x01 */

#define nFAULT          0x08
#define SELECT          0x10
#define PERROR          0x20
#define nACK            0x40
#define nBUSY           0x80

/* Flags indicating ready condition */
#define SPP_READY (SELECT | nFAULT | nBUSY)
#define SPP_MASK (SELECT | nFAULT | PERROR | nBUSY)

/* Byte mode signals */
#define HOSTCLK		STROBE
#define HOSTBUSY	AUTOFEED
#define ACTIVE1284	SELECTIN
#define PTRCLK		nACK
#define PTRBUSY		nBUSY
#define ACKDATAREQ	PERROR
#define XFLAG		SELECT
#define nDATAVAIL	nFAULT

/* interrupt mask & status bit positions */

/* these interrupts are asserted on rising AND falling edges */
#define PI1_PLP_PERROR_INTR	0x80
#define	PI1_PLP_FAULT_INTR	0x40
#define	PI1_PLP_SELECT_INTR	0x20

/* this interupt is only asserted on rising edge */
#define	PI1_PLP_ACK_INTR	0x04

#endif /* __PI1PPCREG_H */
