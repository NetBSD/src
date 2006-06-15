/*	$NetBSD: com_mace.c,v 1.5.16.1 2006/06/15 20:00:36 gdamore Exp $	*/

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
 *          NetBSD Project.  See http://www.NetBSD.org/ for
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_mace.c,v 1.5.16.1 2006/06/15 20:00:36 gdamore Exp $");

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
#include <machine/machtype.h>

#include <sgimips/mace/macevar.h>

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

CFATTACH_DECL(com_mace, sizeof(struct com_mace_softc),
    com_mace_match, com_mace_attach, NULL, NULL);

static int
com_mace_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 1;
}

static void
com_mace_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_mace_softc *msc = (void *)self;
	struct com_softc *sc = &msc->sc_com;
	struct mace_attach_args *maa = aux;
	bus_space_handle_t	ioh;

	/*
	 * XXX should check com_is_console() and 
	 * XXX use bus_space_map().
	 */
	ioh = maa->maa_sh + maa->maa_offset;
	/* note that ioh on mac is *also* the iobase address */
	COM_INIT_REGS(sc->sc_regs, maa->maa_st, ioh, ioh);

	sc->sc_frequency = COM_FREQ;

	delay(10000);
	com_attach_subr(sc);
	delay(10000);

	cpu_intr_establish(maa->maa_intr, maa->maa_intrmask, comintr, sc);

	return;
}
