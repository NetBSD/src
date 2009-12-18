/*	$NetBSD: machdep.c,v 1.108 2009/12/18 23:22:28 matt Exp $	*/

/*-
 * Copyright (c) 2006 Izumi Tsutsui.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.108 2009/12/18 23:22:28 matt Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_modular.h"
#include "opt_execfmt.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>
#include <sys/ksyms.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/bootinfo.h>
#include <machine/psl.h>

#include <mips/locore.h>

#include <dev/cons.h>

#include <cobalt/dev/gtreg.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#define ELFSIZE		DB_ELFSIZE
#include <sys/exec_elf.h>
#endif

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

/* Maps for VM objects. */
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

int	physmem;		/* Total physical memory */
void	*bootinfo = NULL;	/* pointer to bootinfo structure */

char	bootstring[512];	/* Boot command */
int	netboot;		/* Are we netbooting? */

char	*nfsroot_bstr = NULL;
char	*root_bstr = NULL;
int	bootunit = -1;
int	bootpart = -1;

int cpuspeed;

u_int cobalt_id;
static const char * const cobalt_model[] =
{
	[COBALT_ID_QUBE2700] = "Cobalt Qube 2700",
	[COBALT_ID_RAQ]      = "Cobalt RaQ",
	[COBALT_ID_QUBE2]    = "Cobalt Qube 2",
	[COBALT_ID_RAQ2]     = "Cobalt RaQ 2"
};
#define COBALT_MODELS	__arraycount(cobalt_model)

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

void	mach_init(intptr_t, u_int, int32_t);
void	decode_bootstring(void);
static char *strtok_light(char *, const char);
static u_int read_board_id(void);

/*
 * safepri is a safe priority for sleep to set for a spin-wait during
 * autoconfiguration or after a panic.  Used as an argument to splx().
 */
int	safepri = MIPS1_PSL_LOWIPL;

extern char *esym;

/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init(intptr_t memsize, u_int bim, int32_t bip32)
{
	void *bip = (void *)(intptr_t)bip32;
	char *kernend;
	u_long first, last;
	extern char edata[], end[];
	const char *bi_msg;
#if NKSYMS || defined(DDB) || defined(MODULAR)
	int nsym = 0;
	char *ssym = 0;
	struct btinfo_symtab *bi_syms;
#endif
	struct btinfo_howto *bi_howto;

	/*
	 * Clear the BSS segment (if needed).
	 */
	if (memcmp(((Elf_Ehdr *)end)->e_ident, ELFMAG, SELFMAG) == 0 &&
	    ((Elf_Ehdr *)end)->e_ident[EI_CLASS] == ELFCLASS) {
		esym = end;
#if NKSYMS || defined(DDB) || defined(MODULAR)
		esym += ((Elf_Ehdr *)end)->e_entry;
#endif
		kernend = (char *)mips_round_page(esym);
		/*
		 * We don't have to clear BSS here
		 * since our bootloader already does it.
		 */
#if 0
		memset(edata, 0, end - edata);
#endif
	} else {
		kernend = (void *)mips_round_page(end);
		/*
		 * No symbol table, so assume we are loaded by
		 * the firmware directly with "bfd" command.
		 * The firmware loader doesn't clear BSS of
		 * a loaded kernel, so do it here.
		 */
		memset(edata, 0, kernend - edata);

		/*
		 * XXX
		 * lwp0 and cpu_info_store are allocated in BSS
		 * and initialized before mach_init() is called,
		 * so restore them again.
		 */
		lwp0.l_cpu = &cpu_info_store;
		cpu_info_store.ci_curlwp = &lwp0;
	}

	/* Check for valid bootinfo passed from bootstrap */
	if (bim == BOOTINFO_MAGIC) {
		struct btinfo_magic *bi_magic;

		bootinfo = bip;
		bi_magic = lookup_bootinfo(BTINFO_MAGIC);
		if (bi_magic == NULL) {
			bi_msg = "missing bootinfo structure";
			bim = (uintptr_t)bip;
		} else if (bi_magic->magic != BOOTINFO_MAGIC) {
			bi_msg = "invalid bootinfo structure";
			bim = bi_magic->magic;
		} else
			bi_msg = NULL;
	} else {
		bi_msg = "invalid bootinfo (standalone boot?)";
	}

#if NKSYMS || defined(DDB) || defined(MODULAR)
	bi_syms = lookup_bootinfo(BTINFO_SYMTAB);

	/* Load symbol table if present */
	if (bi_syms != NULL) {
		nsym = bi_syms->nsym;
		ssym = (void *)(intptr_t)bi_syms->ssym;
		esym = (void *)(intptr_t)bi_syms->esym;
		kernend = (void *)mips_round_page(esym);
	}
#endif

	bi_howto = lookup_bootinfo(BTINFO_HOWTO);
	if (bi_howto != NULL)
		boothowto = bi_howto->bi_howto;

	cobalt_id = read_board_id();
	if (cobalt_id >= COBALT_MODELS || cobalt_model[cobalt_id] == NULL)
		sprintf(cpu_model, "Cobalt unknown model (board ID %u)",
		    cobalt_id);
	else
		strcpy(cpu_model, cobalt_model[cobalt_id]);

	switch (cobalt_id) {
	case COBALT_ID_QUBE2700:
	case COBALT_ID_RAQ:
		cpuspeed = 150; /* MHz */
		break;
	case COBALT_ID_QUBE2:
	case COBALT_ID_RAQ2:
		cpuspeed = 250; /* MHz */
		break;
	default:
		/* assume the fastest, so that delay(9) works */
		cpuspeed = 250;
		break;
	}
	curcpu()->ci_cpu_freq = cpuspeed * 1000 * 1000;
	curcpu()->ci_cycles_per_hz = (curcpu()->ci_cpu_freq + hz / 2) / hz;
	curcpu()->ci_divisor_delay =
	    ((curcpu()->ci_cpu_freq + (1000000 / 2)) / 1000000);
	/* all models have Rm5200, which is CPU_MIPS_DOUBLE_COUNT */
	curcpu()->ci_cycles_per_hz /= 2;
	curcpu()->ci_divisor_delay /= 2;

	physmem = btoc(memsize - MIPS_KSEG0_START);

	consinit();

	if (bi_msg != NULL)
		printf("%s: magic=%#x\n", bi_msg, bim);

	uvm_setpagesize();

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */
	mips_vector_init();

	/*
	 * The boot command is passed in the top 512 bytes,
	 * so don't clobber that.
	 */
	mem_clusters[0].start = 0;
	mem_clusters[0].size = ctob(physmem) - 512;
	mem_cluster_cnt = 1;

	memcpy(bootstring, (char *)(memsize - 512), 512);
	memset((char *)(memsize - 512), 0, 512);
	bootstring[511] = '\0';

	decode_bootstring();

#if NKSYMS || defined(DDB) || defined(MODULAR)
	/* init symbols if present */
	if ((bi_syms != NULL) && (esym != NULL))
		ksyms_addsyms_elf(esym - ssym, ssym, esym);
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
#ifdef KGDB
	if (boothowto & RB_KDB)
		kgdb_connect(0);
#endif

	/*
	 * Load the rest of the available pages into the VM system.
	 */
	first = round_page(MIPS_KSEG0_TO_PHYS(kernend));
	last = mem_clusters[0].start + mem_clusters[0].size;
	uvm_page_physload(atop(first), atop(last), atop(first), atop(last),
	    VM_FREELIST_DEFAULT);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	pmap_bootstrap();

	mips_init_lwp0_uarea();
}

/*
 * Allocate memory for variable-sized tables,
 */
void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	printf("%s\n", cpu_model);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for physio.
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, false, NULL);

	/*
	 * (No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use KSEG to
	 * map those pages.)
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
}

static int waittime = -1;

void
cpu_reboot(int howto, char *bootstr)
{

	/* Take a snapshot before clobbering any registers. */
	if (curlwp)
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

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(1);	/* For proper keyboard command handling */
		cngetc();
		cnpollc(0);
	}

	printf("rebooting...\n\n");
	delay(500000);

	*(volatile char *)MIPS_PHYS_TO_KSEG1(LED_ADDR) = LED_RESET;
	printf("WARNING: reboot failed!\n");

	for (;;)
		;
}


void
decode_bootstring(void)
{
	char *work;
	char *equ;
	int i;

	/* break apart bootstring on ' ' boundries and itterate */
	work = strtok_light(bootstring, ' ');
	while (work != '\0') {
		/* if starts with '-', we got options, walk its decode */
		if (work[0] == '-') {
			i = 1;
			while (work[i] != ' ' && work[i] != '\0') {
				BOOT_FLAG(work[i], boothowto);
				i++;
			}
		} else

		/* if it has a '=' its an assignment, switch and set */
		if ((equ = strchr(work, '=')) != '\0') {
			if (memcmp("nfsroot=", work, 8) == 0) {
				nfsroot_bstr = (equ + 1);
			} else
			if (memcmp("root=", work, 5) == 0) {
				root_bstr = (equ + 1);
			}
		} else

		/* else it a single value, switch and process */
		if (memcmp("single", work, 5) == 0) {
			boothowto |= RB_SINGLE;
		} else
		if (memcmp("ro", work, 2) == 0) {
			/* this is also inserted by the firmware */
		}

		/* grab next token */
		work = strtok_light(NULL, ' ');
	}

	if (root_bstr != NULL) {
		/* this should be of the form "/dev/hda1" */
		/* [abcd][1234]    drive partition  linux probe order */
		if ((memcmp("/dev/hd", root_bstr, 7) == 0) &&
		    (strlen(root_bstr) == 9) ){
			bootunit = root_bstr[7] - 'a';
			bootpart = root_bstr[8] - '1';
		}
	}

	if (nfsroot_bstr != NULL)
		netboot = 1;
}


static char *
strtok_light(char *str, const char sep)
{
	static char *proc;
	char *head;
	char *work;

	if (str != NULL)
		proc = str;
	if (proc == NULL)	/* end of string return NULL */
		return proc;

	head = proc;

	work = strchr(proc, sep);
	if (work == NULL) {	/* we hit the end */
		proc = work;
	} else {
		proc = (work + 1);
		*work = '\0';
	}

	return head;
}

/*
 * Look up information in bootinfo of boot loader.
 */
void *
lookup_bootinfo(unsigned int type)
{
	struct btinfo_common *bt;
	char *help = bootinfo;

	/* Check for a bootinfo record first. */
	if (help == NULL) {
		printf("##### help == NULL\n");
		return NULL;
	}

	do {
		bt = (struct btinfo_common *)help;
		printf("Type %d @%p\n", bt->type, (void *)(intptr_t)bt);
		if (bt->type == type)
			return (void *)help;
		help += bt->next;
	} while (bt->next != 0 &&
	    (size_t)help < (size_t)bootinfo + BOOTINFO_SIZE);

	return NULL;
}

/*
 * Get board ID of cobalt models.
 *
 * The board ID info is stored at the PCI config register
 * on the PCI-ISA bridge part of the VIA VT82C586 chipset.
 * We can't use pci_conf_read(9) yet here, so read it directly.
 */
static u_int
read_board_id(void)
{
	volatile uint32_t *pcicfg_addr, *pcicfg_data;
	uint32_t reg;

#define PCIB_PCI_BUS		0
#define PCIB_PCI_DEV		9
#define PCIB_PCI_FUNC		0
#define PCIB_BOARD_ID_REG	0x94
#define COBALT_BOARD_ID(reg)	((reg & 0x000000f0) >> 4)

	pcicfg_addr = (uint32_t *)MIPS_PHYS_TO_KSEG1(GT_BASE + GT_PCICFG_ADDR);
	pcicfg_data = (uint32_t *)MIPS_PHYS_TO_KSEG1(GT_BASE + GT_PCICFG_DATA);

	*pcicfg_addr = PCICFG_ENABLE |
	    (PCIB_PCI_BUS << 16) | (PCIB_PCI_DEV << 11) | (PCIB_PCI_FUNC << 8) |
	    PCIB_BOARD_ID_REG;
	reg = *pcicfg_data;
	*pcicfg_addr = 0;

	return COBALT_BOARD_ID(reg);
}
