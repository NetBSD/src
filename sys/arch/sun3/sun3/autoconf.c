/*	$NetBSD: autoconf.c,v 1.46 1997/03/26 22:39:20 gwr Exp $	*/

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

static void	rootconf __P((void));

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
	 *
	 * XXX - This would also be the "natrual" place to
	 * set cold=0 but we need to leave it set until the
	 * end so that kern_synch.c:tsleep() will not try to
	 * use the proc0 that is not yet ready...
	 * XXX - Maybe procinit should be earlier...
	 */
	printf("enabling interrupts\n");
	(void)spl0();
	cold = 0;
}

/****************************************************************/

/*
 * bus_scan:
 * This function is passed to config_search() by the attach function
 * for each of the "bus" drivers (obctl, obio, obmem, vmes, vmel).
 * The purpose of this function is to copy the "locators" into our
 * confargs structure, so child drivers may use the confargs both
 * as match parameters and as temporary storage for the defaulted
 * locator values determined in the child_match and preserved for
 * the child_attach function.  If the bus attach functions just
 * used config_found, then we would not have an opportunity to
 * setup the confargs for each child match and attach call.
 *
 * bus_print:
 * Just prints out the final (non-default) locators.
 */
int bus_scan(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;
	cfmatch_t mf;

#ifdef	DIAGNOSTIC
	if (cf->cf_fstate == FSTATE_STAR)
		panic("bus_scan: FSTATE_STAR");
#endif

	/* ca->ca_bustype set by parent */
	ca->ca_paddr  = cf->cf_loc[0];
	ca->ca_intpri = cf->cf_loc[1];
	ca->ca_intvec = -1;

	if ((ca->ca_bustype == BUS_VME16) ||
		(ca->ca_bustype == BUS_VME32))
	{
		ca->ca_intvec = cf->cf_loc[2];
	}

	/*
	 * Note that this allows the match function to save
	 * defaulted locators in the confargs that will be
	 * preserved for the related attach call.
	 */
	mf = cf->cf_attach->ca_match;
	if ((*mf)(parent, cf, ca) > 0) {
		config_attach(parent, cf, ca, bus_print);
	}
	return (0);
}

/*
 * Print out the confargs.  The parent name is non-NULL
 * when there was no match found by config_found().
 */
int
bus_print(args, name)
	void *args;
	const char *name;
{
	struct confargs *ca = args;

	if (name)
		printf("%s:", name);

	if (ca->ca_paddr != -1)
		printf(" addr 0x%x", ca->ca_paddr);
	if (ca->ca_intpri != -1)
		printf(" level %d", ca->ca_intpri);
	if (ca->ca_intvec != -1)
		printf(" vector 0x%x", ca->ca_intvec);

	return(UNCONF);
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

static struct device * find_dev_byname __P((char *));


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
struct device *
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
struct device *
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
struct device *
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
