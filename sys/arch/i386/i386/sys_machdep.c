/*	$NetBSD: sys_machdep.c,v 1.22 1995/10/09 06:34:25 mycroft Exp $	*/

/*-
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)sys_machdep.c	5.5 (Berkeley) 1/19/91
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/mtio.h>
#include <sys/buf.h>
#include <sys/trace.h>
#include <sys/signal.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/sysarch.h>

extern vm_map_t kernel_map;

#ifdef TRACE
int	nvualarm;

sys_vtrace(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	register struct sys_vtrace_args /* {
		syscallarg(int) request;
		syscallarg(int) value;
	} */ *uap = v;
	int vdoualarm();

	switch (SCARG(uap, request)) {

	case VTR_DISABLE:		/* disable a trace point */
	case VTR_ENABLE:		/* enable a trace point */
		if (SCARG(uap, value) < 0 || SCARG(uap, value) >= TR_NFLAGS)
			return (EINVAL);
		*retval = traceflags[SCARG(uap, value)];
		traceflags[SCARG(uap, value)] = SCARG(uap, request);
		break;

	case VTR_VALUE:		/* return a trace point setting */
		if (SCARG(uap, value) < 0 || SCARG(uap, value) >= TR_NFLAGS)
			return (EINVAL);
		*retval = traceflags[SCARG(uap, value)];
		break;

	case VTR_UALARM:	/* set a real-time ualarm, less than 1 min */
		if (SCARG(uap, value) <= 0 || SCARG(uap, value) > 60 * hz ||
		    nvualarm > 5)
			return (EINVAL);
		nvualarm++;
		timeout(vdoualarm, (caddr_t)p->p_pid, SCARG(uap, value));
		break;

	case VTR_STAMP:
		trace(TR_STAMP, SCARG(uap, value), p->p_pid);
		break;
	}
	return (0);
}

vdoualarm(arg)
	int arg;
{
	register struct proc *p;

	p = pfind(arg);
	if (p)
		psignal(p, 16);
	nvualarm--;
}
#endif

#ifdef USER_LDT
void
set_user_ldt(pcb)
	struct pcb *pcb;
{

	gdt[GUSERLDT_SEL] = pcb->pcb_ldt_desc;
	lldt(currentldt = GSEL(GUSERLDT_SEL, SEL_KPL));
}

int
i386_get_ldt(p, args, retval)
	struct proc *p;
	char *args;
	register_t *retval;
{
	int error = 0;
	struct pcb *pcb = &p->p_addr->u_pcb;
	int nldt, num;
	union descriptor *lp;
	int s;
	struct i386_get_ldt_args ua, *uap;

	if ((error = copyin(args, &ua, sizeof(struct i386_get_ldt_args))) < 0)
		return (error);

	uap = &ua;

#ifdef	DEBUG
	printf("i386_get_ldt: start=%d num=%d descs=%x\n", uap->start,
	    uap->num, uap->desc);
#endif

	if (uap->start < 0 || uap->num < 0)
		return (EINVAL);

	if (pcb->pcb_ldt) {
		nldt = pcb->pcb_ldt_len;
		lp = (union descriptor *)pcb->pcb_ldt;
	} else {
		nldt = NLDT;
		lp = ldt;
	}
	if (uap->start > nldt) {
		splx(s);
		return (EINVAL);
	}
	lp += uap->start;
	num = min(uap->num, nldt - uap->start);

	if (error = copyout(lp, uap->desc, num * sizeof(union descriptor)))
		return (error);

	*retval = num;
	return (0);
}

int
i386_set_ldt(p, args, retval)
	struct proc *p;
	char *args;
	register_t *retval;
{
	int error = 0, i, n;
	struct pcb *pcb = &p->p_addr->u_pcb;
	union descriptor *lp;
	int s;
	struct i386_set_ldt_args ua, *uap;

	if (error = copyin(args, &ua, sizeof(struct i386_set_ldt_args)))
		return (error);

	uap = &ua;

#ifdef	DEBUG
	printf("i386_set_ldt: start=%d num=%d descs=%x\n", uap->start,
	    uap->num, uap->desc);
#endif

	if (uap->start < 0 || uap->num < 0)
		return (EINVAL);

	if (uap->start > 8192 || (uap->start + uap->num) > 8192)
		return (EINVAL);

	/* allocate user ldt */
	if (!pcb->pcb_ldt || (uap->start + uap->num) > pcb->pcb_ldt_len) {
		size_t oldlen, len;
		union descriptor *new_ldt;

		if (!pcb->pcb_ldt)
			pcb->pcb_ldt_len = 512;
		while ((uap->start + uap->num) > pcb->pcb_ldt_len)
			pcb->pcb_ldt_len *= 2;
		len = pcb->pcb_ldt_len * sizeof(union descriptor);
		new_ldt = (union descriptor *)kmem_alloc(kernel_map, len);
		if (!pcb->pcb_ldt) {
			oldlen = NLDT * sizeof(union descriptor);
			bcopy(ldt, new_ldt, oldlen);
		} else {
			oldlen = pcb->pcb_ldt_len * sizeof(union descriptor);
			bcopy(pcb->pcb_ldt, new_ldt, oldlen);
			kmem_free(kernel_map, (vm_offset_t)pcb->pcb_ldt,
			    oldlen);
		}
		bzero((caddr_t)new_ldt + oldlen, len - oldlen);
		pcb->pcb_ldt = (caddr_t)new_ldt;
		setsegment(&pcb->pcb_ldt_desc, new_ldt, len - 1, SDT_SYSLDT,
		    SEL_KPL, 0, 0);
		if (p == curproc)
			set_user_ldt(pcb);
#ifdef DEBUG
		printf("i386_set_ldt(%d): new_ldt=%x\n", p->p_pid, new_ldt);
#endif
	}

	/* Check descriptors for access violations */
	for (i = 0, n = uap->start; i < uap->num; i++, n++) {
		union descriptor desc, *dp;
		dp = &uap->desc[i];
		error = copyin(dp, &desc, sizeof(union descriptor));
		if (error)
			return (error);

		/* Only user (ring-3) descriptors */
		if (desc.sd.sd_dpl != SEL_UPL)
			return (EACCES);

		/* Must be "present" */
		if (desc.sd.sd_p == 0)
			return (EACCES);

		switch (desc.sd.sd_type) {
		case SDT_SYSNULL:
		case SDT_SYS286CGT:
		case SDT_SYS386CGT:
			break;
		case SDT_MEMRO:
		case SDT_MEMROA:
		case SDT_MEMRW:
		case SDT_MEMRWA:
		case SDT_MEMROD:
		case SDT_MEMRODA:
		case SDT_MEME:
		case SDT_MEMEA:
		case SDT_MEMER:
		case SDT_MEMERA:
		case SDT_MEMEC:
		case SDT_MEMEAC:
		case SDT_MEMERC:
		case SDT_MEMERAC: {
#if 0
			unsigned long base = (desc.sd.sd_hibase << 24)&0xFF000000;
			base |= (desc.sd.sd_lobase&0x00FFFFFF);
			if (base >= KERNBASE)
				return (EACCES);
#endif
			break;
		}
		default:
			return (EACCES);
			/*NOTREACHED*/
		}
	}

	s = splhigh();

	/* Fill in range */
	for (i = 0, n = uap->start; i < uap->num && !error; i++, n++) {
		union descriptor desc, *dp;
		dp = &uap->desc[i];
		lp = &((union descriptor *)(pcb->pcb_ldt))[n];
#ifdef DEBUG
		printf("i386_set_ldt(%d): ldtp=%x\n", p->p_pid, lp);
#endif
		error = copyin(dp, lp, sizeof(union descriptor));
	}
	if (!error)
		*retval = uap->start;

	splx(s);
	return (error);
}
#endif	/* USER_LDT */

int
sys_sysarch(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_sysarch_args /* {
		syscallarg(int) op;
		syscallarg(char *) parms;
	} */ *uap = v;
	int error = 0;

	switch(SCARG(uap, op)) {
#ifdef	USER_LDT
	case I386_GET_LDT: 
		error = i386_get_ldt(p, SCARG(uap, parms), retval);
		break;

	case I386_SET_LDT: 
		error = i386_set_ldt(p, SCARG(uap, parms), retval);
		break;
#endif
	default:
		error = EINVAL;
		break;
	}
	return (error);
}
