/*	$NetBSD: autoconf.c,v 1.11 1999/01/09 23:23:47 eeh Exp $ */

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

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/dkstat.h>
#include <sys/conf.h>
#include <sys/dmap.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/msgbuf.h>
#if 0
#include <sys/user.h>
#endif

#include <net/if.h>

#include <dev/cons.h>

#include <vm/vm.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/openfirm.h>
#include <machine/sparc64.h>
#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <machine/pmap.h>
#include <sparc64/sparc64/timerreg.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif


int printspl = 0;

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	cold;		/* if 1, still working on cold-start */
int	fbnode;		/* node ID of ROM's console frame buffer */
int	optionsnode;	/* node ID of ROM's options */

#ifdef KGDB
extern	int kgdb_debug_panic;
#endif

static	int rootnode;
static	char *str2hex __P((char *, int *));
static	int mbprint __P((void *, const char *));
static	void crazymap __P((char *, int *));
int	st_crazymap __P((int));
void	sync_crash __P((void));
int	mainbus_match __P((struct device *, struct cfdata *, void *));
static	void mainbus_attach __P((struct device *, struct device *, void *));

struct	bootpath bootpath[8];
int	nbootpath;
static	void bootpath_build __P((void));
static	void bootpath_print __P((struct bootpath *));
int	search_prom __P((int, char *));

/* Global interrupt mappings for all device types.  Match against the OBP
 * 'device_type' property. 
 */
struct intrmap intrmap[] = {
	{ "block",	PIL_FD },	/* Floppy disk */
	{ "serial",	PIL_SER },	/* zs */
	{ "scsi",	PIL_SCSI },
	{ "network",	PIL_NET },
	{ "display",	PIL_VIDEO },
	{ NULL,		0 }
};

/*
 * Most configuration on the SPARC is done by matching OPENPROM Forth
 * device names with our internal names.
 */
int
matchbyname(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	printf("%s: WARNING: matchbyname\n", cf->cf_driver->cd_name);
	return (0);
}

/*
 * Convert hex ASCII string to a value.  Returns updated pointer.
 * Depends on ASCII order (this *is* machine-dependent code, you know).
 */
static char *
str2hex(str, vp)
	register char *str;
	register int *vp;
{
	register int v, c;

	for (v = 0;; v = v * 16 + c, str++) {
		c = *(u_char *)str;
		if (c <= '9') {
			if ((c -= '0') < 0)
				break;
		} else if (c <= 'F') {
			if ((c -= 'A' - 10) < 10)
				break;
		} else if (c <= 'f') {
			if ((c -= 'a' - 10) < 10)
				break;
		} else
			break;
	}
	*vp = v;
	return (str);
}

/*
 * locore.s code calls bootstrap() just before calling main().
 *
 * What we try to do is as follows:
 *
 * 1) We will try to re-allocate the old message buffer.
 *
 * 2) We will then get the list of the total and available
 *	physical memory and available virtual memory from the
 *	prom.
 *
 * 3) We will pass the list to pmap_bootstrap to manage them.
 *
 * We will try to run out of the prom until we get to cpu_init().
 */
void
bootstrap(nctx)
	int nctx;
{
	extern int end;	/* End of kernel */
	extern void *ssym, *esym;

	/* 
	 * Initialize ddb first and register OBP callbacks.
	 * We can do this because ddb_machine_init() and 
	 * ddb_init() do not allocate anything, just initialze
	 * some pointers to important things like the symtab.
	 *
	 * By doing this first and installing the OBP callbacks
	 * we get to do symbolic debugging of pmap_bootstrap().
	 */
#ifdef KGDB
/* Moved zs_kgdb_init() to dev/zs.c:consinit(). */
	zs_kgdb_init();		/* XXX */
#endif
#ifdef DDB
	db_machine_init();
#ifdef DB_ELF_SYMBOLS
	ddb_init((int)(esym - ssym), ssym, esym); /* No symbols as yet */
#else
	ddb_init();
#endif
#ifdef _LP64
	/* This can only be installed on an 64-bit system cause otherwise our stack is screwed */
	OF_set_symbol_lookup(OF_sym2val, OF_val2sym);
#endif
#endif

	/* 
	 * These are the best approximations for the spitfire: 
	 *
	 * Contexts are 13 bits.
	 *
	 * Other values are not relevant, but used to simulate a sun4
	 * 3-level MMU so we can address a full 32-bit virtual address
	 * space.  
	 *
	 * Eventually we should drop all of this in favor of traversing
	 * process address spaces during MMU faults.
	 */
	pmap_bootstrap(KERNBASE, (u_int)&end, nctx);
}

/*
 * bootpath_build: build a bootpath. Used when booting a generic
 * kernel to find our root device.  Newer proms give us a bootpath,
 * for older proms we have to create one.  An element in a bootpath
 * has 4 fields: name (device name), val[0], val[1], and val[2]. Note that:
 * Interpretation of val[] is device-dependent. Some examples:
 *
 * if (val[0] == -1) {
 *	val[1] is a unit number    (happens most often with old proms)
 * } else {
 *	[sbus device] val[0] is a sbus slot, and val[1] is an sbus offset
 *	[scsi disk] val[0] is target, val[1] is lun, val[2] is partition
 *	[scsi tape] val[0] is target, val[1] is lun, val[2] is file #
 * }
 *
 */

static void
bootpath_build()
{
	register char *cp, *pp;
	register struct bootpath *bp;
	register long chosen;
	char buf[128];

	bzero((void*)bootpath, sizeof(bootpath));
	bp = bootpath;

	/*
	 * Grab boot path from PROM
	 */
	chosen = OF_finddevice("/chosen");
	OF_getprop(chosen, "bootpath", buf, sizeof(buf));
	cp = buf;
	while (cp != NULL && *cp == '/') {
		/* Step over '/' */
		++cp;
		/* Extract name */
		pp = bp->name;
		while (*cp != '@' && *cp != '/' && *cp != '\0')
			*pp++ = *cp++;
		*pp = '\0';
		if (*cp == '@') {
			cp = str2hex(++cp, &bp->val[0]);
			if (*cp == ',')
				cp = str2hex(++cp, &bp->val[1]);
			if (*cp == ':')
				/* XXX - we handle just one char */
				bp->val[2] = *++cp - 'a', ++cp;
		} else {
			bp->val[0] = -1; /* no #'s: assume unit 0, no
					    sbus offset/adddress */
		}
		++bp;
		++nbootpath;
	}
	bp->name[0] = 0;
	
	bootpath_print(bootpath);
	
	/* Setup pointer to boot flags */
	OF_getprop(chosen, "bootargs", buf, sizeof(buf));
	cp = buf;
	if (cp == NULL)
		return;
	while (*cp != '-')
		if (*cp++ == '\0')
			return;

	for (;;) {
		switch (*++cp) {

		case '\0':
			return;

		case 'a':
			boothowto |= RB_ASKNAME;
			break;

		case 'b':
			boothowto |= RB_DFLTROOT;
			break;

		case 'd':	/* kgdb - always on zs	XXX */
#if defined(KGDB)
			boothowto |= RB_KDB;	/* XXX unused */
			kgdb_debug_panic = 1;
			kgdb_connect(1);
#elif defined(DDB)
			Debugger();
#else
			printf("kernel has no debugger\n");
#endif
			break;

		case 's':
			boothowto |= RB_SINGLE;
			break;
		}
	}
}

/*
 * print out the bootpath
 * the %x isn't 0x%x because the Sun EPROMs do it this way, and
 * consistency with the EPROMs is probably better here.
 */

static void
bootpath_print(bp)
	struct bootpath *bp;
{
	printf("bootpath: ");
	while (bp->name[0]) {
		if (bp->val[0] == -1)
			printf("/%s%x", bp->name, bp->val[1]);
		else
			printf("/%s@%x,%x", bp->name, bp->val[0], bp->val[1]);
		if (bp->val[2] != 0)
			printf(":%c", bp->val[2] + 'a');
		bp++;
	}
	printf("\n");
}


/*
 * save or read a bootpath pointer from the boothpath store.
 *
 * XXX. required because of SCSI... we don't have control over the "sd"
 * device, so we can't set boot device there.   we patch in with
 * dk_establish(), and use this to recover the bootpath.
 */

struct bootpath *
bootpath_store(storep, bp)
	int storep;
	struct bootpath *bp;
{
	static struct bootpath *save;
	struct bootpath *retval;

	retval = save;
	if (storep)
		save = bp;

	return (retval);
}
/* TEMP: */
struct bootpath *altbootpath_store(int, struct bootpath *);
struct bootpath *
altbootpath_store(storep, bp)
	int storep;
	struct bootpath *bp;
{
	static struct bootpath *save;
	struct bootpath *retval;

	retval = save;
	if (storep)
		save = bp;

	return (retval);
}
/* END TEMP */

/*
 * Set up the sd target mappings for non SUN4 PROMs.
 * Find out about the real SCSI target, given the PROM's idea of the
 * target of the (boot) device (i.e., the value in bp->v0val[0]).
 */
static void
crazymap(prop, map)
	char *prop;
	int *map;
{
	int i;

	/*
	 * Set up the identity mapping for old sun4 monitors
	 * and v[2-] OpenPROMs. Note: dkestablish() does the
	 * SCSI-target juggling for sun4 monitors.
	 */
	for (i = 0; i < 8; ++i)
		map[i] = i;
}

int
sd_crazymap(n)
	int	n;
{
	static int prom_sd_crazymap[8]; /* static: compute only once! */
	static int init = 0;

	if (init == 0) {
		crazymap("sd-targets", prom_sd_crazymap);
		init = 1;
	}
	return prom_sd_crazymap[n];
}

int
st_crazymap(n)
	int	n;
{
	static int prom_st_crazymap[8]; /* static: compute only once! */
	static int init = 0;

	if (init == 0) {
		crazymap("st-targets", prom_st_crazymap);
		init = 1;
	}
	return prom_st_crazymap[n];
}


/*
 * Determine mass storage and memory configuration for a machine.
 * We get the PROM's root device and make sure we understand it, then
 * attach it as `mainbus0'.  We also set up to handle the PROM `sync'
 * command.
 */
void
configure()
{
#if 0
	extern struct user *proc0paddr;	/* XXX see below */
#endif

	/* build the bootpath */
	bootpath_build();

#if notyet
        /* FIXME FIXME FIXME  This is probably *WRONG!!!**/
        OF_set_callback(sync_crash);
#endif

	/* block clock interrupts and anything below */
	splclock();
	/* Enable device interrupts */
        setpstate(getpstate()|PSTATE_IE);

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("mainbus not configured");

	/* Enable device interrupts */
        setpstate(getpstate()|PSTATE_IE);

#if 0
	/*
	 * XXX Re-zero proc0's user area, to nullify the effect of the
	 * XXX stack running into it during auto-configuration.
	 * XXX - should fix stack usage.
	 */
	bzero(proc0paddr, sizeof(struct user));
#endif

	(void)spl0();
	cold = 0;
}

struct device *altbootdev;

void
cpu_rootconf()
{
	struct bootpath *bp;
	struct device *bootdv;
	int bootpartition;

	bp = nbootpath == 0 ? NULL : &bootpath[nbootpath-1];
	bootdv = bp == NULL ? NULL : bp->dev;
	bootpartition = bp == NULL ? 0 : bp->val[2];

	setroot(bootdv, bootpartition, dev_name2blk);
}

/*
 * Console `sync' command.  SunOS just does a `panic: zero' so I guess
 * no one really wants anything fancy...
 */
void
sync_crash()
{

	panic("PROM sync command");
}

char *
clockfreq(freq)
	long freq;
{
	char *p;
	static char buf[10];

	freq /= 1000;
	sprintf(buf, "%ld", freq / 1000);
	freq %= 1000;
	if (freq) {
		freq += 1000;	/* now in 1000..1999 */
		p = buf + strlen(buf);
		sprintf(p, "%ld", freq);
		*p = '.';	/* now buf = %d.%3d */
	}
	return (buf);
}

/* ARGSUSED */
static int
mbprint(aux, name)
	void *aux;
	const char *name;
{
	struct mainbus_attach_args *ma = aux;

	if (name)
		printf("%s at %s", ma->ma_name, name);
	if (ma->ma_address)
		printf(" addr 0x%08lx", (u_long)ma->ma_address[0]);
	if (ma->ma_pri)
		printf(" ipl %d", ma->ma_pri);
	return (UNCONF);
}

int
findroot()
{
	register int node;

	if ((node = rootnode) == 0 && (node = nextsibling(0)) == 0)
		panic("no PROM root device");
	rootnode = node;
	return (node);
}

/*
 * Given a `first child' node number, locate the node with the given name.
 * Return the node number, or 0 if not found.
 */
int
findnode(first, name)
	int first;
	register const char *name;
{
	int node;
	char buf[32];

	for (node = first; node; node = nextsibling(node))
		if (strcmp(getpropstringA(node, "name", buf), name) == 0)
			return (node);
	return (0);
}

#if 0
/*
 * Fill in a romaux.  Returns 1 on success, 0 if the register property
 * was not the right size.
 */
int
romprop(rp, cp, node)
	register struct romaux *rp;
	const char *cp;
	register int node;
{
	register int len;
	static union { char regbuf[256]; struct rom_reg rr[RA_MAXREG]; struct rom_reg64 rr64[RA_MAXREG]; } u;
	static const char pl[] = "property length";

	bzero(u.regbuf, sizeof u);
	len = getprop(node, "reg", (void *)u.regbuf, sizeof(u.regbuf));
	if (len == -1 &&
	    node_has_property(node, "device_type") &&
	    strcmp(getpropstring(node, "device_type"), "hierarchical") == 0)
		len = 0;
	if (len > RA_MAXREG * sizeof(struct rom_reg))
		printf("warning: %s \"reg\" %s %d > %d, excess ignored\n",
		       cp, pl, len, RA_MAXREG * sizeof(struct rom_reg));
	if (len % sizeof(struct rom_reg) && len % sizeof(struct rom_reg64)) {
		printf("%s \"reg\" %s = %d (need multiple of %d)\n",
		       cp, pl, len, sizeof(struct rom_reg));
		return (0);
	}
#ifdef NOTDEF_DEBUG
	printf("romprop: reg len=%d; len % sizeof(struct rom_reg64)=%d; len % sizeof(struct rom_reg)=%d\n",
	       len, len % sizeof(struct rom_reg), len % sizeof(struct rom_reg64));
#endif
	if (len % sizeof(struct rom_reg64) == 0) {
		rp->ra_node = node;
		rp->ra_name = cp;
		rp->ra_nreg = len / sizeof(struct rom_reg64);
		for( len=0; len<rp->ra_nreg; len++ ) {
			/* Convert to 32-bit format */
			rp->ra_reg[len].rr_iospace = (u.rr64[len].rr_paddr>>32);
			rp->ra_reg[len].rr_paddr = (void*)(u.rr64[len].rr_paddr);
			rp->ra_reg[len].rr_len = u.rr64[len].rr_len;
#ifdef NOTDEF_DEBUG
			printf("romprop: reg[%d]=(%x,%x,%x)\n",
			       len, rp->ra_reg[len].rr_iospace, 
			       rp->ra_reg[len].rr_paddr,
			       rp->ra_reg[len].rr_len);
#endif
		}
	} else {
		rp->ra_node = node;
		rp->ra_name = cp;
		rp->ra_nreg = len / sizeof(struct rom_reg);
		bcopy(u.rr, rp->ra_reg, len);
	}

	len = getprop(node, "address", (void *)rp->ra_vaddrs,
		      sizeof(rp->ra_vaddrs));
	if (len == -1) {
		rp->ra_vaddr = 0;	/* XXX - driver compat */
		len = 0;
	}
	if (len & 3) {
		printf("%s \"address\" %s = %d (need multiple of 4)\n",
		       cp, pl, len);
		len = 0;
	}
	rp->ra_nvaddrs = len >> 2;
	
	len = getprop(node, "intr", (void *)&rp->ra_intr, sizeof rp->ra_intr);
	if (len == -1)
		len = 0;
	if (len & 7) {
		printf("%s \"intr\" %s = %d (need multiple of 8)\n",
		    cp, pl, len);
		len = 0;
	}
	rp->ra_nintr = len >>= 3;
	/* SPARCstation interrupts are not hardware-vectored */
	while (--len >= 0) {
		if (rp->ra_intr[len].int_vec) {
			printf("WARNING: %s interrupt %d has nonzero vector\n",
			    cp, len);
			break;
		}
	}
	/* Sun4u interrupts */
	len = getprop(node, "interrupts", (void *)&rp->ra_interrupt, sizeof rp->ra_interrupt);
	if (len == -1)
		len = 0;
	if (len & 2) {
		printf("%s \"interrupts\" %s = %d (need multiple of 4)\n",
		    cp, pl, len);
		len = 0;
	}
	rp->ra_ninterrupt = len >>= 2;
	len = getprop(node, "ranges", (void *)&rp->ra_range,
		      sizeof rp->ra_range);
	if (len == -1)
		len = 0;
	rp->ra_nrange = len / sizeof(struct rom_range);

	return (1);
}
#endif

int
mainbus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	return (1);
}

int autoconf_nzs = 0;	/* must be global so obio.c can see it */

/*
 * Attach the mainbus.
 *
 * Our main job is to attach the CPU (the root node we got in configure())
 * and iterate down the list of `mainbus devices' (children of that node).
 * We also record the `node id' of the default frame buffer, if any.
 */
static void
mainbus_attach(parent, dev, aux)
	struct device *parent, *dev;
	void *aux;
{
extern struct sparc_bus_dma_tag mainbus_dma_tag;
extern struct sparc_bus_space_tag mainbus_space_tag;

	struct mainbus_attach_args ma;
	char namebuf[32];
	const char *const *ssp, *sp = NULL;
	int node0, node;

	static const char *const openboot_special[] = {
		/* find these first */
		"counter-timer",
		"",

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

	printf(": %s\n", getpropstringA(findroot(), "name", namebuf));


	/*
	 * Locate and configure the ``early'' devices.  These must be
	 * configured before we can do the rest.  For instance, the
	 * EEPROM contains the Ethernet address for the LANCE chip.
	 * If the device cannot be located or configured, panic.
	 */

/*
 * The rest of this routine is for OBP machines exclusively.
 */

	node = findroot();

	/* the first early device to be configured is the cpu */
	{
		/* XXX - what to do on multiprocessor machines? */
		register const char *cp;
		
		for (node = firstchild(node); node; node = nextsibling(node)) {
			cp = getpropstringA(node, "device_type", namebuf);
			if (strcmp(cp, "cpu") == 0) {
				bzero(&ma, sizeof(ma));
				ma.ma_bustag = &mainbus_space_tag;
				ma.ma_dmatag = &mainbus_dma_tag;
				ma.ma_node = node;
				ma.ma_name = "cpu";
				config_found(dev, (void *)&ma, mbprint);
				break;
			}
		}
		if (node == 0)
			panic("None of the CPUs found\n");
	}


	node = findroot();	/* re-init root node */

	/* Find the "options" node */
	node0 = firstchild(node);
	optionsnode = findnode(node0, "options");
	if (optionsnode == 0)
		panic("no options in OPENPROM");

	for (ssp = openboot_special; *(sp = *ssp) != 0; ssp++) {

		if ((node = findnode(node0, sp)) == 0) {
			printf("could not find %s in OPENPROM\n", sp);
			panic(sp);
		}

		bzero(&ma, sizeof ma);
		ma.ma_bustag = &mainbus_space_tag;
		ma.ma_dmatag = &mainbus_dma_tag;
		ma.ma_name = getpropstringA(node, "name", namebuf);
		ma.ma_node = node;
		if (getprop(node, "reg", sizeof(ma.ma_reg[0]), 
			     &ma.ma_nreg, (void**)&ma.ma_reg) != 0)
{
panic("mainbus_attach(): %s has no \"reg\"\n", sp);
			continue;
}
		if (getprop(node, "interrupts", sizeof(ma.ma_interrupts[0]), 
			     &ma.ma_ninterrupts, (void**)&ma.ma_interrupts) != 0) {
			free(ma.ma_reg, M_DEVBUF);
panic("mainbus_attach(): %s has no \"interrupts\"\n", sp);
			continue;
		}
		if (getprop(node, "address", sizeof(*ma.ma_address), 
			     &ma.ma_naddress, (void**)&ma.ma_address) != 0) {
			free(ma.ma_reg, M_DEVBUF);
			free(ma.ma_interrupts, M_DEVBUF);
panic("mainbus_attach(): %s has no \"address\"\n", sp);
			continue;
		}
		/* Start at the beginning of the bootpath */
		ma.ma_bp = bootpath;

		if (config_found(dev, (void *)&ma, mbprint) == NULL)
			panic(sp);
		free(ma.ma_reg, M_DEVBUF);
		free(ma.ma_interrupts, M_DEVBUF);
		free(ma.ma_address, M_DEVBUF);
	}

	/*
	 * Configure the rest of the devices, in PROM order.  Skip
	 * PROM entries that are not for devices, or which must be
	 * done before we get here.
	 */
	for (node = node0; node; node = nextsibling(node)) {
		const char *cp;

		if (node_has_property(node, "device_type") &&
		    strcmp(getpropstringA(node, "device_type", namebuf),
			   "cpu") == 0)
			continue;
		cp = getpropstringA(node, "name", namebuf);
		for (ssp = openboot_special; (sp = *ssp) != NULL; ssp++)
			if (strcmp(cp, sp) == 0)
				break;
		if (sp != NULL)
			continue; /* an "early" device already configured */

		bzero(&ma, sizeof ma);
		ma.ma_bustag = &mainbus_space_tag;
		ma.ma_dmatag = &mainbus_dma_tag;
		ma.ma_name = getpropstringA(node, "name", namebuf);
		ma.ma_node = node;

		if (getprop(node, "reg", sizeof(*ma.ma_reg), 
			     &ma.ma_nreg, (void**)&ma.ma_reg) != 0)
			continue;

		if (getprop(node, "interrupts", sizeof(*ma.ma_interrupts), 
			     &ma.ma_ninterrupts, (void**)&ma.ma_interrupts) != 0) {
			free(ma.ma_reg, M_DEVBUF);
			continue;
		}
		if (getprop(node, "address", sizeof(*ma.ma_address), 
			     &ma.ma_naddress, (void**)&ma.ma_address) != 0) {
			free(ma.ma_reg, M_DEVBUF);
			free(ma.ma_interrupts, M_DEVBUF);
			continue;
		}
		/* Start at the beginning of the bootpath */
		ma.ma_bp = bootpath;

		if (config_found(dev, (void *)&ma, mbprint) == NULL)
			panic(sp);
		free(ma.ma_reg, M_DEVBUF);
		free(ma.ma_interrupts, M_DEVBUF);
		free(ma.ma_address, M_DEVBUF);
	}
}

struct cfattach mainbus_ca = {
	sizeof(struct device), mainbus_match, mainbus_attach
};

/*
 * findzs() is called from the zs driver (which is, at least in theory,
 * generic to any machine with a Zilog ZSCC chip).  It should return the
 * address of the corresponding zs channel.  It may not fail, and it
 * may be called before the VM code can be used.  Here we count on the
 * FORTH PROM to map in the required zs chips.
 */
void *
findzs(zs)
	int zs;
{
	int node, n;
	unsigned int addr;

	node = firstchild(findroot());
	/* Ultras have zs on the sbus */
	node = findnode(node, "sbus");
	if (!node)
		panic("findzs: no sbus node");
	node = firstchild(node);
	n=0;
	while ((node = findnode(node, "zs")) != 0) {
		/* There is no way to identify a node by its number */
		if ( n++ == zs ) { 
			if ((addr = getpropint(node, "address", 0)) == 0)
				/* We really should just map it in ourselves */
				panic("findzs: zs%d not mapped by PROM", zs);
			return ((void *)(unsigned long)addr);
		}
		node = nextsibling(node);
	}
	panic("findzs: cannot find zs%d", zs);
	/* NOTREACHED */
}

#if 0
int
makememarr(ap, max, which)
	register struct memarr *ap;
	int max, which;
{
	struct v2rmi {
		int	zero;
		int	addr;
		int	len;
	} v2rmi[200];		/* version 2 rom meminfo layout */
#define	MAXMEMINFO (sizeof(v2rmi) / sizeof(*v2rmi))
	register int i, node, len;
	char *prop;

	/*
	 * Version 2 PROMs use a property array to describe them.
	 */
	if (max > MAXMEMINFO) {
		printf("makememarr: limited to %d\n", MAXMEMINFO);
		max = MAXMEMINFO;
	}
	if ((node = findnode(firstchild(findroot()), "memory")) == 0)
		panic("makememarr: cannot find \"memory\" node");
	switch (which) {
		
	case MEMARR_AVAILPHYS:
		prop = "available";
		break;
		
	case MEMARR_TOTALPHYS:
		prop = "reg";
		break;
		
	default:
		panic("makememarr");
	}
	len = getprop(node, prop, (void *)v2rmi, sizeof v2rmi) /
		sizeof(struct v2rmi);
	for (i = 0; i < len; i++) {
		if (i >= max)
			goto overflow;
		ap->addr = v2rmi[i].addr;
		ap->len = v2rmi[i].len;
		ap++;
	}

	/*
	 * Success!  (Hooray)
	 */
	if (i == 0)
		panic("makememarr: no memory found");
	return (i);

overflow:
	/*
	 * Oops, there are more things in the PROM than our caller
	 * provided space for.  Truncate any extras.
	 */
	printf("makememarr: WARNING: lost some memory\n");
	return (i);
}
#endif

int
getprop(node, name, size, nitem, bufp)
	int	node;
	char	*name;
	size_t	size;
	int	*nitem;
	void	**bufp;
{
	void	*buf;
	long	len;

	len = getproplen(node, name);
	if (len <= 0)
		return (ENOENT);

	if ((len % size) != 0)
		return (EINVAL);

	buf = *bufp;
	if (buf == NULL) {
		/* No storage provided, so we allocate some */
		buf = malloc(len, M_DEVBUF, M_NOWAIT);
		if (buf == NULL)
			return (ENOMEM);
	}

	OF_getprop(node, name, buf, len);
	*bufp = buf;
	*nitem = len / size;
	return (0);
}

int
getprop_reg1(node, rrp)
	int node;
	struct sbus_reg *rrp;
{
	int error, n;
	struct sbus_reg *rrp0 = NULL;
	char buf[32];

	error = getprop(node, "reg", sizeof(struct sbus_reg),
			 &n, (void **)&rrp0);
	if (error != 0) {
		if (error == ENOENT &&
		    node_has_property(node, "device_type") &&
		    strcmp(getpropstringA(node, "device_type", buf),
			   "hierarchical") == 0) {
			bzero(rrp, sizeof(struct sbus_reg));
			error = 0;
		}
		return (error);
	}

	*rrp = rrp0[0];
	free(rrp0, M_DEVBUF);
	return (0);
}

int
getprop_intr1(node, ip)
	int node;
	int *ip;
{
	int error, n;
	struct sbus_intr *sip = NULL;
	int *interrupts;
	char buf[32];

	if (getprop(node, "interrupts", sizeof(*interrupts), 
		     &n, (void**)&interrupts) != 0) {
		/* Now things get ugly.  We need to take this value which is
		 * the interrupt vector number and encode the IPL into it
		 * somehow. Luckily, the interrupt vector has lots of free
		 * space and we can easily stuff the IPL in there for a while.  
		 */
		getpropstringA(node, "device_type", buf);
		for (n=0; intrmap[n].in_class; n++) {
			if (strcmp(intrmap[n].in_class, buf) == 0) {
				interrupts[0] |= (intrmap[n].in_lev << 11);
				break;
			}
		}
		*ip = interrupts[0];
		free(interrupts, M_DEVBUF);
		return 0;
	}
	/* Go with old-style intr property.  I don't know what to do with
         * this.  Need to get to the vector.
	 */
	error = getprop(node, "intr", sizeof(struct sbus_intr),
			 &n, (void **)&sip);
	if (error != 0) {
		if (error == ENOENT) {
			*ip = 0;
			error = 0;
		}
		return (error);
	}

	*ip = sip[0].sbi_pri;
	free(sip, M_DEVBUF);
	return (0);
}

int
getprop_address1(node, vpp)
	int node;
	void **vpp;
{
	int error, n;
	void **vp = NULL;

	error = getprop(node, "address", sizeof(u_int32_t), &n, (void **)&vp);
	if (error != 0) {
		*vpp = 0;
		if (error == ENOENT) {
			error = 0;
		}
		return (error);
	}

	*vpp = vp[0];
	free(vp, M_DEVBUF);
	return (0);
}

/*
 * Internal form of proplen().  Returns the property length.
 */
long
getproplen(node, name)
	int node;
	char *name;
{
	return (OF_getproplen(node, name));
}

/*
 * Return a string property.  There is a (small) limit on the length;
 * the string is fetched into a static buffer which is overwritten on
 * subsequent calls.
 */
char *
getpropstring(node, name)
	int node;
	char *name;
{
	static char stringbuf[32];

	return (getpropstringA(node, name, stringbuf));
}

/* Alternative getpropstring(), where caller provides the buffer */
char *
getpropstringA(node, name, buffer)
	int node;
	char *name;
	char *buffer;
{
	int blen;

	if (getprop(node, name, 1, &blen, (void **)&buffer) != 0)
		blen = 0;

	buffer[blen] = '\0';	/* usually unnecessary */
	return (buffer);
}

/*
 * Fetch an integer (or pointer) property.
 * The return value is the property, or the default if there was none.
 */
int
getpropint(node, name, deflt)
	int node;
	char *name;
	int deflt;
{
	int intbuf, *ip = &intbuf;
	int len;

	if (getprop(node, name, sizeof(int), &len, (void **)&ip) != 0)
		return (deflt);

	return (*ip);
}

/*
 * OPENPROM functions.  These are here mainly to hide the OPENPROM interface
 * from the rest of the kernel.
 */
int
firstchild(node)
	int node;
{

	return OF_child(node);
}

int
nextsibling(node)
	int node;
{

	return OF_peer(node);
}

/* The following recursively searches a PROM tree for a given node */
int
search_prom(rootnode, name)
        register int rootnode;
        register char *name;
{
	int rtnnode;
	int node = rootnode;
	char buf[32];

	if (node == findroot() ||
	    !strcmp("hierarchical", getpropstringA(node, "device_type", buf)))
		node = firstchild(node);

	if (node == 0)
		panic("search_prom: null node");

	do {
		if (strcmp(getpropstringA(node, "name", buf), name) == 0)
			return (node);

		if (node_has_property(node,"device_type") &&
		    (strcmp(getpropstringA(node, "device_type", buf),
			     "hierarchical") == 0
		     || strcmp(getpropstringA(node, "name", buf), "iommu") == 0)
		    && (rtnnode = search_prom(node, name)) != 0)
			return (rtnnode);

	} while ((node = nextsibling(node)));

	return (0);
}

/* The following are used primarily in consinit() */

int
opennode(path)		/* translate phys. device path to node */
	register char *path;
{
	return OF_open(path);
}

int
node_has_property(node, prop)	/* returns 1 if node has given property */
	register int node;
	register const char *prop;
{
	return (OF_getproplen(node, (caddr_t)prop) != -1);
}

#ifdef RASTERCONSOLE
/* Pass a string to the FORTH PROM to be interpreted */
void
rominterpret(s)
	register char *s;
{

	if (promvec->pv_romvec_vers < 2)
		promvec->pv_fortheval.v0_eval(strlen(s), s);
	else
		promvec->pv_fortheval.v2_eval(s);
}

/*
 * Try to figure out where the PROM stores the cursor row & column
 * variables.  Returns nonzero on error.
 */
int
romgetcursoraddr(rowp, colp)
	register int **rowp, **colp;
{
	char buf[100];

	/*
	 * line# and column# are global in older proms (rom vector < 2)
	 * and in some newer proms.  They are local in version 2.9.  The
	 * correct cutoff point is unknown, as yet; we use 2.9 here.
	 */
	if (promvec->pv_romvec_vers < 2 || promvec->pv_printrev < 0x00020009)
		sprintf(buf,
		    "' line# >body >user %lx ! ' column# >body >user %lx !",
		    (u_long)rowp, (u_long)colp);
	else
		sprintf(buf,
		    "stdout @ is my-self addr line# %lx ! addr column# %lx !",
		    (u_long)rowp, (u_long)colp);
	*rowp = *colp = NULL;
	rominterpret(buf);
	return (*rowp == NULL || *colp == NULL);
}
#endif

void
romhalt()
{
	OF_exit();
	panic("PROM exit failed");
}

void
romboot(bootargs)
	char *bootargs;
{
	OF_boot(bootargs);
	panic("PROM boot failed");
}

void
callrom()
{

	__asm __volatile("wrpr	%%g0, 0, %%tl" : );
	OF_enter();
}

/*
 * find a device matching "name" and unit number
 */
struct device *
getdevunit(name, unit)
	char *name;
	int unit;
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
 * Device registration used to determine the boot device.
 * 
 * Copied from the sparc port.
 */
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#define BUSCLASS_GENERIC	0
#define BUSCLASS_MAINBUS	1
#define BUSCLASS_IOMMU		2
#define BUSCLASS_OBIO		3
#define BUSCLASS_SBUS		4
#define BUSCLASS_VME		5

static int bus_class __P((struct device *));
static int instance_match __P((struct device *, void *, struct bootpath *));
static void nail_bootdev __P((struct device *, struct bootpath *));

static struct {
	char	*name;
	int	class;
} bus_class_tab[] = {
	{ "mainbus",	BUSCLASS_MAINBUS },
	{ "obio",	BUSCLASS_OBIO },
	{ "iommu",	BUSCLASS_IOMMU },
	{ "sbus",	BUSCLASS_SBUS },
	{ "xbox",	BUSCLASS_SBUS },
	{ "esp",	BUSCLASS_SBUS },
	{ "dma",	BUSCLASS_SBUS },
	{ "espdma",	BUSCLASS_SBUS },
	{ "ledma",	BUSCLASS_SBUS },
	{ "vme",	BUSCLASS_VME }
};

static int
bus_class(dev)
	struct device *dev;
{
	struct device *parent = dev->dv_parent;
	char *name = parent->dv_cfdata->cf_driver->cd_name;
	int i, class;

	class = BUSCLASS_GENERIC;
	for (i = sizeof(bus_class_tab)/sizeof(bus_class_tab[0]); i-- > 0;) {
		if (strcmp(name, bus_class_tab[i].name) == 0) {
			class = bus_class_tab[i].class;
			break;
		}
	}

	/* sun4m obio special case */
	if (CPU_ISSUN4M && class == BUSCLASS_OBIO)
		class = BUSCLASS_SBUS;

	return (class);
}

int
instance_match(dev, aux, bp)
	struct device *dev;
	void *aux;
	struct bootpath *bp;
{
	struct mainbus_attach_args *ma;
	struct sbus_attach_args *sa;
	struct iommu_attach_args *iom;

	/*
	 * Several Sbus devices are represented on bootpaths in one of
	 * two formats:
	 *	(1) ../sbus@.../esp@<offset>,<slot>/sd@..  (PROM v3 style)
	 *	(2) /sbus0/esp0/sd@..                      (PROM v2 style)
	 *
	 * hence we fall back on a `unit number' check if the Sbus-specific
	 * instance parameter check does not produce a match.
	 */

	switch (bus_class(dev)) {
	case BUSCLASS_MAINBUS:
		ma = aux;
		if (bp->val[0] == ma->ma_address)
			return (1);
		break;
	case BUSCLASS_SBUS:
		sa = aux;
		if (bp->val[0] == sa->sa_slot && bp->val[1] == sa->sa_offset)
			return (1);
		break;
	default:
		break;
	}

	if (bp->val[0] == -1 && bp->val[1] == dev->dv_unit)
		return (1);

	return (0);
}

void
nail_bootdev(dev, bp)
	struct device *dev;
	struct bootpath *bp;
{
	/*bp->dev = dev;	-* got it! */
	if (altbootdev != NULL)
		panic("device_register: already got a boot device: %s",
			altbootdev->dv_xname);
	altbootdev = dev;

	/*
	 * Clear current bootpath component, so we don't spuriously
	 * match similar instances on other busses, e.g. a disk on
	 * another SCSI bus with the same target.
	 */
	altbootpath_store(1, NULL);
}

void
device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	struct bootpath *bp = altbootpath_store(0, NULL);
	char *dvname = dev->dv_cfdata->cf_driver->cd_name;

	/*
	 * If device name does not match current bootpath component
	 * then there's nothing interesting to consider.
	 */
	if (bp == NULL)
		return;

	if (strcmp(bp->name, "espdma") == 0) {
		/* espdma special case */
		if (strcmp(dvname, "dma") != 0)
			return;
	} else if (strcmp(dvname, bp->name) != 0)
		return;

	if (strcmp(dvname, "obio") == 0 ||
	    strcmp(dvname, "vme") == 0 ||
	    strcmp(dvname, "iommu") == 0 ||
	    strcmp(dvname, "sbus") == 0 ||
	    strcmp(dvname, "xbox") == 0 ||
	    strcmp(dvname, "dma") == 0 ||
	    strcmp(dvname, "ledma") == 0 ||
	    strcmp(dvname, "espdma") == 0 ||
	    strcmp(dvname, "esp") == 0 ||
	    strcmp(dvname, "xdc") == 0 ||
	    strcmp(dvname, "xyc") == 0 ) {
		/*
		 * A bus or controller device of sorts. Check instance
		 * parameters and advance boot path on match.
		 */
		if (instance_match(dev, aux, bp) != 0) {
			altbootpath_store(1, bp + 1);
			return;
		}
	} else if (strcmp(dvname, "le") == 0) {
		/*
		 * LANCE ethernet device
		 */
		if (instance_match(dev, aux, bp) != 0) {
			nail_bootdev(dev, bp);
			return;
		}
	} else if (strcmp(dvname, "sd") == 0 || strcmp(dvname, "cd") == 0) {
		/*
		 * A SCSI disk or cd; retrieve target/lun information
		 * from parent and match with current bootpath component.
		 */
		struct scsipibus_attach_args *sa = aux;
		struct scsipi_link *sc_link = sa->sa_sc_link;
		struct scsibus_softc *sbsc =
			(struct scsibus_softc *)dev->dv_parent;
		u_int target = bp->val[0];
		u_int lun = bp->val[1];

		/*
		 * Bounds check; XXX - guess a reasonable target/lun bound.
		 */
		if (target >= 16 || lun >= 16) {
			printf("SCSI disk bootpath component not accepted: "
			       "target %u; lun %u\n", target, lun);
			return;
		}

		if (CPU_ISSUN4 && dvname[0] == 's' &&
		    target == 0 && sbsc->sc_link[0][0] == NULL) {
			/*
			 * disk unit 0 is magic: if there is actually no
			 * target 0 scsi device, the PROM will call
			 * target 3 `sd0'.
			 * XXX - what if someone puts a tape at target 0?
			 */
			target = 3;	/* remap to 3 */
			lun = 0;
		}

		if (CPU_ISSUN4C && dvname[0] == 's')
			target = sd_crazymap(target);

		if (sc_link->scsipi_scsi.target == target &&
		    sc_link->scsipi_scsi.lun == lun) {
			nail_bootdev(dev, bp);
			return;
		}
	} else if (strcmp("xd", dvname) == 0 || strcmp("xy", dvname) == 0) {

		/*
		 * XXX - x[dy]c attach args are not exported right now..
		 * XXX   we happen to know they look like this:
		 */
		struct xxxx_attach_args { int driveno; } *aap = aux;

		if (aap->driveno == bp->val[0]) {
			/* We've found the boot device */
			nail_bootdev(dev, bp);
			return;
		}
	} else {
		/*
		 * Generic match procedure.
		 */
		if (instance_match(dev, aux, bp) != 0) {
			nail_bootdev(dev, bp);
			return;
		}
	}

}
