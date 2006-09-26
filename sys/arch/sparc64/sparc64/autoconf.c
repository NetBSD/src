/*	$NetBSD: autoconf.c,v 1.122 2006/09/26 19:06:46 martin Exp $ */

/*
 * Copyright (c) 1996
 *    The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by Harvard University.
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
 *	@(#)autoconf.c	8.4 (Berkeley) 10/1/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.122 2006/09/26 19:06:46 martin Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/queue.h>
#include <sys/msgbuf.h>
#include <sys/boot_flag.h>
#include <sys/ksyms.h>
#include <sys/kauth.h>
#include <prop/proplib.h>

#include <net/if.h>

#include <dev/cons.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/openfirm.h>
#include <machine/sparc64.h>
#include <machine/cpu.h>
#include <machine/pmap.h>
#include <machine/bootinfo.h>
#include <sparc64/sparc64/timerreg.h>

#include <dev/ata/atavar.h>
#include <dev/pci/pcivar.h>
#include <dev/sbus/sbusvar.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

#ifdef RASTERCONSOLE
#error options RASTERCONSOLE is obsolete for sparc64 - remove it from your config file
#endif

#include "ksyms.h"

struct evcnt intr_evcnts[] = {
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intr", "spur"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intr", "lev1"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intr", "lev2"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intr", "lev3"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intr", "lev4"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intr", "lev5"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intr", "lev6"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intr", "lev7"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intr", "lev8"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intr", "lev9"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intr", "clock"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intr", "lev11"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intr", "lev12"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intr", "lev13"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intr", "prof"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "intr",  "lev15")
};

void *bootinfo = 0;

#ifdef KGDB
extern	int kgdb_debug_panic;
#endif

#define	OFPATHLEN	128
#define	OFNODEKEY	"OFpnode"

char	machine_model[100];
char	ofbootpath[OFPATHLEN], *ofboottarget, *ofbootpartition;
int	ofbootpackage;

static	int mbprint(void *, const char *);
int	mainbus_match(struct device *, struct cfdata *, void *);
static	void mainbus_attach(struct device *, struct device *, void *);
static  void get_ncpus(void);
static	void get_bootpath_from_prom(void);

/*
 * Kernel 4MB mappings.
 */
struct tlb_entry *kernel_tlbs;
int kernel_tlb_slots;

/* Global interrupt mappings for all device types.  Match against the OBP
 * 'device_type' property. 
 */
struct intrmap intrmap[] = {
	{ "block",	PIL_FD },	/* Floppy disk */
	{ "serial",	PIL_SER },	/* zs */
	{ "scsi",	PIL_SCSI },
	{ "scsi-2",	PIL_SCSI },
	{ "network",	PIL_NET },
	{ "display",	PIL_VIDEO },
	{ "audio",	PIL_AUD },
	{ "ide",	PIL_SCSI },
/* The following devices don't have device types: */
	{ "SUNW,CS4231",	PIL_AUD },
	{ NULL,		0 }
};

#ifdef DEBUG
#define ACDB_BOOTDEV	0x1
#define	ACDB_PROBE	0x2
int autoconf_debug = 0x0;
#define DPRINTF(l, s)   do { if (autoconf_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

static void
get_ncpus()
{
	int node;
	char sbuf[32];

	node = findroot();

	sparc_ncpus = 0;
	for (node = OF_child(node); node; node = OF_peer(node)) {
		if (OF_getprop(node, "device_type", sbuf, sizeof(sbuf)) <= 0)
			continue;
		if (strcmp(sbuf, "cpu") != 0)
			continue;
		sparc_ncpus++;
	}
}

/*
 * lookup_bootinfo:
 * Look up information in bootinfo of boot loader.
 */
void *
lookup_bootinfo(int type)
{
	struct btinfo_common *bt;
	char *help = bootinfo;

	/* Check for a bootinfo record first. */
	if (help == NULL)
		return (NULL);

	do {
		bt = (struct btinfo_common *)help;
		if (bt->type == type)
			return ((void *)help);
		help += bt->next;
	} while (bt->next != 0 &&
		(size_t)help < (size_t)bootinfo + BOOTINFO_SIZE);

	return (NULL);
}

/*
 * locore.s code calls bootstrap() just before calling main().
 *
 * What we try to do is as follows:
 * - Initialize PROM and the console
 * - Read in part of information provided by a bootloader and find out
 *   kernel load and end addresses
 * - Initialize ksyms
 * - Find out number of active CPUs
 * - Finalize the bootstrap by calling pmap_bootstrap() 
 *
 * We will try to run out of the prom until we get out of pmap_bootstrap().
 */
void
bootstrap(void *o0, void *bootargs, void *bootsize, void *o3, void *ofw)
{
	void *bi;
	long bmagic;

	struct btinfo_symtab *bi_sym;
	struct btinfo_count *bi_count;
	struct btinfo_kernend *bi_kend;
	struct btinfo_tlb *bi_tlb;

	extern void *romtba;
	extern void* get_romtba(void);
	extern void  OF_val2sym32(void *);
	extern void OF_sym2val32(void *);

	/* Save OpenFrimware entry point */
	romp   = ofw;
	romtba = get_romtba();

	prom_init();

	/* Initialize the PROM console so printf will not panic */
	(*cn_tab->cn_init)(cn_tab);

	printf("sparc64_init(%p, %p, %p, %p, %p)\n", o0, bootargs, bootsize,
			o3, ofw);

	/* Extract bootinfo pointer */
	if ((long)bootsize >= (4 * sizeof(uint64_t))) {
		/* Loaded by 64-bit bootloader */
		bi = (void*)(u_long)(((uint64_t*)bootargs)[3]);
		bmagic = (long)(((uint64_t*)bootargs)[0]);
	} else if ((long)bootsize >= (4 * sizeof(uint32_t))) {
		/* Loaded by 32-bit bootloader */
		bi = (void*)(u_long)(((uint32_t*)bootargs)[3]);
		bmagic = (long)(((uint32_t*)bootargs)[0]);
	} else {
		printf("Bad bootinfo size.\n"
				"This kernel requires NetBSD boot loader.\n");
		panic("sparc64_init.");
	}

	printf("sparc64_init: bmagic=%lx, bi=%p\n", bmagic, bi);

	/* Read in the information provided by NetBSD boot loader */
	if (SPARC_MACHINE_OPENFIRMWARE != bmagic) {
		printf("No bootinfo information.\n"
				"This kernel requires NetBSD boot loader.\n");
		panic("sparc64_init.");
	}

	bootinfo = (void*)(u_long)((uint64_t*)bi)[1];
	LOOKUP_BOOTINFO(bi_kend, BTINFO_KERNEND);

	if (bi_kend->addr == (vaddr_t)0) {
		panic("Kernel end address is not found in bootinfo.\n");
	}

#if NKSYMS || defined(DDB) || defined(LKM)
	LOOKUP_BOOTINFO(bi_sym, BTINFO_SYMTAB);
	ksyms_init(bi_sym->nsym, (int *)(u_long)bi_sym->ssym,
			(int *)(u_long)bi_sym->esym);
#ifdef DDB
#ifdef __arch64__
	/* This can only be installed on an 64-bit system cause otherwise our stack is screwed */
	OF_set_symbol_lookup(OF_sym2val, OF_val2sym);
#else
	OF_set_symbol_lookup(OF_sym2val32, OF_val2sym32);
#endif
#endif
#endif

	LOOKUP_BOOTINFO(bi_count, BTINFO_DTLB_SLOTS);
	kernel_tlb_slots = bi_count->count;
	LOOKUP_BOOTINFO(bi_tlb, BTINFO_DTLB);
	kernel_tlbs = &bi_tlb->tlb[0];

	get_ncpus();
	pmap_bootstrap(KERNBASE, bi_kend->addr);
}

/*
 * get_bootpath_from_prom()
 * fetch the OF settings to identify our boot device during autoconfiguration
 */

static void
get_bootpath_from_prom()
{
	char sbuf[OFPATHLEN], *cp;
	int chosen;

	/*
	 * Grab boot path from PROM
	 */
	if ((chosen = OF_finddevice("/chosen")) == -1 ||
	    OF_getprop(chosen, "bootpath", sbuf, sizeof(sbuf)) < 0)
		return;

	strcpy(ofbootpath, sbuf);
	ofbootpackage = prom_finddevice(ofbootpath);
	/*
	 * Strip partition or boot protocol
	 */
	cp = strrchr(ofbootpath, ':');
	if (cp) {
		*cp = '\0';
		ofbootpartition = cp+1;
	}
	cp = strrchr(ofbootpath, '@');
	if (cp)
		ofboottarget = cp;

	printf("bootpath: %s\n", sbuf);
	DPRINTF(ACDB_BOOTDEV, ("bootpath phandle: 0x%x\n", ofbootpackage));
	if (ofboottarget)
		DPRINTF(ACDB_BOOTDEV, ("boot target: %s\n", ofboottarget));
	if (ofbootpartition)
		DPRINTF(ACDB_BOOTDEV, ("boot partition: %s\n",
		    ofbootpartition));

	/* Setup pointer to boot flags */
	if (OF_getprop(chosen, "bootargs", sbuf, sizeof(sbuf)) == -1)
		return;

	cp = sbuf;

	/* Find start of boot flags */
	while (*cp) {
		while(*cp == ' ' || *cp == '\t') cp++;
		if (*cp == '-' || *cp == '\0')
			break;
		while(*cp != ' ' && *cp != '\t' && *cp != '\0') cp++;
		
	}
	if (*cp != '-')
		return;

	for (;*++cp;) {
		int fl;

		fl = 0;
		BOOT_FLAG(*cp, fl);
		if (!fl) {
			printf("unknown option `%c'\n", *cp);
			continue;
		}
		boothowto |= fl;

		/* specialties */
		if (*cp == 'd') {
#if defined(KGDB)
			kgdb_debug_panic = 1;
			kgdb_connect(1);
#elif defined(DDB)
			Debugger();
#else
			printf("kernel has no debugger\n");
#endif
		} else if (*cp == 't') {
			/* turn on traptrace w/o breaking into kdb */
			extern int trap_trace_dis;

			trap_trace_dis = 0;
		}
	}
}

/*
 * Determine mass storage and memory configuration for a machine.
 * We get the PROM's root device and make sure we understand it, then
 * attach it as `mainbus0'.  We also set up to handle the PROM `sync'
 * command.
 */
void
cpu_configure()
{

	/* fetch boot device settings */
	get_bootpath_from_prom();

	/* block clock interrupts and anything below */
	splclock();
	/* Enable device interrupts */
        setpstate(getpstate()|PSTATE_IE);

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("mainbus not configured");

	/* Enable device interrupts */
        setpstate(getpstate()|PSTATE_IE);

	(void)spl0();
}

static struct vnode *
opendisk(struct device *dv)
{
	int bmajor, bminor;
	struct vnode *tmpvn;
	int error;
	dev_t dev;
	
	/*
	 * Lookup major number for disk block device.
	 */
	bmajor = devsw_name2blk(dv->dv_xname, NULL, 0);
	if (bmajor == -1)
		return NULL;
	
	bminor = minor(device_unit(dv));
	/*
	 * Fake a temporary vnode for the disk, open it, and read
	 * and hash the sectors.
	 */
	dev = device_is_a(dv, "dk") ? makedev(bmajor, bminor) :
	    MAKEDISKDEV(bmajor, bminor, RAW_PART);
	if (bdevvp(dev, &tmpvn))
		panic("%s: can't alloc vnode for %s", __func__, dv->dv_xname);
	error = VOP_OPEN(tmpvn, FREAD, NOCRED, 0);
	if (error) {
#ifndef DEBUG
		/*
		 * Ignore errors caused by missing device, partition,
		 * or medium.
		 */
		if (error != ENXIO && error != ENODEV)
#endif
			printf("%s: can't open dev %s (%d)\n",
			    __func__, dv->dv_xname, error);
		vput(tmpvn);
		return NULL;
	}

	return tmpvn;
}

void
cpu_rootconf()
{
	struct dkwedge_list wl;
	struct dkwedge_info *wi;
	struct vnode *vn;
	char diskname[16];
	int i, error;

	if (booted_device == NULL) {
		printf("FATAL: boot device not found, check your firmware "
		    "settings!\n");
		setroot(NULL, 0);
		return;
	}

	if ((vn = opendisk(booted_device)) == NULL)
		goto nowedge;

	wl.dkwl_bufsize = sizeof(*wi) * 16;
	wl.dkwl_buf = wi = malloc(wl.dkwl_bufsize, M_TEMP, M_WAITOK);
	error = VOP_IOCTL(vn, DIOCLWEDGES, &wl, FREAD, NOCRED, 0);
	vput(vn);
	if (error)
		goto nowedge2;

	snprintf(diskname, sizeof(diskname), "%s%c", booted_device->dv_xname, 
	    booted_partition + 'a');

	for (i = 0; i < wl.dkwl_ncopied; i++) {
		if (strcmp(wi[i].dkw_wname, diskname) == 0)
			break;
	}
	if (i == wl.dkwl_ncopied)
		goto nowedge2;

	dkwedge_set_bootwedge(booted_device, wi[i].dkw_offset, wi[i].dkw_size);
	free(wi, M_TEMP);
	setroot(booted_wedge, 0);
	return;

nowedge2:
	free(wi, M_TEMP);
nowedge:
	setroot(booted_device, booted_partition);
}

char *
clockfreq(long freq)
{
	char *p;
	static char sbuf[10];

	freq /= 1000;
	sprintf(sbuf, "%ld", freq / 1000);
	freq %= 1000;
	if (freq) {
		freq += 1000;	/* now in 1000..1999 */
		p = sbuf + strlen(sbuf);
		sprintf(p, "%ld", freq);
		*p = '.';	/* now sbuf = %d.%3d */
	}
	return (sbuf);
}

/* ARGSUSED */
static int
mbprint(void *aux, const char *name)
{
	struct mainbus_attach_args *ma = aux;

	if (name)
		aprint_normal("%s at %s", ma->ma_name, name);
	if (ma->ma_address)
		aprint_normal(" addr 0x%08lx", (u_long)ma->ma_address[0]);
	if (ma->ma_pri)
		aprint_normal(" ipl %d", ma->ma_pri);
	return (UNCONF);
}

int
mainbus_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (1);
}

/*
 * Attach the mainbus.
 *
 * Our main job is to attach the CPU (the root node we got in configure())
 * and iterate down the list of `mainbus devices' (children of that node).
 * We also record the `node id' of the default frame buffer, if any.
 */
static void
mainbus_attach(struct device *parent, struct device *dev, void *aux)
{
extern struct sparc_bus_dma_tag mainbus_dma_tag;
extern struct sparc_bus_space_tag mainbus_space_tag;

	struct mainbus_attach_args ma;
	char sbuf[32];
	const char *const *ssp, *sp = NULL;
	int node0, node, rv, i;

	static const char *const openboot_special[] = {
		/* ignore these (end with NULL) */
		/*
		 * These are _root_ devices to ignore. Others must be handled
		 * elsewhere.
		 */
		"virtual-memory",
		"aliases",
		"memory",
		"openprom",
		"options",
		"packages",
		"chosen",
		NULL
	};

	OF_getprop(findroot(), "name", machine_model, sizeof machine_model);
	prom_getidprom();
	printf(": %s: hostid %lx\n", machine_model, hostid);

	/*
	 * Locate and configure the ``early'' devices.  These must be
	 * configured before we can do the rest.  For instance, the
	 * EEPROM contains the Ethernet address for the LANCE chip.
	 * If the device cannot be located or configured, panic.
	 */
	if (sparc_ncpus == 0)
		panic("None of the CPUs found");

	/*
	 * Init static interrupt eventcounters
	 */
	for (i = 0; i < sizeof(intr_evcnts)/sizeof(intr_evcnts[0]); i++)
		evcnt_attach_static(&intr_evcnts[i]);

	node = findroot();

	/* first early device to be configured is the CPU */
	for (node = OF_child(node); node; node = OF_peer(node)) {
		if (OF_getprop(node, "device_type", sbuf, sizeof(sbuf)) <= 0)
			continue;
		if (strcmp(sbuf, "cpu") != 0)
			continue;
		memset(&ma, 0, sizeof(ma));
		ma.ma_bustag = &mainbus_space_tag;
		ma.ma_dmatag = &mainbus_dma_tag;
		ma.ma_node = node;
		ma.ma_name = "cpu";
		config_found(dev, &ma, mbprint);
	}

	node = findroot();	/* re-init root node */

	/* Find the "options" node */
	node0 = OF_child(node);

	/*
	 * Configure the devices, in PROM order.  Skip
	 * PROM entries that are not for devices, or which must be
	 * done before we get here.
	 */
	for (node = node0; node; node = OF_peer(node)) {
		int portid;

		DPRINTF(ACDB_PROBE, ("Node: %x", node));
		if ((OF_getprop(node, "device_type", sbuf, sizeof(sbuf)) > 0) &&
		    strcmp(sbuf, "cpu") == 0)
			continue;
		OF_getprop(node, "name", sbuf, sizeof(sbuf));
		DPRINTF(ACDB_PROBE, (" name %s\n", sbuf));
		for (ssp = openboot_special; (sp = *ssp) != NULL; ssp++)
			if (strcmp(sbuf, sp) == 0)
				break;
		if (sp != NULL)
			continue; /* an "early" device already configured */

		memset(&ma, 0, sizeof ma);
		ma.ma_bustag = &mainbus_space_tag;
		ma.ma_dmatag = &mainbus_dma_tag;
		ma.ma_name = sbuf;
		ma.ma_node = node;
		if (OF_getprop(node, "upa-portid", &portid, sizeof(portid)) !=
		    sizeof(portid)) 
			portid = -1;
		ma.ma_upaid = portid;

		if (prom_getprop(node, "reg", sizeof(*ma.ma_reg), 
				 &ma.ma_nreg, &ma.ma_reg) != 0)
			continue;
#ifdef DEBUG
		if (autoconf_debug & ACDB_PROBE) {
			if (ma.ma_nreg)
				printf(" reg %08lx.%08lx\n",
					(long)ma.ma_reg->ur_paddr, 
					(long)ma.ma_reg->ur_len);
			else
				printf(" no reg\n");
		}
#endif
		rv = prom_getprop(node, "interrupts", sizeof(*ma.ma_interrupts),
			&ma.ma_ninterrupts, &ma.ma_interrupts);
		if (rv != 0 && rv != ENOENT) {
			free(ma.ma_reg, M_DEVBUF);
			continue;
		}
#ifdef DEBUG
		if (autoconf_debug & ACDB_PROBE) {
			if (ma.ma_interrupts)
				printf(" interrupts %08x\n", *ma.ma_interrupts);
			else
				printf(" no interrupts\n");
		}
#endif
		rv = prom_getprop(node, "address", sizeof(*ma.ma_address), 
			&ma.ma_naddress, &ma.ma_address);
		if (rv != 0 && rv != ENOENT) {
			free(ma.ma_reg, M_DEVBUF);
			if (ma.ma_ninterrupts)
				free(ma.ma_interrupts, M_DEVBUF);
			continue;
		}
#ifdef DEBUG
		if (autoconf_debug & ACDB_PROBE) {
			if (ma.ma_naddress)
				printf(" address %08x\n", *ma.ma_address);
			else
				printf(" no address\n");
		}
#endif
		(void) config_found(dev, (void *)&ma, mbprint);
		free(ma.ma_reg, M_DEVBUF);
		if (ma.ma_ninterrupts)
			free(ma.ma_interrupts, M_DEVBUF);
		if (ma.ma_naddress)
			free(ma.ma_address, M_DEVBUF);
	}
	/* Try to attach PROM console */
	memset(&ma, 0, sizeof ma);
	ma.ma_name = "pcons";
	(void) config_found(dev, (void *)&ma, mbprint);
}

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);


/*
 * Try to figure out where the PROM stores the cursor row & column
 * variables.  Returns nonzero on error.
 */
int
romgetcursoraddr(int **rowp, int **colp)
{
	cell_t row = 0UL, col = 0UL;

	OF_interpret("stdout @ is my-self addr line# addr column# ", 0, 2,
		&col, &row);
	/*
	 * We are running on a 64-bit machine, so these things point to
	 * 64-bit values.  To convert them to pointers to integers, add
	 * 4 to the address.
	 */
	*rowp = (int *)(intptr_t)(row+4);
	*colp = (int *)(intptr_t)(col+4);
	return (row == 0UL || col == 0UL);
}

/*
 * find a device matching "name" and unit number
 */
struct device *
getdevunit(const char *name, int unit)
{
	struct device *dev = alldevs.tqh_first;
	char num[10], fullname[16];
	int lunit;

	/* compute length of name and decimal expansion of unit number */
	sprintf(num, "%d", unit);
	lunit = strlen(num);
	if (strlen(name) + lunit >= sizeof(fullname) - 1)
		panic("config_attach: device name too long");

	strcpy(fullname, name);
	strcat(fullname, num);

	while (strcmp(dev->dv_xname, fullname) != 0) {
		if ((dev = dev->dv_list.tqe_next) == NULL)
			return NULL;
	}
	return dev;
}

/*
 * Match a struct device against the bootpath, by
 * comparing it's firmware package handle. If they match
 * exactly, we found the boot device.
 */
static void
dev_path_exact_match(struct device *dev, int ofnode)
{

	if (ofnode != ofbootpackage)
		return;

	booted_device = dev;
	DPRINTF(ACDB_BOOTDEV, ("found bootdevice: %s\n", dev->dv_xname));
}

/*
 * Match a struct device against the bootpath, by
 * comparing it's firmware package handle and calculating
 * the target/lun suffix and compmaring that against
 * the bootpath remainder.
 */
static void
dev_path_drive_match(struct device *dev, int ctrlnode, int target, int lun)
{
	int child = 0;
	char buf[OFPATHLEN];

	/*
	 * The ofbootpackage points to a disk on this controller, so
	 * iterate over all child nodes and compare.
	 */
	for (child = prom_firstchild(ctrlnode); child != 0;
	    child = prom_nextsibling(child))
		if (child == ofbootpackage)
			break;

	if (child == ofbootpackage) {
		/* boot device is on this controller */
		DPRINTF(ACDB_BOOTDEV, ("found controller of bootdevice\n"));
		sprintf(buf, "@%d,%d", target, lun);
		if (strcmp(buf, ofboottarget) == 0) {
			booted_device = dev;
			if (ofbootpartition)
				booted_partition = *ofbootpartition - 'a';
			DPRINTF(ACDB_BOOTDEV, ("found boot device: %s"
			    ", partition %d\n", dev->dv_xname,
			    booted_partition));
		}
	}
}

/*
 * Get the firmware package handle from a struct device.
 * Assuming we have previously stored it in the device properties
 * dictionary.
 */
static int
device_ofnode(struct device *dev)
{
	prop_dictionary_t props;
	prop_object_t obj;

	if (dev == NULL)
		return 0;
	props = device_properties(dev);
	if (props == NULL)
		return 0;
	obj = prop_dictionary_get(props, OFNODEKEY);
	if (obj == NULL)
		return 0;

	return prop_number_integer_value(obj);
}

/*
 * Save the firmware package handle inside the properties dictionary
 * of a struct device.
 */
static void
device_setofnode(struct device *dev, int node)
{
	prop_dictionary_t props;
	prop_object_t obj;

	if (dev == NULL)
		return;
	props = device_properties(dev);
	if (props == NULL)
		return;
	obj = prop_number_create_integer(node);
	if (obj == NULL)
		return;
	prop_dictionary_set(props, OFNODEKEY, obj);
	prop_object_release(obj);
}

/*
 * Called back during autoconfiguration for each device found
 */
void
device_register(struct device *dev, void *aux)
{
	struct device *busdev = device_parent(dev);
	int ofnode;

	/*
	 * We don't know the type of 'aux' - it depends on the
	 * bus this device attaches to. We are only interested in
	 * certain bus types, this only is used to find the boot
	 * device.
	 */
	if (busdev == NULL) {
		/*
		 * Ignore mainbus0 itself, it certainly is not a boot
		 * device.
		 */
	} else if (device_is_a(busdev, "mainbus")) {
		struct mainbus_attach_args *ma = aux;

		device_setofnode(dev, ma->ma_node);
		dev_path_exact_match(dev, ma->ma_node);
	} else if (device_is_a(busdev, "pci")) {
		struct pci_attach_args *pa = aux;

		ofnode = PCITAG_NODE(pa->pa_tag);
		device_setofnode(dev, ofnode);
		dev_path_exact_match(dev, ofnode);
	} else if (device_is_a(busdev, "sbus")) {
		struct sbus_attach_args *sa = aux;

		ofnode = sa->sa_node;
		device_setofnode(dev, ofnode);
		dev_path_exact_match(dev, sa->sa_node);
	} else if (device_is_a(dev, "sd") || device_is_a(dev, "cd")) {
		struct scsipibus_attach_args *sa = aux;
		struct scsipi_periph *periph = sa->sa_periph;

		ofnode = device_ofnode(
		    device_parent(device_parent(dev)));
		dev_path_drive_match(dev, ofnode, periph->periph_target,
		    periph->periph_lun);
	} else if (device_is_a(busdev, "wd")) {
		struct ata_device *adev = aux;

		ofnode = device_ofnode(
		    device_parent(device_parent(dev)));
		dev_path_drive_match(dev, ofnode, adev->adev_channel*2+
		    adev->adev_drv_data->drive, 0);
	}
}
