/*	$NetBSD: autoconf.c,v 1.49.20.3 2002/04/01 07:39:55 nathanw Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: autoconf.c 1.36 92/12/20$
 *
 *	@(#)autoconf.c	8.2 (Berkeley) 1/12/94
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.49.20.3 2002/04/01 07:39:55 nathanw Exp $");                                                  

#include "hil.h"
#include "dvbox.h"
#include "gbox.h"
#include "hyper.h"
#include "rbox.h"
#include "rbox.h"
#include "topcat.h"
#include "dca.h"
#include "dcm.h"
#include "apci.h"
#include "ite.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/mount.h>
#include <sys/queue.h>
#include <sys/reboot.h>
#include <sys/tty.h>

#include <dev/cons.h>

#include <machine/autoconf.h>
#include <machine/vmparam.h>
#include <machine/cpu.h>
#include <machine/hp300spu.h>
#include <machine/intr.h>
#include <machine/pte.h>

#include <hp300/dev/dioreg.h>
#include <hp300/dev/diovar.h>
#include <hp300/dev/diodevs.h>

#include <hp300/dev/dmavar.h>
#include <hp300/dev/grfreg.h>
#include <hp300/dev/hilreg.h>
#include <hp300/dev/hilioctl.h>
#include <hp300/dev/hilvar.h>

#include <hp300/dev/hpibvar.h>
#include <hp300/dev/scsivar.h>


/* should go away with a cleanup */
extern int dcacnattach(bus_space_tag_t, bus_addr_t, int);
extern int dcmcnattach(bus_space_tag_t, bus_addr_t, int);
extern int apcicnattach(bus_space_tag_t, bus_addr_t, int);
extern int dvboxcnattach(bus_space_tag_t, bus_addr_t, int);
extern int gboxcnattach(bus_space_tag_t, bus_addr_t, int);
extern int rboxcnattach(bus_space_tag_t, bus_addr_t, int);
extern int hypercnattach(bus_space_tag_t, bus_addr_t, int);
extern int topcatcnattach(bus_space_tag_t, bus_addr_t, int);
extern int hilkbdcnattach(bus_space_tag_t, bus_addr_t);
extern int dnkbdcnattach(bus_space_tag_t, bus_addr_t);

int dio_scan(int (*func)(bus_space_tag_t, bus_addr_t, int));
int dio_scode_probe(int, int (*func)(bus_space_tag_t, bus_addr_t, int));

/* XXX must be allocated statically because of early console init */
struct	map extiomap[EIOMAPSIZE/16];

extern	caddr_t internalhpib;
extern	char *extiobase;

/* The boot device. */
struct	device *booted_device;
int	booted_partition;

/* How we were booted. */
u_int	bootdev;

/*
 * This information is built during the autoconfig process.
 * A little explanation about the way this works is in order.
 *
 *	device_register() links all devices into dev_data_list.
 *	If the device is an hpib controller, it is also linked
 *	into dev_data_list_hpib.  If the device is a scsi controller,
 *	it is also linked into dev_data_list_scsi.
 *
 *	dev_data_list_hpib and dev_data_list_scsi are sorted
 *	by select code, from lowest to highest.
 *
 *	After autoconfiguration is complete, we need to determine
 *	which device was the boot device.  The boot block assigns
 *	controller unit numbers in order of select code.  Thus,
 *	providing the controller is configured in the kernel, we
 *	can determine our version of controller unit number from
 *	the sorted hpib/scsi list.
 *
 *	At this point, we know the controller (device type
 *	encoded in bootdev tells us "scsi disk", or "hpib tape",
 *	etc.).  The next step is to find the device which
 *	has the following properties:
 *
 *		- A child of the boot controller.
 *		- Same slave as encoded in bootdev.
 *		- Same physical unit as encoded in bootdev.
 *
 *	Later, after we've set the root device in stone, we
 *	reverse the process to re-encode bootdev so it can be
 *	passed back to the boot block.
 */
struct dev_data {
	LIST_ENTRY(dev_data)	dd_list;  /* dev_data_list */
	LIST_ENTRY(dev_data)	dd_clist; /* ctlr list */
	struct device		*dd_dev;  /* device described by this entry */
	int			dd_scode; /* select code of device */
	int			dd_slave; /* ...or slave */
	int			dd_punit; /* and punit... */
};
typedef LIST_HEAD(, dev_data) ddlist_t;
ddlist_t	dev_data_list;	  	/* all dev_datas */
ddlist_t	dev_data_list_hpib;	/* hpib controller dev_datas */
ddlist_t	dev_data_list_scsi;	/* scsi controller dev_datas */

void	findbootdev __P((void));
void	findbootdev_slave __P((ddlist_t *, int, int, int));
void	setbootdev __P((void));

static	struct dev_data *dev_data_lookup __P((struct device *));
static	void dev_data_insert __P((struct dev_data *, ddlist_t *));

int	mainbusmatch __P((struct device *, struct cfdata *, void *));
void	mainbusattach __P((struct device *, struct device *, void *));
int	mainbussearch __P((struct device *, struct cfdata *, void *));

struct cfattach mainbus_ca = {
	sizeof(struct device), mainbusmatch, mainbusattach
};

int
mainbusmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	static int mainbus_matched = 0;

	/* Allow only one instance. */
	if (mainbus_matched)
		return (0);

	mainbus_matched = 1;
	return (1);
}

void
mainbusattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{

	printf("\n");

	/* Search for and attach children. */
	config_search(mainbussearch, self, NULL);
}

int
mainbussearch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	if ((*cf->cf_attach->ca_match)(parent, cf, NULL) > 0)
		config_attach(parent, cf, NULL, NULL);
	return (0);
}

/*
 * Determine the device configuration for the running system.
 */
void
cpu_configure()
{

	/*
	 * Initialize the dev_data_lists.
	 */
	LIST_INIT(&dev_data_list);
	LIST_INIT(&dev_data_list_hpib);
	LIST_INIT(&dev_data_list_scsi);

	/* Kick off autoconfiguration. */
	(void)splhigh();

	softintr_init();

	if (config_rootfound("mainbus", "mainbus") == NULL)
		panic("no mainbus found");

	/* Configuration is finished, turn on interrupts. */
	(void)spl0();

	intr_printlevels();
}

/**********************************************************************
 * Code to find and set the boot device
 **********************************************************************/

void
cpu_rootconf()
{
	struct dev_data *dd;
	struct device *dv;
	struct vfsops *vops;

	/*
	 * Find boot device.
	 */
	if ((bootdev & B_MAGICMASK) != B_DEVMAGIC) {
		printf("WARNING: boot program didn't supply boot device.\n");
		printf("Please update your boot program.\n");
	} else {
		findbootdev();
		if (booted_device == NULL) {
			printf("WARNING: can't find match for bootdev:\n");
			printf(
		    "type = %d, ctlr = %d, slave = %d, punit = %d, part = %d\n",
			    B_TYPE(bootdev), B_ADAPTOR(bootdev),
			    B_CONTROLLER(bootdev), B_UNIT(bootdev),
			    B_PARTITION(bootdev));
			bootdev = 0;		/* invalidate bootdev */
		} else {
			printf("boot device: %s\n", booted_device->dv_xname);
		}
	}

	dv = booted_device;

	/*
	 * If wild carded root device and wired down NFS root file system,
	 * pick the network interface device to use.
	 */
	if (rootspec == NULL) {
		vops = vfs_getopsbyname("nfs");
		if (vops != NULL && vops->vfs_mountroot == mountroot) {
			for (dd = dev_data_list.lh_first;
			    dd != NULL; dd = dd->dd_list.le_next) {
				if (dd->dd_dev->dv_class == DV_IFNET) {
					/* Got it! */
					dv = dd->dd_dev;
					break;
				}
			}
			if (dd == NULL) {
				printf("no network interface for NFS root");
				dv = NULL;
			}
		}
	}

	/*
	 * If bootdev is bogus, ask the user anyhow.
	 */
	if (bootdev == 0)
		boothowto |= RB_ASKNAME;

	/*
	 * If we booted from tape, ask the user.
	 */
	if (booted_device != NULL && booted_device->dv_class == DV_TAPE)
		boothowto |= RB_ASKNAME;

	setroot(dv, booted_partition);

	/*
	 * Set bootdev based on what we found as the root.
	 * This is given to the boot program when we reboot.
	 */
	setbootdev();

}

/*
 * Register a device.  We're passed the device and the arguments
 * used to attach it.  This is used to find the boot device.
 */
void
device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	struct dev_data *dd;
	static int seen_netdevice = 0;

	/*
	 * Allocate a dev_data structure and fill it in.
	 * This means making some tests twice, but we don't
	 * care; this doesn't really have to be fast.
	 *
	 * Note that we only really care about devices that
	 * we can mount as root.
	 */

	MALLOC(dd, struct dev_data *, sizeof(struct dev_data),
	    M_DEVBUF, M_NOWAIT | M_ZERO);
	if (dd == NULL)
		panic("device_register: can't allocate dev_data");

	dd->dd_dev = dev;

	/*
	 * BOOTROM and boot program can really only understand
	 * using the lowest select code network interface,
	 * so we ignore all but the first.
	 */
	if (dev->dv_class == DV_IFNET && seen_netdevice == 0) {
		struct dio_attach_args *da = aux;

		seen_netdevice = 1;
		dd->dd_scode = da->da_scode;
		goto linkup;
	}

	if (memcmp(dev->dv_xname, "fhpib", 5) == 0 ||
	    memcmp(dev->dv_xname, "nhpib", 5) == 0 ||
	    memcmp(dev->dv_xname, "oscsi", 5) == 0) {
		struct dio_attach_args *da = aux;

		dd->dd_scode = da->da_scode;
		goto linkup;
	}

	if (memcmp(dev->dv_xname, "rd", 2) == 0) {
		struct hpibbus_attach_args *ha = aux;

		dd->dd_slave = ha->ha_slave;
		dd->dd_punit = ha->ha_punit;
		goto linkup;
	}

	if (memcmp(dev->dv_xname, "sd", 2) == 0) {
		struct oscsi_attach_args *osa = aux;

		dd->dd_slave = osa->osa_target;
		dd->dd_punit = osa->osa_lun;
		goto linkup;
	}

	/*
	 * Didn't need the dev_data.
	 */
	free(dd, M_DEVBUF);
	return;

 linkup:
	LIST_INSERT_HEAD(&dev_data_list, dd, dd_list);

	if (memcmp(dev->dv_xname, "fhpib", 5) == 0 ||
	    memcmp(dev->dv_xname, "nhpib", 5) == 0) {
		dev_data_insert(dd, &dev_data_list_hpib);
		return;
	}

	if (memcmp(dev->dv_xname, "oscsi", 5) == 0) {
		dev_data_insert(dd, &dev_data_list_scsi);
		return;
	}
}

void
findbootdev()
{
	int type, ctlr, slave, punit, part;
	int scsiboot, hpibboot, netboot;
	struct dev_data *dd;

	booted_device = NULL;
	booted_partition = 0;

	if ((bootdev & B_MAGICMASK) != B_DEVMAGIC)
		return;

	type  = B_TYPE(bootdev);
	ctlr  = B_ADAPTOR(bootdev);
	slave = B_CONTROLLER(bootdev);
	punit = B_UNIT(bootdev);
	part  = B_PARTITION(bootdev);

	scsiboot = (type == 4);			/* sd major */
	hpibboot = (type == 0 || type == 2);	/* ct/rd major */
	netboot  = (type == 6);			/* le - special */

	/*
	 * Check for network boot first, since it's a little
	 * different.  The BOOTROM/boot program can only boot
	 * off of the first (lowest select code) ethernet
	 * device.  device_register() knows this and only
	 * registers one DV_IFNET.  This is a safe assumption
	 * since the code that finds devices on the DIO bus
	 * always starts at scode 0 and works its way up.
	 */
	if (netboot) {
		for (dd = dev_data_list.lh_first; dd != NULL;
		    dd = dd->dd_list.le_next) {
			if (dd->dd_dev->dv_class == DV_IFNET) {
				/*
				 * Found it!
				 */
				booted_device = dd->dd_dev;
				break;
			}
		}
		return;
	}

	/*
	 * Check for HP-IB boots next.
	 */
	if (hpibboot) {
		findbootdev_slave(&dev_data_list_hpib, ctlr,
		    slave, punit);
		if (booted_device == NULL)
			return;

		/*
		 * Sanity check.
		 */
		if ((type == 0 && memcmp(booted_device->dv_xname, "ct", 2)) ||
		    (type == 2 && memcmp(booted_device->dv_xname, "rd", 2))) {
			printf("WARNING: boot device/type mismatch!\n");
			printf("device = %s, type = %d\n",
			    booted_device->dv_xname, type);
			booted_device = NULL;
		}
		goto out;
	}

	/*
	 * Check for SCSI boots last.
	 */
	if (scsiboot) {
		findbootdev_slave(&dev_data_list_scsi, ctlr,
		     slave, punit);
		if (booted_device == NULL)  
			return; 

		/*
		 * Sanity check.
		 */
		if ((type == 4 && memcmp(booted_device->dv_xname, "sd", 2))) {
			printf("WARNING: boot device/type mismatch!\n");
			printf("device = %s, type = %d\n",
			    booted_device->dv_xname, type);
			booted_device = NULL; 
		}
		goto out;
	}

	/* Oof! */
	printf("WARNING: UNKNOWN BOOT DEVICE TYPE = %d\n", type);

 out:
	if (booted_device != NULL)
		booted_partition = part;
}

void
findbootdev_slave(ddlist, ctlr, slave, punit)
	ddlist_t *ddlist;
	int ctlr, slave, punit;
{
	struct dev_data *cdd, *dd;

	/*
	 * Find the booted controller.
	 */
	for (cdd = ddlist->lh_first; ctlr != 0 && cdd != NULL;
	    cdd = cdd->dd_clist.le_next)
		ctlr--;
	if (cdd == NULL) {
		/*
		 * Oof, couldn't find it...
		 */
		return;
	}

	/*
	 * Now find the device with the right slave/punit
	 * that's a child of the controller.
	 */
	for (dd = dev_data_list.lh_first; dd != NULL;
	    dd = dd->dd_list.le_next) {
		/*
		 * XXX We don't yet have the extra bus indirection
		 * XXX for SCSI, so we have to do a little bit of
		 * XXX extra work.
		 */
		if (memcmp(dd->dd_dev->dv_xname, "sd", 2) == 0) {
			/*
			 * "sd" -> "oscsi"
			 */
			if (dd->dd_dev->dv_parent != cdd->dd_dev)
				continue;
		} else {
			/*
			 * "rd" -> "hpibbus" -> "fhpib"
			 */
			if (dd->dd_dev->dv_parent->dv_parent != cdd->dd_dev)
				continue;
		}

		if (dd->dd_slave == slave &&
		    dd->dd_punit == punit) {
			/*
			 * Found it!
			 */
			booted_device = dd->dd_dev;
			break;
		}
	}
}

void
setbootdev()
{
	struct dev_data *cdd, *dd;
	int type, ctlr;

	/*
	 * Note our magic numbers for type:
	 *
	 *	0 == ct
	 *	2 == rd
	 *	4 == sd
	 *	6 == le
	 *
	 * Allare bdevsw major numbers, except for le, which
	 * is just special.
	 *
	 * We can't mount root on a tape, so we ignore those.
	 */

	/*
	 * Start with a clean slate.
	 */
	bootdev = 0;

	/*
	 * If the root device is network, we're done
	 * early.
	 */
	if (root_device->dv_class == DV_IFNET) {
		bootdev = MAKEBOOTDEV(6, 0, 0, 0, 0);
		goto out;
	}

	/*
	 * Determine device type.
	 */
	if (memcmp(root_device->dv_xname, "rd", 2) == 0)
		type = 2;
	else if (memcmp(root_device->dv_xname, "sd", 2) == 0)
		type = 4;
	else if (memcmp(root_device->dv_xname, "md", 2) == 0)
		goto out;
	else {
		printf("WARNING: strange root device!\n");
		goto out;
	}

	dd = dev_data_lookup(root_device);

	/*
	 * Get parent's info.
	 */
	switch (type) {
	case 2:
		/*
		 * "rd" -> "hpibbus" -> "fhpib"
		 */
		for (cdd = dev_data_list_hpib.lh_first, ctlr = 0;
		    cdd != NULL; cdd = cdd->dd_clist.le_next, ctlr++) {
			if (cdd->dd_dev == root_device->dv_parent->dv_parent) {
				/*
				 * Found it!
				 */
				bootdev = MAKEBOOTDEV(type,
				    ctlr, dd->dd_slave, dd->dd_punit,
				    DISKPART(rootdev));
				break;
			}
		}
		break;

	case 4:
		/*
		 * "sd" -> "oscsi"
		 */
		for (cdd = dev_data_list_scsi.lh_first, ctlr = 0;
		    cdd != NULL; cdd = cdd->dd_clist.le_next, ctlr++) { 
			if (cdd->dd_dev == root_device->dv_parent) {
				/*
				 * Found it!
				 */
				bootdev = MAKEBOOTDEV(type,
				    ctlr, dd->dd_slave, dd->dd_punit,
				    DISKPART(rootdev));
				break;
			}
		}
		break;
	}

 out:
	/* Don't need this anymore. */
	for (dd = dev_data_list.lh_first; dd != NULL; ) {
		cdd = dd;
		dd = dd->dd_list.le_next;
		free(cdd, M_DEVBUF);
	}
}

/*
 * Return the dev_data corresponding to the given device.
 */
static struct dev_data *
dev_data_lookup(dev)
	struct device *dev;
{
	struct dev_data *dd;

	for (dd = dev_data_list.lh_first; dd != NULL; dd = dd->dd_list.le_next)
		if (dd->dd_dev == dev)
			return (dd);

	panic("dev_data_lookup");
}

/*
 * Insert a dev_data into the provided list, sorted by select code.
 */
static void
dev_data_insert(dd, ddlist)
	struct dev_data *dd;
	ddlist_t *ddlist;
{
	struct dev_data *de;

#ifdef DIAGNOSTIC
	if (dd->dd_scode < 0 || dd->dd_scode > 255) {
		printf("bogus select code for %s\n", dd->dd_dev->dv_xname);
		panic("dev_data_insert");
	}
#endif

	de = ddlist->lh_first;

	/*
	 * Just insert at head if list is empty.
	 */
	if (de == NULL) {
		LIST_INSERT_HEAD(ddlist, dd, dd_clist);
		return;
	}

	/*
	 * Traverse the list looking for a device who's select code
	 * is greater than ours.  When we find it, insert ourselves
	 * into the list before it.
	 */
	for (; de->dd_clist.le_next != NULL; de = de->dd_clist.le_next) {
		if (de->dd_scode > dd->dd_scode) {
			LIST_INSERT_BEFORE(de, dd, dd_clist);
			return;
		}
	}

	/*
	 * Our select code is greater than everyone else's.  We go
	 * onto the end.
	 */
	LIST_INSERT_AFTER(de, dd, dd_clist);
}

/**********************************************************************
 * Code to find and initialize the console
 **********************************************************************/

void
hp300_cninit()
{
	/*
	 * Look for serial consoles first.
	 */
#if NDCA > 0
	if (!dio_scan(dcacnattach))
		return;
#endif
#if NAPCI > 0
	if (!dio_scan(apcicnattach))
		return;
#endif
#if NDCM > 0
	if (!dio_scan(dcmcnattach))
		return;
#endif

#if NITE > 0
#ifndef CONSCODE
	/*
	 * Look for internal framebuffers.
	 */
#if NDVBOX > 0
	if (!dvboxcnattach(HP300_BUS_SPACE_INTIO, 0x160000,-1))
		goto find_kbd;
#endif
#if NGBOX > 0
	if (!gboxcnattach(HP300_BUS_SPACE_INTIO, 0x160000,-1))
		goto find_kbd;
#endif
#if NRBOX > 0
	if (!rboxcnattach(HP300_BUS_SPACE_INTIO, 0x160000,-1))
		goto find_kbd;
#endif
#if NTOPCAT > 0
	if (!topcatcnattach(HP300_BUS_SPACE_INTIO, 0x160000,-1))
		goto find_kbd;
#endif
#endif

	/*
	 * Look for external framebuffers.
	 */
#if NDVBOX > 0
	if (!dio_scan(dvboxcnattach))
		goto find_kbd;
#endif
#if NGBOX > 0
	if (!dio_scan(gboxcnattach))
		goto find_kbd;
#endif
#if NHYPER > 0
	if (!dio_scan(hypercnattach))
		goto find_kbd;
#endif
#if NRBOX > 0
	if (!dio_scan(rboxcnattach))
		goto find_kbd;
#endif
#if NTOPCAT > 0
	if (!dio_scan(topcatcnattach))
		goto find_kbd;
#endif

find_kbd:

#if NDNKBD > 0
	dnkbdcnattach(HP300_BUS_SPACE_INTIO, 0x1c000)
#endif

#if NHIL > 0
	hilkbdcnattach(HP300_BUS_SPACE_INTIO, 0x28000);
#endif
#endif	/* NITE */
}

int
dio_scan(func)
	int (*func)(bus_space_tag_t, bus_addr_t, int);
{
#ifndef CONSCODE
	int scode, sctop;

	sctop = DIO_SCMAX(machineid);
	for (scode = 0; scode < sctop; ++scode) {
		if (DIO_INHOLE(scode) || ((scode == 7) && internalhpib))
			continue;
		if (!dio_scode_probe(scode, func))
			return (0);
	}
#else
		if (!dio_scode_probe(CONSCODE, func))
			return (0);
#endif

	return (1);
}

int
dio_scode_probe(scode, func)
	int scode;
	int (*func)(bus_space_tag_t, bus_addr_t, int);
{
	caddr_t pa, va;

	pa = dio_scodetopa(scode);
	va = iomap(pa, NBPG);
	if (va == 0)
		return (1);
	if (badaddr(va)) {
		iounmap(va, NBPG);
		return (1);
	}
	iounmap(va, NBPG);

	return ((*func)(HP300_BUS_SPACE_DIO, (bus_addr_t)pa, scode));
}


/**********************************************************************
 * Mapping functions
 **********************************************************************/

/*
 * Allocate/deallocate a cache-inhibited range of kernel virtual address
 * space mapping the indicated physical address range [pa - pa+size)
 */
caddr_t
iomap(pa, size)
	caddr_t pa;
	int size;
{
	int ix, npf;
	caddr_t kva;

#ifdef DEBUG
	if (((int)pa & PGOFSET) || (size & PGOFSET))
		panic("iomap: unaligned");
#endif
	npf = btoc(size);
	ix = rmalloc(extiomap, npf);
	if (ix == 0)
		return(0);
	kva = extiobase + ctob(ix-1);
	physaccess(kva, pa, size, PG_RW|PG_CI);
	return(kva);
}

/*
 * Unmap a previously mapped device.
 */
void
iounmap(kva, size)
	caddr_t kva;
	int size;
{
	int ix;

#ifdef DEBUG
	if (((int)kva & PGOFSET) || (size & PGOFSET))
		panic("iounmap: unaligned");
	if (kva < extiobase || kva >= extiobase + ctob(EIOMAPSIZE))
		panic("iounmap: bad address");
#endif
	physunaccess(kva, size);
	ix = btoc(kva - extiobase) + 1;
	rmfree(extiomap, btoc(size), ix);
}
