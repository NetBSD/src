/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software developed by the Computer Systems
 * Engineering group at Lawrence Berkeley Laboratory under DARPA contract
 * BG 91-66 and contributed to Berkeley.
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
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)kvm_sparc.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

/*
 * Sparc machine dependent routines for kvm.  Hopefully, the forthcoming 
 * vm code will one day obsolete this module.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <unistd.h>
#include <nlist.h>
#include <kvm.h>

#include <vm/vm.h>
#include <vm/vm_param.h>

#include <limits.h>
#include <db.h>

#include "kvm_private.h"

#define NPMEG 128

/* XXX from sparc/pmap.c */
#define MAXMEM  (128 * 1024 * 1024)     /* no more than 128 MB phys mem */
#define NPGBANK 16                      /* 2^4 pages per bank (64K / bank) */
#define BSHIFT  4                       /* log2(NPGBANK) */
#define BOFFSET (NPGBANK - 1)
#define BTSIZE  (MAXMEM / 4096 / NPGBANK)
#define HWTOSW(pmap_stod, pg) (pmap_stod[(pg) >> BSHIFT] | ((pg) & BOFFSET))

struct vmstate {
	struct regmap regmap[NKREG];
	int *pmeg;
	int pmap_stod[BTSIZE];              /* dense to sparse */
};

static int cputyp = -1;

static int pgshift, nptesg;

#define VA_VPG(va)	(cputyp==CPU_SUN4C ? VA_SUN4C_VPG(va) : VA_SUN4_VPG(va))

static void
_kvm_mustinit(kd)
	kvm_t *kd;
{
	if (cputyp != -1)
		return;
	for (pgshift = 12; (1 << pgshift) != kd->nbpg; pgshift++)
		;
	nptesg = NBPSG / kd->nbpg;

#if 1
	if (cputyp == -1) {
		if (kd->nbpg == 8192)
			cputyp = CPU_SUN4;
		else
			cputyp = CPU_SUN4C;
	}
#endif
}

void
_kvm_freevtop(kd)
	kvm_t *kd;
{
	if (kd->vmst != 0) {
		if (kd->vmst->pmeg != 0)
			free(kd->vmst->pmeg);
		free(kd->vmst);
		kd->vmst  = 0;
	}
}

int
_kvm_initvtop(kd)
	kvm_t *kd;
{
	register int i;
	register int off;
	register struct vmstate *vm;
	struct stat st;
	struct nlist nlist[2];

	_kvm_mustinit(kd);

#if XXX
	if (kd->vmst == 0) {
		kd->vmst = (struct vmstate *)_kvm_malloc(kd, sizeof(*vm));
		if (kd->vmst == 0)
			return (-1);
		kd->vmst->pmeg = (int *)_kvm_malloc(kd,
		    NPMEG * nptesg * sizeof(int));
		if (kd->vmst->pmeg == 0) {
			free(kd->vmst);
			kd->vmst = 0;
			return (-1);
		}
	}

	if (fstat(kd->pmfd, &st) < 0)
		return (-1);
	/*
	 * Read segment table.
	 */
	off = st.st_size - roundup(sizeof(vm->regmap), kd->nbpg);
	errno = 0;
	if (lseek(kd->pmfd, (off_t)off, 0) == -1 && errno != 0 || 
	    read(kd->pmfd, (char *)vm->regmap, sizeof(vm->regmap)) < 0) {
		_kvm_err(kd, kd->program, "cannot read segment map");
		return (-1);
	}
	/*
	 * Read PMEGs.
	 */
	off = st.st_size - roundup(NPMEG * nptesg * sizeof(int), kd->nbpg) +
	    ((sizeof(vm->regmap) + kd->nbpg - 1) >> pgshift);
	errno = 0;
	if (lseek(kd->pmfd, (off_t)off, 0) == -1 && errno != 0 || 
	    read(kd->pmfd, (char *)vm->pmeg, NPMEG * nptesg * sizeof(int)) < 0) {
		_kvm_err(kd, kd->program, "cannot read PMEG table");
		return (-1);
	}
	/*
	 * Make pmap_stod be an identity map so we can bootstrap it in.
	 * We assume it's in the first contiguous chunk of physical memory.
	 */
	for (i = 0; i < BTSIZE; ++i) 
		vm->pmap_stod[i] = i << 4;

	/*
	 * It's okay to do this nlist separately from the one kvm_getprocs()
	 * does, since the only time we could gain anything by combining
	 * them is if we do a kvm_getprocs() on a dead kernel, which is
	 * not too common.
	 */
	nlist[0].n_name = "_pmap_stod";
	nlist[1].n_name = 0;
	(void)kvm_nlist(kd, nlist);

	/*
	 * a kernel compiled only for the sun4 will not contain the symbol
	 * pmap_stod. Instead, we are happy to use the identity map
	 * initialized earlier.
	 * If we are not a sun4, the lack of this symbol is fatal.
	 */
	if (nlist[0].n_value != 0) {
		if (kvm_read(kd, (u_long)nlist[0].n_value, 
		    (char *)vm->pmap_stod, sizeof(vm->pmap_stod))
		    != sizeof(vm->pmap_stod)) {
			_kvm_err(kd, kd->program, "cannot read pmap_stod");
			return (-1);
		}
	} else {
		if (cputyp != CPU_SUN4) {
			_kvm_err(kd, kd->program, "pmap_stod: no such symbol");
			return (-1);
		}
	}
#endif

	return (0);
}

#define VA_OFF(va) (va & (kd->nbpg - 1))

/*
 * Translate a kernel virtual address to a physical address using the
 * mapping information in kd->vm.  Returns the result in pa, and returns
 * the number of bytes that are contiguously available from this 
 * physical address.  This routine is used only for crashdumps.
 */
int
_kvm_kvatop(kd, va, pa)
	kvm_t *kd;
	u_long va;
	u_long *pa;
{
#if XXX
	register struct vmstate *vm;
	register int s;
	register int pte;
	register int off;

	_kvm_mustinit(kd);

	if (va >= KERNBASE) {
		vm = kd->vmst;
		s = vm->regmap[VA_VREG(va) - NUREG];
		pte = vm->pmeg[VA_VPG(va) + nptesg * s];
		if ((pte & PG_V) != 0) {
			off = VA_OFF(va);
			*pa = (HWTOSW(vm->pmap_stod, pte & PG_PFNUM)
			       << pgshift) | off;

			return (kd->nbpg - off);
		}
	}
#endif
	_kvm_err(kd, 0, "invalid address (%x)", va);
	return (0);
}

#if 0
static int
getcputyp()
{
	int mib[2];
	size_t size;

	mib[0] = CTL_HW;
	mib[1] = HW_CLASS;
	size = sizeof cputyp;
	if (sysctl(mib, 2, &cputyp, &size, NULL, 0) == -1)
		return (-1);
}
#endif
