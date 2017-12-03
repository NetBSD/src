/* $NetBSD: pl031.c,v 1.1.8.2 2017/12/03 11:37:03 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pl031.c,v 1.1.8.2 2017/12/03 11:37:03 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/clock_subr.h>

#include <dev/ic/pl031var.h>

#define	RTCDR	0x000
#define	RTCLR	0x008
#define	RTCCR	0x00c
#define	 RTCCR_START	__BIT(0)

#define	RTC_READ(sc, reg)			\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	RTC_WRITE(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
plrtc_gettime(todr_chip_handle_t tch, struct timeval *tv)
{
	struct plrtc_softc * const sc = tch->cookie;

	tv->tv_sec = RTC_READ(sc, RTCDR);
	tv->tv_usec = 0;

	return 0;
}

static int
plrtc_settime(todr_chip_handle_t tch, struct timeval *tv)
{
	struct plrtc_softc * const sc = tch->cookie;

	RTC_WRITE(sc, RTCLR, tv->tv_sec);

	return 0;
}

void
plrtc_attach(struct plrtc_softc *sc)
{
	aprint_naive("\n");
	aprint_normal(": RTC\n");

	sc->sc_todr.todr_gettime = plrtc_gettime;
	sc->sc_todr.todr_settime = plrtc_settime;
	sc->sc_todr.cookie = sc;

	RTC_WRITE(sc, RTCCR, RTCCR_START);
}
