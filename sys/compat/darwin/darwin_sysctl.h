/*	$NetBSD: darwin_sysctl.h,v 1.5 2003/09/06 23:54:47 manu Exp $ */

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

#ifndef	_DARWIN_SYSCTL_H_
#define	_DARWIN_SYSCTL_H_

#include <machine/darwin_machdep.h>

extern pid_t darwin_init_pid;

#define EMUL_DARWIN_INIT_PID	1
#define EMUL_DARWIN_MAXID	2

#define EMUL_DARWIN_NAMES { \
	{ 0, 0 }, \
	{ "init_pid", CTLTYPE_INT }, \
}

int darwin_sysctl(int *, u_int, void *, 
    size_t *, void *, size_t, struct proc *);

#define DARWIN_CTL_UNSPEC	0
#define DARWIN_CTL_KERN		1
#define DARWIN_CTL_VM		2
#define DARWIN_CTL_VFS		3
#define DARWIN_CTL_NET		4
#define DARWIN_CTL_DEBUG	5
#define DARWIN_CTL_HW		6
#define DARWIN_CTL_MACHDEP	7
#define DARWIN_CTL_USER		8
#define DARWIN_CTL_MAXID	9

#define DARWIN_KERN_OSTYPE		1
#define DARWIN_KERN_OSRELEASE		2
#define DARWIN_KERN_OSREV		3
#define DARWIN_KERN_VERSION		4
#define DARWIN_KERN_MAXVNODES		5
#define DARWIN_KERN_MAXPROC		6
#define DARWIN_KERN_MAXFILES		7
#define DARWIN_KERN_ARGMAX		8
#define DARWIN_KERN_SECURELVL		9
#define DARWIN_KERN_HOSTNAME		10
#define DARWIN_KERN_HOSTID		11
#define DARWIN_KERN_CLOCKRATE		12
#define DARWIN_KERN_VNODE		13
#define DARWIN_KERN_PROC		14
#define DARWIN_KERN_FILE		15
#define DARWIN_KERN_PROF		16
#define DARWIN_KERN_POSIX1		17
#define DARWIN_KERN_NGROUPS		18
#define DARWIN_KERN_JOB_CONTROL	19
#define DARWIN_KERN_SAVED_IDS		20
#define DARWIN_KERN_BOOTTIME		21
#define DARWIN_KERN_NISDOMAINNAME	22
#define DARWIN_KERN_MAXPARTITIONS	23
#define DARWIN_KERN_KDEBUG		24
#define DARWIN_KERN_UPDATEINTERVAL	25
#define DARWIN_KERN_OSRELDATE		26
#define DARWIN_KERN_NTP_PLL		27
#define DARWIN_KERN_BOOTFILE		28
#define DARWIN_KERN_MAXFILESPERPROC	29
#define DARWIN_KERN_MAXPROCPERUID	30
#define DARWIN_KERN_DUMPDEV		31
#define DARWIN_KERN_IPC		32
#define DARWIN_KERN_DUMMY		33
#define DARWIN_KERN_PS_STRINGS		34
#define DARWIN_KERN_USRSTACK		35
#define DARWIN_KERN_LOGSIGEXIT		36
#define DARWIN_KERN_SYMFILE		37
#define DARWIN_KERN_PROCARGS		38
#define DARWIN_KERN_PCSAMPLES		39
#define DARWIN_KERN_NETBOOT		40
#define DARWIN_KERN_PANICINFO		41
#define DARWIN_KERN_SYSV		42
#define DARWIN_KERN_MAXID		43

#define DARWIN_KERN_KDEFLAGS		1
#define DARWIN_KERN_KDDFLAGS		2
#define DARWIN_KERN_KDENABLE		3
#define DARWIN_KERN_KDSETBUF		4
#define DARWIN_KERN_KDGETBUF		5
#define DARWIN_KERN_KDSETUP		6
#define DARWIN_KERN_KDREMOVE		7
#define DARWIN_KERN_KDSETREG		8
#define DARWIN_KERN_KDGETREG		9
#define DARWIN_KERN_KDREADTR		10
#define DARWIN_KERN_KDPIDTR		11
#define DARWIN_KERN_KDTHRMAP		12
#define DARWIN_KERN_KDPIDEX		14
#define DARWIN_KERN_KDSETRTCDEC	15
#define DARWIN_KERN_KDGETENTROPY	16

#define DARWIN_KERN_PCDISABLE		1
#define DARWIN_KERN_PCSETBUF		2
#define DARWIN_KERN_PCGETBUF		3
#define DARWIN_KERN_PCSETUP		4
#define DARWIN_KERN_PCREMOVE		5
#define DARWIN_KERN_PCREADBUF		6
#define DARWIN_KERN_PCSETREG		7
#define DARWIN_KERN_PCCOMM		8

#define DARWIN_KERN_PANICINFO_MAXSIZE		1
#define DARWIN_KERN_PANICINFO_IMAGE16		2
#define DARWIN_KERN_PANICINFO_IMAGE32		3

#define DARWIN_KSYSV_SHMMAX		1
#define DARWIN_KSYSV_SHMMIN		2
#define DARWIN_KSYSV_SHMMNI		3
#define DARWIN_KSYSV_SHMSEG		4
#define DARWIN_KSYSV_SHMALL		5

#define DARWIN_KERN_PROC_ALL		0
#define DARWIN_KERN_PROC_PID		1
#define DARWIN_KERN_PROC_PGRP		2
#define DARWIN_KERN_PROC_SESSION	3
#define DARWIN_KERN_PROC_TTY		4
#define DARWIN_KERN_PROC_UID		5
#define DARWIN_KERN_PROC_RUID		6

#define DARWIN_KIPC_MAXSOCKBUF		1
#define DARWIN_KIPC_SOCKBUF_WASTE	2
#define DARWIN_KIPC_SOMAXCONN		3
#define DARWIN_KIPC_MAX_LINKHDR	4
#define DARWIN_KIPC_MAX_PROTOHDR	5
#define DARWIN_KIPC_MAX_HDR		6
#define DARWIN_KIPC_MAX_DATALEN	7
#define DARWIN_KIPC_MBSTAT		8
#define DARWIN_KIPC_NMBCLUSTERS	9

#define DARWIN_VM_METER		1
#define DARWIN_VM_LOADAVG		2
#define DARWIN_VM_MAXID		3
#define DARWIN_VM_MACHFACTOR		4

#define DARWIN_HW_MACHINE		1
#define DARWIN_HW_MODEL		2
#define DARWIN_HW_NCPU			3
#define DARWIN_HW_BYTEORDER		4
#define DARWIN_HW_PHYSMEM		5
#define DARWIN_HW_USERMEM		6
#define DARWIN_HW_PAGESIZE		7
#define DARWIN_HW_DISKNAMES		8
#define DARWIN_HW_DISKSTATS		9
#define DARWIN_HW_EPOCH		10
#define DARWIN_HW_FLOATINGPT		11
#define DARWIN_HW_MACHINE_ARCH		12
#define DARWIN_HW_VECTORUNIT		13
#define DARWIN_HW_BUS_FREQ		14
#define DARWIN_HW_CPU_FREQ		15
#define DARWIN_HW_CACHELINE		16
#define DARWIN_HW_L1ICACHESIZE		17
#define DARWIN_HW_L1DCACHESIZE		18
#define DARWIN_HW_L2SETTINGS		19
#define DARWIN_HW_L2CACHESIZE		20
#define DARWIN_HW_L3SETTINGS		21
#define DARWIN_HW_L3CACHESIZE		22
#define DARWIN_HW_MAXID		23

#define DARWIN_USER_CS_PATH		1
#define DARWIN_USER_BC_BASE_MAX	2
#define DARWIN_USER_BC_DIM_MAX		3
#define DARWIN_USER_BC_SCALE_MAX	4
#define DARWIN_USER_BC_STRING_MAX	5
#define DARWIN_USER_COLL_WEIGHTS_MAX	6
#define DARWIN_USER_EXPR_NEST_MAX	7
#define DARWIN_USER_LINE_MAX		8
#define DARWIN_USER_RE_DUP_MAX		9
#define DARWIN_USER_POSIX2_VERSION	10
#define DARWIN_USER_POSIX2_C_BIND	11
#define DARWIN_USER_POSIX2_C_DEV	12
#define DARWIN_USER_POSIX2_CHAR_TERM	13
#define DARWIN_USER_POSIX2_FORT_DEV	14
#define DARWIN_USER_POSIX2_FORT_RUN	15
#define DARWIN_USER_POSIX2_LOCALEDEF	16
#define DARWIN_USER_POSIX2_SW_DEV	17
#define DARWIN_USER_POSIX2_UPE		18
#define DARWIN_USER_STREAM_MAX		19
#define DARWIN_USER_TZNAME_MAX		20
#define DARWIN_USER_MAXID		21

#define DARWIN_CTL_DEBUG_NAME		0
#define DARWIN_CTL_DEBUG_VALUE		1
#define DARWIN_CTL_DEBUG_MAXID		20


/* For KERN_PROC */
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

#endif /* _DARWIN_SYSCTL_H_ */
