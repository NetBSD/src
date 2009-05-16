/*	$NetBSD: oosiop_gsc.c,v 1.5.4.2 2009/05/16 10:41:13 yamt Exp $	*/

/*
 * Copyright (c) 2001 Matt Fredette.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 2001,2002 Izumi Tsutsui.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*	$OpenBSD: siop_gsc.c,v 1.1 1998/11/04 17:01:35 mickey Exp $	*/

/*
 * Copyright (c) 1998 Michael Shalayeff
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
 *	This product includes software developed by Michael Shalayeff.
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
__KERNEL_RCSID(0, "$NetBSD: oosiop_gsc.c,v 1.5.4.2 2009/05/16 10:41:13 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/malloc.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>
#include <machine/bus.h>

#include <dev/ic/oosiopreg.h>
#include <dev/ic/oosiopvar.h>

#include <hp700/dev/cpudevs.h>
#include <hp700/gsc/gscbusvar.h>
#include <hp700/hp700/machdep.h>

#define OOSIOP_GSC_RESET	0x0000
#define OOSIOP_GSC_OFFSET	0x0100

int oosiop_gsc_match(device_t, cfdata_t, void *);
void oosiop_gsc_attach(device_t, device_t, void *);
int oosiop_gsc_intr(void *);

CFATTACH_DECL_NEW(oosiop_gsc, sizeof(struct oosiop_softc),
    oosiop_gsc_match, oosiop_gsc_attach, NULL, NULL);

int
oosiop_gsc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct gsc_attach_args *ga = aux;
	bus_space_handle_t ioh;
	int rv = 1;

	if (ga->ga_type.iodc_type != HPPA_TYPE_FIO ||
	    ga->ga_type.iodc_sv_model != HPPA_FIO_SCSI)
		return 0;

	if (bus_space_map(ga->ga_iot, ga->ga_hpa,
	    OOSIOP_GSC_OFFSET + OOSIOP_NREGS, 0, &ioh))
		return 0;


	bus_space_unmap(ga->ga_iot, ioh, OOSIOP_GSC_OFFSET + OOSIOP_NREGS);
	return rv;
}

void
oosiop_gsc_attach(device_t parent, device_t self, void *aux)
{
	struct oosiop_softc *sc = device_private(self);
	struct gsc_attach_args *ga = aux;
	bus_space_handle_t ioh;

	sc->sc_dev = self;
	sc->sc_bst = ga->ga_iot;
	sc->sc_dmat = ga->ga_dmatag;
	if (bus_space_map(sc->sc_bst, ga->ga_hpa,
	    OOSIOP_GSC_OFFSET + OOSIOP_NREGS, 0, &ioh))
		panic("%s: couldn't map I/O ports", __func__);
	if (bus_space_subregion(sc->sc_bst, ioh,
	    OOSIOP_GSC_OFFSET, OOSIOP_NREGS, &sc->sc_bsh))
		panic("%s: couldn't get chip ports", __func__);

	sc->sc_freq = ga->ga_ca.ca_pdc_iodc_read->filler2[14];
	if (sc->sc_freq == 0)
		sc->sc_freq = 50000000;

	sc->sc_chip = OOSIOP_700;
	sc->sc_id = ga->ga_scsi_target;

	/*
	 * Reset SCSI subsystem.
	 */
	bus_space_write_1(sc->sc_bst, ioh, OOSIOP_GSC_RESET, 0);
	DELAY(1000);

	/*
	 * Call common attachment
	 */
	oosiop_attach(sc);

	(void)hp700_intr_establish(self, IPL_BIO,
	    oosiop_gsc_intr, sc, ga->ga_int_reg, ga->ga_irq);
}

/*
 * interrupt handler
 */
int
oosiop_gsc_intr(void *arg)
{
	struct oosiop_softc *sc = arg;
	int rv;

	rv = oosiop_intr(sc);

	/* Blink the LED. */
	hp700_led_blink(HP700_LED_DISK);

	return rv;
}
