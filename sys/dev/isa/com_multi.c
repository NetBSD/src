/*	$NetBSD: com_multi.c,v 1.2 1997/07/17 00:58:48 jtk Exp $	*/

/*-
 * Copyright (c) 1993, 1994, 1995, 1996
 *	Charles M. Hannum.  All rights reserved.
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

/*
 * COM driver, uses National Semiconductor NS16450/NS16550AF UART
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

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/isa/isavar.h>
#include <dev/isa/comreg.h>
#include <dev/isa/comvar.h>
#include <dev/isa/com_multi.h>

int com_multi_probe __P((struct device *, void *, void *));
void com_multi_attach __P((struct device *, struct device *, void *));

struct cfattach com_multi_ca = {
	sizeof(struct com_softc), com_multi_probe, com_multi_attach
};

int
com_multi_probe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	int iobase;
	struct cfdata *cf = match;
	struct commulti_attach_args *ca = aux;
 
	if (cf->cf_loc[COMMULTICF_SLAVE] != COMMULTICF_SLAVE_DEFAULT &&
	    cf->cf_loc[COMMULTICF_SLAVE] != ca->ca_slave)
		return (0);

	iobase = ca->ca_iobase;

	/* if it's in use as console, it's there. */
	if (iobase == comconsaddr && !comconsattached)
		return 1;

	return comprobe1(ca->ca_iot, ca->ca_ioh, iobase);
}

void
com_multi_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct com_softc *sc = (void *)self;
	struct commulti_attach_args *ca = aux;

	/*
	 * We're living on a commulti.
	 */
	sc->sc_iot = ca->ca_iot;
	sc->sc_ioh = ca->ca_ioh;
	sc->sc_iobase = ca->ca_iobase;

	if (ca->ca_noien)
		sc->sc_hwflags |= COM_HW_NOIEN;

	com_attach_subr(sc);
}
