/*	$NetBSD: darwin_proc.h,v 1.1 2003/09/07 08:05:47 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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

#ifndef	_DARWIN_PROC_H_
#define	_DARWIN_PROC_H_

#include <machine/darwin_machdep.h>

struct darwin_ucred {
	u_long	cr_ref;
	uid_t	cr_uid;
	short	cr_ngroups;
#define DARWIN_NGROUPS 16
	gid_t	cr_groups[DARWIN_NGROUPS];
};

struct darwin_pcred {
	struct  darwin_lock__bsd__ {
		struct darwin_slock lk_interlock;
		u_int	lk_flags;
		int	lk_sharecount;
		int	lk_waitcount;
		short	lk_exclusivecount;
#undef lk_prio /* Defined in <sys/lock.h> */
		short	lk_prio;
		char	*lk_wmesg;
#undef lk_timo /* Defined in <sys/lock.h> */
		int	lk_timo;
#undef lk_lockholder /* Defined in <sys/lock.h> */
		pid_t	lk_lockholder;
		void	*lk_lockthread;
	} pc_lock;
	struct  darwin_ucred *pc_ucred;
	uid_t   pc_ruid;
	uid_t   pc_svuid;
	gid_t   pc_rgid;
	gid_t   pc_svgid;
	int     pc_refcnt;
}; 

struct darwin_vmspace {
	int     vm_refcnt;
	caddr_t vm_shm;
	segsz_t vm_rssize;
	segsz_t vm_swrss;
	segsz_t vm_tsize;
	segsz_t vm_dsize;
	segsz_t vm_ssize;
	caddr_t vm_taddr;
	caddr_t vm_daddr;
	caddr_t vm_maxsaddr;
};

struct darwin_extern_proc {
	union {
		struct {
			struct darwin_proc *__p_forw;
			struct darwin_proc *__p_back;
		} p_st1;
		struct timeval __p_starttime;
	} p_un;
	struct darwin_vmspace *p_vmspace;
	struct darwin_sigacts *p_sigacts;
	int	p_flag;
	char	p_stat;
	pid_t	p_pid;
	pid_t	p_oppid;
	int	p_dupfd;
	caddr_t user_stack;
	void	*exit_thread;
	int	p_debugger;
	mach_boolean_t	sigwait;
	u_int	p_estcpu;
	int	p_cpticks;
	fixpt_t p_pctcpu;
	void	*p_wchan;
	char	*p_wmesg;
	u_int	p_swtime;
	u_int	p_slptime;
	struct	itimerval p_realtimer;
	struct	timeval p_rtime;
	u_quad_t p_uticks;
	u_quad_t p_sticks;
	u_quad_t p_iticks;
	int	p_traceflag;
	struct darwin_vnode *p_tracep;
	int	p_siglist;
	struct darwin_vnode *p_textvp;
	int	p_holdcnt;
	sigset13_t p_sigmask;
	sigset13_t p_sigignore;
	sigset13_t p_sigcatch;
	u_char	p_priority;
	u_char	p_usrpri;
	char	p_nice;	
#define DARWIN_MAXCOMLEN 16 
	char	p_comm[DARWIN_MAXCOMLEN + 1];
	struct	darwin_pgrp *p_pgrp;
	struct	darwin_user *p_addr;
	u_short p_xstat;
	u_short p_acflag;
	struct	darwin_rusage *p_ru;
};

struct darwin_kinfo_proc {
	/* 
	 * kp_proc is a struct darwin_extern_proc.
	 * Declaring struct darwin_extern_proc here causes an allignement
	 * on a word boundary. We replace it by a char array to avoid that.
	 */ 
	char kp_proc[196];
	struct	darwin_eproc {
		struct	darwin_proc *e_paddr;
		struct	darwin_session *e_sess;
		struct	darwin_pcred e_pcred;
		struct	darwin_ucred e_ucred;
		struct	darwin_vmspace e_vm;
		pid_t	e_ppid;
		pid_t	e_pgid;
		short	e_jobc;
		dev_t	e_tdev;
		pid_t	e_tpgid;
		struct	darwin_session *e_tsess;
#define DARWIN_WMESGLEN 7
		char	e_wmesg[DARWIN_WMESGLEN + 1];
		segsz_t e_xsize;
		short	e_xrssize;
		short	e_xccount;
		short	e_xswrss;
		long	e_flag;
#define DARWIN_EPROC_CTTY	0x01
#define DARWIN_EPROC_SLEADER	0x02
#define DARWIN_COMAPT_MAXLOGNAME	12
		char	e_login[DARWIN_COMAPT_MAXLOGNAME];
		long	e_spare[4];
	} kp_eproc;
};

/* p_flag for Darwin */
#define DARWIN_P_ADVLOCK       0x00001
#define DARWIN_P_CONTROLT      0x00002
#define DARWIN_P_INMEM         0x00004
#define DARWIN_P_NOCLDSTOP     0x00008
#define DARWIN_P_PPWAIT        0x00010
#define DARWIN_P_PROFIL        0x00020
#define DARWIN_P_SELECT        0x00040
#define DARWIN_P_SINTR         0x00080
#define DARWIN_P_SUGID         0x00100
#define DARWIN_P_SYSTEM        0x00200
#define DARWIN_P_TIMEOUT       0x00400
#define DARWIN_P_TRACED        0x00800
#define DARWIN_P_WAITED        0x01000
#define DARWIN_P_WEXIT         0x02000
#define DARWIN_P_EXEC          0x04000
#define DARWIN_P_OWEUPC        0x08000
#define DARWIN_P_FSTRACE       0x10000
#define DARWIN_P_SSTEP         0x20000
#define DARWIN_P_WAITING       0x0040000
#define DARWIN_P_KDEBUG        0x0080000
#define DARWIN_P_TTYSLEEP      0x0100000
#define DARWIN_P_REBOOT        0x0200000
#define DARWIN_P_TBE           0x0400000
#define DARWIN_P_SIGEXC        0x0800000
#define DARWIN_P_BTRACE        0x1000000
#define DARWIN_P_VFORK         0x2000000
#define DARWIN_P_NOATTACH      0x4000000
#define DARWIN_P_INVFORK       0x8000000
#define DARWIN_P_NOSHLIB       0x10000000
#define DARWIN_P_FORCEQUOTA    0x20000000
#define DARWIN_P_NOCLDWAIT     0x40000000

#endif /* _DARWIN_PROC_H_ */
