/*	$NetBSD: com_mace.c,v 1.2.4.2 2002/04/01 07:42:17 nathanw Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/bus.h>

#include <sgimips/dev/macevar.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

struct com_mace_softc {
	struct com_softc sc_com;

	/* XXX intr cookie */
};

static int	com_mace_match(struct device *, struct cfdata *, void *);
static void	com_mace_attach(struct device *, struct device *, void *);

struct cfattach com_mace_ca = {
	sizeof(struct com_mace_softc), com_mace_match, com_mace_attach
};

static int
com_mace_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return 1;
}

extern void *crime_intr_establish(int, int, int, int (*)(void *), void *);

int console = 0;

static void
com_mace_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct com_mace_softc *msc = (void *)self;
	struct com_softc *sc = &msc->sc_com;
	struct mace_attach_args *maa = aux;
	unsigned long rate;
	char *dbaud;

	sc->sc_iot = maa->maa_st;
	sc->sc_ioh = maa->maa_sh;
	sc->sc_iobase = maa->maa_sh;

	sc->sc_frequency = COM_FREQ;

	/* XXX console check */
	/* XXX map */

#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */

#if 0
console = 1;
#endif

#if 0
	console = GetEnvironmentVariable("console");

	if console[0] = 'd'
#endif
	dbaud = ARCBIOS->GetEnvironmentVariable("dbaud");
	if (dbaud != NULL)
		rate = strtoul(dbaud, NULL, 10);
	else
		rate = 9600;

	delay(10000);
	/* if console & dbaud */
	if (console == 0) {
		comcnattach(sc->sc_iot, sc->sc_ioh, rate, COM_FREQ, CONMODE);
		console = 1;
	}
	delay(10000);
	com_attach_subr(sc);
	delay(10000);

	crime_intr_establish(0, 0, 0, comintr, sc);

	return;
}
