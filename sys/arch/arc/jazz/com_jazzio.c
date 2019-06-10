/*	$NetBSD: com_jazzio.c,v 1.12.54.1 2019/06/10 22:05:50 christos Exp $	*/
/*	$OpenBSD: com_lbus.c,v 1.7 1998/03/16 09:38:41 pefo Exp $	*/
/*	NetBSD: com_isa.c,v 1.12 1998/08/15 17:47:17 mycroft Exp 	*/

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
__KERNEL_RCSID(0, "$NetBSD: com_jazzio.c,v 1.12.54.1 2019/06/10 22:05:50 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <machine/autoconf.h>
#include <sys/bus.h>
#include <machine/intr.h>

#include <arc/jazz/jazziovar.h>

#include <dev/isa/isavar.h>	/* XXX for isa_chipset_tag_t in com_softc */

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#include <dev/ic/ns16550reg.h>

extern int com_freq;

int	com_jazzio_probe(device_t, cfdata_t , void *);
void	com_jazzio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_jazzio, sizeof(struct com_softc),
    com_jazzio_probe, com_jazzio_attach, NULL, NULL);

int
com_jazzio_probe(device_t parent, cfdata_t match, void *aux)
{
	struct jazzio_attach_args *ja = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int iobase;
	int rv = 1;

	if (strcmp(ja->ja_name, "COM1") != 0 &&
	    strcmp(ja->ja_name, "COM2") != 0)
		return 0;

	iot = ja->ja_bust;
	iobase = ja->ja_addr;

	/* if it's in use as console, it's there. */
	if (!com_is_console(iot, iobase, 0)) {
		if (bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh)) {
			return 0;
		}
		rv = comprobe1(iot, ioh);
		bus_space_unmap(iot, ioh, COM_NPORTS);
	}
	return rv;
}

void
com_jazzio_attach(device_t parent, device_t self, void *aux)
{
	struct jazzio_attach_args *ja = aux;
	struct com_softc *sc = device_private(self);
	int iobase;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	sc->sc_dev = self;
	iobase = ja->ja_addr;
	iot = ja->ja_bust;

	if (!com_is_console(iot, iobase, &ioh) &&
	    bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh)) {
		aprint_error(": can't map i/o space\n");
		return;
	}
	com_init_regs(&sc->sc_regs, iot, ioh, iobase);
	sc->sc_frequency = com_freq;
	SET(sc->sc_hwflags, COM_HW_TXFIFO_DISABLE); /* XXX - NEC M403 */
	jazzio_intr_establish(ja->ja_intr, comintr, sc);

	com_attach_subr(sc);
}
