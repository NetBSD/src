/*	$NetBSD: com_mainbus.c,v 1.9 2008/04/28 20:23:29 martin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: com_mainbus.c,v 1.9 2008/04/28 20:23:29 martin Exp $");

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

int com_mainbus_match(device_t, cfdata_t , void *);
void com_mainbus_attach(device_t, device_t, void *);
void comcnprobe(struct consdev *);
void comcninit(struct consdev *);

CFATTACH_DECL_NEW(com_mainbus, sizeof(struct com_mainbus_softc),
    com_mainbus_match, com_mainbus_attach, NULL, NULL);

int
com_mainbus_match(device_t parent, cfdata_t match, void *aux)
{
	extern struct cfdriver com_cd;
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, com_cd.cd_name) == 0)
		return (1);

	return (0);
}

void
com_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	struct com_mainbus_softc *sc = device_private(self);
	struct com_softc *csc = &sc->sc_com;

	csc->sc_dev = self;
	csc->sc_frequency = COM_FREQ;
	COM_INIT_REGS(csc->sc_regs, 0, ma->ma_addr1, 0);

	/* sanity check */
	if (!comprobe1(0, ma->ma_addr1)) {
		aprint_error(": device problem. don't attach.\n");
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
comcninit(struct consdev *cp)
{

	comcnattach(0, CONADDR, COMCN_SPEED, COM_FREQ, COM_TYPE_NORMAL,
	    CONMODE);
}
