/*	$NetBSD: com_obio.c,v 1.5.2.1 2000/07/19 02:53:10 mrg Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/intr.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <sparc/sparc/auxreg.h>

struct com_obio_softc {
	struct com_softc osc_com;	/* real "com" softc */

	/* OBIO-specific goo. */
	struct evcnt osc_intrcnt;	/* interrupt counting */
};

static int com_obio_match __P((struct device *, struct cfdata *, void *));
static void com_obio_attach __P((struct device *, struct device *, void *));
static void com_obio_cleanup __P((void *));

struct cfattach com_obio_ca = {
	sizeof(struct com_obio_softc), com_obio_match, com_obio_attach
};

static int
com_obio_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;

	if (uoba->uoba_isobio4 != 0) {
		return (0);
	}

	/* Tadpole 3GX/3GS uses "modem" for a 16450 port
	 */
	if (strcmp("modem", sa->sa_name) == 0) {
		bus_space_handle_t ioh;
		int rv = 0;
		u_int8_t auxregval = *AUXIO4M_REG;
		 *AUXIO4M_REG = auxregval | (AUXIO4M_LED|AUXIO4M_LTE);
		DELAY(100);
		if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
				 sa->sa_offset, sa->sa_size,
				 BUS_SPACE_MAP_LINEAR, 0,
				 &ioh) == 0) {
			rv = comprobe1(sa->sa_bustag, ioh);
#if 0
			printf("modem: probe: lcr=0x%02x iir=0x%02x\n",
				bus_space_read_1(sa->sa_bustag, ioh, 3),
				bus_space_read_1(sa->sa_bustag, ioh, 2));
#endif
			bus_space_unmap(sa->sa_bustag, ioh, sa->sa_size);
		}
		*AUXIO4M_REG = auxregval;
		return (rv);
	}
	return (0);
}

static void
com_obio_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct com_obio_softc *osc = (void *)self;
	struct com_softc *sc = &osc->osc_com;
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;

	/*
	 * We're living on an obio that looks like an sbus slot.
	 */
	sc->sc_iot = sa->sa_bustag;
	sc->sc_iobase = sa->sa_offset;
	if (!com_is_console(sc->sc_iot, sc->sc_iobase, &sc->sc_ioh) &&
	    sbus_bus_map(sc->sc_iot, sa->sa_slot,
			 sc->sc_iobase, sa->sa_size,
			 BUS_SPACE_MAP_LINEAR, 0,
			 &sc->sc_ioh) != 0) {
		printf(": can't map registers\n");
		return;
	}

	*AUXIO4M_REG |= (AUXIO4M_LED|AUXIO4M_LTE);
	do {
		DELAY(100);
	} while (!comprobe1(sc->sc_iot, sc->sc_ioh));
#if 0
	printf("modem: attach: lcr=0x%02x iir=0x%02x\n",
		bus_space_read_1(sc->sc_iot, sc->sc_ioh, 3),
		bus_space_read_1(sc->sc_iot, sc->sc_ioh, 2));
#endif

	sc->sc_frequency = COM_FREQ;
	com_attach_subr(sc);

	if (sa->sa_nintr != 0) {
		(void)bus_intr_establish(sc->sc_iot, sa->sa_pri, IPL_SERIAL,
					 0, comintr, sc);
		evcnt_attach_dynamic(&osc->osc_intrcnt, EVCNT_TYPE_INTR, NULL,
		    osc->osc_com.sc_dev.dv_xname, "intr");
	}

	/*
	 * Shutdown hook for buggy BIOSs that don't recognize the UART
	 * without a disabled FIFO.
	 */
	if (shutdownhook_establish(com_obio_cleanup, sc) == NULL) {
		panic("com_obio_attach: could not establish shutdown hook");
	}
}

static void
com_obio_cleanup(arg)
	void *arg;
{
	struct com_softc *sc = arg;

	if (ISSET(sc->sc_hwflags, COM_HW_FIFO))
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, com_fifo, 0);
}
