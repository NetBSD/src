/*	$NetBSD: sys_machdep.c,v 1.14 2007/12/31 13:38:51 ad Exp $	*/

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
 *	@(#)sys_machdep.c	8.2 (Berkeley) 1/13/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/mount.h>

#include <uvm/uvm_extern.h>

#include <sys/syscallargs.h>

#include <machine/cpu.h>
#include <m68k/cacheops.h>

/* XXX should be in an include file somewhere */
#define CC_PURGE	1
#define CC_FLUSH	2
#define CC_IPURGE	4
#define CC_EXTPURGE	0x80000000
/* XXX end should be */

/*
 * Note that what we do here on 040/060 for BSD is different than for HP-UX.
 *
 * In 'pux they either act on a line (len == 16), a page (len == PAGE_SIZE)
 * or the whole cache (len == anything else).
 *
 * In BSD we attempt to be more optimal when acting on "odd" sizes.
 * For lengths up to 1024 we do all affected lines, up to 2*PAGE_SIZE we
 * do pages, above that we do the entire cache.
 */
/*ARGSUSED1*/
int
cachectl1(u_long req, vaddr_t addr, size_t len, struct proc *p)
{
	int error = 0;

#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040) {
		int inc = 0;
		bool doall = false;
		paddr_t pa = 0;
		vaddr_t end = 0;

		if (addr == 0 ||
#if defined(M68060)
		    (cputype == CPU_68040 && req & CC_IPURGE) ||
#else
		    (req & CC_IPURGE) ||
#endif
		    ((req & ~CC_EXTPURGE) != CC_PURGE && len > 2 * PAGE_SIZE))
			doall = 1;

		if (!doall) {
			end = addr + len;
			if (len <= 1024) {
				addr = addr & ~0xf;
				inc = 16;
			} else {
				addr = addr & ~PGOFSET;
				inc = PAGE_SIZE;
			}
		}
		do {
			/*
			 * Convert to physical address if needed.
			 * If translation fails, we perform operation on
			 * entire cache.
			 */
			if (!doall &&
			    (pa == 0 || m68k_page_offset(addr) == 0)) {
				if (pmap_extract(p->p_vmspace->vm_map.pmap,
				    addr, &pa) == false)
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
				} else if (inc == PAGE_SIZE) {
					DCFP(pa);
					ICPP(pa);
				}
				break;

			case CC_EXTPURGE|CC_PURGE:
			case CC_PURGE:
				if (doall)
					DCFA();	/* note: flush not purge */
				else if (inc == 16)
					DCPL(pa);
				else if (inc == PAGE_SIZE)
					DCPP(pa);
				break;

			case CC_EXTPURGE|CC_FLUSH:
			case CC_FLUSH:
				if (doall)
					DCFA();
				else if (inc == 16)
					DCFL(pa);
				else if (inc == PAGE_SIZE)
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
		return (error);
	}
#endif
	switch (req) {
	case CC_EXTPURGE|CC_PURGE:
	case CC_EXTPURGE|CC_FLUSH:
#if defined(CACHE_HAVE_PAC)
		if (ectype == EC_PHYS)
			PCIA();
		/* fall into... */
#endif
	case CC_PURGE:
	case CC_FLUSH:
		DCIU();
		break;
	case CC_EXTPURGE|CC_IPURGE:
#if defined(CACHE_HAVE_PAC)
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
	return error;
}

int
sys_sysarch(struct lwp *l, const struct sys_sysarch_args *uap, register_t *retval)
{

	return ENOSYS;
}

#if defined(amiga) || defined(x68k)

/*
 * DMA cache control
 */

/*ARGSUSED1*/
int
dma_cachectl(void *addr, int len)
{
#if defined(M68040) || defined(M68060)
	int inc = 0;
	int pa = 0;
	void *end;

	if (mmutype != MMU_68040) {
		return 0;
	}

	end = (char*)addr + len;
	if (len <= 1024) {
		addr = (void *)((vaddr_t)addr & ~0xf);
		inc = 16;
	} else {
		addr = (void *)((vaddr_t)addr & ~PGOFSET);
		inc = PAGE_SIZE;
	}
	do {
		/*
		 * Convert to physical address.
		 */
		if (pa == 0 || ((vaddr_t)addr & PGOFSET) == 0) {
			pa = kvtop(addr);
		}
		if (inc == 16) {
			DCFL(pa);
			ICPL(pa);
		} else {
			DCFP(pa);
			ICPP(pa);
		}
		pa += inc;
		addr = (char*)addr + inc;
	} while (addr < end);
#endif	/* defined(M68040) || defined(M68060) */
	return 0;
}
#endif	/* defined(amiga) || defined(x68k) */
