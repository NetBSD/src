/*	$NetBSD: m68k_machdep.c,v 1.23 2026/04/28 14:58:31 thorpej Exp $	*/

/*-
 * Copyright (c) 1997, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
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

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: machdep.c 1.74 92/12/20$
 *
 *	@(#)machdep.c	8.10 (Berkeley) 4/20/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: m68k_machdep.c,v 1.23 2026/04/28 14:58:31 thorpej Exp $");

#include "opt_compat_netbsd.h"
#include "opt_compat_sunos.h"
#include "opt_execfmt.h"
#include "opt_m68k_arch.h"

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/exec.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/msgbuf.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/vnode.h>
#include <sys/kernel.h>
#include <sys/kcore.h>

#include <dev/cons.h>

#ifdef EXEC_AOUT
#include <sys/exec_aout.h>	/* for cpu_exec_aout_makecmds() prototype */
#endif

#include <uvm/uvm_extern.h>

#include <m68k/m68k.h>
#include <m68k/kcore.h>
#include <m68k/frame.h>
#include <m68k/pcb.h>
#include <m68k/reg.h>
#ifdef M68060
#include <m68k/pcr.h>
#endif
#include <m68k/seglist.h>

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;		/* from <machine/param.h> */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

/* cpu speed in kHz */
int	cpuspeed_khz;

#if defined(M68020) || defined(M68030)
/* fpu speed in kHz */
int	fpuspeed_khz;
#endif

#ifdef M68K_EC
/* external cache size in bytes */
int	ecsize;
#endif

/*
 * Physical memory segments.  These segments are included in kernel
 * crash dumps, and the available ranges of the segments are loaded
 * into the VM system as managed pages.
 */
phys_seg_list_t phys_seg_list[VM_PHYSSEG_MAX];

/*
 * __HAVE_M68K_PRIVATE_MSGSBUF is a hook for the Sun platforms that
 * are re-using PROM mappings of memory for the messsage buffer that
 * are not guaranteed to be physically contiguous.
 *
 * How we act on this: We don't care about the PA of the message buffer
 * at all, and we assume that the message buffer has already been mapped
 * as part of the VM bootstrap.
 */
#ifndef __HAVE_M68K_PRIVATE_MSGSBUF
paddr_t	msgbufpa = (paddr_t)-1;		/* PA of message buffer */
#endif
void	*msgbufaddr;

/*
 * Common tasks for machine_init().
 */
void
machine_init_common(paddr_t nextpa)
{
	extern paddr_t avail_start, avail_end;
	int i;
#ifndef __HAVE_M68K_PRIVATE_MSGSBUF
	int end_seg = 0;
#endif

	/*
	 * Compute the boundaries of available memory.
	 */
	avail_start = UINT_MAX;
	avail_end = 0;
	for (i = 0; i < VM_PHYSSEG_MAX; i++) {
		/*
		 * Make sure the memory segment begins/ends on
		 * page boundaries.
		 *
		 * If ps_avail_start and ps_avail_end have not already
		 * been initialized, go ahead and validate those.
		 */
		phys_seg_list[i].ps_start =
		    m68k_round_page(phys_seg_list[i].ps_start);
		phys_seg_list[i].ps_end =
		    m68k_trunc_page(phys_seg_list[i].ps_end);

		if (phys_seg_list[i].ps_avail_start == 0 &&
		    phys_seg_list[i].ps_avail_end == 0) {
			phys_seg_list[i].ps_avail_start =
			    phys_seg_list[i].ps_start;
			phys_seg_list[i].ps_avail_end =
			    phys_seg_list[i].ps_end;
		}

		if (phys_seg_list[i].ps_start == phys_seg_list[i].ps_end) {
			/* Empty segment. */
			continue;
		}

		/*
		 * nextpa represents the next available page after
		 * the pages already consumed by the bootstrap
		 * process.  If it falls within this physical segment,
		 * just the available range as necessary.
		 */
		if (nextpa >= phys_seg_list[i].ps_start &&
		    /* this <= is intentional */
		    nextpa <= phys_seg_list[i].ps_end &&
		    nextpa > phys_seg_list[i].ps_avail_start) {
			phys_seg_list[i].ps_avail_start = nextpa;
		}

		if (phys_seg_list[i].ps_avail_start ==
		    phys_seg_list[i].ps_avail_end) {
			/* Segment has been completely gobbled up. */
			continue;
		}

		if (phys_seg_list[i].ps_avail_start < avail_start) {
			avail_start = phys_seg_list[i].ps_avail_start;
		}
		if (phys_seg_list[i].ps_avail_end > avail_end) {
			avail_end = phys_seg_list[i].ps_avail_end;
#ifndef __HAVE_M68K_PRIVATE_MSGSBUF
			end_seg = i;
#endif
		}
	}

#ifndef __HAVE_M68K_PRIVATE_MSGSBUF
	/*
	 * If the message buffer has not already been allocated,
	 * allocate it from the end of physical memory.
	 */
	if (msgbufpa == (paddr_t)-1) {
		KASSERT((phys_seg_list[end_seg].ps_avail_end
			 - phys_seg_list[end_seg].ps_avail_start)
			>= round_page(MSGBUFSIZE));
		phys_seg_list[end_seg].ps_avail_end -= round_page(MSGBUFSIZE);
		msgbufpa = phys_seg_list[end_seg].ps_avail_end;
	}
#endif

#ifndef VM_PHYS_SEG_TO_FREELIST
#define	VM_PHYS_SEG_TO_FREELIST(s) VM_FREELIST_DEFAULT
#endif

	/*
	 * Now load the pages into the VM system.
	 */
	for (i = 0; i < VM_PHYSSEG_MAX; i++) {
		if (phys_seg_list[i].ps_avail_start ==
		    phys_seg_list[i].ps_avail_end) {
			/* Segment has been completely gobbled up. */
			continue;
		}
		uvm_page_physload(atop(phys_seg_list[i].ps_avail_start),
				  atop(phys_seg_list[i].ps_avail_end),
				  atop(phys_seg_list[i].ps_avail_start),
				  atop(phys_seg_list[i].ps_avail_end),
				  VM_PHYS_SEG_TO_FREELIST(i));
	}

	/*
	 * Initialize the kernel message buffer.
	 */
#ifndef __HAVE_M68K_PRIVATE_MSGSBUF
	for (i = 0; i < btoc(round_page(MSGBUFSIZE)); i++) {
		pmap_kenter_pa((vaddr_t)msgbufaddr + i * PAGE_SIZE,
			       msgbufpa + i * PAGE_SIZE,
			       VM_PROT_READ|VM_PROT_WRITE, 0);
	}
	pmap_update(pmap_kernel());
#endif
	initmsgbuf(msgbufaddr, round_page(MSGBUFSIZE));
}

/*
 * Common tasks for cpu_startup().
 */
void
cpu_startup_common(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

	/* Initialize the FPU, if present. */
	fpu_init();

	/* Set the model info. */
	machine_set_model();

	/*
	 * Good {morning,afternoon,evening,night}.
	 * XXX Should augment banner() and then switch to using it.
	 */
	printf("%s%s", copyright, version);
#ifdef __HAVE_CPU_STARTUP_PRINT_MACHINE_MODEL
	cpu_startup_print_machine_model(printf);
#endif
#ifdef __HAVE_CPU_STARTUP_PRINT_TOTAL_MEMORY
	cpu_startup_print_total_memory(printf);
#else
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvm_availmem(false)));
	printf("avail memory = %s\n", pbuf);

	/*
	 * Allocate a submap for physio
	 */
	minaddr = 0;
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, false, NULL);
}
__weak_alias(cpu_startup, cpu_startup_common);

static const char *
mhz_string_from_khz(int speed_khz, char *buf, size_t bufsize)
{
	int whole_mhz = speed_khz / 1000;
	int frac_mhz = (speed_khz % 1000) / 10;
	if ((frac_mhz % 10) == 0) {
		frac_mhz /= 10;
	}
	if (frac_mhz == 0) {
		snprintf(buf, bufsize, " @ %dMHz", whole_mhz);
	} else {
		snprintf(buf, bufsize, " @ %d.%dMHz", whole_mhz, frac_mhz);
	}
	return buf;
}

void
cpu_startup_print_machine_model(void (*pr)(const char *, ...)
				__printflike(1, 2))
{
	char speed_str[sizeof("@ xxx.xxxMHz")] = { 0 };

	/*
	 * Caller should have set the system model already.  We
	 * will print the CPU information after, like so:
	 *
	 * MODEL
	 * CPU MMU FPU CACHE
	 *
	 * Examples:

  Qemu 10.1.2 Virt platform
  MC68040+MMU+FPU, 4k+4k on-chip I/D caches

  Motorola MVME-147
  MC68030 CPU+MMU @ 25MHz, MC68882 FPU

  HP 9000/433s
  MC68040 CPU+MMU+FPU @ 33MHz, 4K+4K on-chip I/D caches

  HP 9000/320
  MC68020 CPU @ 16.67MHz, HP MMU, MC68881 FPU
  External 16K virtual-address cache

  HP 9000/350
  MC68020 CPU @ 25MHz, HP MMU, MC68881 FPU @ 20MHz
  External 32K virtual-address cache

	 * ^^^ Yes, there's at least one HP system where the FPU
	 * has a different clock than the CPU.
	 *
	 * There is a hook that allows machine-specific code to print
	 * a more informative model banner, such as:

  Model: sun3x 80
  MC68030 CPU+MMU @ 20MHz, MC68882 FPU

  Model: sun3 160
  MC68020 CPU @ 16.67MHz, Sun MMU, MC68881 FPU

  Model: sun2 {120,170}
  MC68010 CPU @ 10MHz, Sun MMU

  SONY NET WORK STATION, Model NWS-1710, Machine ID #123456
  MC68030 CPU+MMU @ 25MHz, MC68882 FPU

	 * (In this last example, cpu_model() returns "NWS-1710".)
	 * In the Sun examples, for historical reasons, the model
	 * string is formatted a certain way for use by the installer
	 * miniroot.
	 *
	 * This is a departure from how it has been on some NetBSD
	 * systems historically, but this is intended to bring some
	 * consistency to the platforms while maintaining a reasonable
	 * aesthetic.
	 */
#ifdef __HAVE_M68K_MACHINE_PRINT_MODEL
	machine_print_model(pr);
#else
	(*pr)("%s\n", cpu_getmodel());
#endif

	switch (cputype) {
#ifdef M68010
	case CPU_68010:
		(*pr)("MC68010 CPU");
		break;
#endif
#ifdef M68020
	case CPU_68020:
		(*pr)("MC68020 CPU");
		break;
#endif
#ifdef M68030
	case CPU_68030:
		(*pr)("MC68030 CPU+MMU");
		break;
#endif
#ifdef M68040
	case CPU_68040:
		(*pr)("MC68040 CPU+MMU");
		break;
#endif
#ifdef M68060
	case CPU_68060: {
		u_int pcr = get_pcr();
		(*pr)("MC68%s060 rev.%d CPU+MMU",
		    (PCR_ID(pcr) & 1) ? "LC" : "",
		    (int)PCR_REVISION(pcr));
		if (pcr & PCR_DFP) {
			(*pr)("+FPU(disabled)");
		}
		break;
	    }
#endif
	default:
		(*pr)("unknown CPU type\n");
		panic("startup");
	}

#if defined(M68040) || defined(M68060)
	switch (fputype) {
	case FPU_68040:
	case FPU_68060:
		(*pr)("+FPU");
		break;
	default:
		break;
	}
#endif

	if (cpuspeed_khz) {
		(*pr)("%s",
		    mhz_string_from_khz(cpuspeed_khz,
					speed_str, sizeof(speed_str)));
	}

	switch (mmutype) {
#ifdef M68K_MMU_68851
	case MMU_68851:
		(*pr)(", MC68851 MMU");
		break;
#endif
#ifdef M68K_MMU_HP
	case MMU_HP:
		(*pr)(", HP MMU");
		break;
#endif
#ifdef M68K_MMU_SUN
	case MMU_SUN:
		(*pr)(", Sun MMU");
		break;
#endif
#ifdef M68K_MMU_CUSTOM
	case MMU_CUSTOM:
		(*pr)(", custom MMU");
		break;
#endif
	default:
		break;
	}

	switch (fputype) {
#ifdef FPU_EMULATE
	case FPU_NONE:
		(*pr)(", emulated FPU");
		break;
#endif
#if defined(M68020) || defined(M68030)
	case FPU_68881:
		(*pr)(", MC68881 FPU");
		break;

	case FPU_68882:
		(*pr)(", MC68882 FPU");
		break;
#endif
	default:
		break;
	}

#if defined(M68020) || defined(M68030)
	if (fpuspeed_khz != 0 && fpuspeed_khz != cpuspeed_khz) {
		(*pr)("%s",
		    mhz_string_from_khz(fpuspeed_khz,
					speed_str, sizeof(speed_str)));
	}
#endif

	switch (cputype) {
#if defined(M68040) || defined(M68060)
	case CPU_68040:
	case CPU_68060:
		(*pr)(", %dK+%dK on-chip I/D caches",
		    cputype == CPU_68040 ? 4 : 8,
		    cputype == CPU_68040 ? 4 : 8);
		break;
#endif
	default:
#ifdef M68K_EC
		if (ectype != EC_NONE) {
			(*pr)("\nExternal ");
			if (ecsize >= 1024) {
				(*pr)("%dK ", ecsize / 1024);
			}
			(*pr)("%s-address cache",
			    ectype == EC_PHYS ? "physical" : "virtual");
		}
#endif
		break;
	}
	(*pr)("\n");
}

int
mm_md_physacc_regular(paddr_t pa, vm_prot_t prot)
{
	int i;

	for (i = 0; i < VM_PHYSSEG_MAX; i++) {
		if (phys_seg_list[i].ps_start == phys_seg_list[i].ps_end) {
			continue;
		}
		if (pa < phys_seg_list[i].ps_start) {
			continue;
		}
		if (pa >= phys_seg_list[i].ps_end) {
			continue;
		}
		return 0;
	}
	return EFAULT;
}

int
cpu_reboot_poll_console(bool wait)
{
	int rv;

	cnpollc(true);
	/* If there is no console input device, cngetc() returns 0. */
	while ((rv = cngetc()) == 0 && wait) {
		delay(100000);
	}
	cnpollc(false);

	return rv;
}

void
machine_powerdown_default(void)
{
	/* zip, nada, nothing */
}
__weak_alias(machine_powerdown,machine_powerdown_default);

void
machine_halt_default(void)
{
	printf("Please press any key to reboot.\n");

	cpu_reboot_poll_console(true);
}
__weak_alias(machine_halt,machine_halt_default);

int	waittime = -1;

void
bootsync(void)
{
	if (waittime < 0) {
		waittime = 0;
		vfs_shutdown();
	}
}

void
cpu_reboot_common(int howto, char *bootstr)
{
	struct pcb *pcb = lwp_getpcb(curlwp);

	/* take a snap shot before clobbering any registers */
	if (pcb != NULL) {
		savectx(pcb);
	}

	/* If system is hold, just halt. */
	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	/* Un-blank the screen if appropriate. */
	cnpollc(true);

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0) {
		bootsync();
	}

	/* Disable interrupts. */
	splhigh();

	/* If rebooting and a dump is requested, do it. */
	if (howto & RB_DUMP) {
		dumpsys();
	}

 haltsys:
	/* Run any shutdown hooks. */
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

#if defined(PANICWAIT) && !defined(DDB)
	if ((howto & RB_HALT) == 0 && panicstr) {
		printf("hit any key to reboot...\n");
		cpu_reboot_poll_console(false);
		printf("\n");
	}
#endif

	/* Finally, halt/reboot the system. */
	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		machine_powerdown();
	}
	if (howto & RB_HALT) {
		printf("The operating system has halted.\n");
		machine_halt();
	}

	printf("rebooting...\n");
	delay(1000000);
	machine_reboot(howto, bootstr);
	printf("WARNING: system reboot failed, holding here.\n\n");
	for (;;) {
		/* spin forever. */
	}
}
__weak_alias(cpu_reboot,cpu_reboot_common);

/*
 * mm_md_physacc_common is the standard implementation for all
 * m68k platforms, and covers regular physical memory.  If a
 * platform wants to include other ranges, it can simply
 * define its own mm_md_physacc(), call mm_md_physacc_common()
 * first, and then check its own ranges if mm_md_physacc_common()
 * does not return 0.
 */
__weak_alias(mm_md_physacc, mm_md_physacc_regular);

/*
 * Set registers on exec.
 */
void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct trapframe *tf = (struct trapframe *)l->l_md.md_regs;
	struct pcb *pcb = lwp_getpcb(l);

	memset(tf, 0, sizeof(*tf));

	tf->tf_sr = PSL_USERSET;
	tf->tf_pc = pack->ep_entry & ~1;
	tf->tf_regs[D0] = 0;
	tf->tf_regs[D1] = 0;
	tf->tf_regs[D2] = 0;
	tf->tf_regs[D3] = 0;
	tf->tf_regs[D4] = 0;
	tf->tf_regs[D5] = 0;
	tf->tf_regs[D6] = 0;
	tf->tf_regs[D7] = 0;
	tf->tf_regs[A0] = 0;
	tf->tf_regs[A1] = 0;
	tf->tf_regs[A2] = l->l_proc->p_psstrp;
	tf->tf_regs[A3] = 0;
	tf->tf_regs[A4] = 0;
	tf->tf_regs[A5] = 0;
	tf->tf_regs[A6] = 0;
	tf->tf_regs[SP] = stack;

	/* restore a null state frame */
	pcb->pcb_fpregs.fpf_null = 0;
#if !defined(__mc68010__)
	if (fputype)
		m68881_restore(&pcb->pcb_fpregs);
#endif

#ifdef COMPAT_SUNOS
	/* see m68k/sunos_syscall.c */
	l->l_md.md_flags = 0;
#endif
}

#ifdef EXEC_AOUT
/*
 * cpu_exec_aout_makecmds():
 *	cpu-dependent a.out format hook for execve().
 *
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 *
 * XXX what are the special cases for the hp300?
 * XXX why is this COMPAT_NOMID?  was something generating
 *	hp300 binaries with an a_mid of 0?  i thought that was only
 *	done on little-endian machines...  -- cgd
 * XXX perhaps this whole thing should be relegated to a smoking crater?
 */
int
cpu_exec_aout_makecmds(struct lwp *l, struct exec_package *epp)
{
#ifdef __mc68010__
	/* There were never native a.out binaries for NetBSD on 68010. */
	return ENOEXEC;
#elif defined(COMPAT_NOMID) || defined(COMPAT_44)
	struct exec *execp = epp->ep_hdr;
	u_long midmag, magic;
	u_short mid;
	int error;

	midmag = ntohl(execp->a_midmag);
	mid = (midmag >> 16) & 0xffff;
	magic = midmag & 0xffff;

	midmag = mid << 16 | magic;

	switch (midmag) {
#ifdef COMPAT_NOMID
	case (MID_ZERO << 16) | ZMAGIC:
		error = exec_aout_prep_oldzmagic(l, epp);
		break;
#endif
#ifdef COMPAT_44
	case (MID_HP300 << 16) | ZMAGIC:
		error = exec_aout_prep_oldzmagic(l, epp);
		break;
#endif
	default:
		error = ENOEXEC;
	}
	return error;
#else /* COMPAT_NOMID || COMPAT_44 */
	return ENOEXEC;
#endif
}
#endif /* EXEC_AOUT */
