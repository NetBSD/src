/* $NetBSD: machdep.c,v 1.93.2.1 2013/02/25 00:28:48 tls Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.93.2.1 2013/02/25 00:28:48 tls Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_compat_sunos.h"
#include "opt_modular.h"
#include "opt_panicbutton.h"
#include "opt_m68k_arch.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/exec_aout.h>		/* for MID_* */
#include <sys/core.h>
#include <sys/kauth.h>
#include <sys/kcore.h>
#include <sys/vnode.h>
#include <sys/syscallargs.h>
#include <sys/ksyms.h>
#include <sys/module.h>
#ifdef	KGDB
#include <sys/kgdb.h>
#endif
#include <sys/boot_flag.h>
#define ELFSIZE 32
#include <sys/exec_elf.h>

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/kcore.h>	/* XXX should be pulled in by sys/kcore.h */

#include <dev/cons.h>
#include <dev/mm.h>

#if defined(DDB)
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

#include "ksyms.h"

/*
 * Info for CTL_HW
 */
char	machine[] = MACHINE;
char	cpu_model[120];

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

struct vm_map *phys_map = NULL;

int	maxmem;			/* max memory per process */

extern	u_int lowram;

void luna68k_init(void);
void identifycpu(void);
void dumpsys(void);

void straytrap(int, u_short);
void nmihand(struct frame);

int  cpu_dumpsize(void);
int  cpu_dump(int (*)(dev_t, daddr_t, void *, size_t), daddr_t *);
void cpu_init_kcore_hdr(void);

#if NKSYMS || defined(DDB) || defined(MODULAR)
vsize_t symtab_size(vaddr_t);
#endif
extern char end[];
extern void *esym;

/*
 * Machine-independent crash dump header info.
 */
cpu_kcore_hdr_t cpu_kcore_hdr;

int	machtype;	/* model: 1 for LUNA-1, 2 for LUNA-2 */
int	sysconsole;	/* console: 0 for ttya, 1 for video */

extern struct consdev syscons;
extern void omfb_cnattach(void);
extern void ws_cnattach(void);
extern void syscnattach(int);

/*
 * On the 68020/68030, the value of delay_divisor is roughly
 * 2048 / cpuspeed (where cpuspeed is in MHz).
 *
 * On the 68040/68060(?), the value of delay_divisor is roughly
 * 759 / cpuspeed (where cpuspeed is in MHz).
 * XXX -- is the above formula correct?
 */
int	cpuspeed = 25;		/* only used for printing later */
int	delay_divisor = 30;	/* for delay() loop count */

/*
 * Early initialization, before main() is called.
 */
void
luna68k_init(void)
{
	volatile uint8_t *pio0 = (void *)0x49000000;
	int sw1, i;
	char *cp;
	extern char bootarg[64];

	extern paddr_t avail_start, avail_end;

	/* initialize cn_tab for early console */
#if 1
	cn_tab = &syscons;
#else
	cn_tab = &romcons;
#endif

	/*
	 * Tell the VM system about available physical memory.  The
	 * luna68k only has one segment.
	 */
	uvm_page_physload(atop(avail_start), atop(avail_end),
	    atop(avail_start), atop(avail_end), VM_FREELIST_DEFAULT);

	/*
	 * Initialize error message buffer (at end of core).
	 * avail_end was pre-decremented in pmap_bootstrap to compensate.
	 */
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_kenter_pa((vaddr_t)msgbufaddr + i * PAGE_SIZE,
		    avail_end + i * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, 0);
	pmap_update(pmap_kernel());
	initmsgbuf(msgbufaddr, m68k_round_page(MSGBUFSIZE));


	pio0[3] = 0xb6;
	pio0[2] = 1 << 6;		/* enable parity check */
	pio0[3] = 0xb6;
	sw1 = pio0[0];			/* dip sw1 value */
	sw1 ^= 0xff;
	sysconsole = !(sw1 & 0x2);	/* console selection */

	boothowto = 0;
	i = 0;
	/*
	 * 'bootarg' on LUNA has:
	 *   "<args of x command> ENADDR=<addr> HOST=<host> SERVER=<name>"
	 * where <addr> is MAC address of which network loader used (not
	 * necessarily same as one at 0x4101.FFE0), <host> and <name>
	 * are the values of HOST and SERVER environment variables.
	 *
	 * 'bootarg' on LUNA-II has "<args of x command>" only.
	 *
	 * NetBSD/luna68k cares only the first argment; any of "sda".
	 */
	for (cp = bootarg; *cp != ' ' && *cp != 0; cp++) {
		BOOT_FLAG(*cp, boothowto);
		if (i++ >= sizeof(bootarg))
			break;
	}
#if 0 /* overload 1:sw1, which now means 'go ROM monitor' after poweron */
	if (boothowto == 0)
		boothowto = (sw1 & 0x1) ? RB_SINGLE : 0;
#endif
}

/*
 * Console initialization: called early on from main,
 */
void
consinit(void)
{

	if (sysconsole == 0)
		syscnattach(0);
	else {
		omfb_cnattach();
		ws_cnattach();
	}

#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf((esym != NULL) ? 1 : 0, (void *)&end, esym);
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		cpu_Debugger();
#endif
}

#if NKSYMS || defined(DDB) || defined(MODULAR)

/*
 * Check and compute size of DDB symbols and strings.
 *
 * Note this function could be called from locore.s before MMU is turned on
 * so we should avoid global variables and function calls.
 */
vsize_t
symtab_size(vaddr_t hdr)
{
	int i;
	Elf_Ehdr *ehdr;
	Elf_Shdr *shp;
	vaddr_t maxsym;

	/*
	 * Check the ELF headers.
	 */

	ehdr = (void *)hdr;
	if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
	    ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
	    ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
	    ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
	    ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
		return 0;
	}

	/*
	 * Find the end of the symbols and strings.
	 */

	maxsym = 0;
	shp = (Elf_Shdr *)(hdr + ehdr->e_shoff);
	for (i = 0; i < ehdr->e_shnum; i++) {
		if (shp[i].sh_type != SHT_SYMTAB &&
		    shp[i].sh_type != SHT_STRTAB) {
			continue;
		}
		maxsym = max(maxsym, shp[i].sh_offset + shp[i].sh_size);
	}

	return maxsym;
}
#endif /* NKSYMS || defined(DDB) || defined(MODULAR) */

/*
 * cpu_startup: allocate memory for variable-sized tables.
 */
void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];
	extern void greeting(void);

	if (fputype != FPU_NONE)
		m68k_make_fpu_idle_frame();

	/*
	 * Initialize the kernel crash dump header.
	 */
	cpu_init_kcore_hdr();

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	identifycpu();

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, false, NULL);

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	/*
	 * Say "Hi" to the world
	 */
	greeting();
}

void
identifycpu(void)
{
	extern int cputype;
	const char *model, *fpu;

	memset(cpu_model, 0, sizeof(cpu_model));
	switch (cputype) {
	case CPU_68030:
		model ="LUNA-I";
		switch (fputype) {
		case FPU_68881:
			fpu = "MC68881";
			break;
		case FPU_68882:
			fpu = "MC68882";
			break;
		case FPU_NONE:
			fpu = "no";
			break;
		default:
			fpu = "unknown";
			break;
		}
		snprintf(cpu_model, sizeof(cpu_model),
		    "%s (MC68030 CPU+MMU, %s FPU)", model, fpu);
		machtype = LUNA_I;
		/* 20MHz 68030 */
		cpuspeed = 20;
		delay_divisor = 102;
		hz = 60;
		break;
#if defined(M68040)
	case CPU_68040:
		model ="LUNA-II";
		snprintf(cpu_model, sizeof(cpu_model),
		    "%s (MC68040 CPU+MMU+FPU, 4k on-chip physical I/D caches)",
		    model);
		machtype = LUNA_II;
		/* 25MHz 68040 */
		cpuspeed = 25;
		delay_divisor = 30;
		/* hz = 100 on LUNA-II */
		break;
#endif
	default:
		panic("unknown CPU type");
	}
	printf("%s\n", cpu_model);
}

/*
 * machine dependent system variables.
 */
SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "console_device", NULL,
		       sysctl_consdev, 0, NULL, sizeof(dev_t),
		       CTL_MACHDEP, CPU_CONSDEV, CTL_EOL);
}

int	waittime = -1;

void
cpu_reboot(int howto, char *bootstr)
{
	struct pcb *pcb = lwp_getpcb(curlwp);
	extern void doboot(void);

	/* take a snap shot before clobbering any registers */
	if (pcb != NULL)
		savectx(pcb);

	/* If system is hold, just halt. */
	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	/* Disable interrupts. */
	splhigh();

	/* If rebooting and a dump is requested, do it. */
	if (howto & RB_DUMP)
		dumpsys();

haltsys:
	/* Run any shutdown hooks. */
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	/* Finally, halt/reboot the system. */
	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		volatile uint8_t *pio = (void *)0x4d000000;

		printf("power is going down.\n");
		DELAY(100000);
		pio[3] = 0x94;
		pio[2] = 0 << 4;
		for (;;)
			/* NOP */;
	}
	if (howto & RB_HALT) {
		printf("System halted.	Hit any key to reboot.\n\n");
		(void)cngetc();
	}

	printf("rebooting...\n");
	DELAY(100000);
	doboot();
	/*NOTREACHED*/
	for (;;)
		;
}

/*
 * Initialize the kernel crash dump header.
 */
void
cpu_init_kcore_hdr(void)
{
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;

	memset(&cpu_kcore_hdr, 0, sizeof(cpu_kcore_hdr));

	/*
	 * Initialize the `dispatcher' portion of the header.
	 */
	strcpy(h->name, machine);
	h->page_size = PAGE_SIZE;
	h->kernbase = KERNBASE;

	/*
	 * Fill in information about our MMU configuration.
	 */
	m->mmutype	= mmutype;
	m->sg_v		= SG_V;
	m->sg_frame	= SG_FRAME;
	m->sg_ishift	= SG_ISHIFT;
	m->sg_pmask	= SG_PMASK;
	m->sg40_shift1	= SG4_SHIFT1;
	m->sg40_mask2	= SG4_MASK2;
	m->sg40_shift2	= SG4_SHIFT2;
	m->sg40_mask3	= SG4_MASK3;
	m->sg40_shift3	= SG4_SHIFT3;
	m->sg40_addr1	= SG4_ADDR1;
	m->sg40_addr2	= SG4_ADDR2;
	m->pg_v		= PG_V;
	m->pg_frame	= PG_FRAME;

	/*
	 * Initialize pointer to kernel segment table.
	 */
	m->sysseg_pa = (uint32_t)(pmap_kernel()->pm_stpa);

	/*
	 * Initialize relocation value such that:
	 *
	 *	pa = (va - KERNBASE) + reloc
	 *
	 * Since we're linked and loaded at the same place,
	 * and the kernel is mapped va == pa, this is 0.
	 */
	m->reloc = 0;

	/*
	 * Define the end of the relocatable range.
	 */
	m->relocend = (uint32_t)end;

	/*
	 * The luna68k has one contiguous memory segment.
	 */
	m->ram_segs[0].start = 0 /* lowram */;
	m->ram_segs[0].size  = ctob(physmem);
}

/*
 * Compute the size of the machine-dependent crash dump header.
 * Returns size in disk blocks.
 */

#define CHDRSIZE (ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t)))
#define MDHDRSIZE roundup(CHDRSIZE, dbtob(1))

int
cpu_dumpsize(void)
{

	return btodb(MDHDRSIZE);
}

/*
 * Called by dumpsys() to dump the machine-dependent header.
 */
int
cpu_dump(int (*dump)(dev_t, daddr_t, void *, size_t), daddr_t *blknop)
{
	int buf[MDHDRSIZE / sizeof(int)];
	cpu_kcore_hdr_t *chdr;
	kcore_seg_t *kseg;
	int error;

	kseg = (kcore_seg_t *)buf;
	chdr = (cpu_kcore_hdr_t *)&buf[ALIGN(sizeof(kcore_seg_t)) /
	    sizeof(int)];

	/* Create the segment header. */
	CORE_SETMAGIC(*kseg, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	kseg->c_size = MDHDRSIZE - ALIGN(sizeof(kcore_seg_t));

	memcpy(chdr, &cpu_kcore_hdr, sizeof(cpu_kcore_hdr_t));
	error = (*dump)(dumpdev, *blknop, (void *)buf, sizeof(buf));
	*blknop += btodb(sizeof(buf));
	return error;
}

/*
 * These variables are needed by /sbin/savecore
 */
uint32_t dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first PAGE_SIZE of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf(void)
{
	int chdrsize;	/* size of dump header */
	int nblks;	/* size of dump area */

	if (dumpdev == NODEV)
		return;
	nblks = bdev_size(dumpdev);
	chdrsize = cpu_dumpsize();

	dumpsize = btoc(cpu_kcore_hdr.un._m68k.ram_segs[0].size);

	/*
	 * Check do see if we will fit.  Note we always skip the
	 * first PAGE_SIZE in case there is a disk label there.
	 */
	if (nblks < (ctod(dumpsize) + chdrsize + ctod(1))) {
		dumpsize = 0;
		dumplo = -1;
		return;
	}

	/*
	 * Put dump at the end of the partition.
	 */
	dumplo = (nblks - 1) - ctod(dumpsize) - chdrsize;
}

/*
 * Dump physical memory onto the dump device.  Called by cpu_reboot().
 */
void
dumpsys(void)
{
	const struct bdevsw *bdev;
	daddr_t blkno;		/* current block to write */
				/* dump routine */
	int (*dump)(dev_t, daddr_t, void *, size_t);
	int pg;			/* page being dumped */
	paddr_t maddr;		/* PA being dumped */
	int error;		/* error code from (*dump)() */

	/* XXX initialized here because of gcc lossage */
	maddr = 0 /* lowram */;
	pg = 0;

	/* Make sure dump device is valid. */
	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL)
		return;
	if (dumpsize == 0) {
		cpu_dumpconf();
		if (dumpsize == 0)
			return;
	}
	if (dumplo <= 0) {
		printf("\ndump to dev %u,%u  not possible\n",
		    major(dumpdev), minor(dumpdev));
		return;
	}
	dump = bdev->d_dump;
	blkno = dumplo;

	printf("\ndumping to dev %u,%u offset %ld\n",
	    major(dumpdev), minor(dumpdev), dumplo);

	printf("dump ");

	/* Write the dump header. */
	error = cpu_dump(dump, &blkno);
	if (error)
		goto bad;

	for (pg = 0; pg < dumpsize; pg++) {
#define NPGMB	(1024*1024/PAGE_SIZE)
		/* print out how many MBs we have dumped */
		if (pg && (pg % NPGMB) == 0)
			printf("%d ", pg / NPGMB);
#undef NPGMB
		pmap_enter(pmap_kernel(), (vaddr_t)vmmap, maddr,
		    VM_PROT_READ, VM_PROT_READ|PMAP_WIRED);

		pmap_update(pmap_kernel());
		error = (*dump)(dumpdev, blkno, vmmap, PAGE_SIZE);
 bad:
		switch (error) {
		case 0:
			maddr += PAGE_SIZE;
			blkno += btodb(PAGE_SIZE);
			break;

		case ENXIO:
			printf("device bad\n");
			return;

		case EFAULT:
			printf("device not ready\n");
			return;

		case EINVAL:
			printf("area improper\n");
			return;

		case EIO:
			printf("i/o error\n");
			return;

		case EINTR:
			printf("aborted from console\n");
			return;

		default:
			printf("error %d\n", error);
			return;
		}
	}
	printf("succeeded\n");
}

void
straytrap(int pc, u_short evec)
{

	printf("unexpected trap (vector offset %x) from %x\n",
	    evec & 0xFFF, pc);
}

int	*nofault;

int
badaddr(register void *addr, int nbytes)
{
	register int i;
	label_t faultbuf;

#ifdef lint
	i = *addr; if (i) return (0);
#endif

	nofault = (int *)&faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *)0;
		return 1;
	}

	switch (nbytes) {
	case 1:
		i = *(volatile int8_t *)addr;
		break;

	case 2:
		i = *(volatile int16_t *)addr;
		break;

	case 4:
		i = *(volatile int32_t *)addr;
		break;

	default:
		panic("badaddr: bad request");
	}
	nofault = (int *)0;
	return 0;
}

void luna68k_abort(const char *);

static int innmihand;	/* simple mutex */

/*
 * Level 7 interrupts are caused by e.g. the ABORT switch.
 *
 * If we have DDB, then break into DDB on ABORT.  In a production
 * environment, bumping the ABORT switch would be bad, so we enable
 * panic'ing on ABORT with the kernel option "PANICBUTTON".
 */
void
nmihand(struct frame frame)
{

	/* Prevent unwanted recursion */
	if (innmihand)
		return;
	innmihand = 1;

	luna68k_abort("ABORT SWITCH");

	innmihand = 0;
}

/*
 * Common code for handling ABORT signals from buttons, switches,
 * serial lines, etc.
 */
void
luna68k_abort(const char *cp)
{

#ifdef DDB
	printf("%s\n", cp);
	cpu_Debugger();
#else
#ifdef PANICBUTTON
	panic(cp);
#else
	printf("%s ignored\n", cp);
#endif /* PANICBUTTON */
#endif /* DDB */
}

/*
 * cpu_exec_aout_makecmds():
 *	CPU-dependent a.out format hook for execve().
 *
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 */
int
cpu_exec_aout_makecmds(struct lwp *l, struct exec_package *epp)
{
	int error = ENOEXEC;
#ifdef COMPAT_SUNOS
	extern sunos_exec_aout_makecmds(struct proc *, struct exec_package *);

	if ((error = sunos_exec_aout_makecmds(l->l_proc, epp)) == 0)
		return 0;
#endif
	return error;
}

#ifdef MODULAR
/*
 * Push any modules loaded by the bootloader etc.
 */
void
module_init_md(void)
{
}
#endif

#ifdef notyet
/*
 * romcons is useful until m68k TC register is initialized.
 */
int  romcngetc(dev_t);
void romcnputc(dev_t, int);

struct consdev romcons = {
	NULL,
	NULL,
	romcngetc,
	romcnputc,
	nullcnpollc,
	makedev(7, 0), /* XXX */
	CN_DEAD,
};

#define __		((int **)0x41000000)
#define GETC()		(*(int (*)())__[6])()
#define PUTC(x)		(*(void (*)())__[7])(x)

#define ROMPUTC(x) \
({					\
	register _r;			\
	__asm volatile ("			\
		movc	%%vbr,%0	; \
		movel	%0,%%sp@-	; \
		clrl	%0		; \
		movc	%0,%%vbr"	\
		: "=r" (_r));		\
	PUTC(x);			\
	__asm volatile ("			\
		movel	%%sp@+,%0	; \
		movc	%0,%%vbr"	\
		: "=r" (_r));		\
})

#define ROMGETC() \
({					\
	register _r, _c;		\
	__asm volatile ("			\
		movc	%%vbr,%0	; \
		movel	%0,%%sp@-	; \
		clrl	%0		; \
		movc	%0,%%vbr"	\
		: "=r" (_r));		\
	_c = GETC();			\
	__asm volatile ("			\
		movel	%%sp@+,%0	; \
		movc	%0,%%vbr"	\
		: "=r" (_r));		\
	_c;				\
})

void
romcnputc(dev_t dev, int c)
{
	int s;

	s = splhigh();
	ROMPUTC(c);
	splx(s);
}

int
romcngetc(dev_t dev)
{
	int s, c;

	do {
		s = splhigh();
		c = ROMGETC();
		splx(s);
	} while (c == -1);
	return c;
}
#endif

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{

	return (pa < lowram || pa >= 0xfffffffc) ? EFAULT : 0;
}
