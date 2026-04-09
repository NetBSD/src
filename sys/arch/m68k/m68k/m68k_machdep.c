/*	$NetBSD: m68k_machdep.c,v 1.20 2026/04/09 14:36:55 thorpej Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: m68k_machdep.c,v 1.20 2026/04/09 14:36:55 thorpej Exp $");

#include "opt_compat_netbsd.h"
#include "opt_compat_sunos.h"
#include "opt_execfmt.h"

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/msgbuf.h>

#ifdef EXEC_AOUT
#include <sys/exec_aout.h>	/* for cpu_exec_aout_makecmds() prototype */
#endif

#include <uvm/uvm_extern.h>

#include <m68k/m68k.h>
#include <m68k/frame.h>
#include <m68k/pcb.h>
#include <m68k/reg.h>

#include <m68k/seglist.h>

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;		/* from <machine/param.h> */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

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
#define	VM_PHYS_SEG_TO_FREELIST(s)	VM_FREELIST_DEFAULT
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
