/* $NetBSD: wss_acpi.c,v 1.4 2002/12/30 07:29:26 matt Exp $ */

/*
 * Copyright (c) 2002 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wss_acpi.c,v 1.4 2002/12/30 07:29:26 matt Exp $");

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

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/isa/ad1848var.h>
#include <dev/isa/wssreg.h>
#include <dev/isa/wssvar.h>

int	wss_acpi_match(struct device *, struct cfdata *, void *);
void	wss_acpi_attach(struct device *, struct device *, void *);

CFATTACH_DECL(wss_acpi, sizeof(struct wss_softc), wss_acpi_match,
    wss_acpi_attach, NULL, NULL);

/*
 * Supported device IDs
 */

static const char * const wss_acpi_ids[] = {
	"CSC0100",	/* NeoMagic 256AV with CS4232 codec */
	NULL
};

/*
 * wss_acpi_match: autoconf(9) match routine
 */
int
wss_acpi_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;
	const char *id;
	int i;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	for (i = 0; (id = wss_acpi_ids[i]) != NULL; ++i) {
		if (strcmp(aa->aa_node->ad_devinfo.HardwareId, id) == 0)
			return 1;
	}

	/* No matches found */
	return 0;
}

/*
 * wss_acpi_attach: autoconf(9) attach routine
 */
void
wss_acpi_attach(struct device *parent, struct device *self, void *aux)
{
	struct wss_softc *sc = (struct wss_softc *)self;
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	struct acpi_io *dspio, *oplio;
	struct acpi_irq *irq;
	struct acpi_drq *playdrq, *recdrq;
	struct audio_attach_args arg;
	ACPI_STATUS rv;

	printf(": NeoMagic 256AV audio\n");

	/* Parse our resources */
	rv = acpi_resource_parse(&sc->sc_ad1848.sc_ad1848.sc_dev,
	    aa->aa_node, &res, &acpi_resource_parse_ops_default);
	if (rv != AE_OK) {
		printf("%s: unable to parse resources\n",
		    sc->sc_ad1848.sc_ad1848.sc_dev.dv_xname);
		return;
	}

	/* Find and map our i/o registers */
	sc->sc_iot = aa->aa_iot;
	dspio = acpi_res_io(&res, 0);
	oplio = acpi_res_io(&res, 1);
	if (dspio == NULL || oplio == NULL) {
		printf("%s: unable to find i/o registers resource\n",
		    sc->sc_ad1848.sc_ad1848.sc_dev.dv_xname);
		return;
	}
	if (bus_space_map(sc->sc_iot, dspio->ar_base, dspio->ar_length,
	    0, &sc->sc_ioh) != 0) {
		printf("%s: unable to map i/o registers\n",
		    sc->sc_ad1848.sc_ad1848.sc_dev.dv_xname);
		return;
	}
	if (bus_space_map(sc->sc_iot, oplio->ar_base, oplio->ar_length,
	    0, &sc->sc_opl_ioh) != 0) {
		printf("%s: unable to map opl i/o registers\n",
		    sc->sc_ad1848.sc_ad1848.sc_dev.dv_xname);
		return;
	}

	sc->wss_ic = aa->aa_ic;

	/* Find our IRQ */
	irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		printf("%s: unable to find irq resource\n",
		    sc->sc_ad1848.sc_ad1848.sc_dev.dv_xname);
		/* XXX bus_space_unmap */
		return;
	}
	sc->wss_irq = irq->ar_irq;

	/* Find our playback and record DRQs */
	playdrq = acpi_res_drq(&res, 0);
	recdrq = acpi_res_drq(&res, 1);
	if (playdrq == NULL || recdrq == NULL) {
		printf("%s: unable to find drq resources\n",
		    sc->sc_ad1848.sc_ad1848.sc_dev.dv_xname);
		/* XXX bus_space_unmap */
		return;
	}
	sc->wss_playdrq = playdrq->ar_drq;
	sc->wss_recdrq = recdrq->ar_drq;

	sc->sc_ad1848.sc_ad1848.sc_iot = sc->sc_iot;
	bus_space_subregion(sc->sc_iot, sc->sc_ioh, 0, 4,
	    &sc->sc_ad1848.sc_ad1848.sc_ioh);

	/* Look for the ad1848 */
	if (!ad1848_isa_probe(&sc->sc_ad1848)) {
		printf("%s: ad1848 probe failed\n",
		    sc->sc_ad1848.sc_ad1848.sc_dev.dv_xname);
		/* XXX cleanup */
		return;
	}

	printf("%s", sc->sc_ad1848.sc_ad1848.sc_dev.dv_xname);
	/* Attach our wss device */
	wssattach(sc);

	arg.type = AUDIODEV_TYPE_OPL;
	arg.hwif = 0;
	arg.hdl = 0;
	config_found(self, &arg, audioprint);
}
