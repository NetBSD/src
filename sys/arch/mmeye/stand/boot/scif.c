/*	$NetBSD: scif.c,v 1.1.8.2 2011/06/06 09:06:15 jruoho Exp $	*/
/*
 * Copyright (c) 2011 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef CONS_SCIF

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <sh3/scifreg.h>

#include <machine/cpu.h>

#include "boot.h"
#include "scif.h"

#define BOOT_PCLOCK	40000000

#if defined(SH3) && defined(SH4)
#error "mmeye port don't support SH3,SH4 common boot."
#elif defined(SH3)
#error "don't support SH3 common boot."
#elif defined(SH4)
#define CPU_IS_SH4	1
#endif

void *
scif_init(int speed)
{

#define divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */

	/* Initialize SCR */
	SHREG_SCSCR2 = 0x00;

	SHREG_SCFCR2 = SCFCR2_TFRST | SCFCR2_RFRST;

	/* Serial Mode Register */
	SHREG_SCSMR2 = 0x00;	/* 8bit,NonParity,Even,1Stop */

	/* Bit Rate Register */
	SHREG_SCBRR2 = divrnd(BOOT_PCLOCK, 32 * speed) - 1;

	/*
	 * wait 2m Sec, because Send/Recv must begin 1 bit period after
	 * BRR is set.
	 */
	delay(2000);

	SHREG_SCFCR2 = FIFO_RCV_TRIGGER_14 | FIFO_XMT_TRIGGER_1;

	/* Send permission, Receive permission ON */
	SHREG_SCSCR2 = SCSCR2_TE | SCSCR2_RE;

	/* Serial Status Register */
	SHREG_SCSSR2 = (SHREG_SCSSR2 & SCSSR2_TDFE);	/* Clear Status */

	return NULL;
}

void
scif_putc(int c)
{

	/* wait for ready */
	while ((SHREG_SCFDR2 & SCFDR2_TXCNT) == SCFDR2_TXF_FULL)
		continue;

	/* write send data to send register */
	SHREG_SCFTDR2 = c;

	/* clear ready flag */
	SHREG_SCSSR2 = (SHREG_SCSSR2 & ~(SCSSR2_TDFE | SCSSR2_TEND));
}

int
scif_getc(void)
{
	unsigned char c, err_c;
#ifdef SH4
	unsigned short err_c2 = 0;
#endif

	for (;;) {
		/* wait for ready */
		while ((SHREG_SCFDR2 & SCFDR2_RECVCNT) == 0)
			continue;

		c = SHREG_SCFRDR2;
		err_c = SHREG_SCSSR2;
		SHREG_SCSSR2 = (SHREG_SCSSR2 &
		    ~(SCSSR2_ER | SCSSR2_BRK | SCSSR2_RDF | SCSSR2_DR));
#ifdef SH4
		if (CPU_IS_SH4) {
			err_c2 = SHREG_SCLSR2;
			SHREG_SCLSR2 = (SHREG_SCLSR2 & ~SCLSR2_ORER);
		}
#endif
		if ((err_c &
		    (SCSSR2_ER | SCSSR2_BRK | SCSSR2_FER | SCSSR2_PER)) == 0) {
#ifdef SH4
			if (CPU_IS_SH4 && ((err_c2 & SCLSR2_ORER) == 0))
#endif
				return c;
		}
	}
}

int
scif_scankbd(void)
{
	unsigned char c, err_c;
#ifdef SH4
	unsigned short err_c2 = 0;
#endif

	for (;;) {
		/* wait for ready */
		if ((SHREG_SCFDR2 & SCFDR2_RECVCNT) == 0)
			return -1;

		c = SHREG_SCFRDR2;
		err_c = SHREG_SCSSR2;
		SHREG_SCSSR2 = (SHREG_SCSSR2 &
		    ~(SCSSR2_ER | SCSSR2_BRK | SCSSR2_RDF | SCSSR2_DR));
#ifdef SH4
		if (CPU_IS_SH4) {
			err_c2 = SHREG_SCLSR2;
			SHREG_SCLSR2 = (SHREG_SCLSR2 & ~SCLSR2_ORER);
		}
#endif
		if ((err_c &
		    (SCSSR2_ER | SCSSR2_BRK | SCSSR2_FER | SCSSR2_PER)) == 0) {
#ifdef SH4
			if (CPU_IS_SH4 && ((err_c2 & SCLSR2_ORER) == 0))
#endif
				return c;
		}
	}
}
#endif /* CONS_SCIF */
