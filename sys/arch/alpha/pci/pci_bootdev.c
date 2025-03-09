/* $NetBSD: pci_bootdev.c,v 1.1 2025/03/09 01:06:42 thorpej Exp $ */

/*
 * Copyright (c) 1995, 1996, 1997 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pci_bootdev.c,v 1.1 2025/03/09 01:06:42 thorpej Exp $");

#include <sys/systm.h>
#include <sys/device.h>

#include <machine/alpha.h>
#include <machine/autoconf.h>

#include <dev/ata/atavar.h>
#include <dev/pci/pcivar.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/mlxio.h>		/* XXX */
#include <dev/ic/mlxvar.h>		/* XXX */

#include <dev/i2o/i2o.h>		/* XXX */
#include <dev/i2o/iopio.h>		/* XXX */
#include <dev/i2o/iopvar.h>		/* XXX */

#define	DPRINTF(x)	if (bootdev_debug) printf x

void
pci_find_bootdev(device_t hosedev, device_t dev, void *aux)
{
	static device_t pcidev, ctrlrdev;
	struct bootdev_data *b = bootdev_data;
	device_t parent = device_parent(dev);

	if (booted_device != NULL || b == NULL) {
		return;
	}

	if (pcidev == NULL) {
		if (device_is_a(dev, "pci")) {
			struct pcibus_attach_args *pba = aux;

			/*
			 * If a hose device was specified, ensure that
			 * this PCI instance has that device as an ancestor.
			 */
			if (hosedev) {
				while (parent) {
					if (parent == hosedev) {
						break;
					}
					parent = device_parent(parent);
				}
				if (!parent) {
					return;
				}
			}
			if ((b->slot / 1000) == pba->pba_bus) {
				pcidev = dev;
				DPRINTF(("\npcidev = %s\n", device_xname(dev)));
			}
		}
		return;
	}

	if (ctrlrdev == NULL) {
		if (parent == pcidev) {
			struct pci_attach_args *pa = aux;
			int slot = pa->pa_bus * 1000 + pa->pa_function * 100 +
			    pa->pa_device;

			if (b->slot == slot) {
				if (bootdev_is_net) {
					goto foundit;
				} else {
					ctrlrdev = dev;
					DPRINTF(("\nctrlrdev = %s\n",
					    device_xname(dev)));
				}
			}
		}
		return;
	}

	if (!bootdev_is_disk) {
		return;
	}

	if (device_is_a(dev, "sd") ||
	    device_is_a(dev, "st") ||
	    device_is_a(dev, "cd")) {
		struct scsipibus_attach_args *sa = aux;
		struct scsipi_periph *periph = sa->sa_periph;
		int unit;

		if (device_parent(parent) != ctrlrdev) {
			return;
		}

		unit = periph->periph_target * 100 + periph->periph_lun;
		if (b->unit != unit ||
		    b->channel != periph->periph_channel->chan_channel) {
			return;
		}
		goto foundit;
	}

	if (device_is_a(dev, "wd")) {
		struct ata_device *adev = aux;

		if (!device_is_a(parent, "atabus")) {
			return;
		}
		if (device_parent(parent) != ctrlrdev) {
			return;
		}

		DPRINTF(("\natapi info: drive %d, channel %d\n",
		    adev->adev_drv_data->drive, adev->adev_channel));
		DPRINTF(("bootdev info: unit: %d, channel: %d\n",
		    b->unit, b->channel));
		if (b->unit != adev->adev_drv_data->drive ||
		    b->channel != adev->adev_channel) {
			return;
		}
		goto foundit;
	}

	if (device_is_a(dev, "ld")) {
		/*
		 * XXX Attach arguments for ld devices is not consistent,
		 * XXX so we have to special-case each supported RAID
		 * XXX controller.
		 */
		if (parent != ctrlrdev) {
			return;
		}

		if (device_is_a(parent, "mlx")) {
			struct mlx_attach_args *mlxa = aux;

			if (b->unit != mlxa->mlxa_unit) {
				return;
			}
			goto foundit;
		}

		if (device_is_a(parent, "iop")) {
			struct iop_attach_args *iopa = aux;

			if (b->unit != iopa->ia_tid) {
				return;
			}
			goto foundit;
		}
	}

	return;

 foundit:
	booted_device = dev;
	DPRINTF(("\nbooted_device = %s\n", device_xname(dev)));
}
