/*	$NetBSD: ep93xxvar.h,v 1.4.2.2 2013/01/16 05:32:46 yamt Exp $ */
/*
 * Copyright (c) 2004 Jesse Off
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

#ifndef _EP93XXVAR_H_
#define _EP93XXVAR_H_

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/queue.h>

#include <sys/bus.h>

struct intrhand {
	TAILQ_ENTRY(intrhand) ih_list;	/* link on intrq list */
	int (*ih_func)(void *);		/* interrupt handler */
	void *ih_arg;			/* arg for handler */
	int ih_ipl;			/* IPL_* */
	int ih_irq;			/* IRQ number */
};

#define	IRQNAMESIZE	sizeof("ep93xxintr irq xxx")

struct intrq {
	TAILQ_HEAD(, intrhand) iq_list;	/* handler list */
	struct evcnt iq_ev;		/* event counter */
	uint32_t iq_vic1_mask;		/* VIC1 IRQs to mask while handling */
	uint32_t iq_vic2_mask;		/* VIC2 IRQs to mask while handling */
	uint32_t iq_levels;		/* IPL_*'s this IRQ has */
	char iq_name[IRQNAMESIZE];	/* interrupt name */
	int iq_ist;			/* share type */
};

struct pmap_ent {
	const char*	msg;
	vaddr_t		va;
	paddr_t		pa;
	vsize_t		sz;
	int		prot;
	int		cache;
};

extern struct bus_space	ep93xx_bs_tag;
extern struct arm32_bus_dma_tag ep93xx_bus_dma;

void	ep93xx_intr_init(void);
void	*ep93xx_intr_establish(int irq, int ipl, int (*)(void *), void *);
void	ep93xx_intr_disestablish(void *);
/* Platform needs to provide this */
bus_dma_tag_t ep93xx_bus_dma_init(struct arm32_bus_dma_tag *);
void	ep93xx_reset(void);

#endif /* _EP93XXVAR_H_ */
