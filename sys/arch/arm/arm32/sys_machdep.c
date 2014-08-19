/*	$NetBSD: sys_machdep.c,v 1.15.2.2 2014/08/20 00:02:45 tls Exp $	*/

/*
 * Copyright (c) 1995-1997 Mark Brinicombe.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * sys_machdep.c
 *
 * Machine dependent syscalls
 *
 * Created      : 10/01/96
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_machdep.c,v 1.15.2.2 2014/08/20 00:02:45 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/cpu.h>
#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>
#include <sys/syscallargs.h>

#include <machine/sysarch.h>
#include <machine/pcb.h>
#include <arm/vfpreg.h>
#include <arm/locore.h>

/* Prototypes */
static int arm32_sync_icache(struct lwp *, const void *, register_t *);
static int arm32_drain_writebuf(struct lwp *, const void *, register_t *);
static int arm32_vfp_fpscr(struct lwp *, const void *, register_t *);
static int arm32_fpu_used(struct lwp *, const void *, register_t *);

static int
arm32_sync_icache(struct lwp *l, const void *args, register_t *retval)
{
	struct arm_sync_icache_args ua;
	int error;

	if ((error = copyin(args, &ua, sizeof(ua))) != 0)
		return (error);

	pmap_icache_sync_range(vm_map_pmap(&l->l_proc->p_vmspace->vm_map),
	    ua.addr, ua.addr + ua.len);

	*retval = 0;
	return(0);
}

static int
arm32_drain_writebuf(struct lwp *l, const void *args, register_t *retval)
{
	/* No args. */

	cpu_drain_writebuf();

	*retval = 0;
	return(0);
}

static int
arm32_vfp_fpscr(struct lwp *l, const void *uap, register_t *retval)
{
	struct pcb * const pcb = lwp_getpcb(l);

#ifdef FPU_VFP
	/*
	 * Save the current VFP state (to make sure the FPSCR copy is
	 * up to date).
	 */
	vfp_savecontext();
#endif

	retval[0] = pcb->pcb_vfp.vfp_fpscr;
	if (uap) {
		extern uint32_t vfp_fpscr_changable;
		struct arm_vfp_fpscr_args ua;
		int error;
		if ((error = copyin(uap, &ua, sizeof(ua))) != 0)
			return (error);
		if ((ua.fpscr_clear|ua.fpscr_set) & ~vfp_fpscr_changable)
			return EINVAL;
		pcb->pcb_vfp.vfp_fpscr &= ~ua.fpscr_clear;
		pcb->pcb_vfp.vfp_fpscr |= ua.fpscr_set;
	}

	return 0;
}

static int
arm32_fpu_used(struct lwp *l, const void *uap, register_t *retval)
{
	/* No args */
#ifdef FPU_VFP
	retval[0] = vfp_used_p();
#else
	retval[0] = false;
#endif
	return 0;
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
	case ARM_SYNC_ICACHE : 
		error = arm32_sync_icache(l, SCARG(uap, parms), retval);
		break;

	case ARM_DRAIN_WRITEBUF : 
		error = arm32_drain_writebuf(l, SCARG(uap, parms), retval);
		break;

	case ARM_VFP_FPSCR :
		error = arm32_vfp_fpscr(l, SCARG(uap, parms), retval);
		break;

	case ARM_FPU_USED :
		error = arm32_fpu_used(l, SCARG(uap, parms), retval);
		break;

	default:
		error = EINVAL;
		break;
	}
	return (error);
}
  
int
cpu_lwp_setprivate(lwp_t *l, void *addr)
{
#ifdef _ARM_ARCH_6
	if (l == curlwp) {
		kpreempt_disable();
		__asm("mcr p15, 0, %0, c13, c0, 3" : : "r" (addr));
		kpreempt_enable();
	}
	return 0;
#else
	return 0;
#endif
}
