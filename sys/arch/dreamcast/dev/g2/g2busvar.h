/*	$NetBSD: g2busvar.h,v 1.2 2002/03/24 18:21:24 uch Exp $	*/

/*-
 * Copyright (c) 2001 Marcus Comstedt
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
 *	This product includes software developed by Marcus Comstedt.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _DREAMCAST_G2BUSVAR_H_
#define _DREAMCAST_G2BUSVAR_H_

#include <machine/bus.h>

/*
 * G2 driver attach arguments
 */
struct g2bus_attach_args {
	bus_space_tag_t ga_memt;
};

/*
 * Per-device G2 variables
 */
struct g2busdev {
	struct  device *gd_dev;		/* back pointer to generic */
	TAILQ_ENTRY(g2busdev)
		gd_bchain;		/* bus chain */
};

/*
 * G2 master bus
 */
struct g2bus_softc {
	struct	device sc_dev;		/* base device */
	struct dreamcast_bus_space sc_memt;
	TAILQ_HEAD(, g2busdev)
		sc_subdevs;		/* list of all children */
};

void	g2bus_bus_mem_init(struct g2bus_softc *);

#endif /* _DREAMCAST_SHBVAR_H_ */
