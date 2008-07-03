/*	$NetBSD: at91aicvar.h,v 1.2 2008/07/03 01:15:38 matt Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _AT91AICVAR_H_
#define _AT91AICVAR_H_

struct intrhand {
	TAILQ_ENTRY(intrhand) ih_list;	/* link on intrq list */
	int (*ih_func)(void *);		/* interrupt handler */
	void *ih_arg;			/* arg for handler */
	int ih_ipl;			/* IPL_* */
	int ih_irq;			/* IRQ number */
};

#define	IRQNAMESIZE	sizeof("aic irq xxx")

struct intrq {
	TAILQ_HEAD(, intrhand) iq_list;	/* handler list */
	struct evcnt iq_ev;		/* event counter */
	u_int32_t iq_levels;		/* IPL_*'s this IRQ has */
	char iq_name[IRQNAMESIZE];	/* interrupt name */
	int iq_type;			/* interrupt request type: */
#define	_INTR_LOW_LEVEL		1		/* interrupt when signal at low level */
#define	INTR_HIGH_LEVEL		2		/* interrupt when signal at high level */
#define	INTR_FALLING_EDGE	4		/* interrupt on falling edge */
#define	INTR_RISING_EDGE	8		/* interrupt on rising edge */
#define	INTR_MASK		0xF
	volatile int iq_busy;		/* set if irq is busy */
};

void at91aic_init(void);
void *at91aic_intr_establish(int irq, int ipl, int type, int (*ih_func)(void *), void *arg);
void at91aic_intr_disestablish(void *cookie);
void at91aic_intr_poll(void *ihp, int flags);
void at91aic_intr_dispatch(struct irqframe *frame);

#endif /* _AT91AICVAR_H_ */
