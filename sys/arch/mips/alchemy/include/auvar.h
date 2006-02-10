/* $NetBSD: auvar.h,v 1.7 2006/02/10 20:49:14 gdamore Exp $ */

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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

#ifndef _MIPS_ALCHEMY_AUVAR_H_
#define	_MIPS_ALCHEMY_AUVAR_H_

#include "opt_alchemy.h"

/*
 * 224MB virtual PCI memory.  Note that we probably cannot
 * really use all this because of limits on the number wired
 * entries we can have at once.  In all likelihood, we won't
 * use wired entries for more than a few megs anyway, since
 * big devices like framebuffers are likely to be mapped into
 * USEG using ordinary TLB entries.
 *
 * This tunable is an area for possible investigation and improvement.
 */

#define	AU_PCI_MEM_VA		0xE0000000UL
#define	AU_PCI_MEM_SZ		0x0B000000UL	/* 176 MB */

#define	AU_PCI_IO_VA		0xEB000000UL
#define	AU_PCI_IO_SZ		0x01000000UL	/* 16 MB */

#define	AU_PCI_CFG_VA		0xEC000000UL
#define	AU_PCI_CFG_SZ		0x01000000UL	/* 16 MB, overkill */

#define	AU_PCIMCIA_IO_VA	0xED000000UL
#define	AU_PCIMCCA_IO_SZ	0x01000000UL	/* 16 MB */
#define	AU_PCIMCIA_ATTR_VA	0xEE000000UL
#define	AU_PCIMCCA_ATTR_SZ	0x01000000UL	/* 16 MB */
#define	AU_PCIMCIA_MEM_VA	0xEF000000UL
#define	AU_PCIMCCA_MEM_SZ	0x01000000UL	/* 16 MB */

struct au_dev {
	const char *name;
	bus_addr_t addr[3];
	int irq[2];
};

struct au_chipdep {
	const char	*name;
	bus_addr_t	icus[2];	/* in case it ever changes */
	struct au_dev	*devices;
	const char	**irqnames;
};

struct au_chipdep	*au_chipdep(void);

#ifdef	ALCHEMY_AU1000
boolean_t	au1000_match(struct au_chipdep **);
#endif
#ifdef	ALCHEMY_AU1100
boolean_t	au1100_match(struct au_chipdep **);
#endif
#ifdef	ALCHEMY_AU1500
boolean_t	au1500_match(struct au_chipdep **);
#endif
#ifdef	ALCHEMY_AU1550
boolean_t	au1550_match(struct au_chipdep **);
#endif

void	au_intr_init(void);
void	*au_intr_establish(int, int, int, int, int (*)(void *), void *);
void	au_intr_disestablish(void *);
void	au_intr_enable(int);
void	au_intr_disable(int);
void	au_iointr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);

void	au_cpureg_bus_mem_init(bus_space_tag_t, void *);

void	au_cal_timers(bus_space_tag_t, bus_space_handle_t);
#endif /* _MIPS_ALCHEMY_AUVAR_H_ */
