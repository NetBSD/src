/*	$NetBSD: com_obio.c,v 1.9 2009/11/21 17:40:28 rmind Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: com_obio.c,v 1.9 2009/11/21 17:40:28 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/marvell/gtvar.h>

struct com_obio_softc {
	struct com_softc osc_com;	/* real "com" softc */

	/* OBIO-specific goo. */
};

static int com_obio_match (device_t, cfdata_t , void *);
static void com_obio_attach (device_t, device_t, void *);

CFATTACH_DECL_NEW(com_obio, sizeof(struct com_obio_softc),
    com_obio_match, com_obio_attach, NULL, NULL);

int
com_obio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *oa = aux;
	bus_space_handle_t ioh;
	int rv = 0;

	if (oa->oa_offset == OBIO_UNK_OFFSET ||
	    oa->oa_size == OBIO_UNK_SIZE)
		return (0);

	if (com_is_console(oa->oa_memt, oa->oa_offset, NULL)) {
		rv = 1;
	} else {
		if (bus_space_map(oa->oa_memt, oa->oa_offset,
		    oa->oa_size, 0, &ioh))
			return (0);
		rv = comprobe1(oa->oa_memt, ioh);
		bus_space_unmap(oa->oa_memt, ioh, oa->oa_size);
	}

	return (rv);
}

void
com_obio_attach(device_t parent, device_t self, void *aux)
{
	struct com_obio_softc *osc = device_private(self);
	struct com_softc *sc = &osc->osc_com;
	struct obio_attach_args *oa = aux;
	bus_space_handle_t ioh;

	sc->sc_dev = self;
	sc->sc_frequency = COM_FREQ*2;

	if (!com_is_console(oa->oa_memt, oa->oa_offset, &ioh) &&
	    bus_space_map(oa->oa_memt, oa->oa_offset, oa->oa_size,
		0, &ioh) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	COM_INIT_REGS(sc->sc_regs, oa->oa_memt, ioh, oa->oa_offset);
	sc->sc_regs.cr_nports = oa->oa_size;
	com_attach_subr(sc);

	if (oa->oa_irq >= 0) {
		intr_establish(oa->oa_irq, IST_EDGE, IPL_SERIAL, comintr, sc);
		aprint_normal_dev(self, "interrupting at %s\n",
		    intr_string(oa->oa_irq));
	}
}
