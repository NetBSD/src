/*	$NetBSD: sys_machdep.c,v 1.7.2.2 1999/11/29 21:28:36 he Exp $ */

/*
 * This file was taken from mvme68k/mvme68k/sys_machdep.c
 * should probably be re-synced when needed.
 * Darrin B. Jewell <jewell@mit.edu>  Tue Nov 10 05:07:16 1998
 * original cvs id: NetBSD: sys_machdep.c,v 1.12 1998/08/22 10:55:36 scw Exp
 */

/*
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)sys_machdep.c	8.2 (Berkeley) 1/13/94
 */

#include "opt_compat_hpux.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/trace.h>
#include <sys/mount.h>

#include <vm/vm.h>

#include <sys/syscallargs.h>

#include <machine/cpu.h>
#include <m68k/cacheops.h>

#ifdef TRACE
int	nvualarm;

vtrace(p, v, retval)
	struct proc *p;
	void *v;
	int *retval;
{
	struct vtrace_args /* {
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
		timeout(vdoualarm, (void *)p->p_pid, SCARG(uap, value));
		break;

	case VTR_STAMP:
		trace(TR_STAMP, SCARG(uap, value), p->p_pid);
		break;
	}
	return (0);
}

vdoualarm(arg)
	void *arg;
{
	int pid = (int)arg;
	struct proc *p;

	p = pfind(pid);
	if (p)
		psignal(p, 16);
	nvualarm--;
}
#endif

/* XXX should be in an include file somewhere */
#define CC_PURGE	1
#define CC_FLUSH	2
#define CC_IPURGE	4
#define CC_EXTPURGE	0x80000000
/* XXX end should be */

/*
 * Note that what we do here for a 68040 is different than HP-UX.
 *
 * In 'pux they either act on a line (len == 16), a page (len == NBPG)
 * or the whole cache (len == anything else).
 *
 * In BSD we attempt to be more optimal when acting on "odd" sizes.
 * For lengths up to 1024 we do all affected lines, up to 2*NBPG we
 * do pages, above that we do the entire cache.
 */
/*ARGSUSED1*/
int
cachectl1(req, addr, len, p)
	unsigned long req;
	vaddr_t	addr;
	size_t len;
	struct proc *p;
{
	int error = 0;

#if defined(M68040)
	if (mmutype == MMU_68040) {
		int inc = 0;
		int pa = 0, doall = 0;
		vaddr_t end;
#ifdef COMPAT_HPUX
		extern struct emul emul_hpux;

		if ((p->p_emul == &emul_hpux) &&
		    len != 16 && len != NBPG)
			doall = 1;
#endif

		if (addr == 0 ||
#if defined(M68040)
#if defined(M68060)
		    (cputype == CPU_68040 && req & CC_IPURGE) ||
#else
		    (req & CC_IPURGE) ||
#endif
#endif
		    (req & ~CC_EXTPURGE) != CC_PURGE && len > 2*NBPG)
			doall = 1;

		if (!doall) {
			end = addr + len;
			if (len <= 1024) {
				addr = addr & ~0xF;
				inc = 16;
			} else {
				addr = addr & ~PGOFSET;
				inc = NBPG;
			}
		}
		do {
			/*
			 * Convert to physical address if needed.
			 * If translation fails, we perform operation on
			 * entire cache (XXX is this a rational thing to do?)
			 */
			if (!doall &&
			    (pa == 0 || ((int)addr & PGOFSET) == 0)) {
				pa = pmap_extract(p->p_vmspace->vm_map.pmap,
						  addr);
				if (pa == 0)
					doall = 1;
			}
			switch (req) {
			case CC_EXTPURGE|CC_IPURGE:
			case CC_IPURGE:
				if (doall) {
					DCFA();
					ICPA();
				} else if (inc == 16) {
					DCFL(pa);
					ICPL(pa);
				} else if (inc == NBPG) {
					DCFP(pa);
					ICPP(pa);
				}
				break;

			case CC_EXTPURGE|CC_PURGE:
			case CC_PURGE:
				if (doall)
					DCFA(); /* note: flush not purge */
				else if (inc == 16)
					DCPL(pa);
				else if (inc == NBPG)
					DCPP(pa);
				break;

			case CC_EXTPURGE|CC_FLUSH:
			case CC_FLUSH:
				if (doall)
					DCFA();
				else if (inc == 16)
					DCFL(pa);
				else if (inc == NBPG)
					DCFP(pa);
				break;

			default:
				error = EINVAL;
				break;
			}
			if (doall)
				break;
			pa += inc;
			addr += inc;
		} while (addr < end);
		return(error);
	}
#endif
	switch (req) {
	case CC_EXTPURGE|CC_PURGE:
	case CC_EXTPURGE|CC_FLUSH:
#if defined(HP370)
		if (ectype == EC_PHYS)
			PCIA();
		/* fall into... */
#endif
	case CC_PURGE:
	case CC_FLUSH:
		DCIU();
		break;
	case CC_EXTPURGE|CC_IPURGE:
#if defined(HP370)
		if (ectype == EC_PHYS)
			PCIA();
		else
#endif
		DCIU();
		/* fall into... */
	case CC_IPURGE:
		ICIA();
		break;
	default:
		error = EINVAL;
		break;
	}
	return(error);
}

int
sys_sysarch(p, v, retval)
	struct proc *p;
	void *v;
	int *retval;
{
	struct sysarch_args /* {
		syscallarg(int) op; 
		syscallarg(void *) parms;
	} */ *uap = v;

	return (ENOSYS);
}
