/*	$NetBSD: machdep.c,v 1.60 2001/05/17 14:53:54 tsutsui Exp $	*/
/*	$OpenBSD: machdep.c,v 1.36 1999/05/22 21:22:19 weingart Exp $	*/

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
 *	from: @(#)machdep.c	8.3 (Berkeley) 1/12/94
 */

/* from: Utah Hdr: machdep.c 1.63 91/04/24 */

#include "fs_mfs.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/tty.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/device.h>
#include <sys/syscallargs.h>
#include <sys/kcore.h>
#ifdef MFS
#include <ufs/mfs/mfs_extern.h>
#endif

#include <ufs/mfs/mfs_extern.h>		/* mfs_initminiroot() */

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/pio.h>
#include <machine/bus.h>
#include <machine/trap.h>
#include <machine/autoconf.h>
#include <mips/pte.h>
#include <mips/locore.h>
#include <mips/cpuregs.h>
#include <mips/psl.h>
#ifdef DDB
#include <mips/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <dev/cons.h>

#include <dev/ic/i8042reg.h>
#include <dev/isa/isareg.h>

#include <arc/arc/arctype.h>
#include <arc/arc/arcbios.h>
#include <arc/arc/wired_map.h>
#include <arc/jazz/pica.h>
#include <arc/jazz/rd94.h>
#include <arc/jazz/pckbc_jazzioreg.h>
#include <arc/dti/desktech.h>
#include <arc/algor/algor.h>

#include "rasdisplay_jazzio.h"
#if NRASDISPLAY_JAZZIO > 0
#include <arc/jazz/rasdisplay_jazziovar.h>
#endif

#include "tga.h"
#if NTGA > 0
#include <dev/pci/pcivar.h>
#include <dev/pci/tgavar.h>
#include <arc/pci/necpbvar.h>
#endif

#include "pc.h"
#if NPC > 0
#include <machine/pccons.h>
#endif

#include "vga_jazzio.h"
#if NVGA_JAZZIO > 0
#include <arc/jazz/vga_jazziovar.h>
#endif

#include "vga_isa.h"
#if NVGA_ISA > 0
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/isa/vga_isavar.h>
#endif

#include "pckbc_jazzio.h"

#include "pckbc.h"
#if NPCKBC > 0
#include <dev/ic/pckbcvar.h>
#endif

#include "com.h"
#if NCOM > 0
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#ifndef COMCONSOLE
#define COMCONSOLE	0
#endif

#if NCOM > 0
#ifndef COM_FREQ_MAGNUM
#if 0
#define COM_FREQ_MAGNUM	4233600 /* 4.2336MHz - ARC? */
#else
#define COM_FREQ_MAGNUM	8192000	/* 8.192 MHz - NEC RISCstation M402 */
#endif
#endif /* COM_FREQ_MAGNUM */

#ifndef CONSPEED
#define CONSPEED TTYDEF_SPEED
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
#endif /* NCOM */

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;		/* from <machine/param.h> */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */
char	cpu_model[30];

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

/* maps for VM objects */
vm_map_t exec_map = NULL;
vm_map_t mb_map = NULL;
vm_map_t phys_map = NULL;

int	maxmem;			/* max memory per process */
int	physmem;		/* max supported memory, changes to actual */
int	cputype;		/* Mother board type */
int	ncpu = 1;		/* At least one cpu in the system */
vaddr_t kseg2iobufsize = 0;	/* to reserve PTEs for KSEG2 I/O space */
struct arc_bus_space arc_bus_io;/* Bus tag for bus.h macros */
struct arc_bus_space arc_bus_mem;/* Bus tag for bus.h macros */
struct arc_bus_space pica_bus;	/* picabus for com.c/com_lbus.c */
#if NCOM > 0
int	com_freq = COM_FREQ;	/* unusual clock frequency of dev/ic/com.c */
int	com_console_address;	/* Well, ain't it just plain stupid... */
bus_space_tag_t comconstag = &arc_bus_io;	/* com console bus */
struct arc_bus_space *arc_bus_com = &arc_bus_io; /* com bus */
#endif
int	com_console = COMCONSOLE;
char   **environment;		/* On some arches, pointer to environment */
char	eth_hw_addr[6];		/* HW ether addr not stored elsewhere */

int mem_reserved[VM_PHYSSEG_MAX]; /* the cluster is reserved, i.e. not free */
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

/* initialize bss, etc. from kernel start, before main() is called. */
extern void mach_init __P((int argc, char *argv[], char *envv[]));

void machine_ConfigCache __P((void));
static void tlb_init_pica __P((void));
static void tlb_init_nec_eisa __P((void));
static void tlb_init_nec_pci __P((void));
static void tlb_init_tyne __P((void));
static int get_simm_size __P((int *, int));
static char *getenv __P((char *env));
static void arc_sysreset __P((bus_addr_t, bus_size_t));
static void get_eth_hw_addr __P((char *));
static int atoi __P((const char *, int));

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 * Used as an argument to splx().
 * XXX disables interrupt 5 to disable mips3 on-chip clock.
 */
int	safepri = MIPS3_PSL_LOWIPL;

struct splvec	splvec = {			/* XXX will go XXX */
	MIPS_INT_MASK_SPLHIGH, /* splbio */
	MIPS_INT_MASK_SPLHIGH, /* splnet */
	MIPS_INT_MASK_SPLHIGH, /* spltty */
	MIPS_INT_MASK_SPLHIGH, /* splvm */
	MIPS_INT_MASK_SPLHIGH, /* splclock */
	MIPS_INT_MASK_SPLHIGH, /* splstatclock */
};

extern char kernel_text[], edata[], end[];
extern struct user *proc0paddr;

/*
 * Do all the stuff that locore normally does before calling main().
 * Process arguments passed to us by the BIOS.
 * Reset mapping and set up mapping to hardware and init "wired" reg.
 * Return the first page address following the system.
 */
void
mach_init(argc, argv, envv)
	int argc;
	char *argv[];
	char *envv[];	/* Not on all arches... */
{
	char *cp;
	int i;
	paddr_t kernstartpfn, kernendpfn, first, last;
	caddr_t kernend, v;
	vsize_t size;

	/* clear the BSS segment in kernel code */
	kernend = (caddr_t)mips_round_page(end);
	bzero(edata, kernend - edata);

	environment = &argv[1];

	/* Initialize the CPU type */
	cputype = bios_ident();

	if (cputype >= 0) { /* ARC BIOS present */
		bios_init_console();
		bios_save_info();
		physmem = bios_configure_memory(mem_reserved, mem_clusters,
		    &mem_cluster_cnt);
	}

	/*
	 * Get config register now as mapped from BIOS since we are
	 * going to demap these addresses later. We want as may TLB
	 * entries as possible to do something useful :-).
	 */

	switch (cputype) {
	case ACER_PICA_61:	/* ALI PICA 61 and MAGNUM is almost the */
	case MAGNUM:		/* Same kind of hardware. NEC goes here too */
		switch (cputype) {
		case ACER_PICA_61:
			strcpy(cpu_model, "Acer Pica-61");
			break;
		case MAGNUM:
			strcpy(cpu_model, "MIPS Magnum");
#if NCOM > 0
			com_freq = COM_FREQ_MAGNUM;
#endif
			break;
		}

		arc_bus_space_init(&pica_bus, "picabus",
		    R4030_P_LOCAL_IO_BASE, R4030_V_LOCAL_IO_BASE,
		    R4030_V_LOCAL_IO_BASE, R4030_S_LOCAL_IO_BASE);
		arc_bus_space_init(&arc_bus_io, "picaisaio",
		    PICA_P_ISA_IO, PICA_V_ISA_IO, 0, PICA_S_ISA_IO);
		arc_bus_space_init(&arc_bus_mem, "picaisamem",
		    PICA_P_ISA_MEM, PICA_V_ISA_MEM, 0, PICA_S_ISA_MEM);

#if NCOM > 0
		arc_bus_com = &pica_bus;
		comconstag = &pica_bus;
		com_console_address = PICA_SYS_COM1;
#endif

		/*
		 * Set up interrupt handling and I/O addresses.
		 */
		splvec.splnet = MIPS_INT_MASK_SPL3;
		splvec.splbio = MIPS_INT_MASK_SPL3;
		splvec.splvm = MIPS_INT_MASK_SPL3;
		splvec.spltty = MIPS_INT_MASK_SPL3;
		splvec.splclock = MIPS_INT_MASK_SPL5;
		splvec.splstatclock = MIPS_INT_MASK_SPL5;
		break;

	case NEC_R94:
	case NEC_RAx94:
	case NEC_RD94:
	case NEC_R96:
	case NEC_JC94:
		switch (cputype) {
		case NEC_R94:
			strcpy(cpu_model, "NEC-R94");
			break;
		case NEC_RAx94:
			strcpy(cpu_model, "NEC-RA'94");
			break;
		case NEC_RD94:
			strcpy(cpu_model, "NEC-RD94");
			break;
		case NEC_R96:
			strcpy(cpu_model, "NEC-R96");
			break;
		case NEC_JC94:
			strcpy(cpu_model, "NEC-JC94");
			break;
		}

		arc_bus_space_init(&pica_bus, "picabus",
		    RD94_P_LOCAL_IO_BASE, RD94_V_LOCAL_IO_BASE,
		    RD94_V_LOCAL_IO_BASE, RD94_S_LOCAL_IO_BASE);

		switch (cputype) {
		/* EISA machines */
		case NEC_R94:
		case NEC_R96:
			/* XXX - not really confirmed */
			arc_bus_space_init(&arc_bus_io, "rd94pciio",
			    RD94_P_PCI_IO, RD94_V_PCI_IO, 0, RD94_S_PCI_IO);
			arc_bus_space_init(&arc_bus_mem, "rd94pcimem",
			    RD94_P_PCI_MEM, RD94_V_PCI_MEM, 0, RD94_S_PCI_MEM);
			break;

		/* PCI machines */		
		case NEC_RAx94: /* XXX - not really confirmed */
		case NEC_RD94:
		case NEC_JC94:
			arc_bus_space_init(&arc_bus_io, "rd94pciio",
			    RD94_P_PCI_IO, RD94_V_PCI_IO, 0, RD94_S_PCI_IO);
			arc_bus_space_init(&arc_bus_mem, "rd94pcimem",
			    RD94_P_PCI_MEM, RD94_V_PCI_MEM, 0, RD94_S_PCI_MEM);
		/*
		 * By default, reserve 32MB in KSEG2 for PCI memory space.
		 * Since kseg2iobufsize/NBPG*4 bytes are used for Sysmap,
		 * this consumes 32KB physical memory.
		 *
		 * If a kernel with "options DIAGNOSTIC" panics with
		 * the message "pmap_enter: kva too big", you have to
		 * increase this value by a option like below:
		 *     options KSEG2IOBUFSIZE=0x1b000000 # 432MB consumes 432KB
		 * If you met this symptom, please report it to
		 * port-arc-maintainer@netbsd.org.
		 *
		 * kseg2iobufsize will be refered from pmap_bootstrap().
		 */
			kseg2iobufsize = 0x02000000;
			/* 32MB: consumes 32KB for PTEs */
			break;
		}

#if NCOM > 0
		arc_bus_com = &pica_bus;
		comconstag = &pica_bus;
		com_console_address = RD94_SYS_COM1;
#endif

		/*
		 * Set up interrupt handling and I/O addresses.
		 */
		splvec.splnet = MIPS_INT_MASK_SPL2;
		splvec.splbio = MIPS_INT_MASK_SPL2;
		splvec.splvm = MIPS_INT_MASK_SPL2;
		splvec.spltty = MIPS_INT_MASK_SPL2;
		splvec.splclock = MIPS_INT_MASK_SPL5;
		splvec.splstatclock = MIPS_INT_MASK_SPL5;
		break;

	case DESKSTATION_RPC44:
		strcpy(cpu_model, "Deskstation rPC44");

		arc_bus_space_init(&arc_bus_io, "rpc44isaio",
		    RPC44_P_ISA_IO, RPC44_V_ISA_IO, 0, RPC44_S_ISA_IO);
		arc_bus_space_init(&arc_bus_mem, "rpc44isamem",
		    RPC44_P_ISA_MEM, RPC44_V_ISA_MEM, 0, RPC44_S_ISA_MEM);

#if NCOM > 0
		com_console_address = 0; /* Don't screew the mouse... */
#endif

		/*
		 * XXX
		 *	- rewrite spl handling to allow ISA clock > bio|tty|net
		 * or
		 *	- use MIP3_INTERNAL_TIMER_INTERRUPT for clock
		 */

		break;

	case DESKSTATION_TYNE:
		strcpy(cpu_model, "Deskstation Tyne");

		arc_bus_space_init(&arc_bus_io, "tyneisaio",
		    TYNE_P_ISA_IO, TYNE_V_ISA_IO, 0, TYNE_S_ISA_IO);
		arc_bus_space_init(&arc_bus_mem, "tyneisamem",
		    TYNE_P_ISA_MEM, TYNE_V_ISA_MEM, 0, TYNE_S_ISA_MEM);

#if NCOM > 0
		com_console_address = 0; /* Don't screew the mouse... */
#endif

		/*
		 * XXX
		 *	- rewrite spl handling to allow ISA clock > bio|tty|net
		 * or
		 *	- use MIP3_INTERNAL_TIMER_INTERRUPT for clock
		 */

		break;

	case SNI_RM200:
		strcpy(cpu_model, "Siemens Nixdorf RM200");
#if 0
		arc_bus_space_init(&arc_bus_io, "rm200isaio",
		    RM200_P_ISA_IO, RM200_V_ISA_IO, 0, RM200_S_ISA_IO);
		arc_bus_space_init(&arc_bus_mem, "rm200isamem",
		    RM200_P_ISA_MEM, RM200_V_ISA_MEM, 0, RM200_S_ISA_MEM);
#endif
#if NCOM > 0
		com_console_address = 0; /* Don't screew the mouse... */
#endif
		break;

	case -1:	/* Not identified as an ARC system. We have a couple */
			/* of other options. Systems not having an ARC Bios  */

			/* Make this more fancy when more comes in here */
		environment = envv;
#if 0
		cputype = ALGOR_P4032;
		strcpy(cpu_model, "Algorithmics P-4032");
		arc_bus_space_init(&arc_bus_io, "p4032bus",
		    0LL, MIPS_KSEG1_START,
		    MIPS_KSEG1_START, MIPS_KSEG2_START - MIPS_KSEG1_START);
		/* stride: (1 << 2) == 4 byte alignment */
		arc_bus_space_set_aligned_stride(&arc_bus_io, 2);
#if NCOM > 0
		com_console_address = P4032_COM1;
#endif
#else
		cputype = ALGOR_P5064;
		strcpy(cpu_model, "Algorithmics P-5064");
		arc_bus_space_init(&arc_bus_io, "p5064bus",
		    0LL, MIPS_KSEG1_START,
		    MIPS_KSEG1_START, MIPS_KSEG2_START - MIPS_KSEG1_START);
#if NCOM > 0
		com_console_address = P5064_COM1;
#endif
#endif

		mem_clusters[0].start = 0;
		mem_clusters[0].size =
		    mips_trunc_page(MIPS_KSEG0_TO_PHYS(kernel_text));
		mem_clusters[1].start = MIPS_KSEG0_TO_PHYS((int)kernend);
		if (getenv("memsize") != 0) {
			i = atoi(getenv("memsize"), 10);
			i = 1024 * 1024 * i;
			mem_clusters[1].size =
			    i - (int)(MIPS_KSEG0_TO_PHYS(kernend));
			mem_cluster_cnt = 2;
			physmem = i;
		} else {
			i = get_simm_size((int *)0, 128*1024*1024);
			mem_clusters[1].size =
			    i - (int)(MIPS_KSEG0_TO_PHYS(kernend));
			physmem = i;
/*XXX Ouch!!! */
			mem_clusters[2].start = i;
			mem_clusters[2].size = get_simm_size((int *)(i), 0);
			physmem += mem_clusters[2].size;
			mem_clusters[3].start = i+i/2;
			mem_clusters[3].size = get_simm_size((int *)(i+i/2), 0);
			mem_cluster_cnt = 4;
			physmem += mem_clusters[3].size;
		}
/*XXX*/
		argv[0] = getenv("bootdev");
		if(argv[0] == 0)
			argv[0] = "unknown";


		break;

	default:	/* This is probably the best we can do... */
		printf("kernel not configured for this system\n");
		cpu_reboot(RB_HALT | RB_NOSYNC, NULL);
	}
	physmem = btoc(physmem);

	/* look at argv[0] and compute bootdev for autoconfig setup */
	makebootdev(argv[0]);

	/*
	 * Look at arguments passed to us and compute boothowto.
	 * Default to SINGLE and ASKNAME if no args or
	 * SINGLE and DFLTROOT if this is a ramdisk kernel.
	 */
#ifdef MEMORY_DISK_HOOKS
	boothowto = RB_SINGLE;
#else
	boothowto = RB_SINGLE | RB_ASKNAME;
#endif /* MEMORY_DISK_HOOKS */
#ifdef KADB
	boothowto |= RB_KDB;
#endif
	get_eth_hw_addr(getenv("ethaddr"));
	cp = getenv("osloadoptions");
	if(cp) {
		while(*cp) {
			switch (*cp++) {
			case 'a': /* autoboot */
				boothowto &= ~RB_SINGLE;
				break;

			case 'm': /* mini root present in memory */
				boothowto |= RB_MINIROOT;
				break;

			case 'n': /* ask for names */
				boothowto |= RB_ASKNAME;
				break;

			case 'N': /* don't ask for names */
				boothowto &= ~RB_ASKNAME;
				break;

			case 's': /* use serial console */
				com_console = 1;
				break;

			case 'q': /* boot quietly */
				boothowto |= AB_QUIET;
				break;

			case 'v': /* boot verbosely */
				boothowto |= AB_VERBOSE;
				break;
			}

		}
	}

	/*
	 * Set the VM page size.
	 */
	uvm_setpagesize();
 
	/* make sure that we don't call BIOS console from now on */
	cn_tab = NULL;

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 *
	 * Now its time to abandon the BIOS and be self supplying.
	 * Start with cleaning out the TLB. Bye bye Microsoft....
	 *
	 * This may clobber PTEs needed by the BIOS.
	 */
	mips_vector_init();

	/*
	 * Map critical I/O spaces (e.g. for console printf(9)) on KSEG2.
	 * We cannot call VM functions here, since uvm is not initialized,
	 * yet.
	 * Since printf(9) is called before uvm_init() in main(),
	 * we have to handcraft console I/O space anyway.
	 *
	 * XXX - reserve these KVA space after UVM initialization.
	 */

	arc_init_wired_map();

	switch (cputype) {
	case ACER_PICA_61:
	case MAGNUM:
		tlb_init_pica();
		break;

	case NEC_R94:
	case NEC_R96:
		tlb_init_nec_eisa();
		break;

	case NEC_RAx94:
	case NEC_RD94:
	case NEC_JC94:
		tlb_init_nec_pci();
		break;

	case DESKSTATION_TYNE:
		tlb_init_tyne();
		break;

	case DESKSTATION_RPC44:
		break;

	case ALGOR_P4032:
	case ALGOR_P5064:
		break;

	case SNI_RM200:
		/*XXX*/
		break;
	}

#ifdef MFS
	/*
	 * Check to see if a mini-root was loaded into memory. It resides
	 * at the start of the next page just after the end of BSS.
	 */
	if (boothowto & RB_MINIROOT)
		kernend += round_page(mfs_initminiroot(kernend));
#endif

#ifdef DDB
#if 0 /* XXX */
	/* init symbols if present */
	if (esym)
		ddb_init(1000, &end, (int*)esym);
#endif
#endif

	maxmem = physmem;

	/* XXX: revisit here */

	/*
	 * Load the rest of the pages into the VM system.
	 */
	kernstartpfn = atop(trunc_page(
	    MIPS_KSEG0_TO_PHYS((kernel_text) - UPAGES * PAGE_SIZE)));
	kernendpfn = atop(round_page(MIPS_KSEG0_TO_PHYS(kernend)));
#if 0
	/* give all free memory to VM */
	/* XXX - currently doesn't work, due to "panic: pmap_enter: pmap" */
	for (i = 0; i < mem_cluster_cnt; i++) {
		if (mem_reserved[i])
			continue;
		first = atop(round_page(mem_clusters[i].start));
		last = atop(trunc_page(mem_clusters[i].start +
		    mem_clusters[i].size));
		if (last <= kernstartpfn || kernendpfn <= first) {
			uvm_page_physload(first, last, first, last,
			    VM_FREELIST_DEFAULT);
		} else {
			if (first < kernstartpfn)
				uvm_page_physload(first, kernstartpfn,
				    first, kernstartpfn, VM_FREELIST_DEFAULT);
			if (kernendpfn < last)
				uvm_page_physload(kernendpfn, last,
				    kernendpfn, last, , VM_FREELIST_DEFAULT);
		}
	}
#elif 0 /* XXX */
	/* give all free memory above the kernel to VM (non-contig version) */
	for (i = 0; i < mem_cluster_cnt; i++) {
		if (mem_reserved[i])
			continue;
		first = atop(round_page(mem_clusters[i].start));
		last = atop(trunc_page(mem_clusters[i].start +
		    mem_clusters[i].size));
		if (kernendpfn < last) {
			if (first < kernendpfn)
				first = kernendpfn;
			uvm_page_physload(first, last, first, last,
			    VM_FREELIST_DEFAULT);
		}
	}
#else
	/* give all memory above the kernel to VM (contig version) */
#if 1
	mem_clusters[0].start = 0;
	mem_clusters[0].size  = ctob(physmem);
	mem_cluster_cnt = 1;
#endif

	first = kernendpfn;
	last = physmem;
	uvm_page_physload(first, last, first, last, VM_FREELIST_DEFAULT);
#endif

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	/*
	 * Compute the size of system data structures.  pmap_bootstrap()
	 * needs some of this information.
	 */
	size = (vsize_t)allocsys(NULL, NULL);

	/*
	 * Initialize the virtual memory system.
	 */
	pmap_bootstrap();

	/*
	 * Allocate space for proc0's USPACE.
	 */
	v = (caddr_t)uvm_pageboot_alloc(USPACE); 
	proc0.p_addr = proc0paddr = (struct user *)v;
	proc0.p_md.md_regs = (struct frame *)(v + USPACE) - 1;
	curpcb = &proc0.p_addr->u_pcb;
	curpcb->pcb_context[11] = MIPS_INT_MASK | MIPS_SR_INT_IE; /* SR */

	/*
	 * Allocate space for system data structures.  These data structures
	 * are allocated here instead of cpu_startup() because physical
	 * memory is directly addressable.  We don't have to map these into
	 * virtual address space.
	 */
	v = (caddr_t)uvm_pageboot_alloc(size); 
	if ((allocsys(v, NULL) - v) != size)
		panic("mach_init: table size inconsistency");
}

void
machine_ConfigCache()
{
	switch (cputype) {
	case ACER_PICA_61:
#if 0	/* doesn't work */
		mips_L2CachePresent = 1;
		mips_L2CacheSize = 128 * 1024;
#endif
		/*
		 * if page zero in the idle loop is enabled,
		 * commands dump core due to incoherent cache.
		 */
		vm_page_zero_enable = FALSE; /* XXX - should be enabled */
		break;
	case DESKSTATION_RPC44:
	case DESKSTATION_TYNE:
	case SNI_RM200:
	case ALGOR_P4032:
	case ALGOR_P5064:
		/* XXX - use safe default, since those are not tested. */
		vm_page_zero_enable = FALSE; /* XXX - should be enabled */
		break;
	case NEC_R94:
		mips_L2CacheSize = 512 * 1024;
		break;
	case NEC_RAx94:
	case NEC_RD94:
	case NEC_JC94:
		mips_L2CacheSize = 1024 * 1024;
		break;
	case NEC_R96:
#if 0
		mips_L2CacheSize = 1 * 1024 * 1024;
#else	/* bigger is safer - XXX use arc bios to get the size */
		mips_L2CacheSize = 2 * 1024 * 1024;
#endif
		break;
	}
}

void
tlb_init_pica()
{
	arc_enter_wired(R4030_V_LOCAL_IO_BASE, R4030_P_LOCAL_IO_BASE,
	    PICA_P_INT_SOURCE, MIPS3_PG_SIZE_256K);
	arc_enter_wired(PICA_V_LOCAL_VIDEO_CTRL, PICA_P_LOCAL_VIDEO_CTRL,
	    PICA_P_LOCAL_VIDEO_CTRL + PICA_S_LOCAL_VIDEO_CTRL/2,
	    MIPS3_PG_SIZE_1M);
	arc_enter_wired(PICA_V_EXTND_VIDEO_CTRL, PICA_P_EXTND_VIDEO_CTRL,
	    PICA_P_EXTND_VIDEO_CTRL + PICA_S_EXTND_VIDEO_CTRL/2,
	    MIPS3_PG_SIZE_1M);
	arc_enter_wired(PICA_V_LOCAL_VIDEO, PICA_P_LOCAL_VIDEO,
	    PICA_P_LOCAL_VIDEO + PICA_S_LOCAL_VIDEO/2, MIPS3_PG_SIZE_4M);
	arc_enter_wired(PICA_V_ISA_IO, PICA_P_ISA_IO, PICA_P_ISA_MEM,
	    MIPS3_PG_SIZE_16M);
}

void
tlb_init_nec_eisa()
{
	arc_enter_wired(RD94_V_LOCAL_IO_BASE, RD94_P_LOCAL_IO_BASE, 0,
	    MIPS3_PG_SIZE_256K);
	arc_enter_wired(PICA_V_LOCAL_VIDEO_CTRL, PICA_P_LOCAL_VIDEO_CTRL,
	    PICA_P_LOCAL_VIDEO_CTRL + PICA_S_LOCAL_VIDEO_CTRL/2,
	    MIPS3_PG_SIZE_1M);
	arc_enter_wired(PICA_V_EXTND_VIDEO_CTRL, PICA_P_EXTND_VIDEO_CTRL,
	    PICA_P_EXTND_VIDEO_CTRL + PICA_S_EXTND_VIDEO_CTRL/2,
	    MIPS3_PG_SIZE_1M);
	arc_enter_wired(PICA_V_LOCAL_VIDEO, PICA_P_LOCAL_VIDEO,
	    PICA_P_LOCAL_VIDEO + PICA_S_LOCAL_VIDEO/2, MIPS3_PG_SIZE_4M);
	arc_enter_wired(RD94_V_PCI_IO, RD94_P_PCI_IO, RD94_P_PCI_MEM,
	    MIPS3_PG_SIZE_16M);
}

void
tlb_init_nec_pci()
{
	arc_enter_wired(RD94_V_LOCAL_IO_BASE, RD94_P_LOCAL_IO_BASE, 0,
	    MIPS3_PG_SIZE_256K);
	arc_enter_wired(RD94_V_PCI_IO, RD94_P_PCI_IO, RD94_P_PCI_MEM,
	    MIPS3_PG_SIZE_16M);
}

void
tlb_init_tyne()
{
	arc_enter_wired(TYNE_V_BOUNCE, TYNE_P_BOUNCE, 0, MIPS3_PG_SIZE_256K);
	arc_enter_wired(TYNE_V_ISA_IO, TYNE_P_ISA_IO, 0, MIPS3_PG_SIZE_1M);
	arc_enter_wired(TYNE_V_ISA_MEM, TYNE_P_ISA_MEM, 0, MIPS3_PG_SIZE_1M);
	arc_enter_wired(0xe3000000, 0xfff00000, 0, MIPS3_PG_SIZE_4K);
}

/*
 * Simple routine to figure out SIMM module size.
 * This code is a real hack and can surely be improved on... :-)
 */
static int
get_simm_size(fadr, max)
	int *fadr;
	int max;
{
	int msave;
	int msize;
	int ssize;
	static int a1 = 0, a2 = 0;
	static int s1 = 0, s2 = 0;

	if(!max) {
		if(a1 == (int)fadr)
			return(s1);
		else if(a2 == (int)fadr)
			return(s2);
		else
			return(0);
	}
	fadr = (int *)MIPS_PHYS_TO_KSEG1(MIPS_KSEG0_TO_PHYS((int)fadr));

	msize = max - 0x400000;
	ssize = msize - 0x400000;

	/* Find bank size of last module */
	while(ssize >= 0) {
		msave = fadr[ssize / 4];
		fadr[ssize / 4] = 0xC0DEB00F;
		if(fadr[msize /4 ] == 0xC0DEB00F) {
			fadr[ssize / 4] = msave;
			if(fadr[msize/4] == msave) {
				break;	/* Wrap around */
			}
		}
		fadr[ssize / 4] = msave;
		ssize -= 0x400000;
	}
	msize = msize - ssize;
	if(msize == max)
		return(msize);	/* well it never wrapped... */

	msave = fadr[0];
	fadr[0] = 0xC0DEB00F;
	if(fadr[msize / 4] == 0xC0DEB00F) {
		fadr[0] = msave;
		if(fadr[msize / 4] == msave)
			return(msize);	/* First module wrap = size */
	}

	/* Ooops! Two not equal modules. Find size of first + second */
	s1 = s2 = msize;
	ssize = 0;
	while(ssize < max) {
		msave = fadr[ssize / 4];
		fadr[ssize / 4] = 0xC0DEB00F;
		if(fadr[msize /4 ] == 0xC0DEB00F) {
			fadr[ssize / 4] = msave;
			if(fadr[msize/4] == msave) {
				break;	/* Found end of module 1 */
			}
		}
		fadr[ssize / 4] = msave;
		ssize += s2;
		msize += s2;
	}

	/* Is second bank dual sided? */
	fadr[(ssize+ssize/2)/4] = ~fadr[ssize];
	if(fadr[(ssize+ssize/2)/4] != fadr[ssize]) {
		a2 = ssize+ssize/2;
	}
	a1 = ssize;

	return(ssize);
}

/*
 * Return a pointer to the given environment variable.
 */
static char *
getenv(envname)
	char *envname;
{
	char **env = environment;
	int i;

	i = strlen(envname);

	while(*env) {
		if(strncasecmp(envname, *env, i) == 0 && (*env)[i] == '=') {
			return(&(*env)[i+1]);
		}
		env++;
	}
	return(NULL);
}

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit()
{
	static int initted;

	if (initted)
		return;
	initted = 1;

	if (!com_console) {
		switch (cputype) {
		case ACER_PICA_61:
#if NPC_JAZZIO > 0
			pccnattach();
			return;
#endif
#if NVGA_JAZZIO > 0
			if (vga_jazzio_cnattach() == 0) {
#if NPCKBC_JAZZIO > 0
				pckbc_cnattach(&pica_bus, PICA_SYS_KBD,
				    JAZZIO_KBCMDP, PCKBC_KBD_SLOT);
#endif
				return;
			}
#endif
			break;

		case MAGNUM:
		case NEC_R94:
#if NRASDISPLAY_JAZZIO > 0
			if (rasdisplay_jazzio_cnattach()) {
#if NPCKBC_JAZZIO > 0
				pckbc_cnattach(&pica_bus, PICA_SYS_KBD,
				    JAZZIO_KBCMDP, PCKBC_KBD_SLOT);
#endif
				return;
			}
#endif
			break;

		case NEC_RAx94:
#if 0				/* XXX - physical address unknown */
#if NPC_JAZZIO > 0
			pccnattach();
			return;
#endif
#endif
			break;

		case NEC_RD94:
		case NEC_JC94:
#if NTGA > 0
			necpb_init(&necpb_configuration);
			 /* XXX device number is hardcoded */
			if (tga_cnattach(&necpb_configuration.nc_iot,
			    &necpb_configuration.nc_memt,
			    &necpb_configuration.nc_pc, 0, 3, 0) == 0) {
#if NPCKBC_JAZZIO > 0
				pckbc_cnattach(&pica_bus, PICA_SYS_KBD,
				    JAZZIO_KBCMDP, PCKBC_KBD_SLOT);
#endif
				return;
			}
#endif
			break;

		case NEC_R96:
			/* XXX - some machines have jazz, and others have vga */
#if NPC_JAZZIO > 0
			pccnattach();
			return;
#endif
#if NVGA_JAZZIO > 0
			if (vga_jazzio_cnattach() == 0) {
#if NPCKBC_JAZZIO > 0
				pckbc_cnattach(&pica_bus, PICA_SYS_KBD,
				    JAZZIO_KBCMDP, PCKBC_KBD_SLOT);
#endif
				return;
			}
#endif
#if NRASDISPLAY_JAZZIO > 0
			if (rasdisplay_jazzio_cnattach()) {
#if NPCKBC_JAZZIO > 0
				pckbc_cnattach(&pica_bus, PICA_SYS_KBD,
				    JAZZIO_KBCMDP, PCKBC_KBD_SLOT);
#endif
				return;
			}
#endif
			break;

		case DESKSTATION_TYNE:
		case DESKSTATION_RPC44:
#if NPC_ISA > 0
			pccnattach();
			return;
#endif
#if NVGA_ISA > 0
			if (vga_isa_cnattach(&arc_bus_io, &arc_bus_mem) == 0) {
#if NPCKBC > 0
				pckbc_cnattach(&arc_bus_io, IO_KBD, KBCMDP,
				    PCKBC_KBD_SLOT);
#endif
			}
			return;
#endif
			break;

		case ALGOR_P4032:
		case ALGOR_P5064:
			/* XXX For now... */
			break;

		default:
#if NVGA > 0
			vga_localbus_console();
			return;
#endif
			break;
		}
	}

#if NCOM > 0
	if (com_console_address)
		comcnattach(arc_bus_com, com_console_address,
		    CONSPEED, com_freq, CONMODE);
#endif
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup()
{
	unsigned i;
	int base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	char pbuf[9];
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;		/* Shut up pmap debug during bootstrap */
#endif

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	printf("%s\n", cpu_model);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Allocate virtual address space for file I/O buffers.
	 * Note they are different than the array of headers, 'buf',
	 * and usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *)&buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != 0)
		panic("cpu_startup: cannot allocate VM for buffers");

	minaddr = (vaddr_t)buffers;
	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		bufpages = btoc(MAXBSIZE) * nbuf; /* do not overallocate RAM */
	}
	base = bufpages / nbuf;
	residual = bufpages % nbuf;

	/* now allocate RAM for buffers */
	for (i = 0; i < nbuf; i++) {
		vsize_t curbufsize;
		vaddr_t curbuf;
		struct vm_page *pg;

		/*
		 * Each buffer has MAXBSIZE bytes of VM space allocated.  Of
		 * that MAXBSIZE space, we allocate and map (base+1) pages
		 * for the first "residual" buffers, and then we allocate
		 * "base" pages for the rest.
		 */
		curbuf = (vaddr_t)buffers + (i * MAXBSIZE);
		curbufsize = NBPG * ((i < residual) ? (base+1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for "
				    "buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
				       VM_PROT_READ|VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}
	pmap_update();

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   16 * NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use KSEG to
	 * map those pages.
	 */

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();
}

/*
 * machine dependent system variables.
 */
int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	dev_t consdev;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case CPU_CONSDEV:
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		return (sysctl_rdstruct(oldp, oldlenp, newp, &consdev,
		    sizeof consdev));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

int	waittime = -1;
struct user dumppcb;	/* Actually, struct pcb would do. */

void
cpu_reboot(howto, bootstr)
	register int howto;
	char *bootstr;
{

	/* take a snap shot before clobbering any registers */
	if (curproc)
		savectx((struct user *)curpcb);

#ifdef DEBUG
	if (panicstr)
		stacktrace();
#endif

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		/* fill curproc with live object */
		if (curproc == NULL)
			curproc = &proc0;
		/*
		 * Synchronize the disks....
		 */
		waittime = 0;
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}
	(void) splhigh();		/* extreme priority */

	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

	doshutdownhooks();

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(1);	/* for proper keyboard command handling */
		cngetc();
		cnpollc(0);
	}

	printf("rebooting...\n");
	delay(1000000);

	switch (cputype) {
	case ACER_PICA_61:
	case MAGNUM:
	case NEC_R94:
	case NEC_R96:
	case NEC_RD94:
	case NEC_JC94:
		arc_sysreset(PICA_SYS_KBD, JAZZIO_KBCMDP);
		break;
	case DESKSTATION_TYNE:
		arc_sysreset(TYNE_V_ISA_IO + IO_KBD, KBCMDP);
		break;
	case DESKSTATION_RPC44:
		arc_sysreset(RPC44_V_ISA_IO + IO_KBD, KBCMDP);
		break;
	default:
		break;
	}
	__asm__(" li $2, 0xbfc00000; jr $2; nop\n");
	for (;;)
		; /* Forever */
	/* NOTREACHED */
}

/*
 * Pass system reset command to keyboard controller (8042).
 */
static void
arc_sysreset(addr, cmd_offset)
	bus_addr_t addr; 
	bus_size_t cmd_offset;
{
	volatile u_int8_t *kbdata = (u_int8_t *)addr + KBDATAP;
	volatile u_int8_t *kbcmd = (u_int8_t *)addr + cmd_offset;

#define KBC_ARC_SYSRESET 0xd1

	delay(1000);
	*kbcmd = KBC_ARC_SYSRESET;
	delay(1000);
	*kbdata = 0;
}

/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.  Unfortunately, we can't read the hardware registers.
 * We guarantee that the time will be greater than the value obtained by a
 * previous call.
 */
void
microtime(tvp)
	register struct timeval *tvp;
{
	int s = splclock();
	static struct timeval lasttime;

	*tvp = time;
#ifdef notdef
	tvp->tv_usec += clkread();
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
#endif
	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}

void
initcpu()
{

	switch(cputype) {
	case ACER_PICA_61:
	case MAGNUM:
		/*
		 * Disable all interrupts. New masks will be set up
		 * during system configuration
		 */
		out16(PICA_SYS_LB_IE,0x000);
		out32(R4030_SYS_EXT_IMASK, 0x00);
		break;

	case NEC_R94:
	case NEC_RAx94:
	case NEC_RD94:
	case NEC_R96:
	case NEC_JC94:
		out16(RD94_SYS_LB_IE1, 0);
		out16(RD94_SYS_LB_IE2, 0);
		out32(RD94_SYS_EXT_IMASK, 0);
		break;
	}
}
/*
 * Convert "xx:xx:xx:xx:xx:xx" string to ethernet hardware address.
 */
static void
get_eth_hw_addr(s)
	char *s;
{
	int i;
	if(s != NULL) {
		for(i = 0; i < 6; i++) {
			eth_hw_addr[i] = atoi(s, 16);
			s += 3;		/* Don't get to fancy here :-) */
		}
	}
}

/*
 * Convert an ASCII string into an integer.
 */
static int
atoi(s, b)
	const char *s;
	int   b;
{
	int c;
	unsigned base = b, d;
	int neg = 0, val = 0;

	if (s == 0 || (c = *s++) == 0)
		goto out;

	/* skip spaces if any */
	while (c == ' ' || c == '\t')
		c = *s++;

	/* parse sign, allow more than one (compat) */
	while (c == '-') {
		neg = !neg;
		c = *s++;
	}

	/* parse base specification, if any */
	if (c == '0') {
		c = *s++;
		switch (c) {
		case 'X':
		case 'x':
			base = 16;
			c = *s++;
			break;
		case 'B':
		case 'b':
			base = 2;
			c = *s++;
			break;
		default:
			base = 8;
		}
	}

	/* parse number proper */
	for (;;) {
		if (c >= '0' && c <= '9')
			d = c - '0';
		else if (c >= 'a' && c <= 'z')
			d = c - 'a' + 10;
		else if (c >= 'A' && c <= 'Z')
			d = c - 'A' + 10;
		else
			break;
		val *= base;
		val += d;
		c = *s++;
	}
	if (neg)
		val = -val;
out:
	return val;	
}
