/*	$NetBSD: autoconf.c,v 1.1 2001/04/06 15:05:55 fredette Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/promlib.h>

#include <sun2/sun2/machdep.h>

#ifdef	KGDB
#include <sys/kgdb.h>
#endif

/*
 * Do general device autoconfiguration,
 * then choose root device (etc.)
 * Called by machdep.c: cpu_startup()
 */
void
cpu_configure()
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

	/*
	 * Install handlers for our "soft" interrupts.
	 * There might be a better place to do this?
	 */
	softintr_init();

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

static int 	mainbus_match __P((struct device *, struct cfdata *, void *));
static void	mainbus_attach __P((struct device *, struct device *, void *));

struct cfattach mainbus_ca = {
	sizeof(struct device), mainbus_match, mainbus_attach
};

/*
 * Probe for the mainbus; always succeeds.
 */
static int
mainbus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	return 1;
}

/*
 * Do "direct" configuration for the bus types on mainbus.
 * This controls the order of autoconfig for important things
 * used early.  For example, idprom is used by Ether drivers.
 */
static void
mainbus_attach(parent, self, args)
	struct device *parent;
	struct device *self;
	void *args;
{
extern struct sun2_bus_dma_tag mainbus_dma_tag;
extern struct sun2_bus_space_tag mainbus_space_tag;

	struct confargs ca;
	const char *const *cpp;
	static const char *const special[] = {
		/* find these first */
		"obio",
		"obmem",
		NULL
	};

	printf("\n");

	ca.ca_bustag = &mainbus_space_tag;
	ca.ca_dmatag = &mainbus_dma_tag;

	/* Find all `early' mainbus buses */
	for (cpp = special; *cpp != NULL; cpp++) {
		ca.ca_name = *cpp;
		(void)config_found(self, &ca, NULL);
	}

	/* Find the remaining buses */
	ca.ca_name = NULL;
	(void) config_found(self, &ca, NULL);
}

/*
 * sun2_bus_search:
 * This function is passed to config_search() by the attach function
 * for each of the "bus" drivers (obio, obmem, mbmem, vme, ...).
 * The purpose of this function is to copy the "locators" into our
 * confargs structure, so child drivers may use the confargs both
 * as match parameters and as temporary storage for the defaulted
 * locator values determined in the child_match and preserved for
 * the child_attach function.  If the bus attach functions just
 * used config_found, then we would not have an opportunity to
 * setup the confargs for each child match and attach call.
 */
int sun2_bus_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *cap = aux;
	struct confargs ca;
	cfmatch_t mf;

	/* Check whether we're looking for a specifically named device */
	if (cap->ca_name != NULL && strcmp(cap->ca_name, cf->cf_driver->cd_name) != 0)
		return (0);

#ifdef	DIAGNOSTIC
	if (cf->cf_fstate == FSTATE_STAR)
		panic("bus_scan: FSTATE_STAR");
#endif

	/*
	 * Prepare to copy the locators into our confargs.
	 */
	ca = *cap;
	ca.ca_name = NULL;

	/*
	 * Avoid entries which are missing attach information that
	 * they need, or that have attach information that they
	 * cannot have.  The individual bus attach functions tell
	 * us this by initializing the locator fields in the attach
	 * args they provide us.
	 *
	 * At the same time we copy these values into the confargs
	 * will pass to the device's match and attach functions.
	 */
#ifdef	DIAGNOSTIC
#define BAD_LOCATOR(ca_loc, what) panic("sun2_bus_search: %s %s for: %s%d\n", \
				     cap-> ca_loc == LOCATOR_REQUIRED ? "missing" : "unexpected", \
				     what, cf->cf_driver->cd_name, cf->cf_unit)
#else
#define BAD_LOCATOR(ca_loc, what) return (0)
#endif
#define CHECK_LOCATOR(ca_loc, cf_loc, what) \
	if ((cap-> ca_loc == LOCATOR_FORBIDDEN && cf->cf_loc != -1) || \
	    (cap-> ca_loc == LOCATOR_REQUIRED && cf->cf_loc == -1)) \
		BAD_LOCATOR( ca_loc, what); \
	else \
		ca. ca_loc = cf->cf_loc
	ca.ca_paddr = LOCATOR_REQUIRED;
	CHECK_LOCATOR(ca_paddr, cf_paddr, "address");
	CHECK_LOCATOR(ca_intpri, cf_intpri, "ipl");
	CHECK_LOCATOR(ca_intvec, cf_intvec, "vector");

	/*
	 * Note that this allows the match function to save
	 * defaulted locators in the confargs that will be
	 * preserved for the related attach call.
	 * XXX - This is a hack...
	 */
	mf = cf->cf_attach->ca_match;
	if ((*mf)(parent, cf, &ca) > 0) {
		config_attach(parent, cf, &ca, sun2_bus_print);
	}
	return (0);
}

/*
 * sun2_bus_print:
 * Just print out the final (non-default) locators.
 * The parent name is non-NULL when there was no match
 * found by config_found().
 */
int
sun2_bus_print(args, name)
	void *args;
	const char *name;
{
	struct confargs *ca = args;

	if (name)
		printf("%s:", name);

	if (ca->ca_paddr != -1)
		printf(" addr 0x%x", (unsigned int) ca->ca_paddr);
	if (ca->ca_intpri != -1)
		printf(" ipl %d", ca->ca_intpri);
	if (ca->ca_intvec != -1)
		printf(" vect 0x%x", ca->ca_intvec);

	return(UNCONF);
}

/****************************************************************/

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
 * This converts one hex number to an integer, and returns
 * an updated string pointer.
 */
static const char *str2hex __P((const char *, int *));
static const char *
str2hex(p, _val)
	const char *p;
	int *_val;
{
	int val;
	int c;
	
	for(val = 0;; val = (val << 4) + c, p++) {
		c = *((unsigned char *) p);
		if (c >= 'a') c-= ('a' + 10);
		else if (c >= 'A') c -= ('A' + 10);
		else if (c >= '0') c -= '0';
		if (c < 0 || c > 15) break;
	}
	*_val = val;
	return (p);
}

/*
 * Choose root and swap devices.
 */
void
cpu_rootconf()
{
	struct prom_n2f *nf;
	struct device *boot_device;
	int boot_partition;
	char *devname;
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
		(void) str2hex(++prompath, &prom_part);

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
		if (boot_device->dv_class == DV_DISK) {
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
	struct scsipi_link *sc_link;
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

/*
 * Misc. helpers used by driver match/attach functions.
 */

/* from hp300: badbaddr() */
int
peek_byte(addr)
	register caddr_t addr;
{
	label_t 	faultbuf;
	register int x;

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
	register caddr_t addr;
{
	label_t		faultbuf;
	register int x;

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
	register caddr_t addr;
{
	label_t		faultbuf;
	register int x;

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
