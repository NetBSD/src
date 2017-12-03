/*	$NetBSD: machdep.c,v 1.4.2.3 2017/12/03 11:36:09 jdolecek Exp $	*/

/*
 * Copyright 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)machdep.c   8.3 (Berkeley) 1/12/94
 *	from: Utah Hdr: machdep.c 1.63 91/04/24
 */

/*
 * Copyright (c) 2009, 2010 Miodrag Vallat.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.4.2.3 2017/12/03 11:36:09 jdolecek Exp $");

#include "opt_ddb.h"
#include "opt_execfmt.h"
#include "opt_modular.h"

#define _MIPS_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>
#include <sys/termios.h>
#include <sys/ksyms.h>
#include <sys/device.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#include <sys/exec_elf.h>
#endif

#include <evbmips/loongson/autoconf.h>
#include <evbmips/loongson/loongson_intr.h>
#include <evbmips/loongson/loongson_bus_defs.h>
#include <machine/cpu.h>
#include <machine/psl.h>

#include <mips/locore.h>

#include <mips/bonito/bonitoreg.h>
#include <mips/bonito/bonitovar.h>
#include <mips/pmon/pmon.h>

#include "sisfb.h"
#if NSISFB > 0
#include <dev/pci/sisfb.h>
#endif

#include "lynxfb.h"
#if NLYNXFB > 0
#include <dev/pci/lynxfbvar.h>
#endif

#include "pckbc.h"
#if NPCKBC > 0
#include <dev/isa/isareg.h>
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#endif
#include "pckbd.h"
#include "ukbd.h"
#if NUKBD > 0
#include <dev/usb/ukbdvar.h>
#endif
#if NPCKBD > 0 || NUKBD > 0
#include <dev/wscons/wskbdvar.h>
#endif

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

bus_space_tag_t comconsiot;
bus_addr_t comconsaddr;
int comconsrate = 0;
#endif /* NCOM > 0 */

#ifdef LOW_DEBUG
#define DPRINTF(x) printf x
#define DPPRINTF(x) pmon_printf x
#else
#define DPRINTF(x)
#define DPPRINTF(x)
#endif


int ex_mallocsafe = 0;
struct extent *loongson_io_ex = NULL;
struct extent *loongson_mem_ex = NULL;
struct mips_bus_space bonito_iot;
struct mips_bus_space bonito_memt;
struct mips_bus_dma_tag bonito_dmat;
struct mips_pci_chipset bonito_pc;

uint loongson_ver;

const struct platform *sys_platform;
struct bonito_flavour {
	const char *prefix;
	const struct platform *platform;
};

extern const struct platform fuloong_platform;
extern const struct platform gdium_platform;
extern const struct platform generic2e_platform;
extern const struct platform lynloong_platform;
extern const struct platform yeeloong_platform;

const struct bonito_flavour bonito_flavours[] = {
	/* Lemote Fuloong 2F mini-PC */
	{ "LM6002",	&fuloong_platform }, /* dual Ethernet, no prefix */
	{ "LM6003",	&fuloong_platform },
	{ "LM6004",	&fuloong_platform },
	/* EMTEC Gdium Liberty 1000 */
	{ "Gdium",	&gdium_platform },
	/* Lemote Yeeloong 8.9" netbook */
	{ "LM8089",	&yeeloong_platform },
	/* supposedly Lemote Yeeloong 10.1" netbook, but those found so far
	   report themselves as LM8089 */
	{ "LM8101",	&yeeloong_platform },
	/* Lemote Lynloong all-in-one computer */
	{ "LM9001",	&lynloong_platform },
	{ NULL }
};

/* Maps for VM objects. */
struct vm_map *phys_map = NULL;

int	netboot;		/* Are we netbooting? */

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

void	mach_init(int, int32_t, int32_t, int32_t, char *);

static int  pmoncngetc(dev_t);
static void pmoncnputc(dev_t, int);

struct consdev pmoncons = {
        NULL,		/* probe */
	NULL, 		/* init */
	pmoncngetc, 	/* getc */
	pmoncnputc,	/* putc */
	nullcnpollc,	/* poolc */
	NULL,		/* BELL */
	makedev(0, 0),
	CN_DEAD
};

/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init(int32_t argc, int32_t argva, int32_t enva, int32_t callvec,
    char *boot_esym)
{
	void *kernend;
#ifdef NOTYET
	int howto;
#endif
	const char *env;
	int i;
	psize_t memlo, memhi;
	const struct bonito_flavour *f;
	char *ssym = NULL, *esym = NULL;
	pcireg_t reg;
	pcitag_t pcitag;

	extern char edata[], end[];

	/*
	 * Clear the BSS segment.
	 */
	memset(edata, 0, (char *)end - edata);

	pmon_init(argc, argva, enva, callvec);
	DPPRINTF(("pmon hello\n"));

	cn_tab = &pmoncons;

	DPRINTF(("hello 0x%x %d 0x%x 0x%x %p stack %p\n", pmon_callvec, argc, argva, enva, boot_esym, &i));

	/*
	 * Reserve space for the symbol table, if it exists.
	 */

#if NKSYMS || defined(DDB) || defined(MODULAR)
	/* Attempt to locate ELF header and symbol table after kernel. */
	if (end[0] == ELFMAG0 && end[1] == ELFMAG1 &&
	    end[2] == ELFMAG2 && end[3] == ELFMAG3) {
		/* ELF header exists directly after kernel. */
		ssym = end;
		esym = boot_esym;
		kernend = (void *)mips_round_page(esym);
	} else {
		ssym = (char *)(vaddr_t)*(int32_t *)end;
		if (((long)ssym - (long)end) >= 0 &&
		    ((long)ssym - (long)end) <= 0x1000 &&
		    ssym[0] == ELFMAG0 && ssym[1] == ELFMAG1 &&
		    ssym[2] == ELFMAG2 && ssym[3] == ELFMAG3) {
			/* Pointers exist directly after kernel. */
			esym = (char *)(vaddr_t)*((int32_t *)end + 1);
			kernend = (void *)mips_round_page(esym);
		} else {
			/* Pointers aren't setup either... */
			ssym = NULL;
			esym = NULL;
			kernend = (void *)mips_round_page(end);
		}
	}
	DPRINTF(("ssym %p esym %p\n", ssym, esym));
#endif

	/*
	 * Set up the exception vectors and CPU-specific function
	 * vectors early on.  We need the wbflush() vector set up
	 * before comcnattach() is called (or at least before the
	 * first printf() after that is called).
	 * Also clears the I+D caches.
	 */
	DPRINTF(("mips_vector_init "));
	mips_vector_init(NULL, false);

	DPRINTF(("uvm_md_init\n"));
	uvm_md_init();
#if NKSYMS || defined(DDB) || defined(MODULAR)
	//ksyms_addsyms_elf((vaddr_t)esym - (vaddr_t)ssym, ssym, esym);
#endif

	/*
	 * Try and figure out what kind of hardware we are.
	 */

	env = pmon_getenv("systype");
	if (env == NULL) {
		printf("Unable to figure out system type!\n");
		goto unsupported;
	}
	if (strcmp(env, "Bonito") != 0) {
		printf("This kernel doesn't support system type \"%s\".\n",
		    env);
		goto unsupported;
	}

	/*
	 * While the kernel supports other processor types than Loongson,
	 * we are not expecting a Bonito-based system with a different
	 * processor.  Just to be on the safe side, refuse to run on
	 * non Loongson2 processors for now.
	 */

	switch ((mips_options.mips_cpu_id >> 8) & 0xff) {
	case MIPS_LOONGSON2:
		switch (mips_options.mips_cpu_id & 0xff) {
		case 0x00:
			loongson_ver = 0x2c;
			break;
		case 0x02:
			loongson_ver = 0x2e;
			break;
		case 0x03:
			loongson_ver = 0x2f;
			break;
		case 0x05:
			loongson_ver = 0x3a;
			break;
		}
		if (loongson_ver == 0x2e || loongson_ver == 0x2f)
			break;
		/* FALLTHROUGH */
	default:
		printf("This kernel doesn't support processor type 0x%x"
		    ", version %d.%d.\n",
		    (mips_options.mips_cpu_id >> 8) & 0xff,
		    (mips_options.mips_cpu_id >> 4) & 0x0f,
		    mips_options.mips_cpu_id & 0x0f);
		goto unsupported;
	}

	/*
	 * Try to figure out what particular machine we run on, depending
	 * on the PMON version information.
	 */

	env = pmon_getenv("Version");
	if (env == NULL) {
		/*
		 * If this is a 2E system, use the generic code and hope
		 * for the best.
		 */
		if (loongson_ver == 0x2e) {
			sys_platform = &generic2e_platform;
		} else {
			printf("Unable to figure out model!\n");
			goto unsupported;
		}
	} else {
		for (f = bonito_flavours; f->prefix != NULL; f++)
			if (strncmp(env, f->prefix, strlen(f->prefix)) ==
			    0) {
				sys_platform = f->platform;
				break;
			}

		if (sys_platform == NULL) {
			/*
			 * Early Lemote designs shipped without a model prefix.
			 * Hopefully these well be close enough to the first
			 * generation Fuloong 2F design (LM6002); let's warn
			 * the user and try this if version is 1.2.something
			 * (1.3 onwards are expected to have a model prefix,
			 *  and there are currently no reports of 1.1 and
			 *  below being 2F systems).
			 *
			 * Note that this could be handled by adding a
			 * "1.2." machine type entry to the flavours table,
			 * but I prefer have it stand out.
			 * LM6002 users are encouraged to add the system
			 * model prefix to the `Version' variable.
			 */
			if (strncmp(env, "1.2.", 4) == 0) {
				printf("No model prefix in version"
				    " string \"%s\".\n"
				    "Attempting to match as Lemote Fuloong\n",
				    env);
				sys_platform = &fuloong_platform;
			}
		}

		if (sys_platform == NULL) {
			printf("This kernel doesn't support model \"%s\"."
			    "\n", env);
			goto unsupported;
		}
	}

	cpu_setmodel("%s %s", sys_platform->vendor, sys_platform->product);
	DPRINTF(("Found %s, setting up.\n", cpu_getmodel()));

	/*
	 * Figure out memory information.
	 * PMON reports it in two chunks, the memory under the 256MB
	 * CKSEG limit, and memory above that limit.  We need to do the
	 * math ourselves.
	 */

	env = pmon_getenv("memsize");
	if (env == NULL) {
		printf("Could not get memory information"
		    " from the firmware\n");
		goto unsupported;
	}
	memlo = strtoul(env, NULL, 10);	/* size in MB */
	DPRINTF(("memlo %" PRIdPSIZE, memlo));
	if (memlo < 0 || memlo > 256) {
		printf("Incorrect low memory size `%s'\n", env);
		goto unsupported;
	}

	/* 3A PMON only reports up to 240MB as low memory */
	if (memlo >= 240) {
		env = pmon_getenv("highmemsize");
		if (env == NULL)
			memhi = 0;
		else
			memhi = strtoul(env, NULL, 10);	/* size in MB */
		if (memhi < 0 || memhi > (64 * 1024) - 256) {
			printf("Incorrect high memory size `%s'\n",
			    env);
			/* better expose the problem than limit to 256MB */
			goto unsupported;
		}
	} else
		memhi = 0;

	DPRINTF(("memhi %" PRIdPSIZE "\n", memhi));
	memlo = memlo * 1024 * 1024;
	memhi = memhi * 1024 * 1024;

	switch (loongson_ver) {
	case 0x2e:
		loongson2e_setup(memlo, memhi,
		    MIPS_KSEG0_START, (vaddr_t)kernend, &bonito_dmat);
		break;
	default:
	case 0x2f:
	case 0x3a:
		loongson2f_setup(memlo, memhi,
		    MIPS_KSEG0_START, (vaddr_t)kernend, &bonito_dmat);
		break;
	}

	DPRINTF(("bonito_pci_init "));
	bonito_pci_init(&bonito_pc, sys_platform->bonito_config);
	bonito_pc.pc_intr_v = __UNCONST(sys_platform->bonito_config);
	bonito_pc.pc_intr_map = loongson_pci_intr_map;
	bonito_pc.pc_intr_string = loongson_pci_intr_string;
	bonito_pc.pc_intr_evcnt = loongson_pci_intr_evcnt;
	bonito_pc.pc_intr_establish = loongson_pci_intr_establish;
	bonito_pc.pc_intr_disestablish = loongson_pci_intr_disestablish;
	bonito_pc.pc_conf_interrupt = loongson_pci_conf_interrupt;
	bonito_pc.pc_pciide_compat_intr_establish =
	    loongson_pciide_compat_intr_establish;
	DPRINTF(("bonito_bus_io_init "));
	bonito_bus_io_init(&bonito_iot, NULL);
	/* override mapping function */
	bonito_iot.bs_map = bonito_bus_io_legacy_map;
	DPRINTF(("bonito_bus_mem_init\n"));
	bonito_bus_mem_init(&bonito_memt, NULL);

	bonito_dmat._cookie = __UNCONST(sys_platform);
	bonito_dmat._dmamap_ops = mips_bus_dmamap_ops;
	bonito_dmat._dmamem_ops = mips_bus_dmamem_ops;
	bonito_dmat._dmatag_ops = mips_bus_dmatag_ops;

	DPRINTF(("sys_platform->setup %p\n", sys_platform->setup));
	if (sys_platform->setup != NULL)
		(*(sys_platform->setup))();

#if NCOM > 0
	DPRINTF(("comconsrate %d\n", comconsrate));
	if (comconsrate > 0) {
		if (comcnattach(comconsiot, comconsaddr, comconsrate,
		    COM_FREQ, COM_TYPE_NORMAL,
		    (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8) != 0)
			panic("unable to initialize serial console");
	}
#endif /* NCOM > 0 */

	for (i = 0; i < 32 - 0x11; i++) {
		pcitag = pci_make_tag(&bonito_pc, 0, i, 0);
		reg = pci_conf_read(&bonito_pc, pcitag, PCI_CLASS_REG);
		DPRINTF(("dev %d class 0x%x", i, reg));
		reg = pci_conf_read(&bonito_pc, pcitag, PCI_ID_REG);
		DPRINTF((" id 0x%x; ", reg));
#if NSISFB > 0
		if (cn_tab == &pmoncons)
			sisfb_cnattach(&bonito_memt, &bonito_iot, &bonito_pc,
			    pcitag, reg);
#endif
#if NLYNXFB > 0
		if (cn_tab == &pmoncons)
			lynxfb_cnattach(&bonito_memt, &bonito_iot, &bonito_pc,
			    pcitag, reg);
#endif
		if (cn_tab == &pmoncons)
			gdium_cnattach(&bonito_memt, &bonito_iot, &bonito_pc,
			    pcitag, reg);
		if (cn_tab != &pmoncons)
			break;
	}
#if NPCKBC > 0 || NUKBD > 0
	if (cn_tab != &pmoncons) {
		int rc = ENXIO;
#if NPCKBC > 0
		if (rc != 0)
			rc = pckbc_cnattach(&bonito_iot, IO_KBD, KBCMDP, 0, 0);
#endif
#if NUKBD > 0
		if (rc != 0)
			rc = ukbd_cnattach();
#endif
	}
#endif	/* NPCKBC > 0 || NUKBD > 0 */
	DPRINTF(("\n"));

	/*
	 * Get the timer from PMON.
	 */
	DPRINTF(("search cpuclock "));
	env = pmon_getenv("cpuclock");
	DPRINTF(("got %s ", env));
	if (env != NULL) {
		curcpu()->ci_cpu_freq =
		    strtoul(env, NULL, 10);
	}
	
	DPRINTF(("cpuclock %ld\n", curcpu()->ci_cpu_freq));

	if (mips_options.mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT)
		curcpu()->ci_cpu_freq /= 2;


	/* Compute the number of ticks for hz. */
	curcpu()->ci_cycles_per_hz = (curcpu()->ci_cpu_freq + hz / 2) / hz;

	/* Compute the delay divisor. */
	curcpu()->ci_divisor_delay =
	    ((curcpu()->ci_cpu_freq + 500000) / 1000000);

	/*
	 * Get correct cpu frequency if the CPU runs at twice the
	 * external/cp0-count frequency.
	 */
	if (mips_options.mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT)
		curcpu()->ci_cpu_freq *= 2;

#ifdef DEBUG
	printf("Timer calibration: %lu cycles/sec\n",
	    curcpu()->ci_cpu_freq);
#endif

#if NCOM > 0 && 0
	/*
	 * Delay to allow firmware putchars to complete.
	 * FIFO depth * character time.
	 * character time = (1000000 / (defaultrate / 10))
	 */
	delay(160000000 / comcnrate);
	if (comcnattach(&gc->gc_iot, MALTA_UART0ADR, comcnrate,
	    COM_FREQ, COM_TYPE_NORMAL,
	    (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8) != 0)
		panic("malta: unable to initialize serial console");
#endif /* NCOM > 0 */

	/*
	 * XXX: check argv[0] - do something if "gdb"???
	 */

	/*
	 * Look at arguments passed to us and compute boothowto.
	 */
	boothowto = RB_AUTOBOOT;
#ifdef NOTYET
	for (i = 1; i < argc; i++) {
		for (cp = argv[i]; *cp; cp++) {
			/* Ignore superfluous '-', if there is one */
			if (*cp == '-')
				continue;

			howto = 0;
			BOOT_FLAG(*cp, howto);
			if (! howto)
				printf("bootflag '%c' not recognised\n", *cp);
			else
				boothowto |= howto;
		}
	}
#endif

	/*
	 * Load the rest of the available pages into the VM system.
	 */
	mips_page_physload(MIPS_KSEG0_START, (vaddr_t)kernend,
	    mem_clusters, mem_cluster_cnt, NULL, 0);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	DPRINTF(("mips_init_msgbuf\n"));
	mips_init_msgbuf();

	DPRINTF(("pmap_bootstrap\n"));
	pmap_bootstrap();

	/*
	 * Allocate uarea page for lwp0 and set it.
	 */
	DPRINTF(("curlwp %p ", curlwp));
	DPRINTF(("curlwp stack %p\n", curlwp->l_addr));
	mips_init_lwp0_uarea();

	DPRINTF(("curlwp %p ", curlwp));
	DPRINTF(("curlwp stack %p\n", curlwp->l_addr));

	/*
	 * Initialize debuggers, and break into them, if appropriate.
	 */
#if defined(DDB)
	if (boothowto & RB_KDB)
		Debugger();
#endif
	DPRINTF(("return\n"));
	return;
unsupported:
	panic("unsupported hardware\n");
}

void
consinit(void)
{

	/*
	 * Everything related to console initialization is done
	 * in mach_init().
	 */
}

/*
 * Allocate memory for variable-sized tables,
 */
void
cpu_startup(void)
{
	/*
	 *  Do the common startup items.
	 */
	cpu_startup_common();

	/*
	 * Virtual memory is bootstrapped -- notify the bus spaces
	 * that memory allocation is now safe.
	 */
	ex_mallocsafe = 1;
}

int	waittime = -1;

void
cpu_reboot(int howto, char *bootstr)
{

	/* Take a snapshot before clobbering any registers. */
	savectx(curpcb);

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	/* If "always halt" was specified as a boot flag, obey. */
	if (boothowto & RB_HALT)
		howto |= RB_HALT;

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && (waittime < 0)) {
		waittime = 0;
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	splhigh();

	if (howto & RB_DUMP)
		dumpsys();

haltsys:
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		if (sys_platform->powerdown != NULL)
			sys_platform->powerdown();
	}

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(1);	/* For proper keyboard command handling */
		cngetc();
		cnpollc(0);
	}

	printf("%s\n\n", ((howto & RB_HALT) != 0) ? "halted." : "rebooting...");

	if (sys_platform->reset != NULL)
		sys_platform->reset();

	__asm__ __volatile__ (
		"\t.long 0x3c02bfc0\n"
		"\t.long 0x00400008\n"
	    ::: "v0");
}

/*
 * Early console through pmon routines.
 */

int
pmoncngetc(dev_t dev)
{
	/*
	 * PMON does not give us a getc routine.  So try to get a whole line
	 * and return it char by char, trying not to lose the \n.  Kind
	 * of ugly but should work.
	 *
	 * Note that one could theoretically use pmon_read(STDIN, &c, 1)
	 * but the value of STDIN within PMON is not a constant and there
	 * does not seem to be a way of letting us know which value to use.
	 */
	static char buf[1 + PMON_MAXLN];
	static char *bufpos = buf;
	int c;

	if (*bufpos == '\0') {
		bufpos = buf;
		if (pmon_gets(buf) == NULL) {
			/* either an empty line or EOF. assume the former */
			return (int)'\n';
		} else {
			/* put back the \n sign */
			buf[strlen(buf)] = '\n';
		}
	}

	c = (int)*bufpos++;
	if (bufpos - buf > PMON_MAXLN) {
		bufpos = buf;
		*bufpos = '\0';
	}

	return c;
}

void
pmoncnputc(dev_t dev, int c)
{
	if (c == '\n')
		pmon_printf("\n");
	else
		pmon_printf("%c", c);
}
