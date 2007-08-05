/*	$NetBSD: cpu_data.h,v 1.11.2.2 2007/08/05 13:57:26 ad Exp $	*/

/*-
 * Copyright (c) 2004, 2006, 2007 The NetBSD Foundation, Inc.
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

/*
 * based on arch/i386/include/cpu.h:
 *	NetBSD: cpu.h,v 1.115 2004/05/16 12:32:53 yamt Exp
 */

#ifndef _SYS_CPU_DATA_H_
#define	_SYS_CPU_DATA_H_

struct callout;
struct lwp;
#include <sys/sched.h>	/* for schedstate_percpu */

/*
 * MI per-cpu data
 *
 * this structure is intended to be included in MD cpu_info structure.
 *	struct cpu_info {
 *		struct cpu_data ci_data;
 *	}
 *
 * note that cpu_data is not expected to contain much data,
 * as cpu_info is size-limited on most ports.
 */

struct cpu_data {
	struct schedstate_percpu cpu_schedstate; /* scheduler state */
	struct lwp	*cpu_idlelwp;		/* idle lwp */
	u_int		cpu_index;		/* CPU index */
	u_int		cpu_biglock_count;
	struct lwp	*cpu_biglock_wanted;
	/* For LOCKDEBUG. */
	u_int		cpu_spin_locks;		/* # of spinlockmgr locks */
	u_int		cpu_simple_locks;	/* # of simple locks held */
	void		*cpu_lockstat;		/* lockstat private tables */
	u_int		cpu_spin_locks2;	/* # of spin locks held XXX */
	u_int		cpu_lkdebug_recurse;	/* LOCKDEBUG recursion */
	void		*cpu_softcpu;		/* soft interrupt table */
	TAILQ_HEAD(,buf) cpu_biodone;		/* finished block xfers */
	u_int		cpu_netisrs;		/* legacy netisrs XXX */
	u_int		cpu_callout_nwait;	/* # LWPs waiting on callout */
	void		*cpu_callout;		/* running callout */
	void		*cpu_callout_cancel;	/* callout to be cancelled */
	
};

/* compat definitions */
#define	ci_schedstate		ci_data.cpu_schedstate
#define	ci_index		ci_data.cpu_index
#define	ci_biglock_count	ci_data.cpu_biglock_count
#define	ci_biglock_wanted	ci_data.cpu_biglock_wanted
#define	ci_spin_locks		ci_data.cpu_spin_locks
#define	ci_simple_locks		ci_data.cpu_simple_locks
#define	ci_lockstat		ci_data.cpu_lockstat
#define	ci_spin_locks2		ci_data.cpu_spin_locks2
#define	ci_lkdebug_recurse	ci_data.cpu_lkdebug_recurse

int mi_cpu_attach(struct cpu_info *ci);

#endif /* _SYS_CPU_DATA_H_ */
