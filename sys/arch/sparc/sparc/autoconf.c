/*	$NetBSD: autoconf.c,v 1.125 1999/10/04 19:27:04 pk Exp $ */

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
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/dkstat.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/msgbuf.h>
#include <sys/user.h>

#include <net/if.h>

#include <dev/cons.h>

#include <vm/vm.h>

#include <machine/bus.h>
#include <machine/promlib.h>
#include <machine/openfirm.h>
#include <machine/autoconf.h>
#include <machine/bootinfo.h>

#include <machine/oldmon.h>
#include <machine/idprom.h>
#include <sparc/sparc/memreg.h>
#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <machine/pmap.h>
#include <sparc/sparc/asm.h>
#include <sparc/sparc/cpuvar.h>
#include <sparc/sparc/timerreg.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif


/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	fbnode;		/* node ID of ROM's console frame buffer */
int	optionsnode;	/* node ID of ROM's options */
int	mmu_3l;		/* SUN4_400 models have a 3-level MMU */

#ifdef KGDB
extern	int kgdb_debug_panic;
#endif
extern void *bootinfo;

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
static	void bootpath_fake __P((struct bootpath *, char *));
static	void bootpath_print __P((struct bootpath *));
int	find_cpus __P((void));

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

int
find_cpus()
{
#if defined(MULTIPROCESSOR)
	int node, n;

	/* We only consider sun4m class multi-processor machines */
	if (!CPU_ISSUN4M)
		return (1);

	n = 0;
	node = findroot();
	for (node = firstchild(node); node; node = nextsibling(node)) {
		if (strcmp(getpropstring(node, "device_type"), "cpu") == 0)
			n++;
	}
	return (n);
#else
	return (1);
#endif
}

/*
 * Convert hex ASCII string to a value.  Returns updated pointer.
 * Depends on ASCII order (this *is* machine-dependent code, you know).
 */
static char *
str2hex(str, vp)
	char *str;
	int *vp;
{
	int v, c;

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
 * locore.s code calls bootstrap() just before calling main(), after double
 * mapping the kernel to high memory and setting up the trap base register.
 * We must finish mapping the kernel properly and glean any bootstrap info.
 */
void
bootstrap()
{
	extern struct user *proc0paddr;
#ifdef DDB
	struct btinfo_symtab *bi_sym;
#endif
	prom_init();

	/* Find the number of CPUs as early as possible */
	ncpu = find_cpus();

	/* Attach user structure to proc0 */
	proc0.p_addr = proc0paddr;

	cpuinfo.master = 1;
	getcpuinfo(&cpuinfo, 0);

	pmap_bootstrap(cpuinfo.mmu_ncontext,
		       cpuinfo.mmu_nregion,
		       cpuinfo.mmu_nsegment);

	/* Moved zs_kgdb_init() to dev/zs.c:consinit(). */
#ifdef DDB
	db_machine_init();
	if ((bi_sym = lookup_bootinfo(BTINFO_SYMTAB)) != NULL) {
	   	bi_sym->ssym += KERNBASE; 
	   	bi_sym->esym += KERNBASE; 
		ddb_init(bi_sym->nsym, (int *)bi_sym->ssym,
		    (int *)bi_sym->esym);
	} else {
		/*
		 * Compatibility, will go away.
		 */
		extern int end;
		extern int *esym;

		ddb_init(*(int *)&end, ((int *)&end) + 1, esym);
	}
#endif

	/*
	 * On sun4ms we have to do some nasty stuff here. We need to map
	 * in the interrupt registers (since we need to find out where
	 * they are from the PROM, since they aren't in a fixed place), and
	 * disable all interrupts. We can't do this easily from locore
	 * since the PROM is ugly to use from assembly. We also need to map
	 * in the counter registers because we can't disable the level 14
	 * (statclock) interrupt, so we need a handler early on (ugh).
	 *
	 * NOTE: We *demand* the psl to stay at splhigh() at least until
	 * we get here. The system _cannot_ take interrupts until we map
	 * the interrupt registers.
	 */

#if defined(SUN4M)
	/* First we'll do the interrupt registers */
	if (CPU_ISSUN4M) {
		int node;
		int nvaddrs, *vaddrs, vstore[10];
		u_int pte;
		int i;
		extern void setpte4m __P((u_int, u_int));

		if ((node = prom_opennode("/obio/interrupt")) == 0 &&
		    (node = prom_finddevice("/obio/interrupt")) == 0)
			panic("bootstrap: could not get interrupt "
			      "node from prom");

		vaddrs = vstore;
		nvaddrs = sizeof(vstore)/sizeof(vstore[0]);
		if (getprop(node, "address", sizeof(int),
			    &nvaddrs, (void **)&vaddrs) != 0) {
			printf("bootstrap: could not get interrupt properties");
			prom_halt();
		}
		if (nvaddrs < 2 || nvaddrs > 5) {
			printf("bootstrap: cannot handle %d interrupt regs\n",
				nvaddrs);
			prom_halt();
		}

		for (i = 0; i < nvaddrs - 1; i++) {

			pte = getpte4m((u_int)vaddrs[i]);
			if ((pte & SRMMU_TETYPE) != SRMMU_TEPTE) {
			    panic("bootstrap: PROM has invalid mapping for "
				  "processor interrupt register %d",i);
			    prom_halt();
			}
			pte |= PPROT_S;

			/* Duplicate existing mapping */

			setpte4m(PI_INTR_VA + (_MAXNBPG * i), pte);
		}
		cpuinfo.intreg_4m = (struct icr_pi *)(PI_INTR_VA);

		/*
		 * That was the processor register...now get system register;
		 * it is the last returned by the PROM
		 */
		pte = getpte4m((u_int)vaddrs[i]);
		if ((pte & SRMMU_TETYPE) != SRMMU_TEPTE)
		    panic("bootstrap: PROM has invalid mapping for system "
			  "interrupt register");
		pte |= PPROT_S;

		setpte4m(SI_INTR_VA, pte);

		/* Now disable interrupts */
		ienab_bis(SINTR_MA);

		/* Send all interrupts to primary processor */
		*((u_int *)ICR_ITR) = 0;

#ifdef DEBUG
/*		printf("SINTR: mask: 0x%x, pend: 0x%x\n", *(int*)ICR_SI_MASK,
		       *(int*)ICR_SI_PEND);
*/
#endif
	}
#endif /* SUN4M */

	if (CPU_ISSUN4OR4C) {
		/* Map Interrupt Enable Register */
		pmap_enter(pmap_kernel(), INTRREG_VA,
		    INT_ENABLE_REG_PHYSADR | PMAP_NC | PMAP_OBIO,
		    VM_PROT_READ | VM_PROT_WRITE, 1,
		    VM_PROT_READ | VM_PROT_WRITE);
		/* Disable all interrupts */
		*((unsigned char *)INTRREG_VA) = 0;
	}
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
	char *cp, *pp;
	struct bootpath *bp;

	/*
	 * Grab boot path from PROM and split into `bootpath' components.
	 */
	bzero(bootpath, sizeof(bootpath));
	bp = bootpath;
	cp = prom_getbootpath();
	switch (prom_version()) {
	case PROM_OLDMON:
	case PROM_OBP_V0:
		/*
		 * Build fake bootpath.
		 */
		if (cp != NULL)
			bootpath_fake(bp, cp);
		break;
	case PROM_OBP_V2:
	case PROM_OBP_V3:
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
		break;
	}

	bootpath_print(bootpath);

	/* Setup pointer to boot flags */
	cp = prom_getbootargs();
	if (cp == NULL)
		return;

	/* Skip any whitespace */
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

		default:
			printf("unknown option `%c'\n", *cp);
		}
	}
}

/*
 * Fake a ROM generated bootpath.
 * The argument `cp' points to a string such as "xd(0,0,0)netbsd"
 */

static void
bootpath_fake(bp, cp)
	struct bootpath *bp;
	char *cp;
{
	char *pp;
	int v0val[3];

#define BP_APPEND(BP,N,V0,V1,V2) { \
	strcpy((BP)->name, N); \
	(BP)->val[0] = (V0); \
	(BP)->val[1] = (V1); \
	(BP)->val[2] = (V2); \
	(BP)++; \
	nbootpath++; \
}

#if defined(SUN4)
	if (CPU_ISSUN4M) {
		printf("twas brillig..\n");
		return;
	}
#endif

	pp = cp + 2;
	v0val[0] = v0val[1] = v0val[2] = 0;
	if (*pp == '(' 					/* for vi: ) */
 	    && *(pp = str2hex(++pp, &v0val[0])) == ','
	    && *(pp = str2hex(++pp, &v0val[1])) == ',')
		(void)str2hex(++pp, &v0val[2]);

#if defined(SUN4)
	if (CPU_ISSUN4) {
		char tmpname[8];

		/*
		 *  xylogics VME dev: xd, xy, xt
		 *  fake looks like: /vme0/xdc0/xd@1,0
		 */
		if (cp[0] == 'x') {
			if (cp[1] == 'd') {/* xd? */
				BP_APPEND(bp, "vme", -1, 0, 0);
			} else {
				BP_APPEND(bp, "vme", -1, 0, 0);
			}
			sprintf(tmpname,"x%cc", cp[1]); /* e.g. `xdc' */
			BP_APPEND(bp, tmpname, -1, v0val[0], 0);
			sprintf(tmpname,"x%c", cp[1]); /* e.g. `xd' */
			BP_APPEND(bp, tmpname, v0val[1], v0val[2], 0);
			return;
		}

		/*
		 * ethernet: ie, le (rom supports only obio?)
		 * fake looks like: /obio0/le0
		 */
		if ((cp[0] == 'i' || cp[0] == 'l') && cp[1] == 'e')  {
			BP_APPEND(bp, "obio", -1, 0, 0);
			sprintf(tmpname,"%c%c", cp[0], cp[1]);
			BP_APPEND(bp, tmpname, -1, 0, 0);
			return;
		}

		/*
		 * scsi: sd, st, sr
		 * assume: 4/100 = sw: /obio0/sw0/sd@0,0:a
		 * 4/200 & 4/400 = si/sc: /vme0/si0/sd@0,0:a
 		 * 4/300 = esp: /obio0/esp0/sd@0,0:a
		 * (note we expect sc to mimic an si...)
		 */
		if (cp[0] == 's' &&
			(cp[1] == 'd' || cp[1] == 't' || cp[1] == 'r')) {

			int  target, lun;

			switch (cpuinfo.cpu_type) {
			case CPUTYP_4_200:
			case CPUTYP_4_400:
				BP_APPEND(bp, "vme", -1, 0, 0);
				BP_APPEND(bp, "si", -1, v0val[0], 0);
				break;
			case CPUTYP_4_100:
				BP_APPEND(bp, "obio", -1, 0, 0);
				BP_APPEND(bp, "sw", -1, v0val[0], 0);
				break;
			case CPUTYP_4_300:
				BP_APPEND(bp, "obio", -1, 0, 0);
				BP_APPEND(bp, "esp", -1, v0val[0], 0);
				break;
			default:
				panic("bootpath_fake: unknown system type %d",
				      cpuinfo.cpu_type);
			}
			/*
			 * Deal with target/lun encodings.
			 * Note: more special casing in dk_establish().
			 *
			 * We happen to know how `prom_revision' is
			 * constructed from `monID[]' on sun4 proms...
			 */
			if (prom_revision() > '1') {
				target = v0val[1] >> 3; /* new format */
				lun    = v0val[1] & 0x7;
			} else {
				target = v0val[1] >> 2; /* old format */
				lun    = v0val[1] & 0x3;
			}
			sprintf(tmpname, "%c%c", cp[0], cp[1]);
			BP_APPEND(bp, tmpname, target, lun, v0val[2]);
			return;
		}

		return; /* didn't grok bootpath, no change */
	}
#endif /* SUN4 */

#if defined(SUN4C)
	/*
	 * sun4c stuff
	 */

	/*
	 * floppy: fd
	 * fake looks like: /fd@0,0:a
	 */
	if (cp[0] == 'f' && cp[1] == 'd') {
		/*
		 * Assume `fd(c,u,p)' means:
		 * partition `p' on floppy drive `u' on controller `c'
		 */
		BP_APPEND(bp, "fd", v0val[0], v0val[1], v0val[2]);
		return;
	}

	/*
	 * ethernet: le
	 * fake looks like: /sbus0/le0
	 */
	if (cp[0] == 'l' && cp[1] == 'e') {
		BP_APPEND(bp, "sbus", -1, 0, 0);
		BP_APPEND(bp, "le", -1, v0val[0], 0);
		return;
	}

	/*
	 * scsi: sd, st, sr
	 * fake looks like: /sbus0/esp0/sd@3,0:a
	 */
	if (cp[0] == 's' && (cp[1] == 'd' || cp[1] == 't' || cp[1] == 'r')) {
		char tmpname[8];
		int  target, lun;

		BP_APPEND(bp, "sbus", -1, 0, 0);
		BP_APPEND(bp, "esp", -1, v0val[0], 0);
		if (cp[1] == 'r')
			sprintf(tmpname, "cd"); /* netbsd uses 'cd', not 'sr'*/
		else
			sprintf(tmpname,"%c%c", cp[0], cp[1]);
		/* XXX - is TARGET/LUN encoded in v0val[1]? */
		target = v0val[1];
		lun = 0;
		BP_APPEND(bp, tmpname, target, lun, v0val[2]);
		return;
	}
#endif /* SUN4C */


	/*
	 * unknown; return
	 */

#undef BP_APPEND
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
	char *propval;
	char buf[32];

	if (!CPU_ISSUN4 && prom_version() < 2) {
		/*
		 * Machines with real v0 proms have an `s[dt]-targets' property
		 * which contains the mapping for us to use. v2 proms do not
		 * require remapping.
		 */
		propval = getpropstringA(optionsnode, prop, buf, sizeof(buf));
		if (propval == NULL || strlen(propval) != 8) {
 build_default_map:
			printf("WARNING: %s map is bogus, using default\n",
				prop);
			for (i = 0; i < 8; ++i)
				map[i] = i;
			i = map[0];
			map[0] = map[3];
			map[3] = i;
			return;
		}
		for (i = 0; i < 8; ++i) {
			map[i] = propval[i] - '0';
			if (map[i] < 0 ||
			    map[i] >= 8)
				goto build_default_map;
		}
	} else {
		/*
		 * Set up the identity mapping for old sun4 monitors
		 * and v[2-] OpenPROMs. Note: dkestablish() does the
		 * SCSI-target juggling for sun4 monitors.
		 */
		for (i = 0; i < 8; ++i)
			map[i] = i;
	}
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
cpu_configure()
{
	extern struct user *proc0paddr;	/* XXX see below */

	/* build the bootpath */
	bootpath_build();

#if defined(SUN4)
	if (CPU_ISSUN4) {
#define MEMREG_PHYSADDR	0xf4000000
		bus_space_handle_t bh;
		bus_addr_t paddr = MEMREG_PHYSADDR;

		if (cpuinfo.cpu_type == CPUTYP_4_100)
			/* Clear top bits of physical address on 4/100 */
			paddr &= ~0xf0000000;

		if (obio_find_rom_map(paddr, PMAP_OBIO, NBPG, &bh) != 0)
			panic("configure: ROM hasn't mapped memreg!");

		par_err_reg = (volatile int *)bh;
	}
#endif
#if defined(SUN4C)
	if (CPU_ISSUN4C) {
		char *cp, buf[32];
		int node = findroot();
		cp = getpropstringA(node, "device_type", buf, sizeof buf);
		if (strcmp(cp, "cpu") != 0)
			panic("PROM root device type = %s (need CPU)\n", cp);
	}
#endif

	prom_setcallback(sync_crash);

	/* Enable device interrupts */
#if defined(SUN4M)
	if (CPU_ISSUN4M)
		ienab_bic(SINTR_MA);
#endif
#if defined(SUN4) || defined(SUN4C)
	if (CPU_ISSUN4OR4C)
		ienab_bis(IE_ALLIE);
#endif

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("mainbus not configured");

	/*
	 * XXX Re-zero proc0's user area, to nullify the effect of the
	 * XXX stack running into it during auto-configuration.
	 * XXX - should fix stack usage.
	 */
	bzero(proc0paddr, sizeof(struct user));

	(void)spl0();
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

#if 1
	/*
	 * Old bootpath code no longer works now that SCSI autoconfiguration
	 * can be delayed.  device_register() is the One True Way.
	 */
	bootdv = altbootdev;
#else
	if (bootdv != altbootdev) {
		int c;
		printf("device_register boot device mismatch\n");
		printf("\tbootdv=%s\n",
			bootdv==NULL?"NOT FOUND":bootdv->dv_xname);
		printf("\taltbootdev=%s\n",
			altbootdev==NULL?"NOT FOUND":altbootdev->dv_xname);
		printf("RETURN to continue ");
		cnpollc(1);
		while ((c = cngetc()) != '\r' && c != '\n');
		printf("\n");
		cnpollc(0);
	}
#endif
	setroot(bootdv, bootpartition);
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
	int freq;
{
	char *p;
	static char buf[10];

	freq /= 1000;
	sprintf(buf, "%d", freq / 1000);
	freq %= 1000;
	if (freq) {
		freq += 1000;	/* now in 1000..1999 */
		p = buf + strlen(buf);
		sprintf(p, "%d", freq);
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
	if (ma->ma_paddr)
		printf(" %saddr 0x%lx", ma->ma_iospace ? "io" : "",
			(long)ma->ma_paddr);
	if (ma->ma_pri)
		printf(" ipl %d", ma->ma_pri);
	return (UNCONF);
}

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
 * Helper routines to get some of the more common properties. These
 * only get the first item in case the property value is an array.
 * Drivers that "need to know it all" can call getprop() directly.
 */
#if defined(SUN4C) || defined(SUN4M)
static int	getprop_reg1 __P((int, struct openprom_addr *));
static int	getprop_intr1 __P((int, int *));
static int	getprop_address1 __P((int, void **));
#endif

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
#if defined(SUN4C) || defined(SUN4M)
	const char *const *ssp, *sp = NULL;
	int node0, node;
	const char *const *openboot_special;
#endif

#if defined(SUN4C)
	static const char *const openboot_special4c[] = {
		/* find these first (end with empty string) */
		"memory-error",	/* as early as convenient, in case of error */
		"eeprom",
		"counter-timer",
		"auxiliary-io",
		"",

		/* ignore these (end with NULL) */
		"aliases",
		"interrupt-enable",
		"memory",
		"openprom",
		"options",
		"packages",
		"virtual-memory",
		NULL
	};
#else
#define openboot_special4c	((void *)0)
#endif
#if defined(SUN4M)
	static const char *const openboot_special4m[] = {
		/* find these first */
		"obio",		/* smart enough to get eeprom/etc mapped */
		"",

		/* ignore these (end with NULL) */
		/*
		 * These are _root_ devices to ignore. Others must be handled
		 * elsewhere.
		 */
		"SUNW,sx",		/* XXX: no driver for SX yet */
		"virtual-memory",
		"aliases",
		"memory",
		"openprom",
		"options",
		"packages",
		/* we also skip any nodes with device_type == "cpu" */
		NULL
	};
#else
#define openboot_special4m	((void *)0)
#endif

	if (CPU_ISSUN4)
		printf(": SUN-4/%d series\n", cpuinfo.classlvl);
	else
		printf(": %s\n", getpropstringA(findroot(), "name",
						namebuf, sizeof(namebuf)));

	/* Establish the first component of the boot path */
	altbootpath_store(1, bootpath);

	/*
	 * Locate and configure the ``early'' devices.  These must be
	 * configured before we can do the rest.  For instance, the
	 * EEPROM contains the Ethernet address for the LANCE chip.
	 * If the device cannot be located or configured, panic.
	 */

#if defined(SUN4)
	if (CPU_ISSUN4) {

		bzero(&ma, sizeof(ma));
		/* Configure the CPU. */
		ma.ma_bustag = &mainbus_space_tag;
		ma.ma_dmatag = &mainbus_dma_tag;
		ma.ma_name = "cpu";
		if (config_found(dev, (void *)&ma, mbprint) == NULL)
			panic("cpu missing");

		ma.ma_bustag = &mainbus_space_tag;
		ma.ma_dmatag = &mainbus_dma_tag;
		ma.ma_name = "obio";
		ma.ma_bp = bootpath;
		if (config_found(dev, (void *)&ma, mbprint) == NULL)
			panic("obio missing");

		ma.ma_bustag = &mainbus_space_tag;
		ma.ma_dmatag = &mainbus_dma_tag;
		ma.ma_name = "vme";
		ma.ma_bp = bootpath;
		(void)config_found(dev, (void *)&ma, mbprint);
		return;
	}
#endif

/*
 * The rest of this routine is for OBP machines exclusively.
 */
#if defined(SUN4C) || defined(SUN4M)

	openboot_special = CPU_ISSUN4M
				? openboot_special4m
				: openboot_special4c;

	node = findroot();

	/* the first early device to be configured is the cpu */
	if (CPU_ISSUN4M) {
		/* XXX - what to do on multiprocessor machines? */
		const char *cp;

		for (node = firstchild(node); node; node = nextsibling(node)) {
			cp = getpropstringA(node, "device_type",
					    namebuf, sizeof namebuf);
			if (strcmp(cp, "cpu") == 0) {
				bzero(&ma, sizeof(ma));
				ma.ma_bustag = &mainbus_space_tag;
				ma.ma_dmatag = &mainbus_dma_tag;
				ma.ma_node = node;
				ma.ma_name = "cpu";
				config_found(dev, (void *)&ma, mbprint);
			}
		}
	} else if (CPU_ISSUN4C) {
		bzero(&ma, sizeof(ma));
		ma.ma_bustag = &mainbus_space_tag;
		ma.ma_dmatag = &mainbus_dma_tag;
		ma.ma_node = node;
		ma.ma_name = "cpu";
		config_found(dev, (void *)&ma, mbprint);
	}


	node = findroot();	/* re-init root node */

	if (prom_version() <= 2)
		/* remember which frame buffer, if any, is to be `/dev/fb' */
		fbnode = getpropint(node, "fb", 0);

	/* Find the "options" node */
	node0 = firstchild(node);
	optionsnode = findnode(node0, "options");
	if (optionsnode == 0)
		panic("no options in OPENPROM");

	for (ssp = openboot_special; *(sp = *ssp) != 0; ssp++) {
		struct openprom_addr romreg;

		if ((node = findnode(node0, sp)) == 0) {
			printf("could not find %s in OPENPROM\n", sp);
			panic(sp);
		}

		bzero(&ma, sizeof ma);
		ma.ma_bustag = &mainbus_space_tag;
		ma.ma_dmatag = &mainbus_dma_tag;
		ma.ma_name = getpropstringA(node, "name",
					    namebuf, sizeof namebuf);
		ma.ma_node = node;
		if (getprop_reg1(node, &romreg) != 0)
			continue;

		ma.ma_paddr = (bus_addr_t)romreg.oa_base;
		ma.ma_iospace = (bus_type_t)romreg.oa_space;
		ma.ma_size = romreg.oa_size;
		if (getprop_intr1(node, &ma.ma_pri) != 0)
			continue;
		if (getprop_address1(node, &ma.ma_promvaddr) != 0)
			continue;

		/* Start at the beginning of the bootpath */
		ma.ma_bp = bootpath;

		if (config_found(dev, (void *)&ma, mbprint) == NULL)
			panic(sp);
	}

	/*
	 * Configure the rest of the devices, in PROM order.  Skip
	 * PROM entries that are not for devices, or which must be
	 * done before we get here.
	 */
	for (node = node0; node; node = nextsibling(node)) {
		const char *cp;
		struct openprom_addr romreg;

#if defined(SUN4M)
		if (CPU_ISSUN4M) {	/* skip the CPUs */
			if (strcmp(getpropstringA(node, "device_type",
						  namebuf, sizeof namebuf),
				   "cpu") == 0)
				continue;
		}
#endif
		cp = getpropstringA(node, "name", namebuf, sizeof namebuf);
		for (ssp = openboot_special; (sp = *ssp) != NULL; ssp++)
			if (strcmp(cp, sp) == 0)
				break;
		if (sp != NULL)
			continue; /* an "early" device already configured */

		bzero(&ma, sizeof ma);
		ma.ma_bustag = &mainbus_space_tag;
		ma.ma_dmatag = &mainbus_dma_tag;
		ma.ma_name = getpropstringA(node, "name",
					    namebuf, sizeof namebuf);
		ma.ma_node = node;
		if (getprop_reg1(node, &romreg) != 0)
			continue;

		ma.ma_paddr = (bus_addr_t)romreg.oa_base;
		ma.ma_iospace = (bus_type_t)romreg.oa_space;
		ma.ma_size = romreg.oa_size;

		if (getprop_intr1(node, &ma.ma_pri) != 0)
			continue;

		if (getprop_address1(node, &ma.ma_promvaddr) != 0)
			continue;

		/* Start at the beginning of the bootpath */
		ma.ma_bp = bootpath;

		(void) config_found(dev, (void *)&ma, mbprint);
	}
#endif /* SUN4C || SUN4M */
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

#if defined(SUN4)
#define ZS0_PHYS	0xf1000000
#define ZS1_PHYS	0xf0000000
#define ZS2_PHYS	0xe0000000

	if (CPU_ISSUN4) {
		bus_space_handle_t bh;
		bus_addr_t paddr;

		switch (zs) {
		case 0:
			paddr = ZS0_PHYS;
			break;
		case 1:
			paddr = ZS1_PHYS;
			break;
		case 2:
			paddr = ZS2_PHYS;
			break;
		default:
			panic("findzs: unknown zs device %d", zs);
		}

		if (cpuinfo.cpu_type == CPUTYP_4_100)
			/* Clear top bits of physical address on 4/100 */
			paddr &= ~0xf0000000;

		if (obio_find_rom_map(paddr, PMAP_OBIO, NBPG, &bh) != 0)
			panic("findzs: can't map zs%d registers", zs);

		return ((void *)bh);
	}
#endif

#if defined(SUN4C) || defined(SUN4M)
	if (CPU_ISSUN4COR4M) {
		int node;

		node = firstchild(findroot());
		if (CPU_ISSUN4M) { /* zs is in "obio" tree on Sun4M */
			node = findnode(node, "obio");
			if (node == 0)
				panic("findzs: no obio node");
			node = firstchild(node);
		}
		while ((node = findnode(node, "zs")) != 0) {
			int nvaddrs, *vaddrs, vstore[10];

			if (getpropint(node, "slave", -1) != zs) {
				node = nextsibling(node);
				continue;
			}

			/*
			 * On some machines (e.g. the Voyager), the zs
			 * device has multi-valued register properties.
			 */
			vaddrs = vstore;
			nvaddrs = sizeof(vstore)/sizeof(vstore[0]);
			if (getprop(node, "address", sizeof(int),
				    &nvaddrs, (void **)&vaddrs) != 0) {
				panic("findzs: zs%d not mapped by PROM", zs);
			}
			return ((void *)vaddrs[0]);
		}
	}
#endif
	panic("findzs: cannot find zs%d", zs);
	/* NOTREACHED */
}

int
makememarr(ap, max, which)
	struct memarr *ap;
	int max, which;
{
	struct v2rmi {
		int	zero;
		int	addr;
		int	len;
	} v2rmi[200];		/* version 2 rom meminfo layout */
#define	MAXMEMINFO ((int)sizeof(v2rmi) / (int)sizeof(*v2rmi))
	void *p;

	struct v0mlist *mp;
	int i, node, len;
	char *prop;

	switch (prom_version()) {
		struct promvec *promvec;
		struct om_vector *oldpvec;
	case PROM_OLDMON:
		oldpvec = (struct om_vector *)PROM_BASE;
		switch (which) {
		case MEMARR_AVAILPHYS:
			ap[0].addr = 0;
			ap[0].len = *oldpvec->memoryAvail;
			break;
		case MEMARR_TOTALPHYS:
			ap[0].addr = 0;
			ap[0].len = *oldpvec->memorySize;
			break;
		default:
			printf("pre_panic: makememarr");
			break;
		}
		i = (1);
		break;

	case PROM_OBP_V0:
		/*
		 * Version 0 PROMs use a linked list to describe these
		 * guys.
		 */
		promvec = romp;
		switch (which) {
		case MEMARR_AVAILPHYS:
			mp = *promvec->pv_v0mem.v0_physavail;
			break;

		case MEMARR_TOTALPHYS:
			mp = *promvec->pv_v0mem.v0_phystot;
			break;

		default:
			panic("makememarr");
		}
		for (i = 0; mp != NULL; mp = mp->next, i++) {
			if (i >= max)
				goto overflow;
			ap->addr = (u_int)mp->addr;
			ap->len = mp->nbytes;
			ap++;
		}
		break;

	default:
		printf("makememarr: hope version %d PROM is like version 2\n",
			prom_version());
		/* FALLTHROUGH */

        case PROM_OBP_V3:
	case PROM_OBP_V2:
		/*
		 * Version 2 PROMs use a property array to describe them.
		 */

		/* Consider emulating `OF_finddevice' */
		node = findnode(firstchild(findroot()), "memory");
		goto case_common;

	case PROM_OPENFIRM:
		node = OF_finddevice("/memory");

	case_common:
		if (node == 0)
			panic("makememarr: cannot find \"memory\" node");

		if (max > MAXMEMINFO) {
			printf("makememarr: limited to %d\n", MAXMEMINFO);
			max = MAXMEMINFO;
		}

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

		len = MAXMEMINFO;
		p = v2rmi;
		if (getprop(node, prop, sizeof(struct v2rmi), &len, &p) != 0)
			panic("makememarr: cannot get property");

		for (i = 0; i < len; i++) {
			if (i >= max)
				goto overflow;
			ap->addr = v2rmi[i].addr;
			ap->len = v2rmi[i].len;
			ap++;
		}
		break;
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

#if defined(SUN4C) || defined(SUN4M)
int
getprop_reg1(node, rrp)
	int node;
	struct openprom_addr *rrp;
{
	int error, n;
	struct openprom_addr *rrp0 = NULL;
	char buf[32];

	error = getprop(node, "reg", sizeof(struct openprom_addr),
			&n, (void **)&rrp0);
	if (error != 0) {
		if (error == ENOENT &&
		    strcmp(getpropstringA(node, "device_type", buf, sizeof buf),
			   "hierarchical") == 0) {
			bzero(rrp, sizeof(struct openprom_addr));
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
	struct rom_intr *rip = NULL;

	error = getprop(node, "intr", sizeof(struct rom_intr),
			&n, (void **)&rip);
	if (error != 0) {
		if (error == ENOENT) {
			*ip = 0;
			error = 0;
		}
		return (error);
	}

	*ip = rip[0].int_pri & 0xf;
	free(rip, M_DEVBUF);
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
		if (error == ENOENT) {
			*vpp = 0;
			error = 0;
		}
		return (error);
	}

	*vpp = vp[0];
	free(vp, M_DEVBUF);
	return (0);
}
#endif

#ifdef RASTERCONSOLE
/*
 * Try to figure out where the PROM stores the cursor row & column
 * variables.  Returns nonzero on error.
 */
int
romgetcursoraddr(rowp, colp)
	int **rowp, **colp;
{
	char buf[100];

	/*
	 * line# and column# are global in older proms (rom vector < 2)
	 * and in some newer proms.  They are local in version 2.9.  The
	 * correct cutoff point is unknown, as yet; we use 2.9 here.
	 */
	if (prom_version() < 2 || prom_revision() < 0x00020009)
		sprintf(buf,
		    "' line# >body >user %lx ! ' column# >body >user %lx !",
		    (u_long)rowp, (u_long)colp);
	else
		sprintf(buf,
		    "stdout @ is my-self addr line# %lx ! addr column# %lx !",
		    (u_long)rowp, (u_long)colp);
	*rowp = *colp = NULL;
	prom_interpret(buf);
	return (*rowp == NULL || *colp == NULL);
}
#endif

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
 */
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <sparc/sparc/iommuvar.h>

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
	{ "dma",	BUSCLASS_SBUS },
	{ "esp",	BUSCLASS_SBUS },
	{ "espdma",	BUSCLASS_SBUS },
	{ "isp",	BUSCLASS_SBUS },
	{ "ledma",	BUSCLASS_SBUS },
	{ "lebuffer",	BUSCLASS_SBUS },
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
		if (bp->val[0] == ma->ma_iospace && bp->val[1] == ma->ma_paddr)
			return (1);
		break;
	case BUSCLASS_SBUS:
		sa = aux;
		if (bp->val[0] == sa->sa_slot && bp->val[1] == sa->sa_offset)
			return (1);
		break;
	case BUSCLASS_IOMMU:
		iom = aux;
		if (bp->val[0] == iom->iom_reg[0].ior_iospace &&
		    bp->val[1] == iom->iom_reg[0].ior_pa)
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
	    strcmp(dvname, "lebuffer") == 0 ||
	    strcmp(dvname, "espdma") == 0 ||
	    strcmp(dvname, "esp") == 0 ||
	    strcmp(dvname, "isp") == 0 ||
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

/*
 * lookup_bootinfo:
 * Look up information in bootinfo of boot loader.
 */
void *
lookup_bootinfo(type)
	int type;
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
