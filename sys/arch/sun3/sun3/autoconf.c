/*	$NetBSD: autoconf.c,v 1.48 1997/04/25 18:07:36 gwr Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Gordon W. Ross.
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
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time.  Available devices are
 * determined (from possibilities mentioned in ioconf.c), and
 * the drivers are initialized.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

#include <machine/autoconf.h>
#include <machine/machdep.h>
#include <machine/mon.h>

/* Want compile-time initialization here. */
int cold = 1;

/*
 * Do general device autoconfiguration,
 * then choose root device (etc.)
 * Called by machdep.c: cpu_startup()
 */
void
configure()
{

	/* General device autoconfiguration. */
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not found");

	/*
	 * Now that device autoconfiguration is finished,
	 * we can safely enable interrupts.
	 */
	printf("enabling interrupts\n");
	(void)spl0();
	cold = 0;
}

/****************************************************************/

/*
 * Support code to find the boot device.
 */

static struct devnametobdevmaj nam2blk[] = {
	{ "xy",		3 },
	{ "sd",		7 },
	{ "xd",		10 },
	{ "md",		13 },
	{ "cd",		18 },
	{ NULL,		0 },
};

/* This takes the args: name, ctlr, unit */
typedef struct device * (*findfunc_t) __P((char *, int, int));

static struct device * find_dev_byname __P((char *));
static struct device * net_find  __P((char *, int, int));
static struct device * scsi_find __P((char *, int, int));
static struct device * xx_find   __P((char *, int, int));

struct prom_n2f {
	const char name[4];
	findfunc_t func;
};
static struct prom_n2f prom_dev_table[] = {
	{ "ie",		net_find },
	{ "le",		net_find },
	{ "sd",		scsi_find },
	{ "xy",		xx_find },
	{ "xd",		xx_find },
	{ "",		0 },
};

/*
 * Choose root and swap devices.
 */
void
cpu_rootconf()
{
	MachMonBootParam *bp;
	struct prom_n2f *nf;
	struct device *boot_device;
	int boot_partition;
	char *devname;
	findfunc_t find;
	char promname[4];
	char partname[4];

	/* PROM boot parameters. */
	bp = *romVectorPtr->bootParam;

	/*
	 * Copy PROM boot device name (two letters)
	 * to a normal, null terminated string.
	 * (No terminating null in bp->devName)
	 */
	promname[0] = bp->devName[0];
	promname[1] = bp->devName[1];
	promname[2] = '\0';

	/* Default to "unknown" */
	boot_device = NULL;
	boot_partition = 0;
	devname = "<unknown>";
	partname[0] = '\0';
	find = NULL;

	/* Do we know anything about the PROM boot device? */
	for (nf = prom_dev_table; nf->func; nf++)
		if (!strcmp(nf->name, promname)) {
			find = nf->func;
			break;
		}
	if (find)
		boot_device = (*find)(promname, bp->ctlrNum, bp->unitNum);
	if (boot_device) {
		devname = boot_device->dv_xname;
		if (boot_device->dv_class == DV_DISK) {
			boot_partition = bp->partNum & 7;
			partname[0] = 'a' + boot_partition;
			partname[1] = '\0';
		}
	}

	printf("boot device: %s%s\n", devname, partname);
	setroot(boot_device, boot_partition, nam2blk);
}

/*
 * Functions to find devices using PROM boot parameters.
 */

/*
 * Network device:  Just use controller number.
 */
static struct device *
net_find(name, ctlr, unit)
	char *name;
	int ctlr, unit;
{
	char tname[16];

	sprintf(tname, "%s%d", name, ctlr);
	return (find_dev_byname(tname));
}

/*
 * SCSI device:  The controller number corresponds to the
 * scsibus number, and the unit number is (targ*8 + LUN).
 */
static struct device *
scsi_find(name, ctlr, unit)
	char *name;
	int ctlr, unit;
{
	struct device *scsibus;
	struct scsibus_softc *sbsc;
	struct scsi_link *sc_link;
	int target, lun;
	char tname[16];

	sprintf(tname, "scsibus%d", ctlr);
	scsibus = find_dev_byname(tname);
	if (scsibus == NULL)
		return (NULL);

	/* Compute SCSI target/LUN from PROM unit. */
	target = (unit >> 3) & 7;
	lun = unit & 7;

	/* Find the device at this target/LUN */
	sbsc = (struct scsibus_softc *)scsibus;
	sc_link = sbsc->sc_link[target][lun];
	if (sc_link == NULL)
		return (NULL);

	return (sc_link->device_softc);
}

/*
 * Xylogics SMD disk: (xy, xd)
 * Assume wired-in unit numbers for now...
 */
static struct device *
xx_find(name, ctlr, unit)
	char *name;
	int ctlr, unit;
{
	int diskunit;
	char tname[16];

	diskunit = (ctlr * 2) + unit;
	sprintf(tname, "%s%d", name, diskunit);
	return (find_dev_byname(tname));
}

/*
 * Given a device name, find its struct device
 * XXX - Move this to some common file?
 */
static struct device *
find_dev_byname(name)
	char *name;
{
	struct device *dv;

	for (dv = alldevs.tqh_first; dv != NULL;
	    dv = dv->dv_list.tqe_next) {
		if (!strcmp(dv->dv_xname, name)) {
			return(dv);
		}
	}
	return (NULL);
}
