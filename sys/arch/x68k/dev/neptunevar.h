/*	$NetBSD: neptunevar.h,v 1.4.4.1 2008/06/27 15:11:18 simonb Exp $	*/

/*
 *
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto.
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

/*
 * Neptune-X -- X68k-ISA Bus Bridge
 */

#ifndef _NEPTUNEVAR_H_
#define _NEPTUNEVAR_H_

#include <sys/malloc.h>
#include <sys/extent.h>
#include <machine/frame.h>
#include <arch/x68k/dev/intiovar.h>
#include "locators.h"

#define neptune_cf_addr	cf_loc[NEPTUNECF_ADDR]


struct neptune_attach_args {
	bus_space_tag_t	na_bst;	/* bus_space tag */

	int		na_addr; /* addr */
	int		na_intr; /* intr */
};

struct neptune_softc {
	bus_space_tag_t	sc_bst;
	vaddr_t		sc_addr;
};

#define neptune_intr_establish intio_intr_establish
#endif
