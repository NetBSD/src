/*	$NetBSD: uk.c,v 1.51.4.1 2007/03/12 05:57:11 rmind Exp $	*/

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

/*
 * Dummy driver for a device we can't identify.
 * Originally by Julian Elischer (julian@tfs.com)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uk.c,v 1.51.4.1 2007/03/12 05:57:11 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/vnode.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#define	UKUNIT(z)	(minor(z))

struct uk_softc {
	struct device sc_dev;

	struct scsipi_periph *sc_periph; /* all the inter level info */
};

static int	ukmatch(struct device *, struct cfdata *, void *);
static void	ukattach(struct device *, struct device *, void *);
static int	ukactivate(struct device *, enum devact);
static int	ukdetach(struct device *, int);


CFATTACH_DECL(uk, sizeof(struct uk_softc), ukmatch, ukattach, ukdetach,
    ukactivate);

extern struct cfdriver uk_cd;

static dev_type_open(ukopen);
static dev_type_close(ukclose);
static dev_type_ioctl(ukioctl);

const struct cdevsw uk_cdevsw = {
	ukopen, ukclose, noread, nowrite, ukioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_OTHER,
};

static int
ukmatch(struct device *parent, struct cfdata *match,
    void *aux)
{

	return (1);
}

/*
 * The routine called by the low level scsi routine when it discovers
 * a device suitable for this driver.
 */
static void
ukattach(struct device *parent, struct device *self, void *aux)
{
	struct uk_softc *uk = device_private(self);
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_periph *periph = sa->sa_periph;

	SC_DEBUG(periph, SCSIPI_DB2, ("ukattach: "));

	/*
	 * Store information needed to contact our base driver
	 */
	uk->sc_periph = periph;
	periph->periph_dev = &uk->sc_dev;

	printf("\n");
}

static int
ukactivate(struct device *self, enum devact act)
{
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		/*
		 * Nothing to do; we key off the device's DVF_ACTIVE.
		 */
		break;
	}
	return (rv);
}

static int
ukdetach(struct device *self, int flags)
{
	/*struct uk_softc *uk = device_private(self);*/
	int cmaj, mn;

	/* locate the major number */
	cmaj = cdevsw_lookup_major(&uk_cdevsw);

	/* Nuke the vnodes for any open instances */
	mn = device_unit(self);
	vdevgone(cmaj, mn, mn, VCHR);

	return (0);
}

/*
 * open the device.
 */
static int
ukopen(dev_t dev, int flag, int fmt, struct lwp *l)
{
	int unit, error;
	struct uk_softc *uk;
	struct scsipi_periph *periph;
	struct scsipi_adapter *adapt;

	unit = UKUNIT(dev);
	if (unit >= uk_cd.cd_ndevs)
		return (ENXIO);
	uk = uk_cd.cd_devs[unit];
	if (uk == NULL)
		return (ENXIO);

	periph = uk->sc_periph;
	adapt = periph->periph_channel->chan_adapter;

	SC_DEBUG(periph, SCSIPI_DB1,
	    ("ukopen: dev=0x%x (unit %d (of %d))\n", dev, unit,
		uk_cd.cd_ndevs));

	/*
	 * Only allow one at a time
	 */
	if (periph->periph_flags & PERIPH_OPEN) {
		printf("%s: already open\n", uk->sc_dev.dv_xname);
		return (EBUSY);
	}

	if ((error = scsipi_adapter_addref(adapt)) != 0)
		return (error);
	periph->periph_flags |= PERIPH_OPEN;

	SC_DEBUG(periph, SCSIPI_DB3, ("open complete\n"));
	return (0);
}

/*
 * close the device.. only called if we are the LAST
 * occurence of an open device
 */
static int
ukclose(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct uk_softc *uk = uk_cd.cd_devs[UKUNIT(dev)];
	struct scsipi_periph *periph = uk->sc_periph;
	struct scsipi_adapter *adapt = periph->periph_channel->chan_adapter;

	SC_DEBUG(uk->sc_periph, SCSIPI_DB1, ("closing\n"));

	scsipi_wait_drain(periph);

	scsipi_adapter_delref(adapt);
	periph->periph_flags &= ~PERIPH_OPEN;

	return (0);
}

/*
 * Perform special action on behalf of the user
 * Only does generic scsi ioctls.
 */
static int
ukioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	register struct uk_softc *uk = uk_cd.cd_devs[UKUNIT(dev)];

	return (scsipi_do_ioctl(uk->sc_periph, dev, cmd, addr, flag, l));
}
