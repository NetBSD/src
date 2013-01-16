/*	$NetBSD: a2kbbc.c,v 1.23.2.2 2013/01/16 05:32:41 yamt Exp $ */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: clock.c 1.18 91/01/21$
 *
 *	@(#)clock.c	7.6 (Berkeley) 5/7/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: a2kbbc.c,v 1.23.2.2 2013/01/16 05:32:41 yamt Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <machine/psl.h>
#include <machine/cpu.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/cia.h>
#include <amiga/dev/zbusvar.h>

#include <dev/clock_subr.h>

#include <dev/ic/msm6242bvar.h>
#include <dev/ic/msm6242breg.h>

#define	A2KBBC_ADDR	0xDC0000	/* XXX: possible D80000 on A2000 "A"? */

static int a2kbbc_match(device_t, cfdata_t, void *);
static void a2kbbc_attach(device_t, device_t, void *);

struct a2kbbc_softc {
	struct msm6242b_softc sc_msm6242b;
	struct bus_space_tag sc_bst;
};

CFATTACH_DECL_NEW(a2kbbc, sizeof(struct a2kbbc_softc),
    a2kbbc_match, a2kbbc_attach, NULL, NULL);

static int
a2kbbc_match(device_t parent, cfdata_t cf, void *aux)
{
	//struct clock_ymdhms dt;
	static int a2kbbc_matched = 0;

	if (!matchname("a2kbbc", aux))
		return (0);

	/* Allow only one instance. */
	if (a2kbbc_matched)
		return (0);

	if (/* is_a1200() || */ is_a3000() || is_a4000()
#ifdef DRACO
	    || is_draco()
#endif
	    )
		return (0);

	a2kbbc_matched = 1;
	return (1);
}

/*
 * Attach us to the rtc function pointers.
 */
void
a2kbbc_attach(device_t parent, device_t self, void *aux)
{
	struct a2kbbc_softc *sc;
	struct msm6242b_softc *msc;

	sc = device_private(self);
	msc = &sc->sc_msm6242b;
	msc->sc_dev = self;

	sc->sc_bst.base = (bus_addr_t) __UNVOLATILE(ztwomap(A2KBBC_ADDR+1));
	sc->sc_bst.absm = &amiga_bus_stride_4;

	msc->sc_iot = &sc->sc_bst;

	if (bus_space_map(msc->sc_iot, 0, MSM6242B_SIZE, 0, &msc->sc_ioh)) {
		aprint_error_dev(msc->sc_dev, "couldn't map registers\n");
		return;
	}

	msm6242b_attach(msc);
}


