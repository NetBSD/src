/*	$NetBSD: autoconf.c,v 1.242.8.2 2012/08/08 15:51:11 martin Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.242.8.2 2012/08/08 15:51:11 martin Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_modular.h"
#include "opt_multiprocessor.h"
#include "opt_sparc_arch.h"

#include "scsibus.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/endian.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/msgbuf.h>
#include <sys/boot_flag.h>
#include <sys/ksyms.h>
#include <sys/userconf.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <dev/cons.h>

#include <uvm/uvm_extern.h>

#include <machine/pcb.h>
#include <sys/bus.h>
#include <machine/promlib.h>
#include <machine/autoconf.h>
#include <machine/bootinfo.h>

#include <sparc/sparc/memreg.h>
#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <sparc/sparc/asm.h>
#include <sparc/sparc/cpuvar.h>
#include <sparc/sparc/timerreg.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>
#include <sparc/sparc/msiiepreg.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#include <ddb/ddbvar.h>
#endif

#include "ksyms.h"

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */

#ifdef KGDB
extern	int kgdb_debug_panic;
#endif
extern void *bootinfo;

#if !NKSYMS && !defined(DDB) && !defined(MODULAR)
void bootinfo_relocate(void *);
#endif

static	const char *str2hex(const char *, int *);
static	int mbprint(void *, const char *);
static	void crazymap(const char *, int *);
int	st_crazymap(int);
int	sd_crazymap(int);
void	sync_crash(void);
int	mainbus_match(device_t, cfdata_t, void *);
static	void mainbus_attach(device_t, device_t, void *);

struct	bootpath bootpath[8];
int	nbootpath;
static	void bootpath_build(void);
static	void bootpath_fake(struct bootpath *, const char *);
static	void bootpath_print(struct bootpath *);
static	struct bootpath	*bootpath_store(int, struct bootpath *);
int	find_cpus(void);
char	machine_model[100];

#ifdef DEBUG
#define ACDB_BOOTDEV	0x1
#define	ACDB_PROBE	0x2
int autoconf_debug = 0;
#define DPRINTF(l, s)   do { if (autoconf_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

/*
 * Most configuration on the SPARC is done by matching OPENPROM Forth
 * device names with our internal names.
 */
int
matchbyname(struct device *parent, struct cfdata *cf, void *aux)
{

	printf("%s: WARNING: matchbyname\n", cf->cf_name);
	return (0);
}

/*
 * Get the number of CPUs in the system and the CPUs' SPARC architecture
 * version. We need this information early in the boot process.
 */
int
find_cpus(void)
{
	int n;
#if defined(SUN4M) || defined(SUN4D)
	int node;
#endif
	/*
	 * Set default processor architecture version
	 *
	 * All sun4 and sun4c platforms have v7 CPUs;
	 * sun4m may have v7 (Cyrus CY7C601 modules) or v8 CPUs (all
	 * other models, presumably).
	 */
	cpu_arch = 7;

	/* On sun4 and sun4c we support only one CPU */
	if (!CPU_ISSUN4M && !CPU_ISSUN4D)
		return (1);

	n = 0;
#if defined(SUN4M)
	node = findroot();
	for (node = firstchild(node); node; node = nextsibling(node)) {
		if (strcmp(prom_getpropstring(node, "device_type"), "cpu") != 0)
			continue;
		if (n++ == 0)
			cpu_arch = prom_getpropint(node, "sparc-version", 7);
	}
#endif /* SUN4M */
#if defined(SUN4D)
	node = findroot();
	for (node = firstchild(node); node; node = nextsibling(node)) {
		int unode;

		if (strcmp(prom_getpropstring(node, "name"), "cpu-unit") != 0)
				continue;
		for (unode = firstchild(node); unode;
		     unode = nextsibling(unode)) {
			if (strcmp(prom_getpropstring(unode, "device_type"),
				   "cpu") != 0)
				continue;
			if (n++ == 0)
				cpu_arch = prom_getpropint(unode,
							   "sparc-version", 7);
		}
	}
#endif

	return (n);
}

/*
 * Convert hex ASCII string to a value.  Returns updated pointer.
 * Depends on ASCII order (this *is* machine-dependent code, you know).
 */
static const char *
str2hex(const char *str, int *vp)
{
	int v, c;

	for (v = 0;; v = v * 16 + c, str++) {
		c = (u_char)*str;
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


#if defined(SUN4M)
#if !defined(MSIIEP)
static void bootstrap4m(void);
#else
static void bootstrapIIep(void);
#endif
#endif /* SUN4M */

/*
 * locore.s code calls bootstrap() just before calling main(), after double
 * mapping the kernel to high memory and setting up the trap base register.
 * We must finish mapping the kernel properly and glean any bootstrap info.
 */
void
bootstrap(void)
{
	extern uint8_t u0[];
#if NKSYMS || defined(DDB) || defined(MODULAR)
	struct btinfo_symtab *bi_sym;
#else
	extern int end[];
#endif
	struct btinfo_boothowto *bi_howto;

	prom_init();

	/* Find the number of CPUs as early as possible */
	sparc_ncpus = find_cpus();
	uvm_lwp_setuarea(&lwp0, (vaddr_t)u0);

	cpuinfo.master = 1;
	getcpuinfo(&cpuinfo, 0);
	curlwp = &lwp0;

#if defined(SUN4M) || defined(SUN4D)
	/* Switch to sparc v8 multiply/divide functions on v8 machines */
	if (cpu_arch == 8) {
		extern void sparc_v8_muldiv(void);
		sparc_v8_muldiv();
	}
#endif /* SUN4M || SUN4D */

#if !NKSYMS && !defined(DDB) && !defined(MODULAR)
	/*
	 * We want to reuse the memory where the symbols were stored
	 * by the loader. Relocate the bootinfo array which is loaded
	 * above the symbols (we assume) to the start of BSS. Then
	 * adjust kernel_top accordingly.
	 */

	bootinfo_relocate((void *)ALIGN((u_int)end));
#endif

	pmap_bootstrap(cpuinfo.mmu_ncontext,
		       cpuinfo.mmu_nregion,
		       cpuinfo.mmu_nsegment);

#if !defined(MSGBUFSIZE) || MSGBUFSIZE == 8192
	/*
	 * Now that the kernel map has been set up, we can enable
	 * the message buffer at the first physical page in the
	 * memory bank where we were loaded. There are 8192
	 * bytes available for the buffer at this location (see the
	 * comment in locore.s at the top of the .text segment).
	 */
	initmsgbuf((void *)KERNBASE, 8192);
#endif

#if defined(SUN4M)
	/*
	 * sun4m bootstrap is complex and is totally different for "normal" 4m
	 * and for microSPARC-IIep - so it's split into separate functions.
	 */
	if (CPU_ISSUN4M) {
#if !defined(MSIIEP)
		bootstrap4m();
#else
		bootstrapIIep();
#endif
	}
#endif /* SUN4M */

#if defined(SUN4) || defined(SUN4C)
	if (CPU_ISSUN4 || CPU_ISSUN4C) {
		/* Map Interrupt Enable Register */
		pmap_kenter_pa(INTRREG_VA,
		    INT_ENABLE_REG_PHYSADR | PMAP_NC | PMAP_OBIO,
		    VM_PROT_READ | VM_PROT_WRITE, 0);
		pmap_update(pmap_kernel());
		/* Disable all interrupts */
		*((unsigned char *)INTRREG_VA) = 0;
	}
#endif /* SUN4 || SUN4C */

#if NKSYMS || defined(DDB) || defined(MODULAR)
	if ((bi_sym = lookup_bootinfo(BTINFO_SYMTAB)) != NULL) {
		if (bi_sym->ssym < KERNBASE) {
			/* Assume low-loading boot loader */
			bi_sym->ssym += KERNBASE;
			bi_sym->esym += KERNBASE;
		}
		ksyms_addsyms_elf(bi_sym->nsym, (void*)bi_sym->ssym,
		    (void*)bi_sym->esym);
	}
#endif

	if ((bi_howto = lookup_bootinfo(BTINFO_BOOTHOWTO)) != NULL) {
		boothowto = bi_howto->boothowto;
printf("initialized boothowt from bootloader: %x\n", boothowto);
	}
}

#if defined(SUN4M) && !defined(MSIIEP)
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
static void
bootstrap4m(void)
{
	int node;
	int nvaddrs, *vaddrs, vstore[10];
	u_int pte;
	int i;
	extern void setpte4m(u_int, u_int);

	if ((node = prom_opennode("/obio/interrupt")) == 0
	    && (node = prom_finddevice("/obio/interrupt")) == 0)
		panic("bootstrap: could not get interrupt "
		      "node from prom");

	vaddrs = vstore;
	nvaddrs = sizeof(vstore)/sizeof(vstore[0]);
	if (prom_getprop(node, "address", sizeof(int),
		    &nvaddrs, &vaddrs) != 0) {
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
	cpuinfo.intreg_4m = (struct icr_pi *)
		(PI_INTR_VA + (_MAXNBPG * CPU_MID2CPUNO(bootmid)));

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
	icr_si_bis(SINTR_MA);

	/* Send all interrupts to primary processor */
	*((u_int *)ICR_ITR) = CPU_MID2CPUNO(bootmid);

#ifdef DEBUG
/*	printf("SINTR: mask: 0x%x, pend: 0x%x\n", *(int*)ICR_SI_MASK,
	       *(int*)ICR_SI_PEND);
*/
#endif
}
#endif /* SUN4M && !MSIIEP */


#if defined(SUN4M) && defined(MSIIEP)
/*
 * On ms-IIep all the interrupt registers, counters etc
 * are PCIC registers, so we need to map it early.
 */
static void
bootstrapIIep(void)
{
	extern struct sparc_bus_space_tag mainbus_space_tag;

	int node;
	bus_space_handle_t bh;
	pcireg_t id;

	if ((node = prom_opennode("/pci")) == 0
	    && (node = prom_finddevice("/pci")) == 0)
		panic("bootstrap: could not get pci "
		      "node from prom");

	if (bus_space_map2(&mainbus_space_tag,
			   (bus_addr_t)MSIIEP_PCIC_PA,
			   (bus_size_t)sizeof(struct msiiep_pcic_reg),
			   BUS_SPACE_MAP_LINEAR,
			   MSIIEP_PCIC_VA, &bh) != 0)
		panic("bootstrap: unable to map ms-IIep pcic registers");

	/* verify that it's PCIC */
	id = mspcic_read_4(pcic_id);

	if (PCI_VENDOR(id) != PCI_VENDOR_SUN
	    && PCI_PRODUCT(id) != PCI_PRODUCT_SUN_MS_IIep)
		panic("bootstrap: PCI id %08x", id);
}

#undef msiiep
#endif /* SUN4M && MSIIEP */


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
bootpath_build(void)
{
	const char *cp;
	char *pp;
	struct bootpath *bp;
	int fl;

	/*
	 * Grab boot path from PROM and split into `bootpath' components.
	 */
	memset(bootpath, 0, sizeof(bootpath));
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
	case PROM_OPENFIRM:
		while (cp != NULL && *cp == '/') {
			/* Step over '/' */
			++cp;
			/* Extract name */
			pp = bp->name;
			while (*cp != '@' && *cp != '/' && *cp != '\0')
				*pp++ = *cp++;
			*pp = '\0';
#if defined(SUN4M)
			/*
			 * JS1/OF does not have iommu node in the device
			 * tree, so bootpath will start with the sbus entry.
			 * Add entry for iommu to match attachment. See also
			 * mainbus_attach and iommu_attach.
			 */
			if (CPU_ISSUN4M && bp == bootpath
			    && strcmp(bp->name, "sbus") == 0) {
				printf("bootpath_build: inserting iommu entry\n");
				strcpy(bootpath[0].name, "iommu");
				bootpath[0].val[0] = 0;
				bootpath[0].val[1] = 0x10000000;
				bootpath[0].val[2] = 0;
				++nbootpath;

				strcpy(bootpath[1].name, "sbus");
				if (*cp == '/') {
					/* complete sbus entry */
					bootpath[1].val[0] = 0;
					bootpath[1].val[1] = 0x10001000;
					bootpath[1].val[2] = 0;
					++nbootpath;
					bp = &bootpath[2];
					continue;
				} else
					bp = &bootpath[1];
			}
#endif /* SUN4M */
			if (*cp == '@') {
				cp = str2hex(++cp, &bp->val[0]);
				if (*cp == ',')
					cp = str2hex(++cp, &bp->val[1]);
				if (*cp == ':') {
					/* XXX - we handle just one char */
					/*       skip remainder of paths */
					/*       like "ledma@f,400010:tpe" */
					bp->val[2] = *++cp - 'a';
					while (*++cp != '/' && *cp != '\0')
						/*void*/;
				}
			} else {
				bp->val[0] = -1; /* no #'s: assume unit 0, no
							sbus offset/address */
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

	for (;*++cp;) {
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
		}
	}
}

/*
 * Fake a ROM generated bootpath.
 * The argument `cp' points to a string such as "xd(0,0,0)netbsd"
 */

static void
bootpath_fake(struct bootpath *bp, const char *cp)
{
	const char *pp;
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
		 * Yet, for the purpose of determining the boot device,
		 * we support only one controller, so we encode the
		 * bootpath component by unit number, as on a v2 prom.
		 */
		BP_APPEND(bp, "fd", -1, v0val[1], v0val[2]);
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
bootpath_print(struct bootpath *bp)
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
 */
struct bootpath *
bootpath_store(int storep, struct bootpath *bp)
{
	static struct bootpath *save;
	struct bootpath *retval;

	retval = save;
	if (storep)
		save = bp;

	return (retval);
}

/*
 * Set up the sd target mappings for non SUN4 PROMs.
 * Find out about the real SCSI target, given the PROM's idea of the
 * target of the (boot) device (i.e., the value in bp->v0val[0]).
 */
static void
crazymap(const char *prop, int *map)
{
	int i;
	char propval[8+2];

	if (!CPU_ISSUN4 && prom_version() < 2) {
		/*
		 * Machines with real v0 proms have an `s[dt]-targets' property
		 * which contains the mapping for us to use. v2 proms do not
		 * require remapping.
		 */
		if (prom_getoption(prop, propval, sizeof propval) != 0 ||
		    propval[0] == '\0' || strlen(propval) != 8) {
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
sd_crazymap(int n)
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
st_crazymap(int n)
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
cpu_configure(void)
{
	struct pcb *pcb0;
	bool userconf = (boothowto & RB_USERCONF) != 0;

	/* initialise the softintr system */
	sparc_softintr_init();

	/* build the bootpath */
	bootpath_build();
	if (((boothowto & RB_USERCONF) != 0) && !userconf)
		/*
		 * Old bootloaders do not pass boothowto, and MI code
		 * has already handled userconfig before we get here
		 * and finally fetch the right options. So if we missed
		 * it, just do it here.
 		 */
		userconf_prompt();

#if defined(SUN4)
	if (CPU_ISSUN4) {
#define MEMREG_PHYSADDR	0xf4000000
		bus_space_handle_t bh;
		bus_addr_t paddr = MEMREG_PHYSADDR;

		if (cpuinfo.cpu_type == CPUTYP_4_100)
			/* Clear top bits of physical address on 4/100 */
			paddr &= ~0xf0000000;

		if (obio_find_rom_map(paddr, PAGE_SIZE, &bh) != 0)
			panic("configure: ROM hasn't mapped memreg!");

		par_err_reg = (volatile int *)bh;
	}
#endif
#if defined(SUN4C)
	if (CPU_ISSUN4C) {
		char *cp, buf[32];
		int node = findroot();
		cp = prom_getpropstringA(node, "device_type", buf, sizeof buf);
		if (strcmp(cp, "cpu") != 0)
			panic("PROM root device type = %s (need CPU)", cp);
	}
#endif

	prom_setcallback(sync_crash);

	/* Enable device interrupts */
#if defined(SUN4M)
#if !defined(MSIIEP)
	if (CPU_ISSUN4M)
		icr_si_bic(SINTR_MA);
#else
	if (CPU_ISSUN4M)
		/* nothing for ms-IIep so far */;
#endif /* MSIIEP */
#endif /* SUN4M */

#if defined(SUN4) || defined(SUN4C)
	if (CPU_ISSUN4 || CPU_ISSUN4C)
		ienab_bis(IE_ALLIE);
#endif

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("mainbus not configured");

	/*
	 * XXX Re-zero lwp0's pcb, to nullify the effect of the
	 * XXX stack running into it during auto-configuration.
	 * XXX - should fix stack usage.
	 */
	pcb0 = lwp_getpcb(&lwp0);
	memset(pcb0, 0, sizeof(struct pcb));

	spl0();
}

void
cpu_rootconf(void)
{
	struct bootpath *bp;

	bp = nbootpath == 0 ? NULL : &bootpath[nbootpath-1];
	if (bp == NULL)
		booted_partition = 0;
	else if (booted_device != bp->dev)
		booted_partition = 0;
	else
		booted_partition = bp->val[2];
	rootconf();
}

/*
 * Console `sync' command.  SunOS just does a `panic: zero' so I guess
 * no one really wants anything fancy...
 */
void
sync_crash(void)
{

	panic("PROM sync command");
}

char *
clockfreq(int freq)
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
mbprint(void *aux, const char *name)
{
	struct mainbus_attach_args *ma = aux;

	if (name)
		aprint_normal("%s at %s", ma->ma_name, name);
	if (ma->ma_paddr)
		aprint_normal(" %saddr 0x%lx",
			BUS_ADDR_IOSPACE(ma->ma_paddr) ? "io" : "",
			(u_long)BUS_ADDR_PADDR(ma->ma_paddr));
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
 * Helper routines to get some of the more common properties. These
 * only get the first item in case the property value is an array.
 * Drivers that "need to know it all" can call prom_getprop() directly.
 */
#if defined(SUN4C) || defined(SUN4M) || defined(SUN4D)
static int	prom_getprop_reg1(int, struct openprom_addr *);
static int	prom_getprop_intr1(int, int *);
static int	prom_getprop_address1(int, void **);
#endif

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
	char namebuf[32];
#if defined(SUN4C) || defined(SUN4M) || defined(SUN4D)
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
#if !defined(MSIIEP)
		"obio",		/* smart enough to get eeprom/etc mapped */
#else
		"pci",		/* ms-IIep */
#endif
		"",

		/* ignore these (end with NULL) */
		/*
		 * These are _root_ devices to ignore. Others must be handled
		 * elsewhere.
		 */
		"SUNW,sx",		/* XXX: no driver for SX yet */
		"virtual-memory",
		"aliases",
		"chosen",		/* OpenFirmware */
		"memory",
		"openprom",
		"options",
		"packages",
		"udp",			/* OFW in Krups */
		/* we also skip any nodes with device_type == "cpu" */
		NULL
	};
#else
#define openboot_special4m	((void *)0)
#endif
#if defined(SUN4D)
	static const char *const openboot_special4d[] = {
		"",

		/* ignore these (end with NULL) */
		/*
		 * These are _root_ devices to ignore. Others must be handled
		 * elsewhere.
		 */
		"mem-unit",	/* XXX might need this for memory errors */
		"boards",
		"openprom",
		"virtual-memory",
		"memory",
		"aliases",
		"options",
		"packages",
		NULL
	};
#else
#define	openboot_special4d	((void *)0)
#endif


	if (CPU_ISSUN4)
		snprintf(machine_model, sizeof machine_model, "SUN-4/%d series",
		    cpuinfo.classlvl);
	else
		snprintf(machine_model, sizeof machine_model, "%s",
		    prom_getpropstringA(findroot(), "name", namebuf,
		    sizeof(namebuf)));

	prom_getidprom();
	printf(": %s: hostid %lx\n", machine_model, hostid);

	/* Establish the first component of the boot path */
	bootpath_store(1, bootpath);

	/*
	 * Locate and configure the ``early'' devices.  These must be
	 * configured before we can do the rest.  For instance, the
	 * EEPROM contains the Ethernet address for the LANCE chip.
	 * If the device cannot be located or configured, panic.
	 */

#if defined(SUN4)
	if (CPU_ISSUN4) {

		memset(&ma, 0, sizeof(ma));
		/* Configure the CPU. */
		ma.ma_bustag = &mainbus_space_tag;
		ma.ma_dmatag = &mainbus_dma_tag;
		ma.ma_name = "cpu";
		if (config_found(dev, (void *)&ma, mbprint) == NULL)
			panic("cpu missing");

		ma.ma_bustag = &mainbus_space_tag;
		ma.ma_dmatag = &mainbus_dma_tag;
		ma.ma_name = "obio";
		if (config_found(dev, (void *)&ma, mbprint) == NULL)
			panic("obio missing");

		ma.ma_bustag = &mainbus_space_tag;
		ma.ma_dmatag = &mainbus_dma_tag;
		ma.ma_name = "vme";
		(void)config_found(dev, (void *)&ma, mbprint);
		return;
	}
#endif

/*
 * The rest of this routine is for OBP machines exclusively.
 */
#if defined(SUN4C) || defined(SUN4M) || defined(SUN4D)

	if (CPU_ISSUN4D)
		openboot_special = openboot_special4d;
	else if (CPU_ISSUN4M)
		openboot_special = openboot_special4m;
	else
		openboot_special = openboot_special4c;

	node0 = firstchild(findroot());

	/* The first early device to be configured is the cpu */
	if (CPU_ISSUN4M) {
		const char *cp;
		int mid, bootnode = 0;

		/*
		 * Configure all CPUs.
		 * Make sure to configure the boot CPU as cpu0.
		 */
	rescan:
		for (node = node0; node; node = nextsibling(node)) {
			cp = prom_getpropstringA(node, "device_type",
					    namebuf, sizeof namebuf);
			if (strcmp(cp, "cpu") != 0)
				continue;

			mid = prom_getpropint(node, "mid", -1);
			if (bootnode == 0) {
				/* We're looking for the boot CPU */
				if (bootmid != 0 && mid != bootmid)
					continue;
				bootnode = node;
			} else {
				if (node == bootnode)
					continue;
			}

			memset(&ma, 0, sizeof(ma));
			ma.ma_bustag = &mainbus_space_tag;
			ma.ma_dmatag = &mainbus_dma_tag;
			ma.ma_node = node;
			ma.ma_name = "cpu";
			config_found(dev, (void *)&ma, mbprint);
			if (node == bootnode && bootmid != 0) {
				/* Re-enter loop to find all remaining CPUs */
				goto rescan;
			}
		}
	} else if (CPU_ISSUN4C) {
		memset(&ma, 0, sizeof(ma));
		ma.ma_bustag = &mainbus_space_tag;
		ma.ma_dmatag = &mainbus_dma_tag;
		ma.ma_node = findroot();
		ma.ma_name = "cpu";
		config_found(dev, (void *)&ma, mbprint);
	}

	for (ssp = openboot_special; *(sp = *ssp) != 0; ssp++) {
		struct openprom_addr romreg;

		if ((node = findnode(node0, sp)) == 0) {
			printf("could not find %s in OPENPROM\n", sp);
			panic(sp);
		}

		memset(&ma, 0, sizeof ma);
		ma.ma_bustag = &mainbus_space_tag;
		ma.ma_dmatag = &mainbus_dma_tag;
		ma.ma_name = prom_getpropstringA(node, "name",
					    namebuf, sizeof namebuf);
		ma.ma_node = node;
		if (prom_getprop_reg1(node, &romreg) != 0)
			continue;

		ma.ma_paddr = (bus_addr_t)
			BUS_ADDR(romreg.oa_space, romreg.oa_base);
		ma.ma_size = romreg.oa_size;
		if (prom_getprop_intr1(node, &ma.ma_pri) != 0)
			continue;
		if (prom_getprop_address1(node, &ma.ma_promvaddr) != 0)
			continue;

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

		DPRINTF(ACDB_PROBE, ("Node: %x", node));
#if defined(SUN4M)
		if (CPU_ISSUN4M) {	/* skip the CPUs */
			if (strcmp(prom_getpropstringA(node, "device_type",
						  namebuf, sizeof namebuf),
				   "cpu") == 0)
				continue;
		}
#endif
		cp = prom_getpropstringA(node, "name", namebuf, sizeof namebuf);
		DPRINTF(ACDB_PROBE, (" name %s\n", namebuf));
		for (ssp = openboot_special; (sp = *ssp) != NULL; ssp++)
			if (strcmp(cp, sp) == 0)
				break;
		if (sp != NULL)
			continue; /* an "early" device already configured */

		memset(&ma, 0, sizeof ma);
		ma.ma_bustag = &mainbus_space_tag;
		ma.ma_dmatag = &mainbus_dma_tag;
		ma.ma_name = prom_getpropstringA(node, "name",
					    namebuf, sizeof namebuf);
		ma.ma_node = node;

#if defined(SUN4M)
		/*
		 * JS1/OF does not have iommu node in the device tree,
		 * so if on sun4m we see sbus node under root - attach
		 * implicit iommu.  See also bootpath_build where we
		 * adjust bootpath accordingly and iommu_attach where
		 * we arrange for this sbus node to be attached.
		 */
		if (CPU_ISSUN4M && strcmp(ma.ma_name, "sbus") == 0) {
			printf("mainbus_attach: sbus node under root on sun4m - assuming iommu\n");
			ma.ma_name = "iommu";
			ma.ma_paddr = (bus_addr_t)BUS_ADDR(0, 0x10000000);
			ma.ma_size = 0x300;
			ma.ma_pri = 0;
			ma.ma_promvaddr = 0;

			(void) config_found(dev, (void *)&ma, mbprint);
			continue;
		}
#endif /* SUN4M */

		if (prom_getprop_reg1(node, &romreg) != 0)
			continue;

		ma.ma_paddr = BUS_ADDR(romreg.oa_space, romreg.oa_base);
		ma.ma_size = romreg.oa_size;

		if (prom_getprop_intr1(node, &ma.ma_pri) != 0)
			continue;

		if (prom_getprop_address1(node, &ma.ma_promvaddr) != 0)
			continue;

		(void) config_found(dev, (void *)&ma, mbprint);
	}
#endif /* SUN4C || SUN4M || SUN4D */
}

CFATTACH_DECL_NEW(mainbus, 0, mainbus_match, mainbus_attach, NULL, NULL);


#if defined(SUN4C) || defined(SUN4M) || defined(SUN4D)
int
prom_getprop_reg1(int node, struct openprom_addr *rrp)
{
	int error, n;
	struct openprom_addr *rrp0 = NULL;
	char buf[32];

	error = prom_getprop(node, "reg", sizeof(struct openprom_addr),
			&n, &rrp0);
	if (error != 0) {
		if (error == ENOENT &&
		    strcmp(prom_getpropstringA(node, "device_type", buf, sizeof buf),
			   "hierarchical") == 0) {
			memset(rrp, 0, sizeof(struct openprom_addr));
			error = 0;
		}
		return (error);
	}

	*rrp = rrp0[0];
	free(rrp0, M_DEVBUF);
	return (0);
}

int
prom_getprop_intr1(int node, int *ip)
{
	int error, n;
	struct rom_intr *rip = NULL;

	error = prom_getprop(node, "intr", sizeof(struct rom_intr),
			&n, &rip);
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
prom_getprop_address1(int node, void **vpp)
{
	int error, n;
	void **vp = NULL;

	error = prom_getprop(node, "address", sizeof(uint32_t), &n, &vp);
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
#endif /* SUN4C || SUN4M || SUN4D */

#ifdef RASTERCONSOLE
/*
 * Try to figure out where the PROM stores the cursor row & column
 * variables.  Returns nonzero on error.
 */
int
romgetcursoraddr(int **rowp, int **colp)
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
#endif /* RASTERCONSOLE */

/*
 * Device registration used to determine the boot device.
 */
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <sparc/sparc/iommuvar.h>

#define BUSCLASS_NONE		0
#define BUSCLASS_MAINBUS	1
#define BUSCLASS_IOMMU		2
#define BUSCLASS_OBIO		3
#define BUSCLASS_SBUS		4
#define BUSCLASS_VME		5
#define BUSCLASS_XDC		6
#define BUSCLASS_XYC		7
#define BUSCLASS_FDC		8
#define BUSCLASS_PCIC		9
#define BUSCLASS_PCI		10

static int bus_class(struct device *);
static const char *bus_compatible(const char *);
static int instance_match(struct device *, void *, struct bootpath *);
static void nail_bootdev(struct device *, struct bootpath *);
static void set_network_props(struct device *, void *);

static struct {
	const char	*name;
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
	{ "vme",	BUSCLASS_VME },
	{ "si",		BUSCLASS_VME },
	{ "sw",		BUSCLASS_OBIO },
	{ "xdc",	BUSCLASS_XDC },
	{ "xyc",	BUSCLASS_XYC },
	{ "fdc",	BUSCLASS_FDC },
	{ "mspcic",	BUSCLASS_PCIC },
	{ "pci",	BUSCLASS_PCI },
};

/*
 * A list of PROM device names that differ from our NetBSD
 * device names.
 */
static struct {
	const char	*bpname;
	const char	*cfname;
} dev_compat_tab[] = {
	{ "espdma",	"dma" },
	{ "SUNW,fas",   "esp" },
	{ "QLGC,isp",	"isp" },
	{ "PTI,isp",	"isp" },
	{ "ptisp",	"isp" },
	{ "SUNW,fdtwo",	"fdc" },
	{ "network",	"hme" }, /* Krups */
	{ "SUNW,hme",   "hme" },
};

static const char *
bus_compatible(const char *bpname)
{
	int i;

	for (i = sizeof(dev_compat_tab)/sizeof(dev_compat_tab[0]); i-- > 0;) {
		if (strcmp(bpname, dev_compat_tab[i].bpname) == 0)
			return (dev_compat_tab[i].cfname);
	}

	return (bpname);
}

static int
bus_class(struct device *dev)
{
	int i, class;

	class = BUSCLASS_NONE;
	if (dev == NULL)
		return (class);

	for (i = sizeof(bus_class_tab)/sizeof(bus_class_tab[0]); i-- > 0;) {
		if (device_is_a(dev, bus_class_tab[i].name)) {
			class = bus_class_tab[i].class;
			break;
		}
	}

	/* sun4m obio special case */
	if (CPU_ISSUN4M && class == BUSCLASS_OBIO)
		class = BUSCLASS_SBUS;

	return (class);
}

static void
set_network_props(struct device *dev, void *aux)
{
	struct mainbus_attach_args *ma;
	struct sbus_attach_args *sa;
	struct iommu_attach_args *iom;
	struct pci_attach_args *pa;
	uint8_t eaddr[ETHER_ADDR_LEN];
	prop_dictionary_t dict;
	prop_data_t blob;
	int ofnode;

	ofnode = 0;
	switch (bus_class(device_parent(dev))) {
	case BUSCLASS_MAINBUS:
		ma = aux;
		ofnode = ma->ma_node;
		break;
	case BUSCLASS_SBUS:
		sa = aux;
		ofnode = sa->sa_node;
		break;
	case BUSCLASS_IOMMU:
		iom = aux;
		ofnode = iom->iom_node;
		break;
	case BUSCLASS_PCI:
		pa = aux;
		ofnode = PCITAG_NODE(pa->pa_tag);
		break;
	}

	prom_getether(ofnode, eaddr);
	dict = device_properties(dev);
	blob = prop_data_create_data(eaddr, ETHER_ADDR_LEN);
	prop_dictionary_set(dict, "mac-address", blob);
	prop_object_release(blob);
}

int
instance_match(struct device *dev, void *aux, struct bootpath *bp)
{
	struct mainbus_attach_args *ma;
	struct sbus_attach_args *sa;
	struct iommu_attach_args *iom;
  	struct pcibus_attach_args *pba;
	struct pci_attach_args *pa;

	/*
	 * Several devices are represented on bootpaths in one of
	 * two formats, e.g.:
	 *	(1) ../sbus@.../esp@<offset>,<slot>/sd@..  (PROM v3 style)
	 *	(2) /sbus0/esp0/sd@..                      (PROM v2 style)
	 *
	 * hence we fall back on a `unit number' check if the bus-specific
	 * instance parameter check does not produce a match.
	 */

	/*
	 * Rank parent bus so we know which locators to check.
	 */
	switch (bus_class(device_parent(dev))) {
	case BUSCLASS_MAINBUS:
		ma = aux;
		DPRINTF(ACDB_BOOTDEV, ("instance_match: mainbus device, "
		    "want space %#x addr %#x have space %#x addr %#llx\n",
		    bp->val[0], bp->val[1], (int)BUS_ADDR_IOSPACE(ma->ma_paddr),
			(unsigned long long)BUS_ADDR_PADDR(ma->ma_paddr)));
		if ((u_long)bp->val[0] == BUS_ADDR_IOSPACE(ma->ma_paddr) &&
		    (bus_addr_t)(u_long)bp->val[1] ==
		    BUS_ADDR_PADDR(ma->ma_paddr))
			return (1);
		break;
	case BUSCLASS_SBUS:
		sa = aux;
		DPRINTF(ACDB_BOOTDEV, ("instance_match: sbus device, "
		    "want slot %#x offset %#x have slot %#x offset %#x\n",
		     bp->val[0], bp->val[1], sa->sa_slot, sa->sa_offset));
		if ((uint32_t)bp->val[0] == sa->sa_slot &&
		    (uint32_t)bp->val[1] == sa->sa_offset)
			return (1);
		break;
	case BUSCLASS_IOMMU:
		iom = aux;
		DPRINTF(ACDB_BOOTDEV, ("instance_match: iommu device, "
		    "want space %#x pa %#x have space %#x pa %#x\n",
		     bp->val[0], bp->val[1], iom->iom_reg[0].oa_space,
		     iom->iom_reg[0].oa_base));
		if ((uint32_t)bp->val[0] == iom->iom_reg[0].oa_space &&
		    (uint32_t)bp->val[1] == iom->iom_reg[0].oa_base)
			return (1);
		break;
	case BUSCLASS_XDC:
	case BUSCLASS_XYC:
		{
		/*
		 * XXX - x[dy]c attach args are not exported right now..
		 * XXX   we happen to know they look like this:
		 */
		struct xxxx_attach_args { int driveno; } *aap = aux;

		DPRINTF(ACDB_BOOTDEV,
		    ("instance_match: x[dy]c device, want drive %#x have %#x\n",
		     bp->val[0], aap->driveno));
		if (aap->driveno == bp->val[0])
			return (1);

		}
		break;
	case BUSCLASS_PCIC:
		pba = aux;
		DPRINTF(ACDB_BOOTDEV, ("instance_match: pci bus "
		    "want bus %d pa %#x have bus %d pa %#lx\n",
		    bp->val[0], bp->val[1], pba->pba_bus, MSIIEP_PCIC_PA));
		if ((int)bp->val[0] == pba->pba_bus
		    && (bus_addr_t)bp->val[1] == MSIIEP_PCIC_PA)
			return (1);
		break;
	case BUSCLASS_PCI:
		pa = aux;
		DPRINTF(ACDB_BOOTDEV, ("instance_match: pci device "
		    "want dev %d function %d have dev %d function %d\n",
		    bp->val[0], bp->val[1], pa->pa_device, pa->pa_function));
		if ((u_int)bp->val[0] == pa->pa_device
		    && (u_int)bp->val[1] == pa->pa_function)
			return (1);
		break;
	default:
		break;
	}

	if (bp->val[0] == -1 && bp->val[1] == device_unit(dev))
		return (1);

	return (0);
}

void
nail_bootdev(struct device *dev, struct bootpath *bp)
{

	if (bp->dev != NULL)
		panic("device_register: already got a boot device: %s",
			bp->dev->dv_xname);

	/*
	 * Mark this bootpath component by linking it to the matched
	 * device. We pick up the device pointer in cpu_rootconf().
	 */
	booted_device = bp->dev = dev;

	/*
	 * Then clear the current bootpath component, so we don't spuriously
	 * match similar instances on other busses, e.g. a disk on
	 * another SCSI bus with the same target.
	 */
	bootpath_store(1, NULL);
}

void
device_register(struct device *dev, void *aux)
{
	struct bootpath *bp = bootpath_store(0, NULL);
	const char *bpname;

	/*
	 * If device name does not match current bootpath component
	 * then there's nothing interesting to consider.
	 */
	if (bp == NULL)
		return;

	/*
	 * Translate PROM name in case our drivers are named differently
	 */
	bpname = bus_compatible(bp->name);

	DPRINTF(ACDB_BOOTDEV,
	    ("\n%s: device_register: dvname %s(%s) bpname %s(%s)\n",
	    dev->dv_xname, device_cfdata(dev)->cf_name, dev->dv_xname,
	    bpname, bp->name));

	/* First, match by name */
	if (!device_is_a(dev, bpname))
		return;

	if (bus_class(dev) != BUSCLASS_NONE) {
		/*
		 * A bus or controller device of sorts. Check instance
		 * parameters and advance boot path on match.
		 */
		if (instance_match(dev, aux, bp) != 0) {
			if (device_is_a(dev, "fdc")) {
				/*
				 * XXX - HACK ALERT
				 * Sun PROMs don't really seem to support
				 * multiple floppy drives. So we aren't
				 * going to, either.  Since the PROM
				 * only provides a node for the floppy
				 * controller, we sneakily add a drive to
				 * the bootpath here.
				 */
				strcpy(bootpath[nbootpath].name, "fd");
				nbootpath++;
			}
			booted_device = bp->dev = dev;
			bootpath_store(1, bp + 1);
			DPRINTF(ACDB_BOOTDEV, ("\t-- found bus controller %s\n",
			    dev->dv_xname));
			return;
		}
	} else if (device_is_a(dev, "le") ||
		   device_is_a(dev, "hme") ||
		   device_is_a(dev, "be") ||
		   device_is_a(dev, "ie")) {

		set_network_props(dev, aux);

		/*
		 * LANCE, Happy Meal, or BigMac ethernet device
		 */
		if (instance_match(dev, aux, bp) != 0) {
			nail_bootdev(dev, bp);
			DPRINTF(ACDB_BOOTDEV, ("\t-- found ethernet controller %s\n",
			    dev->dv_xname));
			return;
		}
	} else if (device_is_a(dev, "sd") ||
		   device_is_a(dev, "cd")) {
#if NSCSIBUS > 0
		/*
		 * A SCSI disk or cd; retrieve target/lun information
		 * from parent and match with current bootpath component.
		 * Note that we also have look back past the `scsibus'
		 * device to determine whether this target is on the
		 * correct controller in our boot path.
		 */
		struct scsipibus_attach_args *sa = aux;
		struct scsipi_periph *periph = sa->sa_periph;
		struct scsipi_channel *chan = periph->periph_channel;
		struct scsibus_softc *sbsc =
			device_private(device_parent(dev));
		u_int target = bp->val[0];
		u_int lun = bp->val[1];

		/* Check the controller that this scsibus is on */
		if ((bp-1)->dev != device_parent(sbsc->sc_dev))
			return;

		/*
		 * Bounds check: we know the target and lun widths.
		 */
		if (target >= chan->chan_ntargets || lun >= chan->chan_nluns) {
			printf("SCSI disk bootpath component not accepted: "
			       "target %u; lun %u\n", target, lun);
			return;
		}

		if (CPU_ISSUN4 && device_is_a(dev, "sd") &&
		    target == 0 &&
		    scsipi_lookup_periph(chan, target, lun) == NULL) {
			/*
			 * disk unit 0 is magic: if there is actually no
			 * target 0 scsi device, the PROM will call
			 * target 3 `sd0'.
			 * XXX - what if someone puts a tape at target 0?
			 */
			target = 3;	/* remap to 3 */
			lun = 0;
		}

		if (CPU_ISSUN4C && device_is_a(dev, "sd"))
			target = sd_crazymap(target);

		if (periph->periph_target == target &&
		    periph->periph_lun == lun) {
			nail_bootdev(dev, bp);
			DPRINTF(ACDB_BOOTDEV, ("\t-- found [cs]d disk %s\n",
			    dev->dv_xname));
			return;
		}
#endif /* NSCSIBUS */
	} else if (device_is_a(dev, "xd") ||
		   device_is_a(dev, "xy")) {

		/* A Xylogic disk */
		if (instance_match(dev, aux, bp) != 0) {
			nail_bootdev(dev, bp);
			DPRINTF(ACDB_BOOTDEV, ("\t-- found x[dy] disk %s\n",
			    dev->dv_xname));
			return;
		}

	} else if (device_is_a(dev, "fd")) {
		/*
		 * Sun PROMs don't really seem to support multiple
		 * floppy drives. So we aren't going to, either.
		 * If we get this far, the `fdc controller' has
		 * already matched and has appended a fake `fd' entry
		 * to the bootpath, so just accept that as the boot device.
		 */
		nail_bootdev(dev, bp);
		DPRINTF(ACDB_BOOTDEV, ("\t-- found floppy drive %s\n",
		    dev->dv_xname));
		return;
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

#if !NKSYMS && !defined(DDB) && !defined(MODULAR)
/*
 * Move bootinfo from the current kernel top to the proposed
 * location. As a side-effect, `kernel_top' is adjusted to point
 * at the first free location after the relocated bootinfo array.
 */
void
bootinfo_relocate(void *newloc)
{
	int bi_size;
	struct btinfo_common *bt;
	char *cp, *dp;
	extern char *kernel_top;

	if (bootinfo == NULL) {
		kernel_top = newloc;
		return;
	}

	/*
	 * Find total size of bootinfo array.
	 * The array is terminated with a `nul' record (size == 0);
	 * we account for that up-front by initializing `bi_size'
	 * to size of a `btinfo_common' record.
	 */
	bi_size = sizeof(struct btinfo_common);
	cp = bootinfo;
	do {
		bt = (struct btinfo_common *)cp;
		bi_size += bt->next;
		cp += bt->next;
	} while (bt->next != 0 &&
		(size_t)cp < (size_t)bootinfo + BOOTINFO_SIZE);

	/*
	 * Check propective gains.
	 */
	if ((int)bootinfo - (int)newloc < bi_size)
		/* Don't bother */
		return;

	/*
	 * Relocate the bits
	 */
	cp = bootinfo;
	dp = newloc;
	do {
		bt = (struct btinfo_common *)cp;
		memcpy(dp, cp, bt->next);
		cp += bt->next;
		dp += bt->next;
	} while (bt->next != 0 &&
		(size_t)cp < (size_t)bootinfo + BOOTINFO_SIZE);

	/* Write the terminating record */
	bt = (struct btinfo_common *)dp;
	bt->next = bt->type = 0;

	/* Set new bootinfo location and adjust kernel_top */
	bootinfo = newloc;
	kernel_top = (char *)newloc + ALIGN(bi_size);
}
#endif /* !NKSYMS && !defined(DDB) && !defined(MODULAR) */
