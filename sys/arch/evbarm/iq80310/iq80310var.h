/*	$NetBSD: iq80310var.h,v 1.5 2002/02/07 21:34:24 thorpej Exp $	*/

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _IQ80310_IQ80310VAR_H_
#define	_IQ80310_IQ80310VAR_H_

#include "opt_iop310.h"

/*
 * If no IOP310 board type options are specified, default to IQ80310.
 * Otherwise, make sure only one board type option is specified.
 */
#if (defined(IOP310_TEAMASA_NPWR)) > 1
#error May not define more than one IOP310 board type
#endif

#include <sys/queue.h>
#include <dev/pci/pcivar.h>

/*
 * We currently support 8 interrupt sources.
 */
#define	NIRQ		8

struct intrhand {
	TAILQ_ENTRY(intrhand) ih_list;	/* link on intrq list */
	int (*ih_func)(void *);		/* handler */
	void *ih_arg;			/* arg for handler */
	int ih_ipl;			/* IPL_* */
	int ih_irq;			/* IRQ number */
};

#define	IRQNAMESIZE	sizeof("iq80310 irq 8")

struct intrq {
	TAILQ_HEAD(, intrhand) iq_list;	/* handler list */
	struct evcnt iq_ev;		/* event counter */
	int iq_mask;			/* IRQs to mask while handling */
	int iq_levels;			/* IPL_*'s this IRQ has */
	int iq_ist;			/* share type */
	char iq_name[IRQNAMESIZE];	/* interrupt name */
};

/*
 * XINT3 bits 0-4 are "IRQ 0-4".  XINT0 bits 0-2 are "IRQ 5-7".
 */
#define	XINT3_IRQ(x)	(x)
#define	XINT0_IRQ(x)	((x) + 5)

void	iq80310_calibrate_delay(void);

void	iq80310_7seg(char, char);
void	iq80310_7seg_snake(void);

void	iq80310_pci_init(pci_chipset_tag_t, void *);

void	iq80310_intr_init(void);
void	*iq80310_intr_establish(int, int, int (*)(void *), void *);
void	iq80310_intr_disestablish(void *);

#endif /* _IQ80310_IQ80310VAR_H_ */
