/*	$NetBSD: autoconf.c,v 1.238 2022/01/22 11:49:17 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.238 2022/01/22 11:49:17 thorpej Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_modular.h"
#include "opt_multiprocessor.h"

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
#include <sys/userconf.h>
#include <prop/proplib.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <dev/cons.h>
#include <sparc64/dev/cons.h>

#include <uvm/uvm_extern.h>

#include <sys/bus.h>
#include <machine/autoconf.h>
#include <machine/openfirm.h>
#include <machine/sparc64.h>
#include <machine/cpu.h>
#include <machine/pmap.h>
#include <machine/bootinfo.h>
#include <sparc64/sparc64/cache.h>
#include <sparc64/sparc64/ofw_patch.h>
#include <sparc64/sparc64/timerreg.h>
#include <sparc64/dev/cbusvar.h>

#include <dev/ata/atavar.h>
#include <dev/pci/pcivar.h>
#include <dev/ebus/ebusvar.h>
#include <dev/sbus/sbusvar.h>
#include <dev/i2c/i2cvar.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

#ifdef RASTERCONSOLE
#error options RASTERCONSOLE is obsolete for sparc64 - remove it from your config file
#endif

#include <dev/wsfb/genfbvar.h>

#include "ksyms.h"

int autoconf_debug = 0x0;

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
int kgdb_break_at_attach;
#endif

#define	OFPATHLEN	128

char	machine_banner[100];
char	machine_model[100];
char	ofbootpath[OFPATHLEN], *ofboottarget, *ofbootpartition;
char	ofbootargs[OFPATHLEN], *ofbootfile, *ofbootflags;
int	ofbootpackage;

static	int mbprint(void *, const char *);
int	mainbus_match(device_t, cfdata_t, void *);
static	void mainbus_attach(device_t, device_t, void *);
static  void get_ncpus(void);
static	void get_bootpath_from_prom(void);

/*
 * Kernel 4MB mappings.
 */
struct tlb_entry *kernel_tlbs;
int kernel_dtlb_slots;
int kernel_itlb_slots;

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

#ifdef SUN4V
void	sun4v_soft_state_init(void);
void	sun4v_set_soft_state(int, const char *);

#define __align32 __attribute__((__aligned__(32)))
char sun4v_soft_state_booting[] __align32 = "NetBSD booting";
char sun4v_soft_state_running[] __align32 = "NetBSD running";

void	sun4v_interrupt_init(void);
#if 0
XXX notyet		
void	sun4v_sdio_init(void);
#endif
#endif

int console_node, console_instance;
struct genfb_colormap_callback gfb_cb;
static void of_set_palette(void *, int, int, int, int);
static void copyprops(device_t, int, prop_dictionary_t, int);

static void
get_ncpus(void)
{
#ifdef MULTIPROCESSOR
	int node, l;
	char sbuf[32];

	node = findroot();

	sparc_ncpus = 0;
	for (node = OF_child(node); node; node = OF_peer(node)) {
		if (OF_getprop(node, "device_type", sbuf, sizeof(sbuf)) <= 0)
			continue;
		if (strcmp(sbuf, "cpu") != 0)
			continue;
		sparc_ncpus++;
		l = prom_getpropint(node, "dcache-line-size", 0);
		if (l > dcache_line_size)
			dcache_line_size = l;
		l = prom_getpropint(node, "icache-line-size", 0);
		if (l > icache_line_size)
			icache_line_size = l;
	}
#else
	/* #define sparc_ncpus 1 */
	icache_line_size = dcache_line_size = 8; /* will be fixed later */
#endif
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
	char buf[32];

#if NKSYMS || defined(DDB) || defined(MODULAR)
	struct btinfo_symtab *bi_sym;
#endif
	struct btinfo_count *bi_count;
	struct btinfo_kernend *bi_kend;
	struct btinfo_tlb *bi_tlb;
	struct btinfo_boothowto *bi_howto;

	extern void *romtba;
	extern void* get_romtba(void);
	extern void  OF_val2sym32(void *);
	extern void OF_sym2val32(void *);
	extern struct consdev consdev_prom;

	/* Save OpenFirmware entry point */
	romp   = ofw;
	romtba = get_romtba();

	prom_init();
	console_instance = promops.po_stdout;
	console_node = OF_instance_to_package(promops.po_stdout);

	/* Initialize the PROM console so printf will not panic */
	cn_tab = &consdev_prom;
	(*cn_tab->cn_init)(cn_tab);

	DPRINTF(ACDB_BOOTARGS,
		("sparc64_init(%p, %p, %p, %p, %p)\n", o0, bootargs, bootsize,
			o3, ofw));

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
		printf("Bad bootinfo size.\n");
die_old_boot_loader:
		printf("This kernel requires NetBSD boot loader version 1.9 "
		       "or newer\n");
		panic("sparc64_init.");
	}

	DPRINTF(ACDB_BOOTARGS,
		("sparc64_init: bmagic=%lx, bi=%p\n", bmagic, bi));

	/* Read in the information provided by NetBSD boot loader */
	if (SPARC_MACHINE_OPENFIRMWARE != bmagic) {
		printf("No bootinfo information.\n");
		goto die_old_boot_loader;
	}

	bootinfo = (void*)(u_long)((uint64_t*)bi)[1];
	LOOKUP_BOOTINFO(bi_kend, BTINFO_KERNEND);

	if (bi_kend->addr == (vaddr_t)0) {
		panic("Kernel end address is not found in bootinfo.\n");
	}

#if NKSYMS || defined(DDB) || defined(MODULAR)
	LOOKUP_BOOTINFO(bi_sym, BTINFO_SYMTAB);
	ksyms_addsyms_elf(bi_sym->nsym, (int *)(u_long)bi_sym->ssym,
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
	if (OF_getprop(findroot(), "compatible", buf, sizeof(buf)) > 0) {
		if (strcmp(buf, "sun4us") == 0)
			setcputyp(CPU_SUN4US);
		else if (strcmp(buf, "sun4v") == 0)
			setcputyp(CPU_SUN4V);
	}

	bi_howto = lookup_bootinfo(BTINFO_BOOTHOWTO);
	if (bi_howto)
		boothowto = bi_howto->boothowto;

	LOOKUP_BOOTINFO(bi_count, BTINFO_DTLB_SLOTS);
	kernel_dtlb_slots = bi_count->count;
	kernel_itlb_slots = kernel_dtlb_slots-1;
	bi_count = lookup_bootinfo(BTINFO_ITLB_SLOTS);
	if (bi_count)
		kernel_itlb_slots = bi_count->count;
	LOOKUP_BOOTINFO(bi_tlb, BTINFO_DTLB);
	kernel_tlbs = &bi_tlb->tlb[0];

	get_ncpus();
	pmap_bootstrap(KERNBASE, bi_kend->addr);

#ifdef SUN4V
	if (CPU_ISSUN4V) {
		sun4v_soft_state_init();
		sun4v_set_soft_state(SIS_TRANSITION, sun4v_soft_state_booting);
		sun4v_interrupt_init();
#if 0
XXX notyet		
		sun4v_sdio_init();
#endif 
	}
#endif
}

/*
 * get_bootpath_from_prom()
 * fetch the OF settings to identify our boot device during autoconfiguration
 */

static void
get_bootpath_from_prom(void)
{
	struct btinfo_bootdev *bdev = NULL;
	char sbuf[OFPATHLEN], *cp;
	int chosen;

	/*
	 * Grab boot path from PROM
	 */
	if ((chosen = OF_finddevice("/chosen")) == -1)
		return;

	bdev = lookup_bootinfo(BTINFO_BOOTDEV);
	if (bdev != NULL) {
		strcpy(ofbootpath, bdev->name);
	} else {
		if (OF_getprop(chosen, "bootpath", sbuf, sizeof(sbuf)) < 0)
			return;
		strcpy(ofbootpath, sbuf);
	}
	DPRINTF(ACDB_BOOTDEV, ("bootpath: %s\n", ofbootpath));
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
	if (cp) {
		for (; cp != ofbootpath; cp--) {
			if (*cp == '/') {
				ofboottarget = cp+1;
				break;
			}
		}
	}

	DPRINTF(ACDB_BOOTDEV, ("bootpath phandle: 0x%x\n", ofbootpackage));
	DPRINTF(ACDB_BOOTDEV, ("boot target: %s\n",
	    ofboottarget ? ofboottarget : "<none>"));
	DPRINTF(ACDB_BOOTDEV, ("boot partition: %s\n",
	    ofbootpartition ? ofbootpartition : "<none>"));

	/* Setup pointer to boot flags */
	if (OF_getprop(chosen, "bootargs", sbuf, sizeof(sbuf)) == -1)
		return;
	strcpy(ofbootargs, sbuf);

	cp = ofbootargs;

	/* Find start of boot flags */
	while (*cp) {
		while(*cp == ' ' || *cp == '\t') cp++;
		if (*cp == '-' || *cp == '\0')
			break;
		while(*cp != ' ' && *cp != '\t' && *cp != '\0') cp++;
		if (*cp != '\0')
			*cp++ = '\0';
	}
	if (cp != ofbootargs)
		ofbootfile = ofbootargs;
	ofbootflags = cp;
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
			kgdb_break_at_attach = 1;
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
cpu_configure(void)
{
	
	bool userconf = (boothowto & RB_USERCONF) != 0;

	/* fetch boot device settings */
	get_bootpath_from_prom();
	if (((boothowto & RB_USERCONF) != 0) && !userconf)
		/*
		 * Old bootloaders do not pass boothowto, and MI code
		 * has already handled userconfig before we get here
		 * and finally fetch the right options. So if we missed
		 * it, just do it here.
 		 */
		userconf_prompt();

	/* block clock interrupts and anything below */
	splclock();
	/* Enable device interrupts */
        setpstate(getpstate()|PSTATE_IE);

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("mainbus not configured");

	/* Enable device interrupts */
        setpstate(getpstate()|PSTATE_IE);

	(void)spl0();

#ifdef SUN4V
	if (CPU_ISSUN4V)
		sun4v_set_soft_state(SIS_NORMAL, sun4v_soft_state_running);
#endif
}

#ifdef SUN4V

#define HSVC_GROUP_INTERRUPT	0x002
#define HSVC_GROUP_SOFT_STATE	0x003
#define HSVC_GROUP_SDIO		0x108

int sun4v_soft_state_initialized = 0;

void
sun4v_soft_state_init(void)
{
	uint64_t minor;

	if (prom_set_sun4v_api_version(HSVC_GROUP_SOFT_STATE, 1, 0, &minor))
		return;

	prom_sun4v_soft_state_supported();
	sun4v_soft_state_initialized = 1;
}

void
sun4v_set_soft_state(int state, const char *desc)
{
	paddr_t pa;
	int err;

	if (!sun4v_soft_state_initialized)
		return;

	if (!pmap_extract(pmap_kernel(), (vaddr_t)desc, &pa))
		panic("sun4v_set_soft_state: pmap_extract failed");

	err = hv_soft_state_set(state, pa);
	if (err != H_EOK)
		printf("soft_state_set: %d\n", err);
}

void
sun4v_interrupt_init(void)
{
	uint64_t minor;

	if (prom_set_sun4v_api_version(HSVC_GROUP_INTERRUPT, 3, 0, &minor))
		return;

	sun4v_group_interrupt_major = 3;
}

#if 0
XXX notyet		
void
sun4v_sdio_init(void)
{
	uint64_t minor;

	if (prom_set_sun4v_api_version(HSVC_GROUP_SDIO, 1, 0, &minor))
		return;

	sun4v_group_sdio_major = 1;
}
#endif

#endif

void
cpu_rootconf(void)
{
	if (booted_device == NULL) {
		printf("FATAL: boot device not found, check your firmware "
		    "settings!\n");
	}

	rootconf();
}

char *
clockfreq(uint64_t freq)
{
	static char buf[10];
	size_t len;

	freq /= 1000;
	len = snprintf(buf, sizeof(buf), "%" PRIu64, freq / 1000);
	freq %= 1000;
	if (freq)
		snprintf(buf + len, sizeof(buf) - len, ".%03" PRIu64, freq);
	return buf;
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
mainbus_match(device_t parent, cfdata_t cf, void *aux)
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
mainbus_attach(device_t parent, device_t dev, void *aux)
{
extern struct sparc_bus_dma_tag mainbus_dma_tag;
extern struct sparc_bus_space_tag mainbus_space_tag;

	struct mainbus_attach_args ma;
	char sbuf[32];
	const char *const *ssp, *sp = NULL;
	char *c;
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

	if (OF_getprop(findroot(), "banner-name", machine_banner,
	    sizeof machine_banner) < 0)
		i = 0;
	else {
		i = 1;
		if (((c = strchr(machine_banner, '(')) != NULL) &&
		    c != &machine_banner[0]) {
				while (*c == '(' || *c == ' ') {
					*c = '\0';
					c--;
				}
			}
	}
	OF_getprop(findroot(), "name", machine_model, sizeof machine_model);
	prom_getidprom();
	if (i)
		aprint_normal(": %s (%s): hostid %lx\n", machine_model,
		    machine_banner, hostid);
	else
		aprint_normal(": %s: hostid %lx\n", machine_model, hostid);
	aprint_naive("\n");

	devhandle_t selfh = device_handle(dev);

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
	for (i = 0; i < __arraycount(intr_evcnts); i++)
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
		config_found(dev, &ma, mbprint,
		    CFARGS(.devhandle = devhandle_from_of(selfh, ma.ma_node)));
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
		    sizeof(portid) && 
		    OF_getprop(node, "portid", &portid, sizeof(portid)) !=
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
		(void) config_found(dev, (void *)&ma, mbprint,
		    CFARGS(.devhandle = prom_node_to_devhandle(selfh,
							       ma.ma_node)));
		free(ma.ma_reg, M_DEVBUF);
		if (ma.ma_ninterrupts)
			free(ma.ma_interrupts, M_DEVBUF);
		if (ma.ma_naddress)
			free(ma.ma_address, M_DEVBUF);
	}
	/* Try to attach PROM console */
	memset(&ma, 0, sizeof ma);
	ma.ma_name = "pcons";
	(void) config_found(dev, (void *)&ma, mbprint, CFARGS_NONE);
}

CFATTACH_DECL_NEW(mainbus, 0,
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
 * Match a device_t against the bootpath, by
 * comparing its firmware package handle. If they match
 * exactly, we found the boot device.
 */
static void
dev_path_exact_match(device_t dev, int ofnode)
{

	if (ofnode != ofbootpackage)
		return;

	booted_device = dev;
	DPRINTF(ACDB_BOOTDEV, ("found bootdevice: %s\n", device_xname(dev)));
}

/*
 * Match a device_t against the bootpath, by
 * comparing its firmware package handle and calculating
 * the target/lun suffix and comparing that against
 * the bootpath remainder.
 */
static void
dev_path_drive_match(device_t dev, int ctrlnode, int target,
    uint64_t wwn, int lun)
{
	int child = 0, ide_node = 0;
	char buf[OFPATHLEN];

	DPRINTF(ACDB_BOOTDEV, ("dev_path_drive_match: %s, controller %x, "
	    "target %d wwn %016" PRIx64 " lun %d\n", device_xname(dev),
	    ctrlnode, target, wwn, lun));

	/*
	 * The ofbootpackage points to a disk on this controller, so
	 * iterate over all child nodes and compare.
	 */
	for (child = prom_firstchild(ctrlnode); child != 0;
	    child = prom_nextsibling(child))
		if (child == ofbootpackage)
			break;

	if (child != ofbootpackage) {
		/*
		 * Try Mac firmware style (also used by QEMU/OpenBIOS):
		 * below the controller there is an intermediate node
		 * for each IDE channel, and individual targets always
		 * are "@0"
		 */
		for (ide_node = prom_firstchild(ctrlnode); ide_node != 0;
		    ide_node = prom_nextsibling(ide_node)) {
			const char * name = prom_getpropstring(ide_node,
			    "device_type");
			if (strcmp(name, "ide") != 0) continue;
			for (child = prom_firstchild(ide_node); child != 0;
			    child = prom_nextsibling(child))
				if (child == ofbootpackage)
					break;
			if (child == ofbootpackage)
				break;
		}
	}

	if (child == ofbootpackage) {
		const char * name = prom_getpropstring(child, "name");

		/* boot device is on this controller */
		DPRINTF(ACDB_BOOTDEV, ("found controller of bootdevice\n"));

		/*
		 * Note: "child" here is == ofbootpackage (s.a.), which
		 * may be completely wrong for the device we are checking,
		 * what we really do here is to match "target" and "lun".
		 */
		if (wwn)
			snprintf(buf, sizeof(buf), "%s@w%016" PRIx64 ",%d",
			    name, wwn, lun);
		else if (ide_node)
			snprintf(buf, sizeof(buf), "%s@0",
			    device_is_a(dev, "cd") ? "cdrom" : "disk");
		else
			snprintf(buf, sizeof(buf), "%s@%d,%d",
			    name, target, lun);
		if (ofboottarget && strcmp(buf, ofboottarget) == 0) {
			booted_device = dev;
			if (ofbootpartition)
				booted_partition = *ofbootpartition - 'a';
			DPRINTF(ACDB_BOOTDEV, ("found boot device: %s"
			    ", partition %d\n", device_xname(dev),
			    booted_partition));
		}
	}
}
 
 /*
 * Recursively check for a child node.
 */
static bool
has_child_node(int parent, int search)
{
	int child;

	for (child = prom_firstchild(parent); child != 0;
	    child = prom_nextsibling(child)) {
		if (child == search)
			return true;
		if (has_child_node(child, search))
			return true;
	}

	return false;
}

/*
 * The interposed pseudo-parent node in OpenBIOS has a
 * device_type = "ide" and no "compatible" property.
 * It is the secondary bus if the name is "ide1" or "ide"
 * with newer OpenBIOS versions. In the latter case, read
 * the first reg value to discriminate the two channels.
 */
static bool
openbios_secondary_ata_heuristic(int parent)
{
	char tmp[OFPATHLEN];
	int regs[4];

	if (OF_getprop(parent, "device_type", tmp, sizeof(tmp)) <= 0)
		return false;
	if (strcmp(tmp, "ide") != 0)
		return false;
	DPRINTF(ACDB_BOOTDEV, ("parent device_type is ide\n"));

	if (OF_getprop(parent, "compatible", tmp, sizeof(tmp)) > 0)
		return false;
	DPRINTF(ACDB_BOOTDEV, ("parent has no compatible property\n"));

	if (OF_getprop(parent, "name", tmp, sizeof(tmp)) <= 0)
		return false;
	if (strcmp(tmp, "ide1") == 0) {
		DPRINTF(ACDB_BOOTDEV, ("parent seems to be an (old) OpenBIOS"
		   " secondary ATA bus, applying workaround target+2\n"));
		return true;
	} else if (strcmp(tmp, "ide") == 0) {
		if (OF_getprop(parent, "reg", &regs, sizeof(regs))
		    >= sizeof(regs[0])) {
			DPRINTF(ACDB_BOOTDEV, ("parent seems to be an OpenBIOS"
			   " ATA bus #%u\n", regs[0]));

			return regs[0] == 1;
		}

	}

	return false;
}

/*
 * Match a device_t against the controller/target/lun/wwn
 * info passed in from the bootloader (if available),
 * otherwise fall back to old style string matching
 * heuristics.
 */
static void
dev_bi_unit_drive_match(device_t dev, int ctrlnode, int target,
    uint64_t wwn, int lun)
{
	static struct btinfo_bootdev_unit *bi_unit = NULL;
	uint32_t off = 0;
	static bool passed = false;
#ifdef DEBUG
	char ctrl_path[OFPATHLEN], parent_path[OFPATHLEN], dev_path[OFPATHLEN];
#endif

	if (!passed) {
		bi_unit = lookup_bootinfo(BTINFO_BOOTDEV_UNIT);
		passed = true;
	}

	if (bi_unit == NULL) {
		dev_path_drive_match(dev, ctrlnode, target, wwn, lun);
		return;
	}

#ifdef DEBUG
	DPRINTF(ACDB_BOOTDEV, ("dev_bi_unit_drive_match: %s, controller %x, "
	    "target %d wwn %016" PRIx64 " lun %d\n", device_xname(dev),
	    ctrlnode, target, wwn, lun));

	OF_package_to_path(ctrlnode, ctrl_path, sizeof(ctrl_path));
	OF_package_to_path(bi_unit->phandle, dev_path, sizeof(dev_path));
	OF_package_to_path(bi_unit->parent, parent_path, sizeof(parent_path));
	DPRINTF(ACDB_BOOTDEV, ("controller %x : %s\n", ctrlnode, ctrl_path));
	DPRINTF(ACDB_BOOTDEV, ("phandle %x : %s\n", bi_unit->phandle, dev_path));
	DPRINTF(ACDB_BOOTDEV, ("parent %x : %s\n", bi_unit->parent, parent_path));
#endif
	if (ctrlnode != bi_unit->parent
	    && !has_child_node(ctrlnode, bi_unit->phandle)) {
		DPRINTF(ACDB_BOOTDEV, ("controller %x : %s does not match "
		    "bootinfo: %x : %s\n",
		    ctrlnode, ctrl_path, bi_unit->parent, parent_path));
		return;
	}
	if (ctrlnode == bi_unit->parent) {
		DPRINTF(ACDB_BOOTDEV, ("controller %x : %s is bootinfo"
		    " parent\n", ctrlnode, ctrl_path));
	} else {
		DPRINTF(ACDB_BOOTDEV, ("controller %x : %s is parent of"
		    " %x : %s\n", ctrlnode, ctrl_path, bi_unit->parent,
		    parent_path));

		/*
		 * Our kernel and "real" OpenFirmware use a 0 .. 3 numbering
		 * scheme for IDE devices, but OpenBIOS splits it into
		 * two "buses" and numbers each 0..1.
		 * Check if we are on the secondary "bus" and adjust
		 * if needed...
		 */
		if (openbios_secondary_ata_heuristic(bi_unit->parent))
			off = 2;
	}

	if (bi_unit->wwn != wwn || (bi_unit->target+off) != target
	    || bi_unit->lun != lun) {
		DPRINTF(ACDB_BOOTDEV, ("mismatch: wwn %016" PRIx64 " - %016" PRIx64
		    ", target %d - %d, lun %d - %d\n",
		    bi_unit->wwn, wwn, bi_unit->target, target, bi_unit->lun, lun));
		return;
	}

	booted_device = dev;
	if (ofbootpartition)
		booted_partition = *ofbootpartition - 'a';
	DPRINTF(ACDB_BOOTDEV, ("found boot device: %s"
	    ", partition %d\n", device_xname(dev),
	    booted_partition));
}

/*
 * Called back during autoconfiguration for each device found
 */
void
device_register(device_t dev, void *aux)
{
	device_t busdev = device_parent(dev);
	devhandle_t devhandle;
	int ofnode = 0;

	/*
	 * If the device has a valid OpenFirmware node association,
	 * grab it now.
	 */
	devhandle = device_handle(dev);
	if (devhandle_type(devhandle) == DEVHANDLE_TYPE_OF) {
		ofnode = devhandle_to_of(devhandle);
	}

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
	} else if (device_is_a(busdev, "iic")) {
		struct i2c_attach_args *ia = aux;

		if (ia->ia_name == NULL)	/* indirect config */
			return;

		ofnode = (int)ia->ia_cookie;
		if (device_is_a(dev, "pcagpio")) {
			if (!strcmp(machine_model, "SUNW,Sun-Fire-V240") ||
			    !strcmp(machine_model, "SUNW,Sun-Fire-V210")) {
				add_gpio_props_v210(dev, aux);
			}
		} 
		if (device_is_a(dev, "pcf8574io")) {
			if (!strcmp(machine_model, "SUNW,Ultra-250")) {
				add_gpio_props_e250(dev, aux);
			}
		} 
		return;
	} else if (device_is_a(dev, "sd") || device_is_a(dev, "cd")) {
		struct scsipibus_attach_args *sa = aux;
		struct scsipi_periph *periph = sa->sa_periph;
		int off = 0;

		/*
		 * There are two "cd" attachments:
		 *   atapibus -> atabus -> controller
		 *   scsibus -> controller
		 * We want the node of the controller.
		 */
		if (device_is_a(busdev, "atapibus")) {
			busdev = device_parent(busdev);
			/*
			 * if the atapibus is connected to the secondary
			 * channel of the atabus, we need an offset of 2
			 * to match OF's idea of the target number.
			 * (i.e. on U5/U10 "cdrom" and "disk2" have the
			 * same target encoding, though different names)
			 */
			if (periph->periph_channel->chan_channel == 1)
				off = 2;
		}

		/*
		 * busdev now points to the direct descendent of the
		 * controller ("atabus" or "scsibus").  Get the
		 * controller's devhandle.  Hoist it up one more so
		 * that busdev points at the controller.
		 */
		busdev = device_parent(busdev);
		devhandle = device_handle(busdev);
		KASSERT(devhandle_type(devhandle) == DEVHANDLE_TYPE_OF);
		ofnode = devhandle_to_of(devhandle);

		/*
		 * Special sun4v handling in case the kernel is running in a 
		 * secondary logical domain
		 *
		 * The bootpath looks something like this:
		 *   /virtual-devices@100/channel-devices@200/disk@1:a
		 *
		 * The device hierarchy constructed during autoconfiguration
		 * is:
		 *   /mainbus/vbus/cbus/vdsk/scsibus/sd
		 */
		if (CPU_ISSUN4V && device_is_a(dev, "sd") &&
		    device_is_a(busdev, "vdsk")) {
			dev_path_exact_match(dev, ofnode);
		} else {
			dev_bi_unit_drive_match(dev, ofnode,
			    periph->periph_target + off, 0, periph->periph_lun);
		}

		if (device_is_a(busdev, "scsibus")) {
			/* see if we're in a known SCA drivebay */
			add_drivebay_props(dev, ofnode, aux);
		}
		return;
	} else if (device_is_a(dev, "wd")) {
		struct ata_device *adev = aux;

		/*
		 * busdev points to the direct descendent of the controller,
		 * e.g. "atabus".  Get the controller's devhandle.
		 */
		devhandle = device_handle(device_parent(busdev));
		KASSERT(devhandle_type(devhandle) == DEVHANDLE_TYPE_OF);
		ofnode = devhandle_to_of(devhandle);

		dev_bi_unit_drive_match(dev, ofnode, adev->adev_channel*2+
		    adev->adev_drv_data->drive, 0, 0);
		return;
	} else if (device_is_a(dev, "ld")) {
		/*
		 * Get the devhandle of the RAID (or whatever) controller.
		 */
		devhandle = device_handle(busdev);
		ofnode = devhandle_to_of(devhandle);
	}

	if (busdev == NULL)
		return;

	if (ofnode != 0) {
		uint8_t eaddr[ETHER_ADDR_LEN];
		char tmpstr[32];
		char tmpstr2[32];
		int node;
		uint32_t id = 0;
		uint64_t nwwn = 0, pwwn = 0;
		prop_dictionary_t dict;
		prop_data_t blob;
		prop_number_t pwwnd = NULL, nwwnd = NULL;
		prop_number_t idd = NULL;

		dev_path_exact_match(dev, ofnode);

		if (OF_getprop(ofnode, "name", tmpstr, sizeof(tmpstr)) <= 0)
			tmpstr[0] = 0;
		if (OF_getprop(ofnode, "device_type", tmpstr2, sizeof(tmpstr2)) <= 0)
			tmpstr2[0] = 0;

		/*
		 * If this is a network interface, note the
		 * mac address.
		 */
		if (strcmp(tmpstr, "network") == 0
		   || strcmp(tmpstr, "ethernet") == 0
		   || strcmp(tmpstr2, "network") == 0
		   || strcmp(tmpstr2, "ethernet") == 0
		   || OF_getprop(ofnode, "mac-address", &eaddr, sizeof(eaddr))
		      >= ETHER_ADDR_LEN
		   || OF_getprop(ofnode, "local-mac-address", &eaddr, sizeof(eaddr))
		      >= ETHER_ADDR_LEN) {

			dict = device_properties(dev);

			/*
			 * Is it a network interface with FCode?
			 */
			if (strcmp(tmpstr, "network") == 0 ||
			    strcmp(tmpstr2, "network") == 0) {
				prop_dictionary_set_bool(dict,
				    "without-seeprom", true);
				prom_getether(ofnode, eaddr);
			} else {
				if (!prom_get_node_ether(ofnode, eaddr))
					goto noether;
			}
			blob = prop_data_create_copy(eaddr, ETHER_ADDR_LEN);
			prop_dictionary_set(dict, "mac-address", blob);
			prop_object_release(blob);
			of_to_dataprop(dict, ofnode, "shared-pins",
			    "shared-pins");
		}
noether:

		/* is this a FC node? */
		if (strcmp(tmpstr, "scsi-fcp") == 0) {

			dict = device_properties(dev);

			if (OF_getprop(ofnode, "port-wwn", &pwwn, sizeof(pwwn))
			    == sizeof(pwwn)) {
				pwwnd = 
				    prop_number_create_unsigned(pwwn);
				prop_dictionary_set(dict, "port-wwn", pwwnd);
				prop_object_release(pwwnd);
			}

			if (OF_getprop(ofnode, "node-wwn", &nwwn, sizeof(nwwn))
			    == sizeof(nwwn)) {
				nwwnd = 
				    prop_number_create_unsigned(nwwn);
				prop_dictionary_set(dict, "node-wwn", nwwnd);
				prop_object_release(nwwnd);
			}
		}

		/* is this an spi device?  look for scsi-initiator-id */
		if (strcmp(tmpstr2, "scsi") == 0 ||
		    strcmp(tmpstr2, "scsi-2") == 0) {

			dict = device_properties(dev);

			for (node = ofnode; node != 0; node = OF_parent(node)) {
				if (OF_getprop(node, "scsi-initiator-id", &id,
				    sizeof(id)) <= 0)
					continue;

				idd = prop_number_create_unsigned(id);
				prop_dictionary_set(dict,
						    "scsi-initiator-id", idd);
				prop_object_release(idd);
				break;
			}
		}
	}

	/*
	 * Check for I2C busses and add data for their direct configuration.
	 */
	if (device_is_a(dev, "iic")) {
		devhandle_t bushandle = device_handle(busdev);
		int busnode =
		    devhandle_type(bushandle) == DEVHANDLE_TYPE_OF ?
		    devhandle_to_of(bushandle) : 0;

		if (busnode) {
			prop_dictionary_t props = device_properties(busdev);
			prop_object_t cfg = prop_dictionary_get(props,
				"i2c-child-devices");
			if (!cfg) {
				int node;
				const char *name;

				/*
				 * pmu's i2c devices are under the "i2c" node,
				 * so find it out.
				 */
				name = prom_getpropstring(busnode, "name");
				if (strcmp(name, "pmu") == 0) {
					for (node = OF_child(busnode);
					     node != 0; node = OF_peer(node)) {
						name = prom_getpropstring(node,
						    "name");
						if (strcmp(name, "i2c") == 0) {
							busnode = node;
							break;
						}
					}
				}

				of_enter_i2c_devs(props, busnode,
				    sizeof(cell_t), 1);
			}
		}

		if (!strcmp(machine_model, "TAD,SPARCLE"))
			add_spdmem_props_sparcle(busdev);

		if (device_is_a(busdev, "pcfiic") &&
		    (!strcmp(machine_model, "SUNW,Sun-Fire-V240") ||
		    !strcmp(machine_model, "SUNW,Sun-Fire-V210")))
			add_env_sensors_v210(busdev);

		/* E450 SUNW,envctrl */
		if (device_is_a(busdev, "pcfiic") &&
		    (!strcmp(machine_model, "SUNW,Ultra-4")))
			add_i2c_props_e450(busdev, busnode);

		/* E250 SUNW,envctrltwo */
		if (device_is_a(busdev, "pcfiic") &&
		    (!strcmp(machine_model, "SUNW,Ultra-250")))
			add_i2c_props_e250(busdev, busnode);
	}

	/* set properties for PCI framebuffers */
	if (device_is_a(busdev, "pci")) {
		/* see if this is going to be console */
		struct pci_attach_args *pa = aux;
		prop_dictionary_t dict;
		int sub;
		int console = 0;

		dict = device_properties(dev);

		/* we only care about display devices from here on */
		if (PCI_CLASS(pa->pa_class) != PCI_CLASS_DISPLAY)
			return;

		console = (ofnode == console_node);

		if (!console) {
			/*
			 * see if any child matches since OF attaches
			 * nodes for each head and /chosen/stdout
			 * points to the head rather than the device
			 * itself in this case
			 */
			sub = OF_child(ofnode);
			while ((sub != 0) && (sub != console_node)) {
				sub = OF_peer(sub);
			}
			if (sub == console_node) {
				console = true;
			}
		}

		copyprops(busdev, ofnode, dict, console);

		if (console) {
			uint64_t cmap_cb;
			prop_dictionary_set_uint32(dict,
			    "instance_handle", console_instance);

			gfb_cb.gcc_cookie = 
			    (void *)(intptr_t)console_instance;
			gfb_cb.gcc_set_mapreg = of_set_palette;
			cmap_cb = (uint64_t)(uintptr_t)&gfb_cb;
			prop_dictionary_set_uint64(dict,
			    "cmap_callback", cmap_cb);
		}
#ifdef notyet 
		else {
			int width;

			/*
			 * the idea is to 'open' display devices with no useful
			 * properties, in the hope that the firmware will
			 * properly initialize them and we can run things like
			 * genfb on them
			 */
			if (OF_getprop(node, "width", &width, sizeof(width))
			    != 4) {
				instance = OF_open(name);
			}
		}
#endif
		set_static_edid(dict);
	}

	set_hw_props(dev);
}

/*
 * Called back after autoconfiguration of a device is done
 */
void
device_register_post_config(device_t dev, void *aux)
{
	if (booted_device == NULL && device_is_a(dev, "sd")) {
		struct scsipibus_attach_args *sa = aux;
		struct scsipi_periph *periph = sa->sa_periph;
		uint64_t wwn = 0;

		/*
		 * If this is a FC-AL drive it will have
		 * acquired its WWN device property by now,
		 * so we can properly match it.
		 */
		if (prop_dictionary_get_uint64(device_properties(dev),
		    "port-wwn", &wwn)) {
			/*
			 * Different to what we do in device_register,
			 * we do not pass the "controller" ofnode,
			 * because FC-AL devices attach below a "fp" node,
			 * E.g.: /pci/SUNW,qlc@4/fp@0,0/disk
			 * and we need the parent of "disk" here.
			 */
			devhandle_t ctlr_devhandle = device_handle(
			    device_parent(device_parent(dev)));
			KASSERT(devhandle_type(ctlr_devhandle) ==
			    DEVHANDLE_TYPE_OF);
			int ofnode = devhandle_to_of(ctlr_devhandle);

			for (ofnode = OF_child(ofnode);
			     ofnode != 0 && booted_device == NULL;
			     ofnode = OF_peer(ofnode)) {
				dev_bi_unit_drive_match(dev, ofnode,
				    periph->periph_target,
				    wwn, periph->periph_lun);
			}
		}
	}
}

static void
copyprops(device_t busdev, int node, prop_dictionary_t dict, int is_console)
{
	device_t cntrlr;
	prop_dictionary_t psycho;
	paddr_t fbpa, mem_base = 0;
	uint32_t temp, fboffset;
	uint32_t fbaddr = 0;
	int options;
	char output_device[OFPATHLEN];
	char *pos;

	cntrlr = device_parent(busdev);
	if (cntrlr != NULL) {
		psycho = device_properties(cntrlr);
		prop_dictionary_get_uint64(psycho, "mem_base", &mem_base);
	}

	if (is_console)
		prop_dictionary_set_bool(dict, "is_console", 1);

	of_to_uint32_prop(dict, node, "width", "width");
	of_to_uint32_prop(dict, node, "height", "height");
	of_to_uint32_prop(dict, node, "linebytes", "linebytes");
	if (!of_to_uint32_prop(dict, node, "depth", "depth") &&
	    /* Some cards have an extra space in the property name */
	    !of_to_uint32_prop(dict, node, "depth ", "depth")) {
		/*
		 * XXX we should check linebytes vs. width but those
		 * FBs that don't have a depth property ( /chaos/control... )
		 * won't have linebytes either
		 */
		prop_dictionary_set_uint32(dict, "depth", 8);
	}

	OF_getprop(node, "address", &fbaddr, sizeof(fbaddr));
	if (fbaddr != 0) {
	
		pmap_extract(pmap_kernel(), fbaddr, &fbpa);
#ifdef DEBUG
		printf("membase: %lx fbpa: %lx\n", (unsigned long)mem_base,
		    (unsigned long)fbpa);
#endif
		if (mem_base == 0) {
			/* XXX this is guesswork */
			fboffset = (uint32_t)(fbpa & 0xffffffff);
		}
			fboffset = (uint32_t)(fbpa - mem_base);
		prop_dictionary_set_uint32(dict, "address", fboffset);
	}

	if (!of_to_dataprop(dict, node, "EDID", "EDID"))
		of_to_dataprop(dict, node, "edid", "EDID");

	temp = 0;
	if (OF_getprop(node, "ATY,RefCLK", &temp, sizeof(temp)) != 4) {

		OF_getprop(OF_parent(node), "ATY,RefCLK", &temp,
		    sizeof(temp));
	}
	if (temp != 0)
		prop_dictionary_set_uint32(dict, "refclk", temp / 10);

	/*
	 * finally, let's see if there's a video mode specified in
	 * output-device and pass it on so drivers like radeonfb
	 * can do their thing
	 */

	if (!is_console)
		return;

	options = OF_finddevice("/options");
	if ((options == 0) || (options == -1))
		return;
	if (OF_getprop(options, "output-device", output_device, OFPATHLEN) == 0)
		return;
	/* find the mode string if there is one */
	pos = strstr(output_device, ":r");
	if (pos == NULL)
		return;
	prop_dictionary_set_string(dict, "videomode", pos + 2);
}

static void
of_set_palette(void *cookie, int index, int r, int g, int b)
{
	int ih = (int)((intptr_t)cookie);

	OF_call_method_1("color!", ih, 4, r, g, b, index);
}
