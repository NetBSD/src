#include "idprom.h"

static int idprom_init = 0;

static struct idprom idprom_copy;

static int idprom_ok = 0;

int idprom_open(dev, oflags, devtype, p)
     dev_t dev;
     int oflags;
     int devtype;
     struct proc *p;
{
    int unit,s;
    
    unit = UNIT(dev);
    if (unit >= NIDPROM)
	return ENXIO;
    if (!idprom_init) idprom_ok = idprom_fetch(&idprom_copy, IDPROM_VERSION);
    if (!idprom_ok) return EIO;
    return 0;
}

int idprom_close(dev, fflag, devtype, p)
     dev_t dev;
     int fflag;
     int devtype;
     struct proc *p;
{
    int unit;

    unit = minor(dev);
    if (unit >= NIDPROM)
	return ENXIO;
    return 0;
}

idprom_read(dev, uio, ioflag)
     dev_t dev;
     struct uio *uio;
     int ioflag;
{
    int error, unit,length;


    unit = minor(dev);

    if (unit >= NIDPROM)
	return ENXIO;

    error = 0;
    while (uio->uio_resid > 0) {
	if (uio->uio_offset >= IDPROM_SIZE) break; /* past or at end */
	length = min(uio->uio_resid, (IDPROM_SIZE-uio->uio_offset));
	error = uiomove((caddr_t) &idprom_copy, length, uio);
    }
    return error;
}

