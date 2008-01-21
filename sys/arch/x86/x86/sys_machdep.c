/*	$NetBSD: sys_machdep.c,v 1.3.2.5 2008/01/21 09:40:18 yamt Exp $	*/

/*-
 * Copyright (c) 1998, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, and by Andrew Doran.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_machdep.c,v 1.3.2.5 2008/01/21 09:40:18 yamt Exp $");

#include "opt_compat_netbsd.h"
#include "opt_mtrr.h"
#include "opt_perfctrs.h"
#include "opt_user_ldt.h"
#include "opt_vm86.h"
#include "opt_xen.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/signal.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/kauth.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/gdt.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/sysarch.h>
#include <machine/mtrr.h>

#ifdef __x86_64__
/* Need to be checked. */
#undef	USER_LDT
#undef	PERFCTRS
#undef	VM86
#undef	IOPERM
#else
#if defined(XEN)
#undef	IOPERM
#else /* defined(XEN) */
#define	IOPERM
#endif /* defined(XEN) */
#endif

#ifdef VM86
#include <machine/vm86.h>
#endif

#ifdef PERFCTRS
#include <machine/pmc.h>
#endif

/* XXX needs changes from vmlocking branch */
#define	mutex_enter(x)	/* nothing */
#define	mutex_exit(x)	/* nothing */

extern struct vm_map *kernel_map;

int x86_get_ioperm(struct lwp *, void *, register_t *);
int x86_set_ioperm(struct lwp *, void *, register_t *);
int x86_get_mtrr(struct lwp *, void *, register_t *);
int x86_set_mtrr(struct lwp *, void *, register_t *);
int x86_set_sdbase(void *arg, char which);
int x86_get_sdbase(void *arg, char which);

#ifdef LDT_DEBUG
static void x86_print_ldt(int, const struct segment_descriptor *);

static void
x86_print_ldt(int i, const struct segment_descriptor *d)
{
	printf("[%d] lolimit=0x%x, lobase=0x%x, type=%u, dpl=%u, p=%u, "
	    "hilimit=0x%x, xx=%x, def32=%u, gran=%u, hibase=0x%x\n",
	    i, d->sd_lolimit, d->sd_lobase, d->sd_type, d->sd_dpl, d->sd_p,
	    d->sd_hilimit, d->sd_xx, d->sd_def32, d->sd_gran, d->sd_hibase);
}
#endif

int
x86_get_ldt_len(struct lwp *l)
{
#ifndef USER_LDT
	return -1;
#else
	pmap_t pmap = l->l_proc->p_vmspace->vm_map.pmap;
	int nldt;

	mutex_enter(&pmap->pm_lock);

	if (pmap->pm_flags & PMF_USER_LDT) {
		nldt = pmap->pm_ldt_len;
	} else {
		nldt = NLDT;
	}
	mutex_exit(&pmap->pm_lock);
	return nldt;
#endif
}


int
x86_get_ldt(struct lwp *l, void *args, register_t *retval)
{
#ifndef USER_LDT
	return EINVAL;
#else
	struct x86_get_ldt_args ua;
	union descriptor *cp;
	int error;

	if ((error = copyin(args, &ua, sizeof(ua))) != 0)
		return error;

	if (ua.num < 0 || ua.num > 8192)
		return EINVAL;

	cp = malloc(ua.num * sizeof(union descriptor), M_TEMP, M_WAITOK);
	if (cp == NULL)
		return ENOMEM;

	error = x86_get_ldt1(l, &ua, cp);
	*retval = ua.num;
	if (error == 0)
		error = copyout(cp, ua.desc, ua.num * sizeof(*cp));

	free(cp, M_TEMP);
	return error;
#endif
}

int
x86_get_ldt1(struct lwp *l, struct x86_get_ldt_args *ua, union descriptor *cp)
{
#ifndef USER_LDT
	return EINVAL;
#else
	int error;
	struct proc *p = l->l_proc;
	pmap_t pmap = p->p_vmspace->vm_map.pmap;
	int nldt, num;
	union descriptor *lp;

	error = kauth_authorize_machdep(l->l_cred, KAUTH_MACHDEP_LDT_GET,
	    NULL, NULL, NULL, NULL);
	if (error)
		return (error);

#ifdef	LDT_DEBUG
	printf("x86_get_ldt: start=%d num=%d descs=%p\n", ua->start,
	    ua->num, ua->desc);
#endif

	if (ua->start < 0 || ua->num < 0 || ua->start > 8192 || ua->num > 8192 ||
	    ua->start + ua->num > 8192)
		return (EINVAL);

	mutex_enter(&pmap->pm_lock);

	if (pmap->pm_flags & PMF_USER_LDT) {
		nldt = pmap->pm_ldt_len;
		lp = pmap->pm_ldt;
	} else {
		nldt = NLDT;
		lp = ldt;
	}

	if (ua->start > nldt) {
		mutex_exit(&pmap->pm_lock);
		return (EINVAL);
	}

	lp += ua->start;
	num = min(ua->num, nldt - ua->start);
	ua->num = num;
#ifdef LDT_DEBUG
	{
		int i;
		for (i = 0; i < num; i++)
			x86_print_ldt(i, &lp[i].sd);
	}
#endif

	memcpy(cp, lp, num * sizeof(union descriptor));
	mutex_exit(&pmap->pm_lock);

	return 0;
#endif
}

int
x86_set_ldt(struct lwp *l, void *args, register_t *retval)
{
#ifndef USER_LDT
	return EINVAL;
#else
	struct x86_set_ldt_args ua;
	union descriptor *descv;
	int error;

	if ((error = copyin(args, &ua, sizeof(ua))) != 0)
		return (error);

	if (ua.num < 0 || ua.num > 8192)
		return EINVAL;

	descv = malloc(sizeof (*descv) * ua.num, M_TEMP, M_NOWAIT);
	if (descv == NULL)
		return ENOMEM;

	error = copyin(ua.desc, descv, sizeof (*descv) * ua.num);
	if (error == 0)
		error = x86_set_ldt1(l, &ua, descv);
	*retval = ua.start;

	free(descv, M_TEMP);
	return error;
#endif
}

int
x86_set_ldt1(struct lwp *l, struct x86_set_ldt_args *ua,
    union descriptor *descv)
{
#ifndef USER_LDT
	return EINVAL;
#else
	int error, i, n, sel, free_sel;
	struct proc *p = l->l_proc;
	struct pcb *pcb = &l->l_addr->u_pcb;
	pmap_t pmap = p->p_vmspace->vm_map.pmap;
	size_t old_len, new_len, ldt_len, free_len;
	union descriptor *old_ldt, *new_ldt, *free_ldt;

	error = kauth_authorize_machdep(l->l_cred, KAUTH_MACHDEP_LDT_SET,
	    NULL, NULL, NULL, NULL);
	if (error)
		return (error);

	if (ua->start < 0 || ua->num < 0 || ua->start > 8192 || ua->num > 8192 ||
	    ua->start + ua->num > 8192)
		return (EINVAL);

	/* Check descriptors for access violations. */
	for (i = 0; i < ua->num; i++) {
		union descriptor *desc = &descv[i];

		switch (desc->sd.sd_type) {
		case SDT_SYSNULL:
			desc->sd.sd_p = 0;
			break;
		case SDT_SYS286CGT:
		case SDT_SYS386CGT:
			/*
			 * Only allow call gates targeting a segment
			 * in the LDT or a user segment in the fixed
			 * part of the gdt.  Segments in the LDT are
			 * constrained (below) to be user segments.
			 */
			if (desc->gd.gd_p != 0 &&
			    !ISLDT(desc->gd.gd_selector) &&
			    ((IDXSEL(desc->gd.gd_selector) >= NGDT) ||
			     (gdt[IDXSEL(desc->gd.gd_selector)].sd.sd_dpl !=
				 SEL_UPL))) {
				return EACCES;
			}
			break;
		case SDT_MEMEC:
		case SDT_MEMEAC:
		case SDT_MEMERC:
		case SDT_MEMERAC:
			/* Must be "present" if executable and conforming. */
			if (desc->sd.sd_p == 0)
				return EACCES;
			break;
		case SDT_MEMRO:
		case SDT_MEMROA:
		case SDT_MEMRW:
		case SDT_MEMRWA:
		case SDT_MEMROD:
		case SDT_MEMRODA:
		case SDT_MEMRWD:
		case SDT_MEMRWDA:
		case SDT_MEME:
		case SDT_MEMEA:
		case SDT_MEMER:
		case SDT_MEMERA:
			break;
		default:
			/*
			 * Make sure that unknown descriptor types are
			 * not marked present.
			 */
			if (desc->sd.sd_p != 0)
				return EACCES;
			break;
		}

		if (desc->sd.sd_p != 0) {
			/* Only user (ring-3) descriptors may be present. */
			if (desc->sd.sd_dpl != SEL_UPL)
				return EACCES;
		}
	}

	/* allocate user ldt */
	free_sel = -1;
	new_ldt = NULL;
	new_len = 0;
	free_ldt = NULL;
	free_len = 0;
	mutex_enter(&pmap->pm_lock);
	if (pmap->pm_ldt == 0 || (ua->start + ua->num) > pmap->pm_ldt_len) {
		if (pmap->pm_flags & PMF_USER_LDT)
			ldt_len = pmap->pm_ldt_len;
		else
			ldt_len = 512;
		while ((ua->start + ua->num) > ldt_len)
			ldt_len *= 2;
		new_len = ldt_len * sizeof(union descriptor);

		mutex_exit(&pmap->pm_lock);
		new_ldt = (union descriptor *)uvm_km_alloc(kernel_map,
		    new_len, 0, UVM_KMF_WIRED);
		memset(new_ldt, 0, new_len);
		sel = ldt_alloc(new_ldt, new_len);
		mutex_enter(&pmap->pm_lock);

		if (pmap->pm_ldt != NULL && ldt_len <= pmap->pm_ldt_len) {
			/*
			 * Another thread (re)allocated the LDT to
			 * sufficient size while we were blocked in
			 * uvm_km_alloc. Oh well. The new entries
			 * will quite probably not be right, but
			 * hey.. not our problem if user applications
			 * have race conditions like that.
			 */
			goto copy;
		}

		old_ldt = pmap->pm_ldt;
		free_ldt = old_ldt;
		free_len = pmap->pm_ldt_len * sizeof(union descriptor);

		if (old_ldt != NULL) {
			old_len = pmap->pm_ldt_len * sizeof(union descriptor);
		} else {
			old_len = NLDT * sizeof(union descriptor);
			old_ldt = ldt;
		}

		memcpy(new_ldt, old_ldt, old_len);
		memset((char *)new_ldt + old_len, 0, new_len - old_len);

		pmap->pm_ldt = new_ldt;
		pmap->pm_ldt_len = ldt_len;

		if (pmap->pm_flags & PMF_USER_LDT)
			free_sel = pmap->pm_ldt_sel;
		else {
			pmap->pm_flags |= PMF_USER_LDT;
			free_sel = -1;
		}
		pmap->pm_ldt_sel = sel;
		pcb->pcb_ldt_sel = pmap->pm_ldt_sel;
		if (pcb == curpcb)
			lldt(pcb->pcb_ldt_sel);
		new_ldt = NULL;
	}
copy:
	/* Now actually replace the descriptors. */
	for (i = 0, n = ua->start; i < ua->num; i++, n++)
		pmap->pm_ldt[n] = descv[i];

	mutex_exit(&pmap->pm_lock);

	if (new_ldt != NULL)
		uvm_km_free(kernel_map, (vaddr_t)new_ldt, new_len,
		    UVM_KMF_WIRED);
	if (free_sel != -1)
		ldt_free(free_sel);
	if (free_ldt != NULL)
		uvm_km_free(kernel_map, (vaddr_t)free_ldt, free_len,
		    UVM_KMF_WIRED);

	return (error);
#endif
}

int
x86_iopl(struct lwp *l, void *args, register_t *retval)
{
	int error;
	struct x86_iopl_args ua;
#ifdef XEN
	int iopl;
#else
	struct trapframe *tf = l->l_md.md_regs;
#endif

	error = kauth_authorize_machdep(l->l_cred, KAUTH_MACHDEP_IOPL,
	    NULL, NULL, NULL, NULL);
	if (error)
		return (error);

	if ((error = copyin(args, &ua, sizeof(ua))) != 0)
		return error;

#ifdef XEN
	if (ua.iopl)
		iopl = SEL_UPL;
	else
		iopl = SEL_KPL;
	l->l_addr->u_pcb.pcb_iopl = iopl;
	/* Force the change at ring 0. */
#ifdef XEN3
	{
		struct physdev_op physop;
		physop.cmd = PHYSDEVOP_SET_IOPL;
		physop.u.set_iopl.iopl = iopl;
		HYPERVISOR_physdev_op(&physop);
	}
#else /* XEN3 */
	{
		dom0_op_t op;
		op.cmd = DOM0_IOPL;
		op.u.iopl.domain = DOMID_SELF;
		op.u.iopl.iopl = iopl;
		HYPERVISOR_dom0_op(&op);
	}
#endif /* XEN3 */
#elif defined(__x86_64__)
	if (ua.iopl)
		tf->tf_rflags |= PSL_IOPL;
	else
		tf->tf_rflags &= ~PSL_IOPL;
#else
	if (ua.iopl)
		tf->tf_eflags |= PSL_IOPL;
	else
		tf->tf_eflags &= ~PSL_IOPL;
#endif

	return 0;
}

int
x86_get_ioperm(struct lwp *l, void *args, register_t *retval)
{
#ifdef IOPERM
	int error;
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct x86_get_ioperm_args ua;
	void *dummymap = NULL;
	void *iomap;

	error = kauth_authorize_machdep(l->l_cred, KAUTH_MACHDEP_IOPERM_GET,
	    NULL, NULL, NULL, NULL);
	if (error)
		return (error);

	if ((error = copyin(args, &ua, sizeof(ua))) != 0)
		return (error);

	iomap = pcb->pcb_iomap;
	if (iomap == NULL) {
		iomap = dummymap = kmem_alloc(IOMAPSIZE, KM_SLEEP);
		memset(dummymap, 0xff, IOMAPSIZE);
	}
	error = copyout(iomap, ua.iomap, IOMAPSIZE);
	if (dummymap != NULL) {
		kmem_free(dummymap, IOMAPSIZE);
	}
	return error;
#else
	return EINVAL;
#endif
}

int
x86_set_ioperm(struct lwp *l, void *args, register_t *retval)
{
#ifdef IOPERM
	struct cpu_info *ci;
	int error;
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct x86_set_ioperm_args ua;
	void *new;
	void *old;

  	error = kauth_authorize_machdep(l->l_cred, KAUTH_MACHDEP_IOPERM_SET,
	    NULL, NULL, NULL, NULL);
	if (error)
		return (error);

	if ((error = copyin(args, &ua, sizeof(ua))) != 0)
		return (error);

	new = kmem_alloc(IOMAPSIZE, KM_SLEEP);
	error = copyin(ua.iomap, new, IOMAPSIZE);
	if (error) {
		kmem_free(new, IOMAPSIZE);
		return error;
	}
	old = pcb->pcb_iomap;
	pcb->pcb_iomap = new;
	if (old != NULL) {
		kmem_free(old, IOMAPSIZE);
	}

	crit_enter();
	ci = curcpu();
	memcpy(ci->ci_iomap, pcb->pcb_iomap, sizeof(ci->ci_iomap));
	ci->ci_tss.tss_iobase =
	    ((uintptr_t)ci->ci_iomap - (uintptr_t)&ci->ci_tss) << 16;
	crit_exit();

	return error;
#else
	return EINVAL;
#endif
}

int
x86_get_mtrr(struct lwp *l, void *args, register_t *retval)
{
#ifdef MTRR
	struct x86_get_mtrr_args ua;
	int error, n;

	if (mtrr_funcs == NULL)
		return ENOSYS;

 	error = kauth_authorize_machdep(l->l_cred, KAUTH_MACHDEP_MTRR_GET,
	    NULL, NULL, NULL, NULL);
	if (error)
		return (error);

	error = copyin(args, &ua, sizeof ua);
	if (error != 0)
		return error;

	error = copyin(ua.n, &n, sizeof n);
	if (error != 0)
		return error;

	error = mtrr_get(ua.mtrrp, &n, l->l_proc, MTRR_GETSET_USER);

	copyout(&n, ua.n, sizeof (int));

	return error;
#else
	return EINVAL;
#endif
}

int
x86_set_mtrr(struct lwp *l, void *args, register_t *retval)
{
#ifdef MTRR
	int error, n;
	struct x86_set_mtrr_args ua;

	if (mtrr_funcs == NULL)
		return ENOSYS;

 	error = kauth_authorize_machdep(l->l_cred, KAUTH_MACHDEP_MTRR_SET,
	    NULL, NULL, NULL, NULL);
	if (error)
		return (error);

	error = copyin(args, &ua, sizeof ua);
	if (error != 0)
		return error;

	error = copyin(ua.n, &n, sizeof n);
	if (error != 0)
		return error;

	error = mtrr_set(ua.mtrrp, &n, l->l_proc, MTRR_GETSET_USER);
	if (n != 0)
		mtrr_commit();

	copyout(&n, ua.n, sizeof n);

	return error;
#else
	return EINVAL;
#endif
}

int
x86_set_sdbase(void *arg, char which)
{
#ifdef i386
	struct segment_descriptor sd;
	vaddr_t base;
	int error;

	error = copyin(arg, &base, sizeof(base));
	if (error != 0)
		return error;

	sd.sd_lobase = base & 0xffffff;
	sd.sd_hibase = (base >> 24) & 0xff;
	sd.sd_lolimit = 0xffff;
	sd.sd_hilimit = 0xf;
	sd.sd_type = SDT_MEMRWA;
	sd.sd_dpl = SEL_UPL;
	sd.sd_p = 1;
	sd.sd_xx = 0;
	sd.sd_def32 = 1;
	sd.sd_gran = 1;

	crit_enter();
	if (which == 'f') {
		memcpy(&curpcb->pcb_fsd, &sd, sizeof(sd));
		memcpy(&curcpu()->ci_gdt[GUFS_SEL], &sd, sizeof(sd));
	} else /* which == 'g' */ {
		memcpy(&curpcb->pcb_gsd, &sd, sizeof(sd));
		memcpy(&curcpu()->ci_gdt[GUGS_SEL], &sd, sizeof(sd));
	}
	crit_exit();

	return 0;
#else
	return EINVAL;
#endif
}

int
x86_get_sdbase(void *arg, char which)
{
#ifdef i386
	struct segment_descriptor *sd;
	vaddr_t base;

	switch (which) {
	case 'f':
		sd = (struct segment_descriptor *)&curpcb->pcb_fsd;
		break;
	case 'g':
		sd = (struct segment_descriptor *)&curpcb->pcb_gsd;
		break;
	default:
		panic("x86_get_sdbase");
	}

	base = sd->sd_hibase << 24 | sd->sd_lobase;
	return copyout(&base, &arg, sizeof(base));
#else
	return EINVAL;
#endif
}

int
sys_sysarch(struct lwp *l, const struct sys_sysarch_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) op;
		syscallarg(void *) parms;
	} */
	int error = 0;

	switch(SCARG(uap, op)) {
	case X86_IOPL: 
		error = x86_iopl(l, SCARG(uap, parms), retval);
		break;

	case X86_GET_LDT: 
		error = x86_get_ldt(l, SCARG(uap, parms), retval);
		break;

	case X86_SET_LDT: 
		error = x86_set_ldt(l, SCARG(uap, parms), retval);
		break;

	case X86_GET_IOPERM: 
		error = x86_get_ioperm(l, SCARG(uap, parms), retval);
		break;

	case X86_SET_IOPERM: 
		error = x86_set_ioperm(l, SCARG(uap, parms), retval);
		break;

	case X86_GET_MTRR:
		error = x86_get_mtrr(l, SCARG(uap, parms), retval);
		break;
	case X86_SET_MTRR:
		error = x86_set_mtrr(l, SCARG(uap, parms), retval);
		break;

#ifdef VM86
	case X86_VM86:
		error = x86_vm86(l, SCARG(uap, parms), retval);
		break;
#ifdef COMPAT_16
	case X86_OLD_VM86:
		error = compat_16_x86_vm86(l, SCARG(uap, parms), retval);
		break;
#endif
#endif

#ifdef PERFCTRS
	case X86_PMC_INFO:
		error = pmc_info(l, SCARG(uap, parms), retval);
		break;

	case X86_PMC_STARTSTOP:
		error = pmc_startstop(l, SCARG(uap, parms), retval);
		break;

	case X86_PMC_READ:
		error = pmc_read(l, SCARG(uap, parms), retval);
		break;
#endif

	case X86_SET_FSBASE:
		error = x86_set_sdbase(SCARG(uap, parms), 'f');
		break;

	case X86_SET_GSBASE:
		error = x86_set_sdbase(SCARG(uap, parms), 'g');
		break;

	case X86_GET_FSBASE:
		error = x86_get_sdbase(SCARG(uap, parms), 'f');
		break;

	case X86_GET_GSBASE:
		error = x86_get_sdbase(SCARG(uap, parms), 'g');
		break;

	default:
		error = EINVAL;
		break;
	}
	return (error);
}
