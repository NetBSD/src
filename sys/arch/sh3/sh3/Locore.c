/*	$NetBSD: Locore.c,v 1.10 2002/04/28 17:10:37 uch Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*-
 * Copyright (c) 1993, 1994, 1995, 1996, 1997
 *	 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1992 Terrence R. Lambert.
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
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
 *	@(#)Locore.c
 */

#include "opt_lockdebug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/sched.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <sh3/cpu.h>
#include <sh3/psl.h>
#include <sh3/mmu.h>
#include <sh3/locore.h>

void (*__sh_switch_resume)(struct proc *);
struct proc *cpu_switch_search(void);
void idle(void);
int want_resched;

#ifdef LOCKDEBUG
#define	SCHED_LOCK_IDLE()	sched_lock_idle()
#define	SCHED_UNLOCK_IDLE()	sched_unlock_idle()
#else
#define	SCHED_LOCK_IDLE()	((void)0)
#define	SCHED_UNLOCK_IDLE()	((void)0)
#endif

/*
 * struct proc *cpu_switch_search(void):
 *	Find the highest priority process.
 */
struct proc *
cpu_switch_search()
{
	struct prochd *q;
	struct proc *p;
	int s;

	curproc = 0;

	s = _cpu_intr_suspend();
	SCHED_LOCK_IDLE();
	while (sched_whichqs == 0) {
		SCHED_UNLOCK_IDLE();
		_cpu_intr_resume(s);
		idle();
		s = _cpu_intr_suspend();
		SCHED_LOCK_IDLE();
	}

	q = &sched_qs[ffs(sched_whichqs) - 1];
	p = q->ph_link;
	remrunqueue(p);
	want_resched = 0;
	_cpu_intr_resume(s);
	SCHED_UNLOCK_IDLE();

	p->p_stat = SONPROC;
	curproc = p;

	return (p);
}

/*
 * void idle(void):
 *	When no processes are on the run queue, wait for something to come
 *	ready. Separated function for profiling.
 */
void
idle()
{

	spl0();
	__asm__ __volatile__("sleep");
	splhigh();
}

/*
 * Put process p on the run queue indicated by its priority.
 * Calls should be made at splstatclock(), and p->p_stat should be SRUN.
 */
void
setrunqueue(struct proc *p)
{
	struct prochd *q;
	struct proc *oldlast;
	int which = p->p_priority >> 2;

#ifdef DIAGNOSTIC
	if (p->p_back || which >= 32 || which < 0)
		panic("setrunqueue");
#endif
	q = &sched_qs[which];
	sched_whichqs |= 0x00000001 << which;
	if (sched_whichqs == 0) {
		panic("setrunqueue[whichqs == 0 ]");
	}
	p->p_forw = (struct proc *)q;
	p->p_back = oldlast = q->ph_rlink;
	q->ph_rlink = p;
	oldlast->p_forw = p;
}

/*
 * Remove process p from its run queue, which should be the one
 * indicated by its priority.
 * Calls should be made at splstatclock().
 */
void
remrunqueue(struct proc *p)
{
	int which = p->p_priority >> 2;
	struct prochd *q;

#ifdef DIAGNOSTIC
	if (!(sched_whichqs & (0x00000001 << which)))
		panic("remrunqueue");
#endif
	p->p_forw->p_back = p->p_back;
	p->p_back->p_forw = p->p_forw;
	p->p_back = NULL;
	q = &sched_qs[which];
	if (q->ph_link == (struct proc *)q)
		sched_whichqs &= ~(0x00000001 << which);
}

/*
 * void sh3_switch_setup(struct proc *p):
 *	prepare kernel stack PTE table. TLB miss handler check these.
 */
void
sh3_switch_setup(struct proc *p)
{
	pt_entry_t *pte;
	struct md_upte *md_upte = p->p_md.md_upte;
	u_int32_t vpn;
	int i;

	vpn = (u_int32_t)p->p_md.md_p3;
	vpn &= ~PGOFSET;
	for (i = 0; i < UPAGES; i++, pte++, vpn += NBPG, md_upte++) {
		pte = vtopte(vpn);
		md_upte->addr = vpn;
		md_upte->data = (*pte & PG_HW_BITS) |
		    SH3_MMUDA_D_D | SH3_MMUDA_D_V;
	}
}

/*
 * void sh4_switch_setup(struct proc *p):
 *	prepare kernel stack PTE table. sh4_switch_resume wired this PTE.
 */
void
sh4_switch_setup(struct proc *p)
{
	pt_entry_t *pte;
	struct md_upte *md_upte = p->p_md.md_upte;
	u_int32_t vpn;
	int i, e;

	vpn = (u_int32_t)p->p_md.md_p3;
	pte = vtopte(vpn);
	vpn &= ~PGOFSET;
	e = SH4_UTLB_ENTRY - UPAGES;
	for (i = 0; i < UPAGES; i++, pte++, e++, vpn += NBPG) {
		/* Address array */
		md_upte->addr = SH4_UTLB_AA | (e << SH4_UTLB_E_SHIFT);
		md_upte->data = vpn | SH4_UTLB_AA_D | SH4_UTLB_AA_V;
		md_upte++;
		/* Data array */
		md_upte->addr = SH4_UTLB_DA1 | (e << SH4_UTLB_E_SHIFT);
		md_upte->data = (*pte & PG_HW_BITS) |
		    SH4_UTLB_DA1_D | SH4_UTLB_DA1_V;
		md_upte++;
	}
}

/*
 * copyout(caddr_t from, caddr_t to, size_t len);
 * Copy len bytes into the user's address space.
 */
int
copyout(const void *kaddr, void *uaddr, size_t len)
{
	const void *from = kaddr;
	char *to = uaddr;

	if (to + len < to || to + len > (char *)VM_MAXUSER_ADDRESS)
		return EFAULT;

	curpcb->pcb_onfault = &&Err999;
	memcpy(to, from, len);
	curpcb->pcb_onfault = 0;
	return 0;

 Err999:
	curpcb->pcb_onfault = 0;
	return EFAULT;
}

/*
 * copyin(caddr_t from, caddr_t to, size_t len);
 * Copy len bytes from the user's address space.
 */
int
copyin(const void *uaddr, void *kaddr, size_t len)
{
	const char *from = uaddr;
	void *to = kaddr;

	if (from + len < from || from + len > (char *)VM_MAXUSER_ADDRESS)
		return EFAULT;

	curpcb->pcb_onfault = &&Err999;
	memcpy(to, from, len);
	curpcb->pcb_onfault = 0;
	return 0;

 Err999:
	curpcb->pcb_onfault = 0;
	return EFAULT;
}

/*
 * copyoutstr(caddr_t from, caddr_t to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long, into the
 * user's address space.  Return the number of characters copied (including the
 * NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG; else
 * return 0 or EFAULT.
 */
int
copyoutstr(const void *kaddr, void *uaddr, size_t maxlen, size_t *lencopied)
{
	const char *from = kaddr;
	char *to = uaddr;
	int cnt;
	int rc = 0;
	const char *from_top = from;

	curpcb->pcb_onfault = &&Err999;

	if ((cnt = (char *)VM_MAXUSER_ADDRESS - to) > maxlen)
		cnt = maxlen;

	while (cnt--) {
		if ((*to++ = *from++) == 0) {
			rc = 0;
			goto out;
		}
	}

	if (to >= (char *)VM_MAXUSER_ADDRESS)
		rc = EFAULT;
	else
		rc = ENAMETOOLONG;

out:
	if (lencopied)
		*lencopied = from - from_top;
	curpcb->pcb_onfault = 0;
	return rc;

 Err999:
	if (lencopied)
		*lencopied = from - from_top;
	curpcb->pcb_onfault = 0;
	return EFAULT;
}

/*
 * copyinstr(caddr_t from, caddr_t to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long, from the
 * user's address space.  Return the number of characters copied (including the
 * NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG; else
 * return 0 or EFAULT.
 */
int
copyinstr(const void *uaddr, void *kaddr, size_t maxlen, size_t *lencopied)
{
	const char *from = uaddr;
	char *to = kaddr;
	int cnt;
	int rc = 0;
	const char *from_top = from;

	curpcb->pcb_onfault = &&Err999;

	if ((cnt = (char *)VM_MAXUSER_ADDRESS - to) > maxlen)
		cnt = maxlen;

	while (cnt--) {
		if ((*to++ = *from++) == 0) {
			rc = 0;
			goto out;
		}
	}

	if (to >= (char *)VM_MAXUSER_ADDRESS)
		rc = EFAULT;
	else
		rc = ENAMETOOLONG;

out:
	if (lencopied)
		*lencopied = from - from_top;
	curpcb->pcb_onfault = 0;
	return rc;

 Err999:
	if (lencopied)
		*lencopied = from - from_top;
	curpcb->pcb_onfault = 0;
	return EFAULT;
}

/*
 * copystr(caddr_t from, caddr_t to, size_t maxlen, size_t *lencopied);
 * Copy a NUL-terminated string, at most maxlen characters long.  Return the
 * number of characters copied (including the NUL) in *lencopied.  If the
 * string is too long, return ENAMETOOLONG; else return 0.
 */
int
copystr(const void *kfaddr, void *kdaddr, size_t maxlen, size_t *lencopied)
{
	const char *from = kfaddr;
	char *to = kdaddr;
	int i;

	for (i = 0; i < maxlen; i++) {
		if ((*to++ = *from++) == NULL) {
			if (lencopied)
				*lencopied = i + 1;
			return (0);
		}
	}

	if (lencopied)
		*lencopied = i;
	return (ENAMETOOLONG);
}

/*
 * fuword(caddr_t uaddr);
 * Fetch an int from the user's address space.
 */
long
fuword(const void *base)
{
	const char *uaddr = base;
	long rc;

	if (uaddr > (char *)VM_MAXUSER_ADDRESS - sizeof(long))
		return -1;

	curpcb->pcb_onfault = &&Err999;

	rc = *(long *)uaddr;

	curpcb->pcb_onfault = 0;
	return rc;

 Err999:
	curpcb->pcb_onfault = 0;
	return -1;
}

/*
 * fusword(caddr_t uaddr);
 * Fetch a short from the user's address space.
 */
int
fusword(const void *base)
{
	const char *uaddr = base;
	int rc;

	if (uaddr > (char *)VM_MAXUSER_ADDRESS - sizeof(short))
		return -1;

	curpcb->pcb_onfault = &&Err999;

	rc = *(unsigned short *)uaddr;

	curpcb->pcb_onfault = 0;
	return rc;

 Err999:
	curpcb->pcb_onfault = 0;
	return -1;
}

/*
 * fuswintr(caddr_t uaddr);
 * Fetch a short from the user's address space.  Can be called during an
 * interrupt.
 */
int
fuswintr(const void *base)
{
	const char *uaddr = base;
	int rc;

	if (uaddr > (char *)VM_MAXUSER_ADDRESS - sizeof(short))
		return -1;

	curpcb->pcb_onfault = &&Err999;
	curpcb->fusubail = 1;

	rc = *(unsigned short *)uaddr;

	curpcb->pcb_onfault = 0;
	curpcb->fusubail = 0;
	return rc;

 Err999:
	curpcb->pcb_onfault = 0;
	curpcb->fusubail = 0;
	return -1;
}

/*
 * fubyte(caddr_t uaddr);
 * Fetch a byte from the user's address space.
 */
int
fubyte(const void *base)
{
	const char *uaddr = base;
	int rc;

	if (uaddr > (char *)VM_MAXUSER_ADDRESS - sizeof(char))
		return -1;

	curpcb->pcb_onfault = &&Err999;

	rc = *(unsigned char *)uaddr;

	curpcb->pcb_onfault = 0;
	return rc;

 Err999:
	curpcb->pcb_onfault = 0;
	return -1;
}

/*
 * suword(caddr_t uaddr, int x);
 * Store an int in the user's address space.
 */
int
suword(void *base, long x)
{
	char *uaddr = base;

	if (uaddr > (char *)VM_MAXUSER_ADDRESS - sizeof(long))
		return -1;

	curpcb->pcb_onfault = &&Err999;

	*(int *)uaddr = x;

	curpcb->pcb_onfault = 0;
	return 0;

 Err999:
	curpcb->pcb_onfault = 0;
	return -1;
}

/*
 * susword(void *uaddr, short x);
 * Store a short in the user's address space.
 */
int
susword(void *base, short x)
{
	char *uaddr = base;

	if (uaddr > (char *)VM_MAXUSER_ADDRESS - sizeof(short))
		return -1;

	curpcb->pcb_onfault = &&Err999;

	*(short *)uaddr = x;

	curpcb->pcb_onfault = 0;
	return 0;

 Err999:
	curpcb->pcb_onfault = 0;
	return -1;
}

/*
 * suswintr(caddr_t uaddr, short x);
 * Store a short in the user's address space.  Can be called during an
 * interrupt.
 */
int
suswintr(void *base, short x)
{
	char *uaddr = base;

	if (uaddr > (char *)VM_MAXUSER_ADDRESS - sizeof(short))
		return -1;

	curpcb->pcb_onfault = &&Err999;
	curpcb->fusubail = 1;

	*(short *)uaddr = x;

	curpcb->pcb_onfault = 0;
	curpcb->fusubail = 0;
	return 0;

 Err999:
	curpcb->pcb_onfault = 0;
	curpcb->fusubail = 0;
	return -1;
}

/*
 * subyte(caddr_t uaddr, char x);
 * Store a byte in the user's address space.
 */
int
subyte(void *base, int x)
{
	char *uaddr = base;

	if (uaddr > (char *)VM_MAXUSER_ADDRESS - sizeof(char))
		return -1;

	curpcb->pcb_onfault = &&Err999;

	*(char *)uaddr = x;

	curpcb->pcb_onfault = 0;
	return 0;

 Err999:
	curpcb->pcb_onfault = 0;
	return -1;
}

int
kcopy(const void *src, void *dst, size_t len)
{
	caddr_t oldfault;

	oldfault = curpcb->pcb_onfault;

	curpcb->pcb_onfault = &&Err999;
	memcpy(dst, src, len);
	curpcb->pcb_onfault = oldfault;

	return 0;

 Err999:
	curpcb->pcb_onfault = oldfault;

	return EFAULT;
}
