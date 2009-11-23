/*	$NetBSD: com_supio.c,v 1.27 2009/11/23 00:11:43 rmind Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: com_supio.c,v 1.27 2009/11/23 00:11:43 rmind Exp $");

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

#include <machine/intr.h>
#include <machine/bus.h>

/*#include <dev/isa/isavar.h>*/
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <amiga/dev/supio.h>

struct comsupio_softc {
	struct com_softc sc_com;
	struct isr sc_isr;
};

int com_supio_match(device_t, cfdata_t , void *);
void com_supio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_supio, sizeof(struct comsupio_softc),
    com_supio_match, com_supio_attach, NULL, NULL);

int
com_supio_match(device_t parent, cfdata_t match, void *aux)
{
	int rv = 1;
	struct supio_attach_args *supa = aux;

	if (strcmp(supa->supio_name,"com"))
		return 0;

	return (rv);
}

void
com_supio_attach(device_t parent, device_t self, void *aux)
{
	struct comsupio_softc *sc = device_private(self);
	struct com_softc *csc = &sc->sc_com;
	int iobase;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct supio_attach_args *supa = aux;
	u_int16_t needpsl;

	csc->sc_dev = self;

	/*
	 * We're living on a superio chip.
	 */
	iobase = supa->supio_iobase;
	iot = supa->supio_iot;
	aprint_normal(" port 0x%04x ipl %d", iobase, supa->supio_ipl);

	if (bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh))
		panic("comattach: io mapping failed");
	COM_INIT_REGS(csc->sc_regs, iot, ioh, iobase);

	csc->sc_frequency = supa->supio_arg;

	com_attach_subr(csc);

	/* XXX this should be really in the interrupt stuff */
	needpsl = PSL_S | (supa->supio_ipl << 8);

	if (ipl2spl_table[IPL_SERIAL] < needpsl) {
		aprint_normal_dev(self, "raising ipl2spl_table[IPL_SERIAL] "
		    "from 0x%x to 0x%x\n", ipl2spl_table[IPL_SERIAL], needpsl);
		ipl2spl_table[IPL_SERIAL] = needpsl;
	}
	sc->sc_isr.isr_intr = comintr;
	sc->sc_isr.isr_arg = csc;
	sc->sc_isr.isr_ipl = supa->supio_ipl;
	add_isr(&sc->sc_isr);

	if (!pmf_device_register1(self, com_suspend, com_resume, com_cleanup)) {
		aprint_error_dev(self, "could not establish shutdown hook");
	}
}
