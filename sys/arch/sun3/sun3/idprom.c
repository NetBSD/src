/*	$NetBSD: idprom.c,v 1.9 1994/12/12 18:59:06 gwr Exp $	*/

/*
 * Copyright (c) 1993 Adam Glass
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
 *	This product includes software developed by Adam Glass.
 * 4. The name of the authors may not be used to endorse or promote products
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

/*
 * Machine ID PROM - system type and serial number
 * All this to read some bytes in control space...
 */

#include "sys/param.h"
#include "sys/systm.h"
#include "sys/device.h"

#include "machine/autoconf.h"
#include "machine/control.h"
#include "machine/idprom.h"
#include "machine/obctl.h"

#include "idprom.h"

struct idprom_softc {
    struct device idprom_driver;
    int           idprom_init;
    struct idprom idprom_copy;
    char         *idprom_addr;
    int           idprom_size;
};

static int  idprom_match __P((struct device *, void *vcf, void *args));
static void idprom_attach __P((struct device *, struct device *, void *));

struct cfdriver idpromcd = {
	NULL, "idprom", idprom_match, idprom_attach,
	DV_DULL, sizeof(struct idprom_softc), 0 };

#define IDPROM_CHECK(unit) \
      if (unit >= idpromcd.cd_ndevs || (idpromcd.cd_devs[unit] == NULL)) \
	  return ENXIO
#define UNIT_TO_IDP(unit) idpromcd.cd_devs[unit]


int idprom_match(parent, vcf, args)
    struct device *parent;
    void *vcf, *args;
{
    struct cfdata *cf = vcf;
	struct confargs *ca = args;

	/* This driver only supports one unit. */
	if (cf->cf_unit != 0)
		return (0);
	if (ca->ca_paddr == -1)
		ca->ca_paddr = IDPROM_BASE;
	return (1);
}

void idprom_attach(parent, self, args)
     struct device *parent;
     struct device *self;
     void *args;
{
    struct idprom_softc *idp = (struct idprom_softc *) self;
    struct confargs *ca = args;

    idp->idprom_init = 0;

    idp->idprom_addr = (char *) ca->ca_paddr;
    idp->idprom_size = IDPROM_SIZE;

    printf("\n");
}

int idpromopen(dev, oflags, devtype, p)
     dev_t dev;
     int oflags;
     int devtype;
     struct proc *p;
{
    struct idprom_softc *idp;
    int unit, idprom_ok;
    
    unit = minor(dev);
    IDPROM_CHECK(unit);
    idp = UNIT_TO_IDP(unit);
    if (!idp->idprom_init)
	idprom_ok = idprom_fetch(&idp->idprom_copy, IDPROM_VERSION);
    if (!idprom_ok) return EIO;
    idp->idprom_init = 1;
    return 0;
}

int idpromclose(dev, fflag, devtype, p)
     dev_t dev;
     int fflag;
     int devtype;
     struct proc *p;
{
    int unit;

    unit = minor(dev);
    IDPROM_CHECK(unit);
    return 0;
}

idpromread(dev, uio, ioflag)
     dev_t dev;
     struct uio *uio;
     int ioflag;
{
    int error, unit, length;
    struct idprom_softc *idp;

    unit = minor(dev);
    IDPROM_CHECK(unit);
    idp = UNIT_TO_IDP(unit);
    if (!idp->idprom_init) return EIO;
    error = 0;
    while (uio->uio_resid > 0) {
	if (uio->uio_offset >= idp->idprom_size) break; /* past or at end */
	length = min(uio->uio_resid, (idp->idprom_size - uio->uio_offset));
	error = uiomove((caddr_t) &idp->idprom_copy, length, uio);
    }
    return error;
}
