/* $NetBSD: aupcmciavar.h,v 1.1.10.2 2006/04/22 11:37:41 simonb Exp $ */

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

#ifndef _MIPS_ALCHEMY_DEV_AUPCMCIAVAR_H
#define	_MIPS_ALCHEMY_DEV_AUPCMCIAVAR_H

#define	AUPCMCIA_NWINS	16
#define	AUPCMCIA_NSLOTS	2	/* current boards only have two slots */

#define	AUPCMCIA_IRQ_CARD	0
#define	AUPCMCIA_IRQ_INSERT	1
#define	AUPCMCIA_MAP_SIZE	16 * 1024 * 1024	/* arbitrary */

struct aupcmcia_machdep {
	int		am_nslots;
	bus_size_t	(*am_slot_offset)(int);
	int		(*am_slot_irq)(int, int);
	void		(*am_slot_enable)(int);
	void		(*am_slot_disable)(int);
	int		(*am_slot_status)(int);
	const char *	(*am_slot_name)(int);
};

/*
 * Machdep code must implement this to supply its slot implementation
 * details to the framework.  The address 
 */
struct aupcmcia_machdep *aupcmcia_machdep(void);

#endif	/* _MIPS_ALCHEMY_DEV_AUPCIVAR_H */
