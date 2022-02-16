/*	$NetBSD: gb225var.h,v 1.4 2022/02/16 20:31:43 andvar Exp $ */

/*
 * Copyright (c) 2002, 2003  Genetec corp.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec corp.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec corp. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORP. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORP.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _EVBARM_GB225VAR_H
#define _EVBARM_GB225VAR_H

#include <sys/queue.h>
#include <sys/callout.h>

/*
 * Interrupt sources for option board CPLD.
 */
#define	OPIO_INTR_CF_INSERT	0
#define	OPIO_INTR_PCMCIA_INSERT	1
#define	OPIO_INTR_USB_POWER	2	/* USB power fault */
#define	OPIO_INTR_CARD_POWER	3	/* PCMCIA/CF power fault */
#define	N_OPIO_INTR 		4


struct opio_intr_handler;

struct opio_softc {
	device_t sc_dev;

	bus_space_tag_t      sc_iot;
	bus_space_handle_t   sc_ioh;

	bus_space_handle_t   sc_memctl_ioh;  /* PXA2x0's memory controller */

	void *sc_ih;			/* interrupt handler */
	/* callout for debounce */
	struct callout sc_callout;

	enum {ST_STABLE, ST_BOUNCING } sc_debounce;

	struct opio_intr_handler {
		int (* func)(void *, int);
		void *arg;
		int level;

		uint8_t	reported_state;
		uint8_t last_state;
		short	debounce_count;
	} sc_intr[N_OPIO_INTR];
};


void *opio_intr_establish(struct opio_softc *, int, int,
    int (*)(void *, int), void *);

#endif /* _EVBARM_GB225VAR_H */
