/*	$NetBSD: autoconf.c,v 1.20.28.1 2008/01/09 01:49:16 matt Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, Gordon W. Ross, and Matthew Fredette.
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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.20.28.1 2008/01/09 01:49:16 matt Exp $");

#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include "locators.h"

#include "scsibus.h"

#if NSCSIBUS > 0
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#endif /* NSCSIBUS > 0 */

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/promlib.h>

#ifdef	KGDB
#include <sys/kgdb.h>
#endif

/*
 * Do general device autoconfiguration,
 * then choose root device (etc.)
 * Called by sys/kern/subr_autoconf.c: configure()
 */
void 
cpu_configure(void)
{

	/*
	 * Consider stopping for a debugger before
	 * autoconfiguration.
	 */
	if (boothowto & RB_KDB) {
#ifdef KGDB
		/* XXX - Ask on console for kgdb_dev? */
		/* Note: this will just return if kgdb_dev==NODEV */
		kgdb_connect(1);
#else	/* KGDB */
		/* Either DDB or no debugger (just PROM). */
		Debugger();
#endif	/* KGDB */
	}

	/* General device autoconfiguration. */
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("%s: mainbus not found", __func__);

	/*
	 * Now that device autoconfiguration is finished,
	 * we can safely enable interrupts.
	 */
	printf("enabling interrupts\n");
	(void)spl0();
}

static int 	mainbus_match(struct device *, struct cfdata *, void *);
static void	mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);

/*
 * Probe for the mainbus; always succeeds.
 */
static int 
mainbus_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return 1;
}

/*
 * Do "direct" configuration for the bus types on mainbus.
 * This controls the order of autoconfig for important things
 * used early.  For example, idprom is used by Ether drivers.
 */
static void 
mainbus_attach(struct device *parent, struct device *self, void *args)
{
	struct mainbus_attach_args ma;
	const char *const *cpp;
	static const char *const special[] = {
		/* find these first */
		"obio",
		"obmem",
		NULL
	};

	printf("\n");

	ma.ma_bustag = &mainbus_space_tag;
	ma.ma_dmatag = &mainbus_dma_tag;
	ma.ma_paddr = LOCATOR_FORBIDDEN;
	ma.ma_pri = LOCATOR_FORBIDDEN;

	/* Find all `early' mainbus buses */
	for (cpp = special; *cpp != NULL; cpp++) {
		ma.ma_name = *cpp;
		(void)config_found(self, &ma, NULL);
	}

	/* Find the remaining buses */
	ma.ma_name = NULL;
	(void)config_found(self, &ma, NULL);

	/* Lastly, find the PROM console */
	ma.ma_name = "pcons";
	(void) config_found(self, &ma, NULL);
}

/*
 * sun68k_bus_search:
 * This function is passed to config_search_ia() by the attach function
 * for each of the "bus" drivers (obio, obmem, mbmem, vme, ...).
 * The purpose of this function is to copy the "locators" into our
 * _attach_args structure, so child drivers may use the _attach_args both
 * as match parameters and as temporary storage for the defaulted
 * locator values determined in the child_match and preserved for
 * the child_attach function.  If the bus attach functions just
 * used config_found, then we would not have an opportunity to
 * setup the _attach_args for each child match and attach call.
 */
int 
sun68k_bus_search(struct device *parent, struct cfdata *cf, const int *ldesc,
    void *aux)
{
	struct mainbus_attach_args *map = aux;
	struct mainbus_attach_args ma;

	/* Check whether we're looking for a specifically named device */
	if (map->ma_name != NULL && strcmp(map->ma_name, cf->cf_name) != 0)
		return 0;

#ifdef	DIAGNOSTIC
	if (cf->cf_fstate == FSTATE_STAR)
		panic("%s: FSTATE_STAR", __func__);
#endif

	/*
	 * Prepare to copy the locators into our _attach_args.
	 */
	ma = *map;
	ma.ma_name = NULL;

	/*
	 * Avoid entries which are missing attach information that
	 * they need, or that have attach information that they
	 * cannot have.  The individual bus attach functions tell
	 * us this by initializing the locator fields in the attach
	 * args they provide us.
	 *
	 * At the same time we copy these values into the _attach_args
	 * will pass to the device's match and attach functions.
	 */
#ifdef	DIAGNOSTIC
#define BAD_LOCATOR(ma_loc, what) \
	panic("%s: %s %s for: %s%d", __func__ \
		map->ma_loc == LOCATOR_REQUIRED ? "missing" : "unexpected", \
		what, cf->cf_name, cf->cf_unit)
#else
#define BAD_LOCATOR(ma_loc, what) return 0
#endif

#define CHECK_LOCATOR(ma_loc, cf_loc, what) \
	if ((map->ma_loc == LOCATOR_FORBIDDEN && cf->cf_loc != -1) || \
	    (map->ma_loc == LOCATOR_REQUIRED && cf->cf_loc == -1)) \
		BAD_LOCATOR( ma_loc, what); \
	else \
		ma.ma_loc = cf->cf_loc

	CHECK_LOCATOR(ma_paddr, cf_loc[MBIOCF_ADDR], "address");
	CHECK_LOCATOR(ma_pri, cf_loc[MBIOCF_IPL], "ipl");

	/*
	 * Note that this allows the match function to save
	 * defaulted locators in the _attach_args that will be
	 * preserved for the related attach call.
	 * XXX - This is a hack...
	 */
	if (config_match(parent, cf, &ma) > 0) {
		config_attach(parent, cf, &ma, sun68k_bus_print);
	}
	return 0;
}

/*
 * sun68k_bus_print:
 * Just print out the final (non-default) locators.
 * The parent name is non-NULL when there was no match
 * found by config_found().
 */
int 
sun68k_bus_print(void *args, const char *name)
{
	struct mainbus_attach_args *ma = args;

	if (name)
		aprint_normal("%s:", name);

	if (ma->ma_paddr != -1)
		aprint_normal(" addr 0x%x", (unsigned int) ma->ma_paddr);
	if (ma->ma_pri != -1)
		aprint_normal(" ipl %d", ma->ma_pri);

	return UNCONF;
}

/****************************************************************/

/* This takes the args: name, ctlr, unit */
typedef struct device * (*findfunc_t)(char *, int, int);

static struct device * find_dev_byname(char *);
static struct device * net_find (char *, int, int);
#if NSCSIBUS > 0
static struct device * scsi_find(char *, int, int);
#endif /* NSCSIBUS > 0 */
static struct device * xx_find  (char *, int, int);

struct prom_n2f {
	const char name[4];
	findfunc_t func;
};
static struct prom_n2f prom_dev_table[] = {
	{ "ie",		net_find },
	{ "ec",		net_find },
	{ "le",		net_find },
#if NSCSIBUS > 0
	{ "sd",		scsi_find },
#endif /* NSCSIBUS > 0 */
	{ "xy",		xx_find },
	{ "xd",		xx_find },
	{ "",		0 },
};

/*
 * This converts one hex number to an integer, and returns
 * an updated string pointer.
 */
static const char *str2hex(const char *, int *);
static const char *
str2hex(const char *p, int *_val)
{
	int val;
	int c;
	
	for (val = 0;; val = (val << 4) + c, p++) {
		c = *((const unsigned char *) p);
		if (c >= 'a') c-= ('a' + 10);
		else if (c >= 'A') c -= ('A' + 10);
		else if (c >= '0') c -= '0';
		if (c < 0 || c > 15) break;
	}
	*_val = val;
	return p;
}

/*
 * Choose root and swap devices.
 */
void 
cpu_rootconf(void)
{
	struct prom_n2f *nf;
	struct device *boot_device;
	int boot_partition;
	const char *devname;
	findfunc_t find;
	char promname[4];
	char partname[4];
	const char *prompath;
	int prom_ctlr, prom_unit, prom_part;

	/* Get the PROM boot path and take it apart. */
	prompath = prom_getbootpath();
	if (prompath == NULL) prompath = "zz(0,0,0)";
	promname[0] = *(prompath++);
	promname[1] = *(prompath++);
	promname[2] = '\0';
	prom_ctlr = prom_unit = prom_part = 0;
	if (*prompath == '(' &&
	    *(prompath = str2hex(++prompath, &prom_ctlr)) == ',' &&
	    *(prompath = str2hex(++prompath, &prom_unit)) == ',') 
		(void)str2hex(++prompath, &prom_part);

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
		boot_device = (*find)(promname, prom_ctlr, prom_unit);
	if (boot_device) {
		devname = boot_device->dv_xname;
		if (device_class(boot_device) == DV_DISK) {
			boot_partition = prom_part & 7;
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
net_find(char *name, int ctlr, int unit)
{
	char tname[16];

	sprintf(tname, "%s%d", name, ctlr);
	return find_dev_byname(tname);
}

#if NSCSIBUS > 0
/*
 * SCSI device:  The controller number corresponds to the
 * scsibus number, and the unit number is (targ*8 + LUN).
 */
static struct device *
scsi_find(char *name, int ctlr, int unit)
{
	struct device *scsibus;
	struct scsibus_softc *sbsc;
	struct scsipi_periph *periph;
	int target, lun;
	char tname[16];

	sprintf(tname, "scsibus%d", ctlr);
	scsibus = find_dev_byname(tname);
	if (scsibus == NULL)
		return NULL;

	/* Compute SCSI target/LUN from PROM unit. */
	target = prom_sd_target((unit >> 3) & 7);
	lun = unit & 7;

	/* Find the device at this target/LUN */
	sbsc = (struct scsibus_softc *)scsibus;
	periph = scsipi_lookup_periph(sbsc->sc_channel, target, lun);
	if (periph == NULL)
		return NULL;

	return periph->periph_dev;
}
#endif /* NSCSIBUS > 0 */

/*
 * Xylogics SMD disk: (xy, xd)
 * Assume wired-in unit numbers for now...
 */
static struct device *
xx_find(char *name, int ctlr, int unit)
{
	int diskunit;
	char tname[16];

	diskunit = (ctlr * 2) + unit;
	sprintf(tname, "%s%d", name, diskunit);
	return find_dev_byname(tname);
}

/*
 * Given a device name, find its struct device
 * XXX - Move this to some common file?
 */
static struct device *
find_dev_byname(char *name)
{
	struct device *dv;

	for (dv = TAILQ_FIRST(&alldevs); dv != NULL;
	    dv = TAILQ_NEXT(dv, dv_list)) {
		if (!strcmp(dv->dv_xname, name)) {
			return dv;
		}
	}
	return NULL;
}
