/*	$NetBSD: autoconf.c,v 1.61 2003/07/15 03:36:16 lukem Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.61 2003/07/15 03:36:16 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>

#include "scsibus.h"

#if NSCSIBUS > 0
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#endif

#include <machine/autoconf.h>
#include <machine/mon.h>

#include <sun3/sun3/machdep.h>


/* Make sure the config is OK. */
#if (defined(_SUN3_) + defined(_SUN3X_)) != 1
#error "Must have exactly one of: SUN3 and SUN3X options"
#endif

/*
 * Do general device autoconfiguration,
 * then choose root device (etc.)
 * Called by machdep.c: cpu_startup()
 */
void
cpu_configure()
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
}

/*
 * bus_scan:
 * This function is passed to config_search() by the attach function
 * for each of the "bus" drivers (obctl, obio, obmem, vme, ...).
 * The purpose of this function is to copy the "locators" into our
 * confargs structure, so child drivers may use the confargs both
 * as match parameters and as temporary storage for the defaulted
 * locator values determined in the child_match and preserved for
 * the child_attach function.  If the bus attach functions just
 * used config_found, then we would not have an opportunity to
 * setup the confargs for each child match and attach call.
 */
int bus_scan(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

#ifdef	DIAGNOSTIC
	if (cf->cf_fstate == FSTATE_STAR)
		panic("bus_scan: FSTATE_STAR");
#endif

	/*
	 * Copy the locators into our confargs.
	 * Our parent set ca->ca_bustype already.
	 */
	ca->ca_paddr  = cf->cf_paddr;
	ca->ca_intpri = cf->cf_intpri;
	ca->ca_intvec = cf->cf_intvec;

	/*
	 * Note that this allows the match function to save
	 * defaulted locators in the confargs that will be
	 * preserved for the related attach call.
	 * XXX - This is a hack...
	 */
	if (config_match(parent, cf, ca) > 0) {
		config_attach(parent, cf, ca, bus_print);
	}
	return (0);
}

/*
 * bus_print:
 * Just print out the final (non-default) locators.
 * The parent name is non-NULL when there was no match
 * found by config_found().
 */
int
bus_print(args, name)
	void *args;
	const char *name;
{
	struct confargs *ca = args;

	if (name)
		aprint_normal("%s:", name);

	if (ca->ca_paddr != -1)
		aprint_normal(" addr 0x%x", ca->ca_paddr);
	if (ca->ca_intpri != -1)
		aprint_normal(" ipl %d", ca->ca_intpri);
	if (ca->ca_intvec != -1)
		aprint_normal(" vect 0x%x", ca->ca_intvec);

	return(UNCONF);
}

/****************************************************************/

/* This takes the args: name, ctlr, unit */
typedef struct device * (*findfunc_t) __P((char *, int, int));

static struct device * find_dev_byname __P((char *));
static struct device * net_find  __P((char *, int, int));
#if NSCSIBUS > 0
static struct device * scsi_find __P((char *, int, int));
#endif
static struct device * xx_find   __P((char *, int, int));

struct prom_n2f {
	const char name[4];
	findfunc_t func;
};
static struct prom_n2f prom_dev_table[] = {
	{ "ie",		net_find },
	{ "le",		net_find },
#if NSCSIBUS > 0
	{ "sd",		scsi_find },
#endif
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
	struct bootparam *bp;
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
	setroot(boot_device, boot_partition);
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

#if NSCSIBUS > 0
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
	struct scsipi_periph *periph;
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
	periph = scsipi_lookup_periph(sbsc->sc_channel, target, lun);
	if (periph == NULL)
		return (NULL);

	return (periph->periph_dev);
}
#endif	/* NSCSIBUS > 0 */

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

/*
 * Misc. helpers used by driver match/attach functions.
 */

/*
 * Read addr with size len (1,2,4) into val.
 * If this generates a bus error, return -1
 *
 *	Create a temporary mapping,
 *	Try the access using peek_*
 *	Clean up temp. mapping
 */
int
bus_peek(bustype, pa, sz)
	int bustype, pa, sz;
{
	caddr_t va;
	int rv;

	va = bus_tmapin(bustype, pa);
	switch (sz) {
	case 1:
		rv = peek_byte(va);
		break;
	case 2:
		rv = peek_word(va);
		break;
	case 4:
		rv = peek_long(va);
		break;
	default:
		printf(" bus_peek: invalid size=%d\n", sz);
		rv = -1;
	}
	bus_tmapout(va);

	return (rv);
}

/* from hp300: badbaddr() */
int
peek_byte(addr)
	caddr_t addr;
{
	label_t faultbuf;
	int x;

	nofault = &faultbuf;
	if (setjmp(&faultbuf))
		x = -1;
	else
		x = *(volatile u_char *)addr;

	nofault = NULL;
	return(x);
}

int
peek_word(addr)
	caddr_t addr;
{
	label_t faultbuf;
	int x;

	nofault = &faultbuf;
	if (setjmp(&faultbuf))
		x = -1;
	else
		x = *(volatile u_short *)addr;

	nofault = NULL;
	return(x);
}

int
peek_long(addr)
	caddr_t addr;
{
	label_t faultbuf;
	int x;

	nofault = &faultbuf;
	if (setjmp(&faultbuf))
		x = -1;
	else {
		x = *(volatile int *)addr;
		if (x == -1) {
			printf("peek_long: uh-oh, actually read -1!\n");
			x &= 0x7FFFffff; /* XXX */
		}
	}

	nofault = NULL;
	return(x);
}
