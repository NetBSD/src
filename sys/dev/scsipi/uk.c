/* 
 * Dummy driver for a device we can't identify.
 * by Julian Elischer (julian@tfs.com)
 *
 *      $Id: uk.c,v 1.4 1994/02/16 02:41:10 mycroft Exp $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

#define	UKUNIT(z)	(minor(z))

struct uk_data {
	struct device sc_dev;

	u_int32 flags;
#define UKINIT		0x01
	struct scsi_link *sc_link;	/* all the inter level info */
};

void ukattach __P((struct device *, struct device *, void *));

struct cfdriver ukcd =
{ NULL, "uk", scsi_targmatch, ukattach, DV_DULL, sizeof(struct uk_data) };

/*
 * This driver is so simple it uses all the default services
 */
struct scsi_device uk_switch = {
	NULL,
	NULL,
	NULL,
	NULL,
	"uk",
	0
};

/*
 * The routine called by the low level scsi routine when it discovers
 * a device suitable for this driver.
 */
void
ukattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct uk_data *uk = (struct uk_data *)self;
	struct scsi_link *sc_link = aux;

	SC_DEBUG(sc_link, SDEV_DB2, ("ukattach: "));

	/*
	 * Store information needed to contact our base driver
	 */
	uk->sc_link = sc_link;
	sc_link->device = &uk_switch;
	sc_link->dev_unit = uk->sc_dev.dv_unit;

	printf(": unknown device\n");
	uk->flags |= UKINIT;
}

/*
 * open the device.
 */
int
ukopen(dev)
	dev_t dev;
{
	int unit;
	struct uk_data *uk;
	struct scsi_link *sc_link;

	unit = UKUNIT(dev);

	if (unit >= ukcd.cd_ndevs)
		return ENXIO;
	uk = ukcd.cd_devs[unit];
	/*
	 * Make sure the device has been initialised
	 */
	if (!uk || !(uk->cflags & UKINIT))
		return ENXIO;
		
	sc_link = uk->sc_link;

	/*
	 * Only allow one at a time
	 */
	if (sc_link->flags & SDEV_OPEN) {
		printf("%s: already open\n", uk->sc_dev.dv_xname);
		return EBUSY;
	}

	sc_link->flags |= SDEV_OPEN;
	SC_DEBUG(sc_link, SDEV_DB1,
	    ("ukopen: dev=0x%x (unit %d (of %d))\n", unit, ukcd.cd_ndevs));

	return 0;
}

/*
 * close the device.. only called if we are the LAST
 * occurence of an open device
 */
int
ukclose(dev)
	dev_t dev;
{
	int unit;
	struct uk_data *uk;

	unit = UKUNIT(dev);
	uk = ukcd.cd_devs[unit];
	uk->sc_link->flags &= ~SDEV_OPEN;
	SC_DEBUG(uk->sc_link, SDEV_DB1, ("closed\n"));
	return 0;
}

/*
 * Perform special action on behalf of the user
 * Only does generic scsi ioctls.
 */
int
ukioctl(dev, cmd, addr, flag)
	dev_t dev;
	int cmd;
	caddr_t addr;
	int flag;
{
	int unit;
	register struct uk_data *uk;
	struct scsi_link *sc_link;

	/*
	 * Find the device that the user is talking about
	 */
	unit = UKUNIT(dev);
	uk = ukcd.cd_devs[unit];
	return scsi_do_ioctl(uk->sc_link, cmd, addr, flag));
}
