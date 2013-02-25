/*	$NetBSD: autoconf.c,v 1.100.2.2 2013/02/25 00:28:41 tls Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 2002 The NetBSD Foundation, Inc.
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
 * 3. Neither the name of the University nor the names of its contributors
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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.100.2.2 2013/02/25 00:28:41 tls Exp $");

#include "dvbox.h"
#include "gbox.h"
#include "hyper.h"
#include "rbox.h"
#include "topcat.h"
#include "tvrx.h"
#include "gendiofb.h"
#include "com_dio.h"
#include "com_frodo.h"
#include "dcm.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <sys/mount.h>
#include <sys/queue.h>
#include <sys/reboot.h>
#include <sys/tty.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/autoconf.h>
#include <machine/vmparam.h>
#include <machine/cpu.h>
#include <machine/hp300spu.h>
#include <machine/intr.h>
#include <machine/pte.h>

#include <hp300/dev/dioreg.h>
#include <hp300/dev/diovar.h>
#include <hp300/dev/diodevs.h>

#include <hp300/dev/intioreg.h>
#include <hp300/dev/dmavar.h>
#include <hp300/dev/frodoreg.h>

#include <hp300/dev/hpibvar.h>

#if NCOM_DIO > 0
#include <hp300/dev/com_diovar.h>
#endif
#if NCOM_FRODO > 0
#include <hp300/dev/com_frodovar.h>
#endif

#include <hp300/dev/diofbreg.h>
#include <hp300/dev/diofbvar.h>

/* should go away with a cleanup */
extern int dcmcnattach(bus_space_tag_t, bus_addr_t, int);
extern int dnkbdcnattach(bus_space_tag_t, bus_addr_t);

static int	dio_scan(int (*func)(bus_space_tag_t, bus_addr_t, int));
static int	dio_scode_probe(int,
		    int (*func)(bus_space_tag_t, bus_addr_t, int));

/* How we were booted. */
u_int	bootdev;

/*
 * Extent map to manage the external I/O (DIO/DIO-II) space.  We
 * allocate storate for 8 regions in the map.  extio_ex_malloc_safe
 * will indicate that it's safe to use malloc() to dynamically allocate
 * region descriptors in case we run out.
 */
static long extio_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
struct extent *extio_ex;
int extio_ex_malloc_safe;

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
	device_t		dd_dev;  /* device described by this entry */
	int			dd_scode; /* select code of device */
	int			dd_slave; /* ...or slave */
	int			dd_punit; /* and punit... */
};
typedef LIST_HEAD(, dev_data) ddlist_t;
static ddlist_t	dev_data_list;	  	/* all dev_datas */
static ddlist_t	dev_data_list_hpib;	/* hpib controller dev_datas */
static ddlist_t	dev_data_list_scsi;	/* scsi controller dev_datas */

static void	findbootdev(void);
static void	findbootdev_slave(ddlist_t *, int, int, int);
static void	setbootdev(void);

static struct dev_data *dev_data_lookup(device_t);
static void	dev_data_insert(struct dev_data *, ddlist_t *);

static int	mainbusmatch(device_t, cfdata_t, void *);
static void	mainbusattach(device_t, device_t, void *);
static int	mainbussearch(device_t, cfdata_t, const int *, void *);

CFATTACH_DECL_NEW(mainbus, 0,
    mainbusmatch, mainbusattach, NULL, NULL);

static int
mainbusmatch(device_t parent, cfdata_t cf, void *aux)
{
	static int mainbus_matched = 0;

	/* Allow only one instance. */
	if (mainbus_matched)
		return 0;

	mainbus_matched = 1;
	return 1;
}

static void
mainbusattach(device_t parent, device_t self, void *aux)
{

	aprint_normal("\n");

	/* Search for and attach children. */
	config_search_ia(mainbussearch, self, "mainbus", NULL);
}

static int
mainbussearch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{

	if (config_match(parent, cf, NULL) > 0)
		config_attach(parent, cf, NULL, NULL);
	return 0;
}

/*
 * Determine the device configuration for the running system.
 */
void
cpu_configure(void)
{

	/*
	 * Initialize the dev_data_lists.
	 */
	LIST_INIT(&dev_data_list);
	LIST_INIT(&dev_data_list_hpib);
	LIST_INIT(&dev_data_list_scsi);

	/* Kick off autoconfiguration. */
	(void)splhigh();

	/* Initialize the interrupt handlers. */
	intr_init();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");

	/* Configuration is finished, turn on interrupts. */
	(void)spl0();
}

/**********************************************************************
 * Code to find and set the boot device
 **********************************************************************/

void
cpu_rootconf(void)
{
	struct dev_data *dd;
	device_t dv;
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
			printf("boot device: %s\n", device_xname(booted_device));
		}
	}

	/*
	 * If wild carded root device and wired down NFS root file system,
	 * pick the network interface device to use.
	 */
	if (rootspec == NULL) {
		vops = vfs_getopsbyname(MOUNT_NFS);
		if (vops != NULL && vops->vfs_mountroot != NULL &&
		    strcmp(rootfstype, MOUNT_NFS) == 0) {
			for (dd = LIST_FIRST(&dev_data_list);
			    dd != NULL; dd = LIST_NEXT(dd, dd_list)) {
				if (device_class(dd->dd_dev) == DV_IFNET) {
					/* Got it! */
					booted_device = dd->dd_dev;
					break;
				}
			}
			if (dd == NULL) {
				printf("no network interface for NFS root");
				dv = NULL;
			}
		}
		if (vops != NULL)
			vfs_delref(vops);
	}

	/*
	 * If bootdev is bogus, ask the user anyhow.
	 */
	if (bootdev == 0)
		boothowto |= RB_ASKNAME;

	/*
	 * If we booted from tape, ask the user.
	 */
	if (booted_device != NULL && device_class(booted_device) == DV_TAPE)
		boothowto |= RB_ASKNAME;

	rootconf();

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
device_register(device_t dev, void *aux)
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

	dd = malloc(sizeof(struct dev_data), M_DEVBUF, M_NOWAIT | M_ZERO);
	if (dd == NULL)
		panic("device_register: can't allocate dev_data");

	dd->dd_dev = dev;

	/*
	 * BOOTROM and boot program can really only understand
	 * using the lowest select code network interface,
	 * so we ignore all but the first.
	 */
	if (device_class(dev) == DV_IFNET && seen_netdevice == 0) {
		struct dio_attach_args *da = aux;

		seen_netdevice = 1;
		dd->dd_scode = da->da_scode;
		goto linkup;
	}

	if (device_is_a(dev, "fhpib") ||
	    device_is_a(dev, "nhpib") ||
	    device_is_a(dev, "spc")) {
		struct dio_attach_args *da = aux;

		dd->dd_scode = da->da_scode;
		goto linkup;
	}

	if (device_is_a(dev, "rd")) {
		struct hpibbus_attach_args *ha = aux;

		dd->dd_slave = ha->ha_slave;
		dd->dd_punit = ha->ha_punit;
		goto linkup;
	}

	if (device_is_a(dev, "sd")) {
		struct scsipibus_attach_args *sa = aux;

		dd->dd_slave = sa->sa_periph->periph_target;
		dd->dd_punit = sa->sa_periph->periph_lun;
		goto linkup;
	}

	/*
	 * Didn't need the dev_data.
	 */
	free(dd, M_DEVBUF);
	return;

 linkup:
	LIST_INSERT_HEAD(&dev_data_list, dd, dd_list);

	if (device_is_a(dev, "fhpib") ||
	    device_is_a(dev, "nhpib")) {
		dev_data_insert(dd, &dev_data_list_hpib);
		return;
	}

	if (device_is_a(dev, "spc")) {
		dev_data_insert(dd, &dev_data_list_scsi);
		return;
	}
}

static void
findbootdev(void)
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
		for (dd = LIST_FIRST(&dev_data_list); dd != NULL;
		    dd = LIST_NEXT(dd, dd_list)) {
			if (device_class(dd->dd_dev) == DV_IFNET) {
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
		if ((type == 0 && !device_is_a(booted_device, "ct")) ||
		    (type == 2 && !device_is_a(booted_device, "rd"))) {
			printf("WARNING: boot device/type mismatch!\n");
			printf("device = %s, type = %d\n",
			    device_xname(booted_device), type);
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
		if ((type == 4 && !device_is_a(booted_device, "sd"))) {
			printf("WARNING: boot device/type mismatch!\n");
			printf("device = %s, type = %d\n",
			    device_xname(booted_device), type);
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

static void
findbootdev_slave(ddlist_t *ddlist, int ctlr, int slave, int punit)
{
	struct dev_data *cdd, *dd;

	/*
	 * Find the booted controller.
	 */
	for (cdd = LIST_FIRST(ddlist); ctlr != 0 && cdd != NULL;
	    cdd = LIST_NEXT(cdd, dd_clist))
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
	for (dd = LIST_FIRST(&dev_data_list); dd != NULL;
	    dd = LIST_NEXT(dd, dd_list)) {
		/*
		 * "sd" -> "scsibus" -> "spc"
		 * "rd" -> "hpibbus" -> "fhpib"
		 */
		if (device_parent(device_parent(dd->dd_dev)) != cdd->dd_dev)
			continue;

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

static void
setbootdev(void)
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
	if (device_class(root_device) == DV_IFNET) {
		bootdev = MAKEBOOTDEV(6, 0, 0, 0, 0);
		goto out;
	}

	/*
	 * Determine device type.
	 */
	if (device_is_a(root_device, "rd"))
		type = 2;
	else if (device_is_a(root_device, "sd"))
		type = 4;
	else if (device_is_a(root_device, "md"))
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
	case 2: /* rd */
	case 4: /* sd */
		/*
		 * "rd" -> "hpibbus" -> "fhpib"
		 * "sd" -> "scsibus" -> "spc"
		 */
		for (cdd = LIST_FIRST(&dev_data_list_hpib), ctlr = 0;
		    cdd != NULL; cdd = LIST_NEXT(cdd, dd_clist), ctlr++) {
			if (cdd->dd_dev ==
			    device_parent(device_parent(root_device))) {
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
	for (dd = LIST_FIRST(&dev_data_list); dd != NULL; ) {
		cdd = dd;
		dd = LIST_NEXT(dd, dd_list);
		free(cdd, M_DEVBUF);
	}
}

/*
 * Return the dev_data corresponding to the given device.
 */
static struct dev_data *
dev_data_lookup(device_t dev)
{
	struct dev_data *dd;

	for (dd = LIST_FIRST(&dev_data_list); dd != NULL;
	    dd = LIST_NEXT(dd, dd_list))
		if (dd->dd_dev == dev)
			return dd;

	panic("dev_data_lookup");
}

/*
 * Insert a dev_data into the provided list, sorted by select code.
 */
static void
dev_data_insert(struct dev_data *dd, ddlist_t *ddlist)
{
	struct dev_data *de;

#ifdef DIAGNOSTIC
	if (dd->dd_scode < 0 || dd->dd_scode > 255) {
		printf("bogus select code for %s\n", device_xname(dd->dd_dev));
		panic("dev_data_insert");
	}
#endif

	de = LIST_FIRST(ddlist);

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
	for (; LIST_NEXT(de, dd_clist) != NULL; de = LIST_NEXT(de, dd_clist)) {
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

int conscode;
void *conaddr;

void
hp300_cninit(void)
{
	struct bus_space_tag tag;
	bus_space_tag_t bst;

	bst = &tag;
	memset(bst, 0, sizeof(struct bus_space_tag));
	bst->bustype = HP300_BUS_SPACE_INTIO;

	/*
	 * Look for serial consoles first.
	 */
#if NCOM_FRODO > 0
	if (!com_frodo_cnattach(bst, FRODO_BASE + FRODO_APCI_OFFSET(1),
	    CONSCODE_INTERNAL))
		return;
#endif
#if NCOM_DIO > 0
	if (!dio_scan(com_dio_cnattach))
		return;
#endif
#if NDCM > 0
	if (!dio_scan(dcmcnattach))
		return;
#endif

#ifndef CONSCODE
	/*
	 * Look for internal framebuffers.
	 */
#if NDVBOX > 0
	if (!dvboxcnattach(bst, FB_BASE, CONSCODE_INTERNAL))
		goto find_kbd;
#endif
#if NGBOX > 0
	if (!gboxcnattach(bst, FB_BASE, CONSCODE_INTERNAL))
		goto find_kbd;
#endif
#if NRBOX > 0
	if (!rboxcnattach(bst, FB_BASE, CONSCODE_INTERNAL))
		goto find_kbd;
#endif
#if NTOPCAT > 0
	if (!topcatcnattach(bst, FB_BASE, CONSCODE_INTERNAL))
		goto find_kbd;
#endif
#endif	/* CONSCODE */

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
#if NTVRX > 0
	if (!dio_scan(tvrxcnattach))
		goto find_kbd;
#endif
#if NGENDIOFB > 0
	if (!dio_scan(gendiofbcnattach))
		goto find_kbd;
#endif

#if (NDVBOX + NGBOX + NRBOX + NTOPCAT + NDVBOX + NGBOX + NHYPER + NRBOX + \
     NTOPCAT + NTVRX + NGENDIOFB) > 0
find_kbd:
#endif

#if NDNKBD > 0
	dnkbdcnattach(bst, FRODO_BASE + FRODO_APCI_OFFSET(0))
#endif

#if NHILKBD > 0
	/* not yet */
	hilkbdcnattach(bst, HIL_BASE);
#endif
;
}

static int
dio_scan(int (*func)(bus_space_tag_t, bus_addr_t, int))
{
#ifndef CONSCODE
	int scode, sctop;

	sctop = DIO_SCMAX(machineid);
	for (scode = 0; scode < sctop; ++scode) {
		if (DIO_INHOLE(scode) || ((scode == 7) && internalhpib))
			continue;
		if (!dio_scode_probe(scode, func))
			return 0;
	}
#else
		if (!dio_scode_probe(CONSCODE, func))
			return 0;
#endif

	return 1;
}

static int
dio_scode_probe(int scode, int (*func)(bus_space_tag_t, bus_addr_t, int))
{
	struct bus_space_tag tag;
	bus_space_tag_t bst;
	void *pa, *va;

	bst = &tag;
	memset(bst, 0, sizeof(struct bus_space_tag));
	bst->bustype = HP300_BUS_SPACE_DIO;
	pa = dio_scodetopa(scode);
	va = iomap(pa, PAGE_SIZE);
	if (va == 0)
		return 1;
	if (badaddr(va)) {
		iounmap(va, PAGE_SIZE);
		return 1;
	}
	iounmap(va, PAGE_SIZE);

	return (*func)(bst, (bus_addr_t)pa, scode);
}


/**********************************************************************
 * Mapping functions
 **********************************************************************/

/*
 * Initialize the external I/O extent map.
 */
void
iomap_init(void)
{

	/* extiobase is initialized by pmap_bootstrap(). */
	extio_ex = extent_create("extio", (u_long) extiobase,
	    (u_long) extiobase + (ptoa(EIOMAPSIZE) - 1),
	    (void *) extio_ex_storage, sizeof(extio_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);
}

/*
 * Allocate/deallocate a cache-inhibited range of kernel virtual address
 * space mapping the indicated physical address range [pa - pa+size)
 */
void *
iomap(void *pa, int size)
{
	u_long kva;
	int error;

#ifdef DEBUG
	if (((int)pa & PGOFSET) || (size & PGOFSET))
		panic("iomap: unaligned");
#endif

	error = extent_alloc(extio_ex, size, PAGE_SIZE, 0,
	    EX_FAST | EX_NOWAIT | (extio_ex_malloc_safe ? EX_MALLOCOK : 0),
	    &kva);
	if (error)
		return 0;

	physaccess((void *) kva, pa, size, PG_RW|PG_CI);
	return (void *)kva;
}

/*
 * Unmap a previously mapped device.
 */
void
iounmap(void *kva, int size)
{

#ifdef DEBUG
	if (((vaddr_t)kva & PGOFSET) || (size & PGOFSET))
		panic("iounmap: unaligned");
	if ((uint8_t *)kva < extiobase ||
	    (uint8_t *)kva >= extiobase + ptoa(EIOMAPSIZE))
		panic("iounmap: bad address");
#endif
	physunaccess(kva, size);
	if (extent_free(extio_ex, (vaddr_t)kva, size,
	    EX_NOWAIT | (extio_ex_malloc_safe ? EX_MALLOCOK : 0)))
		printf("iounmap: kva %p size 0x%x: can't free region\n",
		    kva, size);
}
