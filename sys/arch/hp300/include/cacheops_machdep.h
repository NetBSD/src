/*	$NetBSD: cacheops_machdep.h,v 1.6 2005/12/24 20:07:03 perry Exp $	*/

/*
 * Copyright (c) 1980, 1990, 1993
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
 */

/*
 * Copyright (c) 1994, 1995 Gordon W. Ross
 * Copyright (c) 1988 University of Utah.
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

#ifndef _HP300_CACHEOPS_MACHDEP_H_
#define	_HP300_CACHEOPS_MACHDEP_H_

extern vaddr_t MMUbase;

static inline int __attribute__((__unused__))
DCIA_md(void)
{
	volatile int *ip = (void *)(MMUbase + MMUCMD);

	if (ectype != EC_VIRT) {
		return 0;
	}

	*ip &= ~MMU_CEN;
	*ip |= MMU_CEN;
	return 1;
}

static inline int __attribute__((__unused__))
DCIS_md(void)
{
	volatile int *ip = (void *)(MMUbase + MMUSSTP);

	if (ectype != EC_VIRT) {
		return 0;
	}

	*ip = *ip;
	return 1;
}

static inline int __attribute__((__unused__))
DCIU_md(void)
{
	volatile int *ip = (void *)(MMUbase + MMUUSTP);

	if (ectype != EC_VIRT) {
		return 0;
	}

	*ip = *ip;
	return 1;
}

static inline int __attribute__((__unused__))
PCIA_md(void)
{
	volatile int *ip = (void *)(MMUbase + MMUCMD);

	if (ectype != EC_PHYS || cputype != CPU_68030) {
		return 0;
	}

	*ip &= ~MMU_CEN;
	*ip |= MMU_CEN;

	/*
	 * only some '030 models (345/370/375/400) have external PAC,
	 * so we need to do the standard flushing as well.
	 */

	return 0;
}

static inline int __attribute__((__unused__))
TBIA_md(void)
{
	volatile int *ip = (void *)(MMUbase + MMUTBINVAL);

	if (mmutype != MMU_HP) {
		return 0;
	}

	(void) *ip;
	return 1;
}

static inline int __attribute__((__unused__))
TBIS_md(vaddr_t va)
{
	register vaddr_t r_va __asm("%a1") = va;
	int s;

	if (mmutype != MMU_HP) {
		return 0;
	}

	s = splhigh();
	__asm volatile (" movc   %0, %%dfc;"	/* select purge space */
			  " movsl  %3, %1@;"	/* purge it */
			  " movc   %2, %%dfc;"
			  : : "r" (FC_PURGE), "a" (r_va), "r" (FC_USERD),
			  "r" (0));
	splx(s);
	return 1;
}

static inline int __attribute__((__unused__))
TBIAS_md(void)
{
	volatile int *ip = (void *)(MMUbase + MMUTBINVAL);

	if (mmutype != MMU_HP) {
		return 0;
	}

	*ip = 0x8000;
	return 1;
}

static inline int __attribute__((__unused__))
TBIAU_md(void)
{
	volatile int *ip = (void *)(MMUbase + MMUTBINVAL);

	if (mmutype != MMU_HP) {
		return 0;
	}

	*ip = 0;
	return 1;
}
#endif /* _HP300_CACHEOPS_MACHDEP_H_ */
