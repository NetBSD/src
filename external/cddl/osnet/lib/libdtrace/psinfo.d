/*	$NetBSD: psinfo.d,v 1.5.14.1 2018/06/25 07:25:24 pgoyette Exp $	*/

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 *
 * Portions Copyright 2006 John Birrell jb@freebsd.org
 *
 * $FreeBSD: head/cddl/lib/libdtrace/psinfo.d 304825 2016-08-25 23:24:57Z gnn $
 */
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

typedef struct psinfo {
	int	pr_nlwp;	/* number of threads */
	pid_t	pr_pid;		/* unique process id */
	pid_t	pr_ppid;	/* process id of parent */
	pid_t	pr_pgid;	/* pid of process group leader */
	pid_t	pr_sid;		/* session id */
	uid_t	pr_uid;		/* real user id */
	uid_t	pr_euid;	/* effective user id */
	gid_t	pr_gid;		/* real group id */
	gid_t	pr_egid;	/* effective group id */
	uintptr_t
		pr_addr;	/* address of process */
	string	pr_psargs;	/* process arguments */
	u_int	pr_arglen;	/* process argument length */
} psinfo_t;

#pragma D binding "1.0" translator
translator psinfo_t < struct proc *T > {
	pr_nlwp = T->p_nlwps;
	pr_pid = T->p_pid;
	pr_ppid = (T->p_pptr == 0) ? 0 : T->p_pptr->p_pid;
	pr_pgid = (T->p_pgrp->pg_session->s_leader == 0) ? 0 : T->p_pgrp->pg_session->s_leader->p_pid;
	pr_sid = (T->p_pgrp == 0) ? 0 : ((T->p_pgrp->pg_session == 0) ? 0 : T->p_pgrp->pg_session->s_sid);
	pr_uid = T->p_cred->cr_uid;
	pr_euid = T->p_cred->cr_euid;
	pr_gid = T->p_cred->cr_gid;
	pr_egid = T->p_cred->cr_egid;
	pr_addr = 0;
	pr_psargs = stringof(T->p_comm);
	pr_arglen = strlen(T->p_comm);
};

typedef struct lwpsinfo {
	id_t	pr_lwpid;	/* thread ID. */
	int	pr_flag;	/* thread flags. */
	int	pr_pri;		/* thread priority. */
	char	pr_state;	/* numeric lwp state */
	char	pr_sname;	/* printable character for pr_state */
	short	pr_syscall;	/* system call number (if in syscall) */
	uintptr_t
		pr_addr;	/* internal address of lwp */
	uintptr_t
		pr_wchan;	/* sleep address */
} lwpsinfo_t;

#pragma D binding "1.0" translator
translator lwpsinfo_t < struct lwp *T > {
	pr_lwpid = T->l_lid;
	pr_pri = T->l_priority;
	pr_flag = T->l_flag;
	pr_state = T->l_stat;
	pr_sname = '?';	/* XXX */
	pr_syscall = 0; /* XXX */
	pr_addr = (uintptr_t)T;
	pr_wchan = (uintptr_t)T->l_wchan;
};

inline psinfo_t *curpsinfo = xlate <psinfo_t *> (curthread->l_proc);
#pragma D attributes Stable/Stable/Common curpsinfo
#pragma D binding "1.0" curpsinfo

inline lwpsinfo_t *curlwpsinfo = xlate <lwpsinfo_t *> (curthread);
#pragma D attributes Stable/Stable/Common curlwpsinfo
#pragma D binding "1.0" curlwpsinfo
