/*	$NetBSD: kbms_if.c,v 1.6 2000/03/24 21:25:32 soren Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Sony Corp. and Kazumasa Utashiro of Software Research Associates, Inc.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)kbms_if.c	8.1 (Berkeley) 6/11/93
 */

/* Keyboard Mouse Gate-array control routine */

#include "ms.h"
#include "kb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <machine/adrsmap.h>
#include <newsmips/dev/scc.h>

typedef struct kbm_sw {
	u_char *stat_port;	/* Status port */
	u_char *data_port;	/* Data port */
	u_char *intr_port;	/* Interrupt port */
	u_char *reset_port;	/* Reset port */
	u_char *init1_port;	/* Initialize port 1 */
	u_char *init2_port;	/* Initialize port 2 */
	u_char *buzz_port;	/* Buzzer port */
	u_char *buzzf_port;	/* Buzzer frequency port */
	u_char intr_en;		/* Data for Interrupt Enable */
	u_char intr_in;		/* Interrupt Occur flag */
	u_char data_rdy;	/* Data Ready flag */
	u_char init1;		/* Speed */
	u_char init2;		/* Clock */
	u_char buzzf;		/* Buzzer frequency */
} Kbm_sw;

#define OFF 0x80
static struct kbm_sw Kbm_port[] = {
	{
#ifdef news3400
		(u_char *) MOUSE_STAT,
#else
		(u_char *) KEYB_STAT,
#endif
		(u_char *) MOUSE_DATA,
		(u_char *) MOUSE_INTE,
		(u_char *) MOUSE_RESET,
		(u_char *) MOUSE_INIT1,
		(u_char *) MOUSE_INIT2,
		(u_char *) KEYB_BUZZ,
		(u_char *) KEYB_BUZZF,
		RX_MSINTE,
		RX_MSINT,
		RX_MSRDY,
#ifdef news3400
		0x80,	/* 1200 bps */
		0,
		0
#else
		1,
		0xe0,
		0x0a
#endif
	},
	{
		(u_char *) KEYB_STAT,
		(u_char *) KEYB_DATA,
		(u_char *) KEYB_INTE,
		(u_char *) KEYB_RESET,
		(u_char *) KEYB_INIT1,
		(u_char *) KEYB_INIT2,
		(u_char *) KEYB_BUZZ,
		(u_char *) KEYB_BUZZF,
		RX_KBINTE,
		RX_KBINT,
		RX_KBRDY,
#ifdef news3400
		0xf0,	/* 9600 bps */
		0,
		0
#else
		0,
		0xc0,
		0x0a
#endif
	}
};

extern int kbd_flush();

static struct callout ms_helper_ch = CALLOUT_INITIALIZER;
static struct callout kb_soft_ch = CALLOUT_INITIALIZER;

void
kbm_open(chan)
	int chan;
{
	register Kbm_sw *kbm = &Kbm_port[chan];

#ifdef news3400
	/*
	 * Reset KB I/F.
	 * Disable KB interrupt.
	 * Clear KB overrun flag.
	 */
	*(volatile u_char *)kbm->reset_port = (u_char)0x01;
	*(volatile u_char *)kbm->init1_port = kbm->init1;
	if (chan == SCC_MOUSE)
		*(volatile u_char *)kbm->intr_port |= kbm->intr_en;
#else
	*kbm->reset_port = (u_char)0;
	*kbm->intr_port = (u_char)1;
#endif
	kbd_flush();
}

void
kbm_close(chan)
	int chan;
{
	register Kbm_sw *kbm = &Kbm_port[chan];

#ifdef news3400
	*(volatile u_char *)kbm->reset_port = (u_char)0x01;
#else
	*kbm->reset_port = (u_char)0;
	*kbm->intr_port = (u_char)0;
#endif
}

void
kbm_rint(chan)
	int chan;
{
#ifdef news3400
	volatile u_char *port = (volatile u_char *)Kbm_port[chan].data_port;
	volatile u_char *stat = (volatile u_char *)Kbm_port[chan].stat_port;
	volatile u_char *inte = (volatile u_char *)Kbm_port[chan].intr_port;
#else
	register u_char *port = Kbm_port[chan].data_port;
	register u_char *stat = Kbm_port[chan].stat_port;
	register u_char *inte = Kbm_port[chan].intr_port;
#endif
	int rdy = Kbm_port[chan].data_rdy;
	u_char code;

#ifdef news3400
	*inte &= ~Kbm_port[chan].intr_en;
#endif

	while (*stat & rdy) {
		code = *port;
		switch (chan) {
		    case SCC_MOUSE: {
#if NMS > 0
			extern void _ms_helper();

			if (xputc(code, SCC_MOUSE) < 0)
				printf("mouse queue overflow\n");
			/* KU:XXX softcall? */
			callout_reset(&ms_helper_ch, 0, _ms_helper, NULL);
#endif
			break;
		    }
		    case SCC_KEYBOARD: {
#if NKB > 0
			extern void kb_softint();

			if (xputc(code, SCC_KEYBOARD) < 0)
				printf("keyboard queue overflow\n");
			/* KU:XXX softcall? */
			callout_reset(&kb_soft_ch, 0, kb_softint, NULL);
#endif
			break;
		    }
		    default:
			printf("kb or ms stray intr\n");
			break;
		}
	}

#ifdef news3400
	*inte |= Kbm_port[chan].intr_en;
#else
	*inte = 1;
#endif
}

int
kbm_write(chan, buf, count)
	int chan;
	char *buf;
	register int count;
{
	register u_char *port = Kbm_port[chan].buzz_port;
	int c_save = count;

#ifdef news3400
	*port = count / 3;
#endif

	return (c_save);
}

int
kbm_getc(chan)
	int chan;
{
#ifdef news3400
	volatile u_char *port = (volatile u_char *)Kbm_port[chan].data_port;
	volatile u_char *stat = (volatile u_char *)Kbm_port[chan].stat_port;
#else
	register u_char *port = Kbm_port[chan].data_port;
	register u_char *stat = Kbm_port[chan].stat_port;
#endif
	int rdy = Kbm_port[chan].data_rdy;

	if (*stat & rdy) 
		return (*port & 0xff);
	else
		return -1;
}

#ifdef CPU_SINGLE
/*
 * small ring buffers for keyboard/mouse
 */
static struct ring_buf {
	u_char head;
	u_char tail;
	u_char count;
	u_char buf[13];
} ring_buf[2];

int
xputc(c, chan)
	u_char c;
	int chan;
{
	register struct ring_buf *p = &ring_buf[chan];
	int s = splhigh();

	if (p->count >= sizeof (p->buf)) {
		splx(s);
		return -1;
	}
	p->buf[p->head] = c;
	if (++p->head >= sizeof (p->buf))
		p->head = 0;
	p->count++;
	splx(s);
	return c;
}

int
xgetc(chan)
	int chan;
{
	register struct ring_buf *p = &ring_buf[chan];
	int c;
	int s = splhigh();

	if (p->count == 0) {
		splx(s);
		return -1;
	}
	c = p->buf[p->tail];
	if (++p->tail >= sizeof (p->buf))
		p->tail = 0;
	p->count--;
	splx(s);
	return c;
}
#endif /* CPU_SINGLE */
