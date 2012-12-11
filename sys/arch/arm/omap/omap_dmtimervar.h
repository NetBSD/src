/*	$NetBSD: omap_dmtimervar.h,v 1.1 2012/12/11 19:01:18 riastradh Exp $	*/

/*
 * TI OMAP Dual-mode timers: Driver state
 */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _ARM_OMAP_OMAP_DMTIMERVAR_H_
#define _ARM_OMAP_OMAP_DMTIMERVAR_H_

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device_if.h>

struct omap_module;

struct omap_dmtimer_softc {
	device_t			sc_dev;
	const struct omap_module	*sc_module;
	unsigned int			sc_version;
	int				sc_intr;
	bus_space_tag_t			sc_iot;
	bus_space_handle_t		sc_ioh;

	bool				sc_posted;
	bool				sc_enabled;
};

void	omap_dmtimer_attach_timecounter(struct omap_dmtimer_softc *);
void	omap_dmtimer_attach_hardclock(struct omap_dmtimer_softc *);
void	omap_dmtimer_attach_statclock(struct omap_dmtimer_softc *);

#endif  /* _ARM_OMAP_OMAP_DMTIMERVAR_H_ */
