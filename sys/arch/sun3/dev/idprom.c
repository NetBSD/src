/*
 * Machine ID PROM - system type and serial number
 * $Id: idprom.c,v 1.6 1994/05/06 07:49:15 gwr Exp $
 */

#include "sys/param.h"
#include "sys/systm.h"
#include "sys/device.h"

#include "machine/autoconf.h"
#include "machine/control.h"
#include "machine/idprom.h"
#include "machine/obctl.h"

#include "idprom.h"

extern void idpromattach __P((struct device *, struct device *, void *));

struct idprom_softc {
    struct device idprom_driver;
    int           idprom_init;
    struct idprom idprom_copy;
    char         *idprom_addr;
    int           idprom_size;
};

struct cfdriver idpromcd = 
{ NULL, "idprom", always_match, idpromattach, DV_DULL,
      sizeof(struct idprom_softc), 0};

#define IDPROM_CHECK(unit) \
      if (unit >= idpromcd.cd_ndevs || (idpromcd.cd_devs[unit] == NULL)) \
	  return ENXIO
#define UNIT_TO_IDP(unit) idpromcd.cd_devs[unit]


void idpromattach(parent, self, args)
     struct device *parent;
     struct device *self;
     void *args;
{
    struct idprom_softc *idp = (struct idprom_softc *) self;
    struct obctl_cf_loc *obctl_loc = OBCTL_LOC(self);

    idp->idprom_init = 0;

    idp->idprom_addr =
	OBCTL_DEFAULT_PARAM(char *, obctl_loc->obctl_addr, IDPROM_BASE);
    idp->idprom_size =
	OBCTL_DEFAULT_PARAM(int, obctl_loc->obctl_size, IDPROM_SIZE);
    obctl_print(idp->idprom_addr, idp->idprom_size);
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
