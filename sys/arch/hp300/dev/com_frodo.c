/*	$NetBSD: com_frodo.c,v 1.5.34.1 2008/01/09 01:45:59 matt Exp $	*/

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
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_frodo.c,v 1.5.34.1 2008/01/09 01:45:59 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>
#include <sys/ttydefaults.h>

#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <hp300/dev/intiovar.h>
#include <hp300/dev/frodoreg.h>
#include <hp300/dev/frodovar.h>
#include <hp300/dev/com_frodovar.h>

#include "ioconf.h"

struct com_frodo_softc {
	struct	com_softc sc_com;	/* real "com" softc */
};

static int	com_frodo_match(struct device *, struct cfdata *, void *);
static void	com_frodo_attach(struct device *, struct device *, void *);

CFATTACH_DECL(com_frodo, sizeof(struct com_frodo_softc),
    com_frodo_match, com_frodo_attach, NULL, NULL);

static int com_frodo_speed = TTYDEF_SPEED;
static struct bus_space_tag comcntag;

#define CONMODE	((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#define COM_FRODO_FREQ	8006400

static int
com_frodo_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct frodo_attach_args *fa = aux;

	if (strcmp(fa->fa_name, com_cd.cd_name) != 0)
		return 0;

	switch (fa->fa_offset) {
	case FRODO_APCI_OFFSET(1):
	case FRODO_APCI_OFFSET(2):
	case FRODO_APCI_OFFSET(3):
		return 1;
	}

	return 0;
}

static void
com_frodo_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_frodo_softc *fsc = (void *)self;
	struct com_softc *sc = &fsc->sc_com;
	struct frodo_attach_args *fa = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int isconsole;

	isconsole = com_is_console(&comcntag, fa->fa_base + fa->fa_offset,
	    &ioh);

	if (isconsole)
		iot = &comcntag;
	else
		iot = fa->fa_bst;

	if (!isconsole &&
	    bus_space_map(iot, fa->fa_base + fa->fa_offset, COM_NPORTS << 2,
	     0, &ioh)) {
		printf(": can't map i/o space\n");
		return;
	}
	COM_INIT_REGS(sc->sc_regs, iot, ioh, fa->fa_base + fa->fa_offset);

	sc->sc_frequency = COM_FRODO_FREQ;
	SET(sc->sc_hwflags, COM_HW_NOIEN);

	com_attach_subr(sc);

	frodo_intr_establish(parent, comintr, sc, fa->fa_line, IPL_VM);
}

int
com_frodo_cnattach(bus_space_tag_t bst, bus_addr_t addr, int scode)
{
	bus_space_tag_t iot = &comcntag;
	bus_space_handle_t ioh;

	if (machineid != HP_425 || mmuid != MMUID_425_E)
		return 1;

	frodo_init_bus_space(iot);

	if (bus_space_map(iot, addr, INTIO_DEVSIZE, BUS_SPACE_MAP_LINEAR, &ioh))
		return 1;

	comcnattach(iot, addr, com_frodo_speed, COM_FRODO_FREQ,
	    COM_TYPE_NORMAL, CONMODE);

	return 0;
}
