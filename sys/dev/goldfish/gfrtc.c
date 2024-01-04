/*	$NetBSD: gfrtc.c,v 1.4 2024/01/04 12:02:11 simonb Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson and by Jason R. Thorpe.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gfrtc.c,v 1.4 2024/01/04 12:02:11 simonb Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/device.h>

#include <dev/goldfish/gfrtcvar.h>

#define	GOLDFISH_RTC_TIME_LOW		0x00
#define	GOLDFISH_RTC_TIME_HIGH		0x04
#define	GOLDFISH_RTC_ALARM_LOW		0x08
#define	GOLDFISH_RTC_ALARM_HIGH		0x0c
#define	GOLDFISH_RTC_IRQ_ENABLED	0x10
#define	GOLDFISH_RTC_CLEAR_ALARM	0x14
#define	GOLDFISH_RTC_ALARM_STATUS	0x18
#define	GOLDFISH_RTC_CLEAR_INTERRUPT	0x1c

/*
 * https://android.googlesource.com/platform/external/qemu/+/master/docs/GOLDFISH-VIRTUAL-HARDWARE.TXT
 *
 * (Despite what the Google docs say, the Qemu goldfish-rtc implements
 * the timer functionality.)
 */

#define	GOLDFISH_RTC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	GOLDFISH_RTC_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
gfrtc_gettime(struct todr_chip_handle *ch, struct timeval *tv)
{
	struct gfrtc_softc *sc = ch->cookie;
	const uint64_t nsec = gfrtc_get_time(sc);

	tv->tv_sec = nsec / 1000000000;
	tv->tv_usec = (nsec - tv->tv_sec * 1000000000) / 1000;

	return 0;
}

static int
gfrtc_settime(struct todr_chip_handle *ch, struct timeval *tv)
{
	struct gfrtc_softc *sc = ch->cookie;

	const uint64_t nsec = (tv->tv_sec * 1000000 + tv->tv_usec) * 1000;
	const uint32_t hi = (uint32_t)(nsec >> 32);
	const uint32_t lo = (uint32_t)nsec;

	GOLDFISH_RTC_WRITE(sc, GOLDFISH_RTC_TIME_HIGH, hi);
	GOLDFISH_RTC_WRITE(sc, GOLDFISH_RTC_TIME_LOW, lo);

	return 0;
}


void
gfrtc_attach(struct gfrtc_softc * const sc, bool todr)
{

	aprint_naive("\n");
	aprint_normal(": Google Goldfish RTC + timer\n");

	/* Cancel any alarms, make sure interrupt is disabled. */
	GOLDFISH_RTC_WRITE(sc, GOLDFISH_RTC_IRQ_ENABLED, 0);
	GOLDFISH_RTC_WRITE(sc, GOLDFISH_RTC_CLEAR_ALARM, 0);
	GOLDFISH_RTC_WRITE(sc, GOLDFISH_RTC_CLEAR_INTERRUPT, 0);

	if (todr) {
		aprint_normal_dev(sc->sc_dev,
		    "using as Time of Day Register.\n");
		sc->sc_todr.cookie = sc;
		sc->sc_todr.todr_gettime = gfrtc_gettime;
		sc->sc_todr.todr_settime = gfrtc_settime;
		sc->sc_todr.todr_setwen = NULL;
		todr_attach(&sc->sc_todr);
	}
}


void
gfrtc_delay(device_t dev, unsigned int usec)
{
	struct gfrtc_softc *sc = device_private(dev);
	uint64_t start_ns, end_ns, min_ns;

	/* Get the start time now while we do the setup work. */
	start_ns = gfrtc_get_time(sc);

	/* Delay for this many nsec. */
	min_ns = (uint64_t)usec * 1000;

	do {
		end_ns = gfrtc_get_time(sc);
	} while ((end_ns - start_ns) < min_ns);
}


uint64_t
gfrtc_get_time(struct gfrtc_softc * const sc)
{
	const uint64_t lo = GOLDFISH_RTC_READ(sc, GOLDFISH_RTC_TIME_LOW);
	const uint64_t hi = GOLDFISH_RTC_READ(sc, GOLDFISH_RTC_TIME_HIGH);

	return (hi << 32) | lo;
}


void
gfrtc_set_alarm(struct gfrtc_softc * const sc, const uint64_t nsec)
{
	uint64_t next = gfrtc_get_time(sc) + nsec;

	GOLDFISH_RTC_WRITE(sc, GOLDFISH_RTC_ALARM_HIGH, (uint32_t)(next >> 32));
	GOLDFISH_RTC_WRITE(sc, GOLDFISH_RTC_ALARM_LOW, (uint32_t)next);
}


void
gfrtc_clear_alarm(struct gfrtc_softc * const sc)
{
	GOLDFISH_RTC_WRITE(sc, GOLDFISH_RTC_CLEAR_ALARM, 0);
}


void
gfrtc_enable_interrupt(struct gfrtc_softc * const sc)
{
	GOLDFISH_RTC_WRITE(sc, GOLDFISH_RTC_IRQ_ENABLED, 1);
}


void
gfrtc_disable_interrupt(struct gfrtc_softc * const sc)
{
	GOLDFISH_RTC_WRITE(sc, GOLDFISH_RTC_IRQ_ENABLED, 0);
}


void
gfrtc_clear_interrupt(struct gfrtc_softc * const sc)
{
	GOLDFISH_RTC_WRITE(sc, GOLDFISH_RTC_CLEAR_INTERRUPT, 0);
}
