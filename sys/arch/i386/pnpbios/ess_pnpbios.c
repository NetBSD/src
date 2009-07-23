/*	$NetBSD: ess_pnpbios.c,v 1.17.14.2 2009/07/23 23:31:36 jym Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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
__KERNEL_RCSID(0, "$NetBSD: ess_pnpbios.c,v 1.17.14.2 2009/07/23 23:31:36 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/bus.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/midi_if.h>
#include <dev/mulaw.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <i386/pnpbios/pnpbiosvar.h>

#include <dev/isa/essreg.h>
#include <dev/isa/essvar.h>

int ess_pnpbios_match(device_t, cfdata_t, void *);
void ess_pnpbios_attach(device_t, device_t, void *);

CFATTACH_DECL(ess_pnpbios, sizeof(struct ess_softc),
    ess_pnpbios_match, ess_pnpbios_attach, NULL, NULL);

int
ess_pnpbios_match(device_t parent, cfdata_t match,
    void *aux)
{
	struct pnpbiosdev_attach_args *aa = aux;

	if (strcmp(aa->idstr, "ESS0104") && /* 1788 */
	    strcmp(aa->idstr, "ESS0114") && /* 1788 */
	    strcmp(aa->idstr, "CPQAE27") && /* 1788 */
	    strcmp(aa->idstr, "ESS1869") && /* 1869 */
	    strcmp(aa->idstr, "CPQB0AB") && /* 1869 */
	    strcmp(aa->idstr, "CPQB0AC") && /* 1869 */
	    strcmp(aa->idstr, "CPQB0AD") && /* 1869 */
	    strcmp(aa->idstr, "CPQB0F1") && /* 1869 */
	    strcmp(aa->idstr, "ESS1878") && /* 1878 */
	    strcmp(aa->idstr, "ESS1879"))   /* 1879 */
		return (0);

	return (1);
}

void
ess_pnpbios_attach(device_t parent, device_t self,
    void *aux)
{
	struct ess_softc *sc = device_private(self);
	struct pnpbiosdev_attach_args *aa = aux;

	if (pnpbios_io_map(aa->pbt, aa->resc, 0, &sc->sc_iot, &sc->sc_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	sc->sc_ic = aa->ic;

	/* XXX These are only for setting chip configuration registers. */
	pnpbios_getiobase(aa->pbt, aa->resc, 0, 0, &sc->sc_iobase);

	sc->sc_audio1.ist = IST_EDGE;
	sc->sc_audio2.ist = IST_EDGE;

	if (pnpbios_getirqnum(aa->pbt, aa->resc, 0, &sc->sc_audio1.irq,
	    NULL)) {
		printf(": can't get IRQ\n");
		return;
	}

	if (pnpbios_getirqnum(aa->pbt, aa->resc, 1, &sc->sc_audio2.irq,
	    NULL))
		sc->sc_audio2.irq = -1;

	if (pnpbios_getdmachan(aa->pbt, aa->resc, 0, &sc->sc_audio1.drq)) {
		printf(": can't get DMA channel\n");
		return;
	}

	if (pnpbios_getdmachan(aa->pbt, aa->resc, 1, &sc->sc_audio2.drq))
		sc->sc_audio2.drq = -1;

	printf("\n");
	pnpbios_print_devres(self, aa);

	printf("%s", device_xname(self));

	if (!essmatch(sc)) {
		aprint_error_dev(&sc->sc_dev, "essmatch failed\n");
		pnpbios_io_unmap(aa->pbt, aa->resc, 0, sc->sc_iot, sc->sc_ioh);
		return;
	}

	essattach(sc, 0);
}
