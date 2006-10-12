/*	$NetBSD: mpu_isa.c,v 1.15 2006/10/12 01:31:17 christos Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mpu_isa.c,v 1.15 2006/10/12 01:31:17 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/midiio.h>

#include <machine/bus.h>

#include <dev/midi_if.h>

#include <dev/isa/isavar.h>
#include <dev/ic/mpuvar.h>

struct mpu_isa_softc {
	struct mpu_softc sc_mpu;	/* generic part */
	void	*sc_ih;			/* ISA interrupt handler */
};

int	mpu_isa_match(struct device *, struct cfdata *, void *);
void	mpu_isa_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mpu_isa, sizeof(struct mpu_isa_softc),
    mpu_isa_match, mpu_isa_attach, NULL, NULL);

int
mpu_isa_match(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
{
	struct isa_attach_args *ia = aux;
	struct mpu_isa_softc sc;
	int r;

	if (ia->ia_nio < 1)
		return (0);
	if (ia->ia_nirq < 1)
		return (0);

	if (ISA_DIRECT_CONFIG(ia))
		return (0);

	if (ia->ia_io[0].ir_addr == ISA_UNKNOWN_PORT)
		return (0);
	if (ia->ia_irq[0].ir_irq == ISA_UNKNOWN_IRQ)
		return (0);

	memset(&sc, 0, sizeof sc);
	sc.sc_mpu.iot = ia->ia_iot;
	if (bus_space_map(sc.sc_mpu.iot, ia->ia_io[0].ir_addr, MPU401_NPORT, 0,
			  &sc.sc_mpu.ioh))
		return (0);
	r = mpu_find(&sc.sc_mpu);
        bus_space_unmap(sc.sc_mpu.iot, sc.sc_mpu.ioh, MPU401_NPORT);
	if (r) {
		ia->ia_nio = 1;
		ia->ia_io[0].ir_size = MPU401_NPORT;

		ia->ia_nirq = 1;

		ia->ia_niomem = 0;
		ia->ia_ndrq = 0;
	}
	return (r);
}

void
mpu_isa_attach(struct device *parent __unused, struct device *self, void *aux)
{
	struct mpu_isa_softc *sc = (struct mpu_isa_softc *)self;
	struct isa_attach_args *ia = aux;

	printf("\n");

	if (bus_space_map(sc->sc_mpu.iot, ia->ia_io[0].ir_addr, MPU401_NPORT,
	    0, &sc->sc_mpu.ioh)) {
		printf("mpu_isa_attach: bus_space_map failed\n");
		return;
	}

	sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq[0].ir_irq,
	    IST_EDGE, IPL_AUDIO, mpu_intr, sc);

	sc->sc_mpu.model = "Roland MPU-401 MIDI UART";
	mpu_attach(&sc->sc_mpu);
}
