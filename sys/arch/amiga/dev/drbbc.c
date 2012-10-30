/*	$NetBSD: drbbc.c,v 1.19.8.1 2012/10/30 17:18:47 yamt Exp $ */

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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
__KERNEL_RCSID(0, "$NetBSD: drbbc.c,v 1.19.8.1 2012/10/30 17:18:47 yamt Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>

#if 0
#include <machine/psl.h>
#endif
#include <machine/cpu.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/cia.h>
#include <amiga/amiga/drcustom.h>
#include <amiga/dev/rtc.h>

#include <dev/clock_subr.h>
#include <dev/ic/ds.h>

int draco_ds_read_bit(void *);
void draco_ds_write_bit(void *, int);
void draco_ds_reset(void *);

void drbbc_attach(device_t, device_t, void *);
int drbbc_match(device_t, cfdata_t, void *);

int dracougettod(todr_chip_handle_t, struct timeval *);
int dracousettod(todr_chip_handle_t, struct timeval *);

static struct todr_chip_handle dracotodr;
struct drbbc_softc {
	struct ds_handle sc_dsh;
};

CFATTACH_DECL_NEW(drbbc, sizeof(struct drbbc_softc),
    drbbc_match, drbbc_attach, NULL, NULL);

struct drbbc_softc *drbbc_sc;

int
drbbc_match(device_t parent, cfdata_t cf, void *aux)
{
	static int drbbc_matched = 0;

	/* Allow only one instance. */
	if (!is_draco() || !matchname(aux, "drbbc") || drbbc_matched)
		return (0);

	drbbc_matched = 1;
	return (1);
}

void
drbbc_attach(device_t parent, device_t self, void *aux)
{
	int i;
	struct drbbc_softc *sc;
	u_int8_t rombuf[8];

	sc = device_private(self);

	sc->sc_dsh.ds_read_bit = draco_ds_read_bit;
	sc->sc_dsh.ds_write_bit = draco_ds_write_bit;
	sc->sc_dsh.ds_reset = draco_ds_reset;
	sc->sc_dsh.ds_hw_handle = (void *)(DRCCADDR + DRIOCTLPG*PAGE_SIZE);

	sc->sc_dsh.ds_reset(sc->sc_dsh.ds_hw_handle);

	ds_write_byte(&sc->sc_dsh, DS_ROM_READ);
	for (i=0; i<8; ++i)
		rombuf[i] = ds_read_byte(&sc->sc_dsh);

	hostid = (rombuf[3] << 24) + (rombuf[2] << 16) +
		(rombuf[1] << 8) + rombuf[7];

	printf(": ROM %02x %02x%02x%02x%02x%02x%02x %02x (DraCo sernum %ld)\n",
		rombuf[7], rombuf[6], rombuf[5], rombuf[4],
		rombuf[3], rombuf[2], rombuf[1], rombuf[0],
		hostid);

	drbbc_sc = sc;
	dracotodr.cookie = sc;
	dracotodr.todr_gettime = dracougettod;
	dracotodr.todr_settime = dracousettod;
	todr_attach(&dracotodr);
}

int
draco_ds_read_bit(void *p)
{
	struct drioct *draco_ioctl;

	draco_ioctl = p;

	while (draco_ioctl->io_status & DRSTAT_CLKBUSY);

	draco_ioctl->io_clockw1 = 0;

	while (draco_ioctl->io_status & DRSTAT_CLKBUSY);

	return (draco_ioctl->io_status & DRSTAT_CLKDAT);
}

void
draco_ds_write_bit(void *p, int b)
{
	struct drioct *draco_ioctl;

	draco_ioctl = p;

	while (draco_ioctl->io_status & DRSTAT_CLKBUSY);

	if (b)
		draco_ioctl->io_clockw1 = 0;
	else
		draco_ioctl->io_clockw0 = 0;
}

void
draco_ds_reset(void *p)
{
	struct drioct *draco_ioctl;

	draco_ioctl = p;

	draco_ioctl->io_clockrst = 0;
}

int
dracougettod(todr_chip_handle_t h, struct timeval *tvp)
{
	u_int32_t clkbuf;
	u_int32_t usecs;

	drbbc_sc->sc_dsh.ds_reset(drbbc_sc->sc_dsh.ds_hw_handle);

	ds_write_byte(&drbbc_sc->sc_dsh, DS_ROM_SKIP);

	ds_write_byte(&drbbc_sc->sc_dsh, DS_MEM_READ_MEMORY);
	/* address of seconds/256: */
	ds_write_byte(&drbbc_sc->sc_dsh, 0x02);
	ds_write_byte(&drbbc_sc->sc_dsh, 0x02);

	usecs = (ds_read_byte(&drbbc_sc->sc_dsh) * 1000000) / 256;
	clkbuf = ds_read_byte(&drbbc_sc->sc_dsh)
	    + (ds_read_byte(&drbbc_sc->sc_dsh)<<8)
	    + (ds_read_byte(&drbbc_sc->sc_dsh)<<16)
	    + (ds_read_byte(&drbbc_sc->sc_dsh)<<24);

	/* BSD time is wrt. 1.1.1970; AmigaOS time wrt. 1.1.1978 */

	clkbuf += (8*365 + 2) * 86400;

	tvp->tv_sec = clkbuf;
	tvp->tv_usec = usecs;

	return (0);
}

int
dracousettod(todr_chip_handle_t h, struct timeval *tvp)
{
	return (ENXIO);
}
