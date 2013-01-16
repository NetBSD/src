/*	$NetBSD: z3rambdvar.h,v 1.1.2.2 2013/01/16 05:32:42 yamt Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
#ifndef _AMIGA_Z3RAMBDVAR_H_

#include <sys/bus.h>

/* there are a couple of hacks that check for presence of Z3 RAM boards */
#define ZORRO_MANID_E3B		3643
#define ZORRO_PRODID_ZORRAM	32	/* BigRamPlus has the same */

#define ZORRO_MANID_DKB		2012
#define ZORRO_PRODID_3128	14	

#define ZORRO_MANID_PHASE5	8512	
#define ZORRO_PRODID_FLZ3MEM	10

struct z3rambd_softc {
	device_t		sc_dev;

	size_t			sc_size;

	struct bus_space_tag	sc_bst;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	void			*sc_va;
};

inline int z3rambd_match_id(uint16_t, uint8_t);

inline int
z3rambd_match_id(uint16_t manid, uint8_t prodid)
{
	if (manid == ZORRO_MANID_E3B) 
		if (prodid == ZORRO_PRODID_ZORRAM)
			return 1;

	if (manid == ZORRO_MANID_DKB) 
		if (prodid == ZORRO_PRODID_3128)
			return 1;

	if (manid == ZORRO_MANID_PHASE5) 
		if (prodid == ZORRO_PRODID_FLZ3MEM)
			return 1;

	return 0;
}

#endif /* _AMIGA_Z3RAMBDVAR_H_ */

