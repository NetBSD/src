/*	$NetBSD: mk48txx.c,v 1.10.16.1 2004/08/03 10:46:17 skrll Exp $ */
/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/*
 * Mostek MK48T02, MK48T08, MK48T59 time-of-day chip subroutines.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mk48txx.c,v 1.10.16.1 2004/08/03 10:46:17 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <machine/bus.h>
#include <dev/clock_subr.h>
#include <dev/ic/mk48txxreg.h>
#include <dev/ic/mk48txxvar.h>

int mk48txx_gettime(todr_chip_handle_t, struct timeval *);
int mk48txx_settime(todr_chip_handle_t, struct timeval *);
int mk48txx_getcal(todr_chip_handle_t, int *);
int mk48txx_setcal(todr_chip_handle_t, int);
u_int8_t mk48txx_def_nvrd(struct mk48txx_softc *, int);
void mk48txx_def_nvwr(struct mk48txx_softc *, int, u_int8_t);

struct {
	const char *name;
	bus_size_t nvramsz;
	bus_size_t clkoff;
	int flags;
#define MK48TXX_EXT_REGISTERS	1	/* Has extended register set */
} mk48txx_models[] = {
	{ "mk48t02", MK48T02_CLKSZ, MK48T02_CLKOFF, 0 },
	{ "mk48t08", MK48T08_CLKSZ, MK48T08_CLKOFF, 0 },
	{ "mk48t18", MK48T18_CLKSZ, MK48T18_CLKOFF, 0 },
	{ "mk48t59", MK48T59_CLKSZ, MK48T59_CLKOFF, MK48TXX_EXT_REGISTERS },
};

void
mk48txx_attach(sc)
	struct mk48txx_softc *sc;
{
	todr_chip_handle_t handle;
	int i;

	printf(": %s", sc->sc_model);

	i = sizeof(mk48txx_models) / sizeof(mk48txx_models[0]);
	while (--i >= 0) {
		if (strcmp(sc->sc_model, mk48txx_models[i].name) == 0)
			break;
	}
	if (i < 0)
		panic("mk48txx_attach: unsupported model");

	sc->sc_nvramsz = mk48txx_models[i].nvramsz;
	sc->sc_clkoffset = mk48txx_models[i].clkoff;

	handle = &sc->sc_handle;
	handle->cookie = sc;
	handle->todr_gettime = mk48txx_gettime;
	handle->todr_settime = mk48txx_settime;
	handle->todr_getcal = mk48txx_getcal;
	handle->todr_setcal = mk48txx_setcal;
	handle->todr_setwen = NULL;

	if (sc->sc_nvrd == NULL)
		sc->sc_nvrd = mk48txx_def_nvrd;
	if (sc->sc_nvwr == NULL)
		sc->sc_nvwr = mk48txx_def_nvwr;
}

/*
 * Get time-of-day and convert to a `struct timeval'
 * Return 0 on success; an error number otherwise.
 */
int
mk48txx_gettime(handle, tv)
	todr_chip_handle_t handle;
	struct timeval *tv;
{
	struct mk48txx_softc *sc;
	bus_size_t clkoff;
	struct clock_ymdhms dt;
	int year;
	u_int8_t csr;

	sc = handle->cookie;
	clkoff = sc->sc_clkoffset;

	todr_wenable(handle, 1);

	/* enable read (stop time) */
	csr = (*sc->sc_nvrd)(sc, clkoff + MK48TXX_ICSR);
	csr |= MK48TXX_CSR_READ;
	(*sc->sc_nvwr)(sc, clkoff + MK48TXX_ICSR, csr);

	dt.dt_sec = FROMBCD((*sc->sc_nvrd)(sc, clkoff + MK48TXX_ISEC));
	dt.dt_min = FROMBCD((*sc->sc_nvrd)(sc, clkoff + MK48TXX_IMIN));
	dt.dt_hour = FROMBCD((*sc->sc_nvrd)(sc, clkoff + MK48TXX_IHOUR));
	dt.dt_day = FROMBCD((*sc->sc_nvrd)(sc, clkoff + MK48TXX_IDAY));
	dt.dt_wday = FROMBCD((*sc->sc_nvrd)(sc, clkoff + MK48TXX_IWDAY));
	dt.dt_mon = FROMBCD((*sc->sc_nvrd)(sc, clkoff + MK48TXX_IMON));
	year = FROMBCD((*sc->sc_nvrd)(sc, clkoff + MK48TXX_IYEAR));

	year += sc->sc_year0;
	if (year < POSIX_BASE_YEAR &&
	    (sc->sc_flag & MK48TXX_NO_CENT_ADJUST) == 0)
		year += 100;

	dt.dt_year = year;

	/* time wears on */
	csr = (*sc->sc_nvrd)(sc, clkoff + MK48TXX_ICSR);
	csr &= ~MK48TXX_CSR_READ;
	(*sc->sc_nvwr)(sc, clkoff + MK48TXX_ICSR, csr);
	todr_wenable(handle, 0);

	/* simple sanity checks */
	if (dt.dt_mon > 12 || dt.dt_day > 31 ||
	    dt.dt_hour >= 24 || dt.dt_min >= 60 || dt.dt_sec >= 60)
		return EIO;

	tv->tv_sec = clock_ymdhms_to_secs(&dt);
	tv->tv_usec = 0;
	return 0;
}

/*
 * Set the time-of-day clock based on the value of the `struct timeval' arg.
 * Return 0 on success; an error number otherwise.
 */
int
mk48txx_settime(handle, tv)
	todr_chip_handle_t handle;
	struct timeval *tv;
{
	struct mk48txx_softc *sc;
	bus_size_t clkoff;
	struct clock_ymdhms dt;
	u_int8_t csr;
	int year;
	long sec;

	sc = handle->cookie;
	clkoff = sc->sc_clkoffset;

	/* Note: we ignore `tv_usec' */
	sec = tv->tv_sec + ((tv->tv_usec < 500000) ? 0 : 1);
	clock_secs_to_ymdhms(sec, &dt);

	year = dt.dt_year - sc->sc_year0;
	if (year > 99 &&
	    (sc->sc_flag & MK48TXX_NO_CENT_ADJUST) == 0)
		year -= 100;

	todr_wenable(handle, 1);
	/* enable write */
	csr = (*sc->sc_nvrd)(sc, clkoff + MK48TXX_ICSR);
	csr |= MK48TXX_CSR_WRITE;
	(*sc->sc_nvwr)(sc, clkoff + MK48TXX_ICSR, csr);

	(*sc->sc_nvwr)(sc, clkoff + MK48TXX_ISEC, TOBCD(dt.dt_sec));
	(*sc->sc_nvwr)(sc, clkoff + MK48TXX_IMIN, TOBCD(dt.dt_min));
	(*sc->sc_nvwr)(sc, clkoff + MK48TXX_IHOUR, TOBCD(dt.dt_hour));
	(*sc->sc_nvwr)(sc, clkoff + MK48TXX_IWDAY, TOBCD(dt.dt_wday));
	(*sc->sc_nvwr)(sc, clkoff + MK48TXX_IDAY, TOBCD(dt.dt_day));
	(*sc->sc_nvwr)(sc, clkoff + MK48TXX_IMON, TOBCD(dt.dt_mon));
	(*sc->sc_nvwr)(sc, clkoff + MK48TXX_IYEAR, TOBCD(year));

	/* load them up */
	csr = (*sc->sc_nvrd)(sc, clkoff + MK48TXX_ICSR);
	csr &= ~MK48TXX_CSR_WRITE;
	(*sc->sc_nvwr)(sc, clkoff + MK48TXX_ICSR, csr);
	todr_wenable(handle, 0);
	return 0;
}

int
mk48txx_getcal(handle, vp)
	todr_chip_handle_t handle;
	int *vp;
{

	return EOPNOTSUPP;
}

int
mk48txx_setcal(handle, v)
	todr_chip_handle_t handle;
	int v;
{

	return EOPNOTSUPP;
}

int
mk48txx_get_nvram_size(handle, vp)
	todr_chip_handle_t handle;
	bus_size_t *vp;
{
	struct mk48txx_softc *sc;

	sc = handle->cookie;
	*vp = sc->sc_nvramsz;
	return 0;
}

u_int8_t
mk48txx_def_nvrd(struct mk48txx_softc *sc, int off)
{

	return bus_space_read_1(sc->sc_bst, sc->sc_bsh, off);
}

void
mk48txx_def_nvwr(struct mk48txx_softc *sc, int off, u_int8_t v)
{

	bus_space_write_1(sc->sc_bst, sc->sc_bsh, off, v);
}
