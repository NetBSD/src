/* 	$Id: lwp.h,v 1.1.2.12 2002/03/28 22:07:57 ragge Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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

#ifndef _SYS_LWP_H
#define _SYS_LWP_H


#if defined(_KERNEL)
#include <machine/cpu.h>		/* curcpu() and cpu_info */
#endif
#include <machine/proc.h>		/* Machine-dependent proc substruct. */
#include <sys/queue.h>
#include <sys/callout.h>
#include <sys/ucontext.h>

struct	lwp {
	struct	lwp *l_forw;		/* Doubly-linked run/sleep queue. */
	struct	lwp *l_back;
	LIST_ENTRY(lwp) l_list;		/* Entry on list of all LWPs. */
	LIST_ENTRY(lwp) l_zlist;	/* Entry on zombie list.  */

	struct proc *l_proc;	/* Process which with we are associated. */

	LIST_ENTRY(lwp) l_sibling;	/* Entry on process's list of LWPs. */

	int		l_flag;
	struct cpu_info * __volatile l_cpu; /* CPU we're running on if
					       SONPROC */
	int	l_stat;
	lwpid_t	l_lid;		/* LWP identifier; local to process. */

#define l_startzero l_swtime
	u_int	l_swtime;	/* Time swapped in or out. */
	u_int	l_slptime;	/* Time since last blocked. */
    
	void	*l_wchan;	/* Sleep address. */
	struct callout l_tsleep_ch;	/* callout for tsleep */
	const char *l_wmesg;	/* Reason for sleep. */
	int	l_holdcnt;	/* If non-zero, don't swap. */

#define l_endzero l_priority

#define l_startcopy l_priority

	void	*l_ctxlink;	/* uc_link {get,set}context */

	u_char	l_priority;	/* Process priority. */
	u_char	l_usrpri;	/* User-priority based on p_cpu and p_nice. */

#define l_endcopy l_private

	void	*l_private;	/* svr4-style lwp-private data */

	int	l_locks;       	/* DEBUG: lockmgr count of held locks */

	struct	user *l_addr;	/* Kernel virtual addr of u-area (PROC ONLY). */
	struct	mdlwp l_md;	/* Any machine-dependent fields. */

};

LIST_HEAD(lwplist, lwp);		/* a list of LWPs */

extern struct lwplist alllwp;		/* List of all LWPs. */
extern struct lwplist deadlwp;		/* */
extern struct lwplist zomblwp;

extern struct pool lwp_pool;		/* memory pool for LWPs */
extern struct pool lwp_uc_pool;		/* memory pool for LWP startup args */

extern struct lwp lwp0;			/* LWP for proc0 */

/* These flags are kept in l_flag. */
#define	L_INMEM		0x00004	/* Loaded into memory. */
#define	L_SELECT	0x00040	/* Selecting; wakeup/waiting danger. */
#define	L_SINTR		0x00080	/* Sleep is interruptible. */
#define	L_TIMEOUT	0x00400	/* Timing out during sleep. */
#define	L_DETACHED	0x00800 /* Won't be waited for. */
#define	L_BIGLOCK	0x80000	/* LWP needs kernel "big lock" to run */
#define	L_SA		0x100000 /* Scheduler activations LWP */
#define	L_SA_UPCALL	0x200000 /* SA upcall is pending */
#define	L_SA_BLOCKING	0x400000 /* Blocking in tsleep() */

/*
 * Status values.
 *
 * A note about SRUN and SONPROC: SRUN indicates that a process is
 * runnable but *not* yet running, i.e. is on a run queue.  SONPROC
 * indicates that the process is actually executing on a CPU, i.e.
 * it is no longer on a run queue.
 */
#define	LSIDL	1		/* Process being created by fork. */
#define	LSRUN	2		/* Currently runnable. */
#define	LSSLEEP	3		/* Sleeping on an address. */
#define	LSSTOP	4		/* Process debugging or suspension. */
#define	LSZOMB	5		/* Awaiting collection by parent. */
#define	LSDEAD	6		/* Process is almost a zombie. */
#define	LSONPROC	7	/* Process is currently on a CPU. */
#define	LSSUSPENDED	8	/* Not running, not signalable. */

#ifdef _KERNEL
#define	PHOLD(l)							\
do {									\
	if ((l)->l_holdcnt++ == 0 && ((l)->l_flag & L_INMEM) == 0)	\
		uvm_swapin(l);						\
} while (/* CONSTCOND */ 0)
#define	PRELE(l)	(--(l)->l_holdcnt)


void	preempt (struct lwp *);
int	mi_switch (struct lwp *, struct lwp *);
#ifndef remrunqueue
void	remrunqueue (struct lwp *);
#endif
void	resetpriority (struct lwp *);
void	setrunnable (struct lwp *);
#ifndef setrunqueue
void	setrunqueue (struct lwp *);
#endif
void	unsleep (struct lwp *);
#ifndef cpu_switch
int	cpu_switch (struct lwp *);
#endif
void	cpu_preempt (struct lwp *, struct lwp *);

int newlwp(struct lwp *, struct proc *, vaddr_t, int,
    void *, size_t, void (*)(void *), void *, struct lwp **);

/* Flags for _lwp_wait1 */
#define LWPWAIT_EXITCONTROL	0x00000001
int 	lwp_wait1(struct lwp *, lwpid_t, lwpid_t *, int);
void	cpu_fiddle_uc(struct lwp *, ucontext_t *);
void	cpu_setfunc(struct lwp *, void (*)(void *), void *);
void	startlwp(void *);
void	upcallret(struct lwp *);

void	lwp_exit (struct lwp *);
void	lwp_exit2 (struct lwp *);
struct lwp *proc_representative_lwp(struct proc *);
#endif	/* _KERNEL */

/* Flags for _lwp_create(), as per Solaris. */

#define LWP_DETACHED    0x00000040
#define LWP_SUSPENDED   0x00000080
#define __LWP_ASLWP     0x00000100 /* XXX more icky signal semantics */

#endif	/* !_SYS_LWP_H_ */

