/*	$NetBSD: com_mainbus.c,v 1.1 2002/03/24 18:08:45 uch Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/termios.h>
#include <dev/cons.h>
#include <sys/conf.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/autoconf.h>
#include <machine/mmeye.h>

#include <dev/ic/comvar.h>
#include <dev/ic/comreg.h>

#ifndef COMCN_SPEED
#define COMCN_SPEED	19200
#endif
#ifndef CONADDR
#define CONADDR		0xa4000000
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

struct com_mainbus_softc {
	struct	com_softc sc_com;	/* real "com" softc */
};

int com_mainbus_match(struct device *, struct cfdata *, void *);
void com_mainbus_attach(struct device *, struct device *, void *);
void comcnprobe(struct consdev *);
void comcninit(struct consdev *);

struct cfattach com_mainbus_ca = {
	sizeof(struct com_mainbus_softc), com_mainbus_match, com_mainbus_attach
};

int
com_mainbus_match(struct device *parent, struct cfdata *match, void *aux)
{
	extern struct cfdriver com_cd;
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, com_cd.cd_name) == 0)
		return (1);

	return (0);
}

void
com_mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	struct com_mainbus_softc *sc = (void *)self;
	struct com_softc *csc = &sc->sc_com;

	csc->sc_iot = 0;
	csc->sc_ioh = ma->ma_addr1;
	csc->sc_iobase = 0;
	csc->sc_frequency = COM_FREQ;

	/* sanity check */
	if (!comprobe1(csc->sc_iot, csc->sc_ioh)) {
		printf(": device problem. don't attach.\n");
		return;
	}

	com_attach_subr(csc);

	mmeye_intr_establish(ma->ma_irq1, IST_LEVEL, IPL_SERIAL, comintr, sc);
}

void
comcnprobe(struct consdev *cp)
{

#ifdef  COMCONSOLE
	cp->cn_pri = CN_REMOTE;	/* Force a serial port console */
#else
	cp->cn_pri = CN_NORMAL;
#endif
}

void
comcninit(cp)
	struct consdev *cp;
{

	comcnattach(0, CONADDR, COMCN_SPEED, COM_FREQ, CONMODE);
}
