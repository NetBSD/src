/*	$NetBSD: kern_pax.c,v 1.27.6.6 2016/10/05 20:56:02 skrll Exp $	*/

/*
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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
 * Copyright (c) 2006 Elad Efrat <elad@NetBSD.org>
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_pax.c,v 1.27.6.6 2016/10/05 20:56:02 skrll Exp $");

#include "opt_pax.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/pax.h>
#include <sys/sysctl.h>
#include <sys/kmem.h>
#include <sys/mman.h>
#include <sys/fileassoc.h>
#include <sys/syslog.h>
#include <sys/vnode.h>
#include <sys/queue.h>
#include <sys/bitops.h>
#include <sys/kauth.h>
#include <sys/cprng.h>

#ifdef PAX_ASLR_DEBUG
#define PAX_DPRINTF(_fmt, args...) \
	do if (pax_aslr_debug) uprintf("%s: " _fmt "\n", __func__, ##args); \
	while (/*CONSTCOND*/0)
#else
#define PAX_DPRINTF(_fmt, args...)	do {} while (/*CONSTCOND*/0)
#endif

#ifdef PAX_ASLR
#include <sys/mman.h>

int pax_aslr_enabled = 1;
int pax_aslr_global = PAX_ASLR;

#ifndef PAX_ASLR_DELTA_MMAP_LSB
#define PAX_ASLR_DELTA_MMAP_LSB		PGSHIFT
#endif
#ifndef PAX_ASLR_DELTA_MMAP_LEN
#define PAX_ASLR_DELTA_MMAP_LEN		((sizeof(void *) * NBBY) / 2)
#endif
#ifndef PAX_ASLR_DELTA_MMAP_LEN32
#define PAX_ASLR_DELTA_MMAP_LEN32	((sizeof(uint32_t) * NBBY) / 2)
#endif
#ifndef PAX_ASLR_DELTA_STACK_LSB
#define PAX_ASLR_DELTA_STACK_LSB	PGSHIFT
#endif
#ifndef PAX_ASLR_DELTA_STACK_LEN
#define PAX_ASLR_DELTA_STACK_LEN 	((sizeof(void *) * NBBY) / 4)
#endif
#ifndef PAX_ASLR_DELTA_STACK_LEN32
#define PAX_ASLR_DELTA_STACK_LEN32 	((sizeof(uint32_t) * NBBY) / 4)
#endif
#define PAX_ASLR_MAX_STACK_WASTE	8

#ifdef PAX_ASLR_DEBUG
int pax_aslr_debug;
/* flag set means disable */
int pax_aslr_flags;
uint32_t pax_aslr_rand;
#define PAX_ASLR_STACK		0x01
#define PAX_ASLR_STACK_GAP	0x02
#define PAX_ASLR_MMAP		0x04
#define PAX_ASLR_EXEC_OFFSET	0x08
#define PAX_ASLR_RTLD_OFFSET	0x10
#define PAX_ASLR_FIXED		0x20
#endif

static bool pax_aslr_elf_flags_active(uint32_t);
#endif /* PAX_ASLR */

#ifdef PAX_MPROTECT
static int pax_mprotect_enabled = 1;
static int pax_mprotect_global = PAX_MPROTECT;
static int pax_mprotect_ptrace = 1;
static bool pax_mprotect_elf_flags_active(uint32_t);
#endif /* PAX_MPROTECT */
#ifdef PAX_MPROTECT_DEBUG
int pax_mprotect_debug;
#endif

#ifdef PAX_SEGVGUARD
#ifndef PAX_SEGVGUARD_EXPIRY
#define	PAX_SEGVGUARD_EXPIRY		(2 * 60)
#endif
#ifndef PAX_SEGVGUARD_SUSPENSION
#define	PAX_SEGVGUARD_SUSPENSION	(10 * 60)
#endif
#ifndef	PAX_SEGVGUARD_MAXCRASHES
#define	PAX_SEGVGUARD_MAXCRASHES	5
#endif


static int pax_segvguard_enabled = 1;
static int pax_segvguard_global = PAX_SEGVGUARD;
static int pax_segvguard_expiry = PAX_SEGVGUARD_EXPIRY;
static int pax_segvguard_suspension = PAX_SEGVGUARD_SUSPENSION;
static int pax_segvguard_maxcrashes = PAX_SEGVGUARD_MAXCRASHES;

static fileassoc_t segvguard_id;

struct pax_segvguard_uid_entry {
	uid_t sue_uid;
	size_t sue_ncrashes;
	time_t sue_expiry;
	time_t sue_suspended;
	LIST_ENTRY(pax_segvguard_uid_entry) sue_list;
};

struct pax_segvguard_entry {
	LIST_HEAD(, pax_segvguard_uid_entry) segv_uids;
};

static bool pax_segvguard_elf_flags_active(uint32_t);
static void pax_segvguard_cleanup_cb(void *);
#endif /* PAX_SEGVGUARD */

SYSCTL_SETUP(sysctl_security_pax_setup, "sysctl security.pax setup")
{
	const struct sysctlnode *rnode = NULL, *cnode;

	sysctl_createv(clog, 0, NULL, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "pax",
		       SYSCTL_DESCR("PaX (exploit mitigation) features."),
		       NULL, 0, NULL, 0,
		       CTL_SECURITY, CTL_CREATE, CTL_EOL);

	cnode = rnode;

#ifdef PAX_MPROTECT
	rnode = cnode;
	sysctl_createv(clog, 0, &rnode, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "mprotect",
		       SYSCTL_DESCR("mprotect(2) W^X restrictions."),
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "enabled",
		       SYSCTL_DESCR("Restrictions enabled."),
		       NULL, 0, &pax_mprotect_enabled, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "global",
		       SYSCTL_DESCR("When enabled, unless explicitly "
				    "specified, apply restrictions to "
				    "all processes."),
		       NULL, 0, &pax_mprotect_global, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "ptrace",
		       SYSCTL_DESCR("When enabled, allow ptrace(2) to "
			    "override mprotect permissions on traced "
			    "processes"),
		       NULL, 0, &pax_mprotect_ptrace, 0,
		       CTL_CREATE, CTL_EOL);
#ifdef PAX_MPROTECT_DEBUG
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "debug",
		       SYSCTL_DESCR("print mprotect changes."),
		       NULL, 0, &pax_mprotect_debug, 0,
		       CTL_CREATE, CTL_EOL);
#endif
#endif /* PAX_MPROTECT */

#ifdef PAX_SEGVGUARD
	rnode = cnode;
	sysctl_createv(clog, 0, &rnode, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "segvguard",
		       SYSCTL_DESCR("PaX segvguard."),
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "enabled",
		       SYSCTL_DESCR("segvguard enabled."),
		       NULL, 0, &pax_segvguard_enabled, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "global",
		       SYSCTL_DESCR("segvguard all programs."),
		       NULL, 0, &pax_segvguard_global, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "expiry_timeout",
		       SYSCTL_DESCR("Entry expiry timeout (in seconds)."),
		       NULL, 0, &pax_segvguard_expiry, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "suspend_timeout",
		       SYSCTL_DESCR("Entry suspension timeout (in seconds)."),
		       NULL, 0, &pax_segvguard_suspension, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "max_crashes",
		       SYSCTL_DESCR("Max number of crashes before expiry."),
		       NULL, 0, &pax_segvguard_maxcrashes, 0,
		       CTL_CREATE, CTL_EOL);
#endif /* PAX_SEGVGUARD */

#ifdef PAX_ASLR
	rnode = cnode;
	sysctl_createv(clog, 0, &rnode, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "aslr",
		       SYSCTL_DESCR("Address Space Layout Randomization."),
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "enabled",
		       SYSCTL_DESCR("Restrictions enabled."),
		       NULL, 0, &pax_aslr_enabled, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "global",
		       SYSCTL_DESCR("When enabled, unless explicitly "
				    "specified, apply to all processes."),
		       NULL, 0, &pax_aslr_global, 0,
		       CTL_CREATE, CTL_EOL);
#ifdef PAX_ASLR_DEBUG
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "debug",
		       SYSCTL_DESCR("Pring ASLR selected addresses."),
		       NULL, 0, &pax_aslr_debug, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "flags",
		       SYSCTL_DESCR("Disable/Enable select ASLR features."),
		       NULL, 0, &pax_aslr_flags, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "rand",
		       SYSCTL_DESCR("Use the given fixed random value"),
		       NULL, 0, &pax_aslr_rand, 0,
		       CTL_CREATE, CTL_EOL);
#endif
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "mmap_len",
		       SYSCTL_DESCR("Number of bits randomized for "
				    "mmap(2) calls."),
		       NULL, PAX_ASLR_DELTA_MMAP_LEN, NULL, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "stack_len",
		       SYSCTL_DESCR("Number of bits randomized for "
				    "the stack."),
		       NULL, PAX_ASLR_DELTA_STACK_LEN, NULL, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "exec_len",
		       SYSCTL_DESCR("Number of bits randomized for "
				    "the PIE exec base."),
		       NULL, PAX_ASLR_DELTA_EXEC_LEN, NULL, 0,
		       CTL_CREATE, CTL_EOL);

#endif /* PAX_ASLR */
}

/*
 * Initialize PaX.
 */
void
pax_init(void)
{
#ifdef PAX_SEGVGUARD
	int error;

	error = fileassoc_register("segvguard", pax_segvguard_cleanup_cb,
	    &segvguard_id);
	if (error) {
		panic("pax_init: segvguard_id: error=%d\n", error);
	}
#endif /* PAX_SEGVGUARD */
#ifdef PAX_ASLR
	/* Adjust maximum stack by the size we can consume for ASLR */
	extern rlim_t maxsmap;
	maxsmap = MAXSSIZ - (MAXSSIZ / PAX_ASLR_MAX_STACK_WASTE);
	// XXX: compat32 is not handled.
#endif
}

void
pax_set_flags(struct exec_package *epp, struct proc *p)
{
	p->p_pax = epp->ep_pax_flags;

#ifdef PAX_MPROTECT
	if (pax_mprotect_ptrace == 0)
		return;
	/*
	 * If we are running under the debugger, turn off MPROTECT so
 	 * the debugger can insert/delete breakpoints
	 */
	if (p->p_slflag & PSL_TRACED)
		p->p_pax &= ~P_PAX_MPROTECT;
#endif
}

void
pax_setup_elf_flags(struct exec_package *epp, uint32_t elf_flags)
{
	uint32_t flags = 0;

#ifdef PAX_ASLR
	if (pax_aslr_elf_flags_active(elf_flags)) {
		flags |= P_PAX_ASLR;
	}
#endif
#ifdef PAX_MPROTECT
	if (pax_mprotect_elf_flags_active(elf_flags)) {
		flags |= P_PAX_MPROTECT;
	}
#endif
#ifdef PAX_SEGVGUARD
	if (pax_segvguard_elf_flags_active(elf_flags)) {
		flags |= P_PAX_GUARD;
	}
#endif

	epp->ep_pax_flags = flags;
}

#if defined(PAX_MPROTECT) || defined(PAX_SEGVGUARD) || defined(PAX_ASLR)
static inline bool
pax_flags_active(uint32_t flags, uint32_t opt)
{
	if (!(flags & opt))
		return false;
	return true;
}
#endif /* PAX_MPROTECT || PAX_SEGVGUARD || PAX_ASLR */

#ifdef PAX_MPROTECT
static bool
pax_mprotect_elf_flags_active(uint32_t flags)
{
	if (!pax_mprotect_enabled)
		return false;
	if (pax_mprotect_global && (flags & ELF_NOTE_PAX_NOMPROTECT) != 0) {
		/* Mprotect explicitly disabled */
		return false;
	}
	if (!pax_mprotect_global && (flags & ELF_NOTE_PAX_MPROTECT) == 0) {
		/* Mprotect not requested */
		return false;
	}
	return true;
}

void
pax_mprotect_adjust(
#ifdef PAX_MPROTECT_DEBUG
    const char *file, size_t line,
#endif
    struct lwp *l, vm_prot_t *prot, vm_prot_t *maxprot)
{
	uint32_t flags;

	flags = l->l_proc->p_pax;
	if (!pax_flags_active(flags, P_PAX_MPROTECT))
		return;

	if ((*prot & (VM_PROT_WRITE|VM_PROT_EXECUTE)) != VM_PROT_EXECUTE) {
#ifdef PAX_MPROTECT_DEBUG
		struct proc *p = l->l_proc;
		if ((*prot & VM_PROT_EXECUTE) && pax_mprotect_debug) {
			printf("%s: %s,%zu: %d.%d (%s): -x\n",
			    __func__, file, line,
			    p->p_pid, l->l_lid, p->p_comm);
		}
#endif
		*prot &= ~VM_PROT_EXECUTE;
		*maxprot &= ~VM_PROT_EXECUTE;
	} else {
#ifdef PAX_MPROTECT_DEBUG
		struct proc *p = l->l_proc;
		if ((*prot & VM_PROT_WRITE) && pax_mprotect_debug) {
			printf("%s: %s,%zu: %d.%d (%s): -w\n",
			    __func__, file, line,
			    p->p_pid, l->l_lid, p->p_comm);
		}
#endif
		*prot &= ~VM_PROT_WRITE;
		*maxprot &= ~VM_PROT_WRITE;
	}
}

/*
 * Bypass MPROTECT for traced processes
 */
int
pax_mprotect_prot(struct lwp *l)
{
	uint32_t flags;

	flags = l->l_proc->p_pax;
	if (!pax_flags_active(flags, P_PAX_MPROTECT))
		return 0;
	if (pax_mprotect_ptrace < 2)
		return 0;
	return UVM_EXTRACT_PROT_ALL;
}


#endif /* PAX_MPROTECT */

#ifdef PAX_ASLR
static bool
pax_aslr_elf_flags_active(uint32_t flags)
{
	if (!pax_aslr_enabled)
		return false;
	if (pax_aslr_global && (flags & ELF_NOTE_PAX_NOASLR) != 0) {
		/* ASLR explicitly disabled */
		return false;
	}
	if (!pax_aslr_global && (flags & ELF_NOTE_PAX_ASLR) == 0) {
		/* ASLR not requested */
		return false;
	}
	return true;
}

static bool
pax_aslr_epp_active(struct exec_package *epp)
{
	if (__predict_false((epp->ep_flags & (EXEC_32|EXEC_TOPDOWN_VM)) == 0))
		return false;
	return pax_flags_active(epp->ep_pax_flags, P_PAX_ASLR);
}

static bool
pax_aslr_active(struct lwp *l)
{
	return pax_flags_active(l->l_proc->p_pax, P_PAX_ASLR);
}

void
pax_aslr_init_vm(struct lwp *l, struct vmspace *vm, struct exec_package *ep)
{
	if (!pax_aslr_active(l))
		return;

	if (__predict_false((ep->ep_flags & (EXEC_32|EXEC_TOPDOWN_VM)) == 0))
		return;

#ifdef PAX_ASLR_DEBUG
	if (pax_aslr_flags & PAX_ASLR_MMAP)
		return;
#endif

	uint32_t len = (ep->ep_flags & EXEC_32) ?
	    PAX_ASLR_DELTA_MMAP_LEN32 : PAX_ASLR_DELTA_MMAP_LEN;

	uint32_t rand = cprng_fast32();
#ifdef PAX_ASLR_DEBUG
	if (pax_aslr_flags & PAX_ASLR_FIXED)
		rand = pax_aslr_rand;
#endif
	vm->vm_aslr_delta_mmap = PAX_ASLR_DELTA(rand,
	    PAX_ASLR_DELTA_MMAP_LSB, len);

	PAX_DPRINTF("delta_mmap=%#jx/%u",
	    (uintmax_t)vm->vm_aslr_delta_mmap, len);
}

void
pax_aslr_mmap(struct lwp *l, vaddr_t *addr, vaddr_t orig_addr, int f)
{
	if (!pax_aslr_active(l))
		return;
#ifdef PAX_ASLR_DEBUG
	char buf[256];

	if (pax_aslr_flags & PAX_ASLR_MMAP)
		return;

	if (pax_aslr_debug)
		snprintb(buf, sizeof(buf), MAP_FMT, f);
	else
		buf[0] = '\0';
#endif

	if (!(f & MAP_FIXED) && ((orig_addr == 0) || !(f & MAP_ANON))) {
		PAX_DPRINTF("applying to %#jx orig_addr=%#jx f=%s",
		    (uintmax_t)*addr, (uintmax_t)orig_addr, buf);
		if (!(l->l_proc->p_vmspace->vm_map.flags & VM_MAP_TOPDOWN))
			*addr += l->l_proc->p_vmspace->vm_aslr_delta_mmap;
		else
			*addr -= l->l_proc->p_vmspace->vm_aslr_delta_mmap;
		PAX_DPRINTF("result %#jx", (uintmax_t)*addr);
	} else {
		PAX_DPRINTF("not applying to %#jx orig_addr=%#jx f=%s",
		    (uintmax_t)*addr, (uintmax_t)orig_addr, buf);
	}
}

static vaddr_t
pax_aslr_offset(vaddr_t align)
{
	size_t pax_align, l2, delta;
	uint32_t rand;
	vaddr_t offset;

	pax_align = align == 0 ? PGSHIFT : align;
	l2 = ilog2(pax_align);

	rand = cprng_fast32();
#ifdef PAX_ASLR_DEBUG
	if (pax_aslr_flags & PAX_ASLR_FIXED)
		rand = pax_aslr_rand;
#endif

#define	PAX_TRUNC(a, b)	((a) & ~((b) - 1))

	delta = PAX_ASLR_DELTA(rand, l2, PAX_ASLR_DELTA_EXEC_LEN);
	offset = PAX_TRUNC(delta, pax_align) + PAGE_SIZE;

	PAX_DPRINTF("rand=%#x l2=%#zx pax_align=%#zx delta=%#zx offset=%#jx",
	    rand, l2, pax_align, delta, (uintmax_t)offset);

	return offset;
}

vaddr_t
pax_aslr_exec_offset(struct exec_package *epp, vaddr_t align)
{
	if (!pax_aslr_epp_active(epp))
		goto out;

#ifdef PAX_ASLR_DEBUG
	if (pax_aslr_flags & PAX_ASLR_EXEC_OFFSET)
		goto out;
#endif
	return pax_aslr_offset(align) + PAGE_SIZE;
out:
	return MAX(align, PAGE_SIZE);
}

voff_t
pax_aslr_rtld_offset(struct exec_package *epp, vaddr_t align, int use_topdown)
{
	voff_t offset;

	if (!pax_aslr_epp_active(epp))
		return 0;

#ifdef PAX_ASLR_DEBUG
	if (pax_aslr_flags & PAX_ASLR_RTLD_OFFSET)
		return 0;
#endif
	offset = pax_aslr_offset(align);
	if (use_topdown)
		offset = -offset;

	return offset;
}

void
pax_aslr_stack(struct exec_package *epp, vsize_t *max_stack_size)
{
	if (!pax_aslr_epp_active(epp))
		return;
#ifdef PAX_ASLR_DEBUG
	if (pax_aslr_flags & PAX_ASLR_STACK)
		return;
#endif

	uint32_t len = (epp->ep_flags & EXEC_32) ?
	    PAX_ASLR_DELTA_STACK_LEN32 : PAX_ASLR_DELTA_STACK_LEN;
	uint32_t rand = cprng_fast32();
#ifdef PAX_ASLR_DEBUG
	if (pax_aslr_flags & PAX_ASLR_FIXED)
		rand = pax_aslr_rand;
#endif
	u_long d = PAX_ASLR_DELTA(rand, PAX_ASLR_DELTA_STACK_LSB, len);
	d &= (*max_stack_size / PAX_ASLR_MAX_STACK_WASTE) - 1;
 	u_long newminsaddr = (u_long)STACK_GROW(epp->ep_minsaddr, d);
	PAX_DPRINTF("old minsaddr=%#jx delta=%#lx new minsaddr=%#lx",
	    (uintmax_t)epp->ep_minsaddr, d, newminsaddr);
	epp->ep_minsaddr = (vaddr_t)newminsaddr;
	*max_stack_size -= d;
}

uint32_t
pax_aslr_stack_gap(struct exec_package *epp)
{
	if (!pax_aslr_epp_active(epp))
		return 0;

#ifdef PAX_ASLR_DEBUG
	if (pax_aslr_flags & PAX_ASLR_STACK_GAP)
		return 0;
#endif

	uint32_t rand = cprng_fast32();
#ifdef PAX_ASLR_DEBUG
	if (pax_aslr_flags & PAX_ASLR_FIXED)
		rand = pax_aslr_rand;
#endif
	rand %= PAGE_SIZE;
	PAX_DPRINTF("stack gap=%#x\n", rand);
	return rand;
}
#endif /* PAX_ASLR */

#ifdef PAX_SEGVGUARD
static bool
pax_segvguard_elf_flags_active(uint32_t flags)
{
	if (!pax_segvguard_enabled)
		return false;
	if (pax_segvguard_global && (flags & ELF_NOTE_PAX_NOGUARD) != 0) {
		/* Segvguard explicitly disabled */
		return false;
	}
	if (!pax_segvguard_global && (flags & ELF_NOTE_PAX_GUARD) == 0) {
		/* Segvguard not requested */
		return false;
	}
	return true;
}

static void
pax_segvguard_cleanup_cb(void *v)
{
	struct pax_segvguard_entry *p = v;
	struct pax_segvguard_uid_entry *up;

	if (p == NULL) {
		return;
	}
	while ((up = LIST_FIRST(&p->segv_uids)) != NULL) {
		LIST_REMOVE(up, sue_list);
		kmem_free(up, sizeof(*up));
	}
	kmem_free(p, sizeof(*p));
}

/*
 * Called when a process of image vp generated a segfault.
 */
int
pax_segvguard(struct lwp *l, struct vnode *vp, const char *name,
    bool crashed)
{
	struct pax_segvguard_entry *p;
	struct pax_segvguard_uid_entry *up;
	struct timeval tv;
	uid_t uid;
	uint32_t flags;
	bool have_uid;

	flags = l->l_proc->p_pax;
	if (!pax_flags_active(flags, P_PAX_GUARD))
		return 0;

	if (vp == NULL)
		return EFAULT;

	/* Check if we already monitor the file. */
	p = fileassoc_lookup(vp, segvguard_id);

	/* Fast-path if starting a program we don't know. */
	if (p == NULL && !crashed)
		return 0;

	microtime(&tv);

	/*
	 * If a program we don't know crashed, we need to create a new entry
	 * for it.
	 */
	if (p == NULL) {
		p = kmem_alloc(sizeof(*p), KM_SLEEP);
		fileassoc_add(vp, segvguard_id, p);
		LIST_INIT(&p->segv_uids);

		/*
		 * Initialize a new entry with "crashes so far" of 1.
		 * The expiry time is when we purge the entry if it didn't
		 * reach the limit.
		 */
		up = kmem_alloc(sizeof(*up), KM_SLEEP);
		up->sue_uid = kauth_cred_getuid(l->l_cred);
		up->sue_ncrashes = 1;
		up->sue_expiry = tv.tv_sec + pax_segvguard_expiry;
		up->sue_suspended = 0;
		LIST_INSERT_HEAD(&p->segv_uids, up, sue_list);
		return 0;
	}

	/*
	 * A program we "know" either executed or crashed again.
	 * See if it's a culprit we're familiar with.
	 */
	uid = kauth_cred_getuid(l->l_cred);
	have_uid = false;
	LIST_FOREACH(up, &p->segv_uids, sue_list) {
		if (up->sue_uid == uid) {
			have_uid = true;
			break;
		}
	}

	/*
	 * It's someone else. Add an entry for him if we crashed.
	 */
	if (!have_uid) {
		if (crashed) {
			up = kmem_alloc(sizeof(*up), KM_SLEEP);
			up->sue_uid = uid;
			up->sue_ncrashes = 1;
			up->sue_expiry = tv.tv_sec + pax_segvguard_expiry;
			up->sue_suspended = 0;
			LIST_INSERT_HEAD(&p->segv_uids, up, sue_list);
		}
		return 0;
	}

	if (crashed) {
		/* Check if timer on previous crashes expired first. */
		if (up->sue_expiry < tv.tv_sec) {
			log(LOG_INFO, "PaX Segvguard: [%s] Suspension"
			    " expired.\n", name ? name : "unknown");
			up->sue_ncrashes = 1;
			up->sue_expiry = tv.tv_sec + pax_segvguard_expiry;
			up->sue_suspended = 0;
			return 0;
		}

		up->sue_ncrashes++;

		if (up->sue_ncrashes >= pax_segvguard_maxcrashes) {
			log(LOG_ALERT, "PaX Segvguard: [%s] Suspending "
			    "execution for %d seconds after %zu crashes.\n",
			    name ? name : "unknown", pax_segvguard_suspension,
			    up->sue_ncrashes);

			/* Suspend this program for a while. */
			up->sue_suspended = tv.tv_sec + pax_segvguard_suspension;
			up->sue_ncrashes = 0;
			up->sue_expiry = 0;
		}
	} else {
		/* Are we supposed to be suspended? */
		if (up->sue_suspended > tv.tv_sec) {
			log(LOG_ALERT, "PaX Segvguard: [%s] Preventing "
			    "execution due to repeated segfaults.\n", name ?
			    name : "unknown");
			return EPERM;
		}
	}

	return 0;
}
#endif /* PAX_SEGVGUARD */
