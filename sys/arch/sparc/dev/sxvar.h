/*	$NetBSD: sxvar.h,v 1.5 2023/06/13 10:09:31 macallan Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael Lorenz.
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

#ifndef SXVAR_H
#define SXVAR_H

#include <sparc/dev/sxreg.h>

struct sx_softc {
	device_t		sc_dev;
	bus_addr_t		sc_uregs;
	bus_space_tag_t		sc_tag;
	bus_space_handle_t	sc_regh;
	int			sc_cnt;
};

static inline void
sx_write(struct sx_softc *sc, int addr, uint32_t val)
{
	bus_space_write_4(sc->sc_tag, sc->sc_regh, addr, val);
}

static inline uint32_t
sx_read(struct sx_softc *sc, int addr)
{
	return bus_space_read_4(sc->sc_tag, sc->sc_regh, addr);
}

/*
 * to be used before issuing SX instructions
 * this will periodically allow the instruction queue to drain in order
 * to avoid excessive MBus relinquish & retry cycles during long SX ops
 * which may cause us to lose interrupts
 */
static inline void
sx_wait(struct sx_softc *sc)
{
	uint32_t reg;
	if (sc->sc_cnt > 6) {
		do {
			reg = bus_space_read_4(sc->sc_tag, sc->sc_regh,
						 SX_CONTROL_STATUS);
		} while ((reg & SX_MT) == 0);
		sc->sc_cnt = 0;
	} else
		sc->sc_cnt++;
}

void sx_dump(void);

#endif
