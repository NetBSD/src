/*	$NetBSD: osiop_gsc.c,v 1.11.4.2 2009/05/16 10:41:13 yamt Exp $	*/

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
 * Copyright (c) 2001, 2002 Izumi Tsutsui.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: osiop_gsc.c,v 1.11.4.2 2009/05/16 10:41:13 yamt Exp $");

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

#include <dev/ic/osiopreg.h>
#include <dev/ic/osiopvar.h>

#include <hp700/dev/cpudevs.h>
#include <hp700/gsc/gscbusvar.h>
#include <hp700/hp700/machdep.h>

#define OSIOP_GSC_RESET         0x0000
#define	OSIOP_GSC_OFFSET	0x0100

int osiop_gsc_match(device_t, cfdata_t, void *);
void osiop_gsc_attach(device_t, device_t, void *);
int osiop_gsc_intr(void *);

CFATTACH_DECL_NEW(osiop_gsc, sizeof(struct osiop_softc),
    osiop_gsc_match, osiop_gsc_attach, NULL, NULL);

int
osiop_gsc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct gsc_attach_args *ga = aux;
	bus_space_handle_t ioh;
	int rv = 1;

	if (ga->ga_type.iodc_type != HPPA_TYPE_FIO ||
	    (ga->ga_type.iodc_sv_model != HPPA_FIO_GSCSI))
		return 0;

	if (bus_space_map(ga->ga_iot, ga->ga_hpa,
	    OSIOP_GSC_OFFSET + OSIOP_NREGS, 0, &ioh))
		return 0;


	bus_space_unmap(ga->ga_iot, ioh, OSIOP_GSC_OFFSET + OSIOP_NREGS);
	return rv;
}

void
osiop_gsc_attach(device_t parent, device_t self, void *aux)
{
	struct osiop_softc *sc = device_private(self);
	struct gsc_attach_args *ga = aux;
	bus_space_handle_t ioh;

	sc->sc_dev = self;
	sc->sc_bst = ga->ga_iot;
	sc->sc_dmat = ga->ga_dmatag;
	if (bus_space_map(sc->sc_bst, ga->ga_hpa,
	    OSIOP_GSC_OFFSET + OSIOP_NREGS, 0, &ioh))
		panic("%s: couldn't map I/O ports", __func__);
	if (bus_space_subregion(sc->sc_bst, ioh,
	    OSIOP_GSC_OFFSET, OSIOP_NREGS, &sc->sc_reg))
		panic("%s: couldn't get chip ports", __func__);

	sc->sc_clock_freq = ga->ga_ca.ca_pdc_iodc_read->filler2[14] / 1000000;
	if (!sc->sc_clock_freq)
		sc->sc_clock_freq = 50;

	sc->sc_ctest7 = 0; /* | OSIOP_CTEST7_TT1 */
	sc->sc_dcntl = OSIOP_DCNTL_EA;

	sc->sc_flags = 0;
	sc->sc_id = ga->ga_scsi_target;

	/*
	 * Reset the SCSI subsystem.
	 */
	bus_space_write_1(sc->sc_bst, ioh, OSIOP_GSC_RESET, 0);
	DELAY(1000);

	/*
	 * Call common attachment
	 */
#ifdef OSIOP_DEBUG
	{
		extern int osiop_debug;
		osiop_debug = -1;
	}
#endif /* OSIOP_DEBUG */
	osiop_attach(sc);

	(void)hp700_intr_establish(self, IPL_BIO,
	    osiop_gsc_intr, sc, ga->ga_int_reg, ga->ga_irq);
}

/*
 * interrupt handler
 */
int
osiop_gsc_intr(void *arg)
{
	struct osiop_softc *sc = arg;
	u_int8_t istat;

	/* This is potentially nasty, since the IRQ is level triggered... */
	if (sc->sc_flags & OSIOP_INTSOFF)
		return (0);

	istat = osiop_read_1(sc, OSIOP_ISTAT);

	if ((istat & (OSIOP_ISTAT_SIP | OSIOP_ISTAT_DIP)) == 0)
		return (0);

	/* Save interrupt details for the back-end interrupt handler */
	sc->sc_sstat0 = osiop_read_1(sc, OSIOP_SSTAT0);
	sc->sc_istat = istat;
	/*
	 * Per page 4-18 of the LSI 53C710 Technical Manual,
	 * "insert a delay equivalent to 12 BCLK periods between
	 * the reads [of DSTAT and SSTAT0] to ensure that the
	 * interrupts clear properly."
	 */
	DELAY(100);
	sc->sc_dstat = osiop_read_1(sc, OSIOP_DSTAT);

	/* Deal with the interrupt */
	osiop_intr(sc);

	/* Blink the LED. */
	hp700_led_blink(HP700_LED_DISK);

	return (1);
}
