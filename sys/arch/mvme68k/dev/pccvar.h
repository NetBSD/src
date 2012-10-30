/*	$NetBSD: pccvar.h,v 1.10.34.1 2012/10/30 17:20:03 yamt Exp $	*/

/*-
 * Copyright (c) 1996, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Steve C. Woodford.
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

#ifndef _MVME68K_PCCVAR_H
#define _MVME68K_PCCVAR_H

/* For isrlink_evcnt() definition */
#include <mvme68k/mvme68k/isr.h>

/*
 * Structure used to attach PCC devices.
 */
struct pcc_attach_args {
	const char	*pa_name;	/* name of device */
	int		pa_ipl;		/* interrupt level */
	bus_dma_tag_t	pa_dmat;
	bus_space_tag_t	pa_bust;
	bus_addr_t	pa_offset;

	bus_addr_t	_pa_base;
};

/* Shorthand for locators. */
#include "locators.h"
#define pcccf_ipl	cf_loc[PCCCF_IPL]


struct pcc_softc {
	bus_space_tag_t sc_bust;
	bus_space_handle_t sc_bush;
	struct evcnt sc_evcnt;
};

extern struct pcc_softc *sys_pcc;
extern bus_addr_t pcc_slave_base_addr;

#define	pccintr_evcnt(ipl)	isrlink_evcnt(ipl)
void	pccintr_establish(int, int (*)(void *), int, void *, struct evcnt *);
void	pccintr_disestablish(int);

#endif /* _MVME68K_PCCVAR_H */
