/*	$NetBSD: com.c,v 1.2 2003/08/07 16:27:49 agc Exp $	*/

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 * Copyright (c) 1991 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
 */

#include <sys/param.h>

#include <lib/libsa/stand.h>

#include <hpcmips/vr/vripreg.h>
#include <hpcmips/vr/cmureg.h>

#include <dev/ic/comreg.h>
#include <dev/ic/ns16550reg.h>
#include <dev/ic/st16650reg.h>
#define com_lcr com_cfcr

#include "extern.h"

#if 0
#define	VRCOM_FREQ	18432000	/* 18.432kHz */
#endif

int
iskey(void)
{
	return ISSET(REGREAD_1(VR4181_SIU_ADDR, com_lsr), LSR_RXRDY);
}

int
getchar(void)
{
	u_int8_t	stat;
	u_int8_t	c;

	/* block until a character becomes available */
	while (!ISKEY)
		;

	c = REGREAD_1(VR4181_SIU_ADDR, com_data);
	stat = REGREAD_1(VR4181_SIU_ADDR, com_iir);

	return c;
}

static void
comcnputc(int c)
{
	int	timo;

	/* wait for any pending transmission to finish */
	timo = 150000;
	while (!ISSET(REGREAD_1(VR4181_SIU_ADDR, com_lsr), LSR_TXRDY)
	       && --timo)
		continue;

	REGWRITE_1(VR4181_SIU_ADDR, com_data, c);

	/* wait for this transmission to complete */
	timo = 1500000;
	while (!ISSET(REGREAD_1(VR4181_SIU_ADDR, com_lsr), LSR_TXRDY)
	       && --timo)
		continue;
}

void
putchar(int c)
{
	if (c == '\n')
		comcnputc('\r');
	comcnputc(c);
}

/*
 * Initialize UART for use as console or KGDB line.
 */
void
comcninit(void)
{
	int		rate;

	/* enable divisor latch access and set bit rate */
	REGWRITE_1(VR4181_SIU_ADDR, com_lcr, LCR_DLAB);
	rate = 10; /* 115200bps with VRCOM_FREQ */
	REGWRITE_1(VR4181_SIU_ADDR, com_dlbl, rate);
	REGWRITE_1(VR4181_SIU_ADDR, com_dlbh, rate >> 8);
	
	/*
	 * disable divisor latch access and,
	 * set "8bit non-parity 1 stop bit"
	 */
	REGWRITE_1(VR4181_SIU_ADDR, com_lcr, LCR_8BITS);

	/* disable all interrupt */
	REGWRITE_1(VR4181_SIU_ADDR, com_ier, 0);

	/* enable FIFO */
	REGWRITE_1(VR4181_SIU_ADDR, com_fifo,
		   FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST | FIFO_TRIGGER_1);

	/* set DTR and RTS low */
	REGWRITE_1(VR4181_SIU_ADDR, com_mcr, MCR_DTR | MCR_RTS);
}
