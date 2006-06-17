/* $NetBSD: com_acpi.c,v 1.18.10.3 2006/06/17 05:17:33 gdamore Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: com_acpi.c,v 1.18.10.3 2006/06/17 05:17:33 gdamore Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/termios.h>

#include <machine/bus.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/ic/comvar.h>

static int	com_acpi_match(struct device *, struct cfdata *, void *);
static void	com_acpi_attach(struct device *, struct device *, void *);

struct com_acpi_softc {
	struct com_softc sc_com;
	void *sc_ih;
};

CFATTACH_DECL(com_acpi, sizeof(struct com_acpi_softc), com_acpi_match,
    com_acpi_attach, NULL, NULL);

/*
 * Supported device IDs
 */

static const char * const com_acpi_ids[] = {
	"PNP0500",	/* Standard PC COM port */
	"PNP0501",	/* 16550A-compatible COM port */
	"PNP0510",	/* Generic IRDA-compatible device */
	"PNP0511",	/* Generic IRDA-compatible device */
	"IBM0071",	/* IBM ThinkPad IRDA device */
	"SMCF010",	/* SMC SuperIO IRDA device */
	"NSC6001",	/* NSC IRDA device */
	NULL
};

/*
 * com_acpi_match: autoconf(9) match routine
 */
static int
com_acpi_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo,com_acpi_ids);
}

/*
 * com_acpi_attach: autoconf(9) attach routine
 */
static void
com_acpi_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_acpi_softc *asc = (struct com_acpi_softc *)self;
	struct com_softc *sc = &asc->sc_com;
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	struct acpi_io *io;
	struct acpi_irq *irq;
	bus_space_handle_t ioh;
	ACPI_STATUS rv;

	aprint_naive("\n");
	aprint_normal("\n");

	/* parse resources */
	rv = acpi_resource_parse(&sc->sc_dev, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	/* find our i/o registers */
	io = acpi_res_io(&res, 0);
	if (io == NULL) {
		aprint_error("%s: unable to find i/o register resource\n",
		    sc->sc_dev.dv_xname);
		goto out;
	}

	/* find our IRQ */
	irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		aprint_error("%s: unable to find irq resource\n",
		    sc->sc_dev.dv_xname);
		goto out;
	}

	if (!com_is_console(aa->aa_iot, io->ar_base, &ioh)) {
		if (bus_space_map(sc->sc_iot, io->ar_base, io->ar_length,
		    0, &ioh)) {
			aprint_error("%s: can't map i/o space\n",
			    sc->sc_dev.dv_xname);
			goto out;
		}
	}
	COM_INIT_REGS(sc->sc_regs, aa->aa_iot, ioh, io->ar_base);

	aprint_normal("%s", sc->sc_dev.dv_xname);

	if (com_probe_subr(&sc->sc_regs) == 0) {
		aprint_error(": com probe failed\n");
		goto out;
	}

	sc->sc_frequency = 115200 * 16;

	com_attach_subr(sc);

	asc->sc_ih = isa_intr_establish(aa->aa_ic, irq->ar_irq,
	    (irq->ar_type == ACPI_EDGE_SENSITIVE) ? IST_EDGE : IST_LEVEL,
	    IPL_SERIAL, comintr, sc);
 out:
	acpi_resource_cleanup(&res);
}
