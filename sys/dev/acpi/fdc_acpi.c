/* $NetBSD: fdc_acpi.c,v 1.13 2003/09/25 21:55:49 christos Exp $ */

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

/*
 * ACPI attachment for the PC Floppy Controller driver, based on
 * sys/arch/i386/pnpbios/fdc_pnpbios.c by Jason R. Thorpe
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdc_acpi.c,v 1.13 2003/09/25 21:55:49 christos Exp $");

#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/queue.h>
#include <sys/disk.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/isa/fdcvar.h>
#include <dev/isa/fdvar.h>
#include <dev/isa/fdreg.h>

#include <dev/acpi/fdc_acpireg.h>

int	fdc_acpi_match(struct device *, struct cfdata *, void *);
void	fdc_acpi_attach(struct device *, struct device *, void *);

struct fdc_acpi_softc {
	struct fdc_softc sc_fdc;
	bus_space_handle_t sc_baseioh;
	struct acpi_devnode *sc_node;	/* ACPI devnode */
	struct acpi_resources res;
};

static int	fdc_acpi_enumerate(struct fdc_acpi_softc *);
static void	fdc_acpi_getknownfds(struct fdc_acpi_softc *);

static const struct fd_type *fdc_acpi_nvtotype(char *, int, int);

CFATTACH_DECL(fdc_acpi, sizeof(struct fdc_acpi_softc), fdc_acpi_match,
    fdc_acpi_attach, NULL, NULL);

/*
 * Supported device IDs
 */

static const char * const fdc_acpi_ids[] = {
	"PNP0700",	/* PC standard floppy disk controller */
#if 0 /* XXX do we support this? */
	"PNP0701",	/* Standard floppy controller for MS Device Bay Spec */
#endif
	NULL
};

/*
 * fdc_acpi_match: autoconf(9) match routine
 */
int
fdc_acpi_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;
	const char *id;
	int i;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	for (i = 0; (id = fdc_acpi_ids[i]) != NULL; ++i) {
		if (strcmp(aa->aa_node->ad_devinfo.HardwareId, id) == 0)
			return 1;
	}

	/* No matches found */
	return 0;
}

/*
 * fdc_acpi_attach: autoconf(9) attach routine
 */
void
fdc_acpi_attach(struct device *parent, struct device *self, void *aux)
{
	struct fdc_acpi_softc *asc = (struct fdc_acpi_softc *)self;
	struct fdc_softc *sc = &asc->sc_fdc;
	struct acpi_attach_args *aa = aux;
	struct acpi_io *io, *ctlio;
	struct acpi_irq *irq;
	struct acpi_drq *drq;
	ACPI_STATUS rv;

	printf("\n");

	sc->sc_ic = aa->aa_ic;
	asc->sc_node = aa->aa_node;

	/* parse resources */
	rv = acpi_resource_parse(&sc->sc_dev, aa->aa_node, &asc->res,
	    &acpi_resource_parse_ops_default);
	if (rv != AE_OK) {
		printf("%s: unable to parse resources\n", sc->sc_dev.dv_xname);
		return;
	}

	/* find our i/o registers */
	io = acpi_res_io(&asc->res, 0);
	if (io == NULL) {
		printf("%s: unable to find i/o register resource\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* find our IRQ */
	irq = acpi_res_irq(&asc->res, 0);
	if (irq == NULL) {
		printf("%s: unable to find irq resource\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* find our DRQ */
	drq = acpi_res_drq(&asc->res, 0);
	if (drq == NULL) {
		printf("%s: unable to find drq resource\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_drq = drq->ar_drq;

	sc->sc_iot = aa->aa_iot;
	if (bus_space_map(sc->sc_iot, io->ar_base, io->ar_length,
		    0, &asc->sc_baseioh)) {
		printf("%s: can't map i/o space\n", sc->sc_dev.dv_xname);
		return;
	}

	switch (io->ar_length) {
	case 4:
		sc->sc_ioh = asc->sc_baseioh;
		break;
	case 6:
		if (bus_space_subregion(sc->sc_iot, asc->sc_baseioh, 2, 4,
		    &sc->sc_ioh)) {
			printf("%s: unable to subregion i/o space\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		break;
	default:
		printf("%s: unknown size: %d of io mapping\n",
		    sc->sc_dev.dv_xname, io->ar_length);
		return;
	}

	/*
	 * omitting the controller I/O port. (One has to exist for there to
	 * be a working fdc). Just try and force the mapping in.
	 */
	ctlio = acpi_res_io(&asc->res, 1);
	if (ctlio == NULL) {
		if (bus_space_map(sc->sc_iot, io->ar_base + io->ar_length + 1,
		    1, 0, &sc->sc_fdctlioh)) {
			printf("%s: unable to force map ctl i/o space\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		printf("%s: ctl io %x did't probe. Forced attach\n",
		    sc->sc_dev.dv_xname, io->ar_base + io->ar_length + 1);
	} else {
		if (bus_space_map(sc->sc_iot, ctlio->ar_base, ctlio->ar_length,
		    0, &sc->sc_fdctlioh)) {
			printf("%s: unable to map ctl i/o space\n",
			    sc->sc_dev.dv_xname);
			return;
		}
	}

	sc->sc_ih = isa_intr_establish(aa->aa_ic, irq->ar_irq,
	    (irq->ar_type == ACPI_EDGE_SENSITIVE) ? IST_EDGE : IST_LEVEL,
	    IPL_BIO, fdcintr, sc);

	/* Setup direct configuration of floppy drives */
	sc->sc_present = fdc_acpi_enumerate(asc);
	if (sc->sc_present >= 0) {
		sc->sc_known = 1;
		fdc_acpi_getknownfds(asc);
	} else {
		/*
		 * XXX if there is no _FDE control method, attempt to
		 * probe without pnp
		 */
#ifdef ACPI_FDC_DEBUG
		printf("%s: unable to enumerate, attempting normal probe\n",
		    sc->sc_dev.dv_xname);
#endif
	}

	fdcattach(sc);
}

static int
fdc_acpi_enumerate(struct fdc_acpi_softc *asc)
{
	struct fdc_softc *sc = &asc->sc_fdc;
	ACPI_OBJECT *fde;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;
	UINT32 *p;
	int i, drives = -1;

	rv = acpi_eval_struct(asc->sc_node->ad_handle, "_FDE", &buf);
	if (rv != AE_OK) {
#ifdef ACPI_FDC_DEBUG
		printf("%s: failed to evaluate _FDE: %x\n",
		    sc->sc_dev.dv_xname, rv);
#endif
		return drives;
	}
	fde = (ACPI_OBJECT *)buf.Pointer;
	if (fde->Type != ACPI_TYPE_BUFFER) {
		printf("%s: expected BUFFER, got %d\n", sc->sc_dev.dv_xname,
		    fde->Type);
		goto out;
	}
	if (fde->Buffer.Length < 5 * sizeof(UINT32)) {
		printf("%s: expected buffer len of %lu, got %d\n",
		    sc->sc_dev.dv_xname,
		    (unsigned long)(5 * sizeof(UINT32)), fde->Buffer.Length);
		goto out;
	}

	p = (UINT32 *) fde->Buffer.Pointer;

	/*
	 * Indexes 0 through 3 are each UINT32 booleans. True if a drive
	 * is present.
	 */
	drives = 0;
	for (i = 0; i < 4; i++) {
		if (p[i]) drives |= (1 << i);
#ifdef ACPI_FDC_DEBUG
		printf("%s: drive %d %sattached\n", sc->sc_dev.dv_xname, i,
		    p[i] ? "" : "not ");
#endif
	}

	/*
	 * p[4] reports tape presence. Possible values:
	 * 	0	- Unknown if device is present
	 *	1	- Device is present
	 *	2	- Device is never present
	 *	>2	- Reserved
	 *
	 * we don't currently use this.
	 */

out:
	AcpiOsFree(buf.Pointer);
	return drives;
}

static void
fdc_acpi_getknownfds(struct fdc_acpi_softc *asc)
{
	struct fdc_softc *sc = &asc->sc_fdc;
	ACPI_OBJECT *fdi, *e;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;
	int i;

	for (i = 0; i < 4; i++) {
		if ((sc->sc_present & (1 << i)) == 0)
			continue;
		rv = acpi_eval_struct(asc->sc_node->ad_handle, "_FDI", &buf);
		if (rv != AE_OK) {
#ifdef ACPI_FDC_DEBUG
			printf("%s: failed to evaluate _FDI: %x on drive %d\n",
			    sc->sc_dev.dv_xname, rv, i);
#endif
			/* XXX if _FDI fails, assume 1.44MB floppy */
			sc->sc_knownfds[i] = &fdc_acpi_fdtypes[0];
			continue;
		}
		fdi = (ACPI_OBJECT *)buf.Pointer;
		if (fdi->Type != ACPI_TYPE_PACKAGE) {
			printf("%s: expected PACKAGE, got %d\n",
			    sc->sc_dev.dv_xname, fdi->Type);
			goto out;
		}
		e = fdi->Package.Elements;
		sc->sc_knownfds[i] = fdc_acpi_nvtotype(sc->sc_dev.dv_xname,
		    e[1].Integer.Value, e[0].Integer.Value); 

		/* if fdc_acpi_nvtotype returns NULL, don't attach drive */
		if (!sc->sc_knownfds[i])
			sc->sc_present &= ~(1 << i);

out:
		AcpiOsFree(buf.Pointer);
	}
}

static const struct fd_type *
fdc_acpi_nvtotype(char *fdc, int nvraminfo, int drive)
{
	int type;

	type = (drive == 0 ? nvraminfo : nvraminfo << 4) & 0xf0;
	switch (type) {
	case ACPI_FDC_DISKETTE_NONE:
		return NULL;
	case ACPI_FDC_DISKETTE_12M:
		return &fdc_acpi_fdtypes[1];
	case ACPI_FDC_DISKETTE_TYPE5:
	case ACPI_FDC_DISKETTE_TYPE6:
	case ACPI_FDC_DISKETTE_144M:
		return &fdc_acpi_fdtypes[0];
	case ACPI_FDC_DISKETTE_360K:
		return &fdc_acpi_fdtypes[3];
	case ACPI_FDC_DISKETTE_720K:
		return &fdc_acpi_fdtypes[4];
	default:
#ifdef ACPI_FDC_DEBUG
		printf("%s: drive %d: unknown device type 0x%x\n",
		    fdc, drive, type);
#endif
		return NULL;
	}
}
