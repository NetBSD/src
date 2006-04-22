/* $NetBSD: aurtc.c,v 1.7.6.1 2006/04/22 11:37:41 simonb Exp $ */

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

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aurtc.c,v 1.7.6.1 2006/04/22 11:37:41 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <dev/clock_subr.h>

#include <machine/bus.h>

#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/include/auvar.h>
#include <mips/alchemy/include/aubusvar.h>

#define	REGVAL(x) (*(volatile uint32_t *)(MIPS_PHYS_TO_KSEG1(PC_BASE + (x))))
#define	GETREG(x)	(REGVAL(x))
#define	PUTREG(x,v)	(REGVAL(x) = (v))

struct aurtc_softc {
	struct device		sc_dev;
	struct todr_chip_handle	sc_tch;
	void			*sc_shutdownhook;
};

static int	aurtc_match(struct device *, struct cfdata *, void *);
static void	aurtc_attach(struct device *, struct device *, void *);
static int	aurtc_gettime(todr_chip_handle_t, volatile struct timeval *);
static int	aurtc_settime(todr_chip_handle_t, volatile struct timeval *);
static int	aurtc_getcal(todr_chip_handle_t, int *);
static int	aurtc_setcal(todr_chip_handle_t, int);
static void	aurtc_shutdown(void *);

CFATTACH_DECL(aurtc, sizeof (struct aurtc_softc),
    aurtc_match, aurtc_attach, NULL, NULL);

int
aurtc_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct aubus_attach_args *aa = aux;

	if (strcmp(aa->aa_name, match->cf_name) == 0)
		return (1);

	return (0);
}

void
aurtc_attach(struct device *parent, struct device *self, void *aux)
{
	struct aurtc_softc *sc = (struct aurtc_softc *)self;

	printf(": Au1X00 programmable clock\n");
	
	sc->sc_tch.cookie = sc;
	sc->sc_tch.bus_cookie = NULL;
	sc->sc_tch.todr_gettime = aurtc_gettime;
	sc->sc_tch.todr_settime = aurtc_settime;
	sc->sc_tch.todr_getcal = aurtc_getcal;
	sc->sc_tch.todr_setcal = aurtc_setcal;
	sc->sc_tch.todr_setwen = NULL;

	sc->sc_shutdownhook = shutdownhook_establish(aurtc_shutdown, NULL);

	todr_attach(&sc->sc_tch);
}

/*
 * Note that our RTC only has second resolution.
 */

int
aurtc_gettime(todr_chip_handle_t tch, volatile struct timeval *tv)
{
	int			s;

	s = splclock();
	tv->tv_sec = GETREG(PC_COUNTER_READ_0);
	splx(s);
	return 0;
}

int
aurtc_settime(todr_chip_handle_t tch, volatile struct timeval *tvp)
{
	int			s;
	struct timeval		tv;

	s = splclock();
	tv = *tvp;
	splx(s);

	/* wait for the clock register to be idle */
	while (GETREG(PC_COUNTER_CONTROL) & CC_C0S) {
		continue;
	}

	PUTREG(PC_COUNTER_WRITE0, tv.tv_sec);

	/*
	 * It could take a second or two for the clock change to take effect.
	 * We don't want to make settimeofday() take that long, so we don't
	 * wait here, but instead wait in flush.  This can have a bad effect
	 * for settimeofday() calls with a short window between them.
	 */
	return 0;
}

int
aurtc_getcal(todr_chip_handle_t tch, int *vp)
{

	return EOPNOTSUPP;
}

int
aurtc_setcal(todr_chip_handle_t tch, int v)
{

	return EOPNOTSUPP;
}

void
aurtc_shutdown(void *arg)
{

	/* wait for the clock register to be idle */
	while (GETREG(PC_COUNTER_CONTROL) & CC_C0S) {
		continue;
	}
}
