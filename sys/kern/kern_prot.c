/*	$NetBSD: kern_prot.c,v 1.40 1997/04/23 18:59:58 mycroft Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1990, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)kern_prot.c	8.6 (Berkeley) 1/21/94
 */

/*
 * System calls related to processes and protection
 */

#include <sys/param.h>
#include <sys/acct.h>
#include <sys/systm.h>
#include <sys/ucred.h>
#include <sys/proc.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/malloc.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

/* ARGSUSED */
int
sys_getpid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{

	*retval = p->p_pid;
#if defined(COMPAT_43) || defined(COMPAT_SUNOS) || defined(COMPAT_IBCS2) || \
    defined(COMPAT_FREEBSD)
	retval[1] = p->p_pptr->p_pid;
#endif
	return (0);
}

/* ARGSUSED */
int
sys_getppid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{

	*retval = p->p_pptr->p_pid;
	return (0);
}

/* Get process group ID; note that POSIX getpgrp takes no parameter */
int
sys_getpgrp(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{

	*retval = p->p_pgrp->pg_id;
	return (0);
}

int
sys_getpgid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	register struct sys_getpgid_args /* {
		syscallarg(pid_t) pid;
	} */ *uap = v;

	if (SCARG(uap, pid) == 0)
		goto found;
	if ((p = pfind(SCARG(uap, pid))) == 0)
		return (ESRCH);
found:
	*retval = p->p_pgid;
	return 0;
}

/* ARGSUSED */
int
sys_getuid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{

	*retval = p->p_cred->p_ruid;
#if defined(COMPAT_43) || defined(COMPAT_SUNOS) || defined(COMPAT_IBCS2) || \
    defined(COMPAT_FREEBSD)
	retval[1] = p->p_ucred->cr_uid;
#endif
	return (0);
}

/* ARGSUSED */
int
sys_geteuid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{

	*retval = p->p_ucred->cr_uid;
	return (0);
}

/* ARGSUSED */
int
sys_getgid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{

	*retval = p->p_cred->p_rgid;
#if defined(COMPAT_43) || defined(COMPAT_SUNOS) || defined(COMPAT_FREEBSD)
	retval[1] = p->p_ucred->cr_gid;
#endif
	return (0);
}

/*
 * Get effective group ID.  The "egid" is groups[0], and could be obtained
 * via getgroups.  This syscall exists because it is somewhat painful to do
 * correctly in a library function.
 */
/* ARGSUSED */
int
sys_getegid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{

	*retval = p->p_ucred->cr_gid;
	return (0);
}

int
sys_getgroups(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	register struct sys_getgroups_args /* {
		syscallarg(u_int) gidsetsize;
		syscallarg(gid_t *) gidset;
	} */ *uap = v;
	register struct pcred *pc = p->p_cred;
	register u_int ngrp;
	int error;

	if ((ngrp = SCARG(uap, gidsetsize)) == 0) {
		*retval = pc->pc_ucred->cr_ngroups;
		return (0);
	}
	if (ngrp < pc->pc_ucred->cr_ngroups)
		return (EINVAL);
	ngrp = pc->pc_ucred->cr_ngroups;
	error = copyout((caddr_t)pc->pc_ucred->cr_groups,
			(caddr_t)SCARG(uap, gidset), ngrp * sizeof(gid_t));
	if (error)
		return (error);
	*retval = ngrp;
	return (0);
}

/* ARGSUSED */
int
sys_setsid(p, v, retval)
	register struct proc *p;
	void *v;
	register_t *retval;
{

	if (p->p_pgid == p->p_pid || pgfind(p->p_pid)) {
		return (EPERM);
	} else {
		(void)enterpgrp(p, p->p_pid, 1);
		*retval = p->p_pid;
		return (0);
	}
}

/*
 * set process group (setpgid/old setpgrp)
 *
 * caller does setpgid(targpid, targpgid)
 *
 * pgid must be in valid range (EINVAL)
 * pid must be caller or child of caller (ESRCH)
 * if a child
 *	pid must be in same session (EPERM)
 *	pid can't have done an exec (EACCES)
 * if pgid != pid
 * 	there must exist some pid in same session having pgid (EPERM)
 * pid must not be session leader (EPERM)
 */
/* ARGSUSED */
int
sys_setpgid(curp, v, retval)
	struct proc *curp;
	void *v;
	register_t *retval;
{
	register struct sys_setpgid_args /* {
		syscallarg(int) pid;
		syscallarg(int) pgid;
	} */ *uap = v;
	register struct proc *targp;		/* target process */
	register struct pgrp *pgrp;		/* target pgrp */

	if (SCARG(uap, pgid) < 0)
		return (EINVAL);

	if (SCARG(uap, pid) != 0 && SCARG(uap, pid) != curp->p_pid) {
		if ((targp = pfind(SCARG(uap, pid))) == 0 || !inferior(targp))
			return (ESRCH);
		if (targp->p_session != curp->p_session)
			return (EPERM);
		if (targp->p_flag & P_EXEC)
			return (EACCES);
	} else
		targp = curp;
	if (SESS_LEADER(targp))
		return (EPERM);
	if (SCARG(uap, pgid) == 0)
		SCARG(uap, pgid) = targp->p_pid;
	else if (SCARG(uap, pgid) != targp->p_pid)
		if ((pgrp = pgfind(SCARG(uap, pgid))) == 0 ||
	            pgrp->pg_session != curp->p_session)
			return (EPERM);
	return (enterpgrp(targp, SCARG(uap, pgid), 0));
}

/* ARGSUSED */
int
sys_setuid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_setuid_args /* {
		syscallarg(uid_t) uid;
	} */ *uap = v;
	register struct pcred *pc = p->p_cred;
	register uid_t uid;
	int error;

	uid = SCARG(uap, uid);
	if (uid != pc->p_ruid &&
	    (error = suser(pc->pc_ucred, &p->p_acflag)))
		return (error);
	/*
	 * Everything's okay, do it.
	 * Transfer proc count to new user.
	 * Copy credentials so other references do not see our changes.
	 */
	(void)chgproccnt(pc->p_ruid, -1);
	(void)chgproccnt(uid, 1);
	pc->pc_ucred = crcopy(pc->pc_ucred);
	pc->pc_ucred->cr_uid = uid;
	pc->p_ruid = uid;
	pc->p_svuid = uid;
	p->p_flag |= P_SUGID;
	return (0);
}

/* ARGSUSED */
int
sys_seteuid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_seteuid_args /* {
		syscallarg(uid_t) euid;
	} */ *uap = v;
	register struct pcred *pc = p->p_cred;
	register uid_t euid;
	int error;

	euid = SCARG(uap, euid);
	if (euid != pc->p_ruid && euid != pc->p_svuid &&
	    (error = suser(pc->pc_ucred, &p->p_acflag)))
		return (error);
	/*
	 * Everything's okay, do it.  Copy credentials so other references do
	 * not see our changes.
	 */
	pc->pc_ucred = crcopy(pc->pc_ucred);
	pc->pc_ucred->cr_uid = euid;
	p->p_flag |= P_SUGID;
	return (0);
}

int
sys_setreuid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_setreuid_args /* {
		syscallarg(uid_t) ruid;
		syscallarg(uid_t) euid;
	} */ *uap = v;
	register struct pcred *pc = p->p_cred;
	register uid_t ruid, euid;
	int error;

	ruid = SCARG(uap, ruid);
	euid = SCARG(uap, euid);

	if (ruid != (uid_t)-1 &&
	    ruid != pc->p_ruid && ruid != pc->pc_ucred->cr_uid &&
	    (error = suser(pc->pc_ucred, &p->p_acflag)))
		return (error);

	if (euid != (uid_t)-1 &&
	    euid != pc->p_ruid && euid != pc->pc_ucred->cr_uid &&
	    euid != pc->p_svuid &&
	    (error = suser(pc->pc_ucred, &p->p_acflag)))
		return (error);

	if (euid != (uid_t)-1) {
		pc->pc_ucred = crcopy(pc->pc_ucred);
		pc->pc_ucred->cr_uid = euid;
	}

	if (ruid != (uid_t)-1) {
		(void)chgproccnt(pc->p_ruid, -1);
		(void)chgproccnt(ruid, 1);
		pc->p_ruid = ruid;
		pc->p_svuid = pc->pc_ucred->cr_uid;
	}

	if (euid != (uid_t)-1 && ruid != (uid_t)-1)
		p->p_flag |= P_SUGID;
	return (0);
}

/* ARGSUSED */
int
sys_setgid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_setgid_args /* {
		syscallarg(gid_t) gid;
	} */ *uap = v;
	register struct pcred *pc = p->p_cred;
	register gid_t gid;
	int error;

	gid = SCARG(uap, gid);
	if (gid != pc->p_rgid &&
	    (error = suser(pc->pc_ucred, &p->p_acflag)))
		return (error);
	pc->pc_ucred = crcopy(pc->pc_ucred);
	pc->pc_ucred->cr_gid = gid;
	pc->p_rgid = gid;
	pc->p_svgid = gid;
	p->p_flag |= P_SUGID;
	return (0);
}

/* ARGSUSED */
int
sys_setegid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_setegid_args /* {
		syscallarg(gid_t) egid;
	} */ *uap = v;
	register struct pcred *pc = p->p_cred;
	register gid_t egid;
	int error;

	egid = SCARG(uap, egid);
	if (egid != pc->p_rgid && egid != pc->p_svgid &&
	    (error = suser(pc->pc_ucred, &p->p_acflag)))
		return (error);
	pc->pc_ucred = crcopy(pc->pc_ucred);
	pc->pc_ucred->cr_gid = egid;
	p->p_flag |= P_SUGID;
	return (0);
}

int
sys_setregid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_setregid_args /* {
		syscallarg(gid_t) rgid;
		syscallarg(gid_t) egid;
	} */ *uap = v;
	register struct pcred *pc = p->p_cred;
	register gid_t rgid, egid;
	int error;

	rgid = SCARG(uap, rgid);
	egid = SCARG(uap, egid);

	if (rgid != (gid_t)-1 &&
	    rgid != pc->p_rgid && rgid != pc->pc_ucred->cr_gid &&
	    (error = suser(pc->pc_ucred, &p->p_acflag)))
		return (error);

	if (egid != (gid_t)-1 &&
	    egid != pc->p_rgid && egid != pc->pc_ucred->cr_gid &&
	    egid != pc->p_svgid &&
	    (error = suser(pc->pc_ucred, &p->p_acflag)))
		return (error);

	if (egid != (gid_t)-1) {
		pc->pc_ucred = crcopy(pc->pc_ucred);
		pc->pc_ucred->cr_gid = egid;
	}

	if (rgid != (gid_t)-1) {
		pc->p_rgid = rgid;
		pc->p_svgid = pc->pc_ucred->cr_gid;
	}

	if (egid != (gid_t)-1 && rgid != (gid_t)-1)
		p->p_flag |= P_SUGID;
	return (0);
}

/* ARGSUSED */
int
sys_setgroups(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_setgroups_args /* {
		syscallarg(u_int) gidsetsize;
		syscallarg(const gid_t *) gidset;
	} */ *uap = v;
	register struct pcred *pc = p->p_cred;
	register u_int ngrp;
	int error;

	if ((error = suser(pc->pc_ucred, &p->p_acflag)) != 0)
		return (error);
	ngrp = SCARG(uap, gidsetsize);
	if (ngrp > NGROUPS)
		return (EINVAL);
	pc->pc_ucred = crcopy(pc->pc_ucred);
	error = copyin(SCARG(uap, gidset), pc->pc_ucred->cr_groups,
	    ngrp * sizeof(gid_t));
	if (error)
		return (error);
	pc->pc_ucred->cr_ngroups = ngrp;
	p->p_flag |= P_SUGID;
	return (0);
}

/*
 * Check if gid is a member of the group set.
 */
int
groupmember(gid, cred)
	gid_t gid;
	register struct ucred *cred;
{
	register gid_t *gp;
	gid_t *egp;

	egp = &(cred->cr_groups[cred->cr_ngroups]);
	for (gp = cred->cr_groups; gp < egp; gp++)
		if (*gp == gid)
			return (1);
	return (0);
}

/*
 * Test whether the specified credentials imply "super-user"
 * privilege; if so, and we have accounting info, set the flag
 * indicating use of super-powers.
 * Returns 0 or error.
 */
int
suser(cred, acflag)
	struct ucred *cred;
	u_short *acflag;
{
	if (cred->cr_uid == 0) {
		if (acflag)
			*acflag |= ASU;
		return (0);
	}
	return (EPERM);
}

/*
 * Allocate a zeroed cred structure.
 */
struct ucred *
crget()
{
	register struct ucred *cr;

	MALLOC(cr, struct ucred *, sizeof(*cr), M_CRED, M_WAITOK);
	bzero((caddr_t)cr, sizeof(*cr));
	cr->cr_ref = 1;
	return (cr);
}

/*
 * Free a cred structure.
 * Throws away space when ref count gets to 0.
 */
void
crfree(cr)
	struct ucred *cr;
{
	int s;

	s = splimp();				/* ??? */
	if (--cr->cr_ref == 0)
		FREE((caddr_t)cr, M_CRED);
	(void) splx(s);
}

/*
 * Copy cred structure to a new one and free the old one.
 */
struct ucred *
crcopy(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	if (cr->cr_ref == 1)
		return (cr);
	newcr = crget();
	*newcr = *cr;
	crfree(cr);
	newcr->cr_ref = 1;
	return (newcr);
}

/*
 * Dup cred struct to a new held one.
 */
struct ucred *
crdup(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	newcr = crget();
	*newcr = *cr;
	newcr->cr_ref = 1;
	return (newcr);
}

/*
 * Get login name, if available.
 */
/* ARGSUSED */
int
sys___getlogin(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys___getlogin_args /* {
		syscallarg(char *) namebuf;
		syscallarg(u_int) namelen;
	} */ *uap = v;

	if (SCARG(uap, namelen) > sizeof (p->p_pgrp->pg_session->s_login))
		SCARG(uap, namelen) = sizeof (p->p_pgrp->pg_session->s_login);
	return (copyout((caddr_t) p->p_pgrp->pg_session->s_login,
	    (caddr_t) SCARG(uap, namebuf), SCARG(uap, namelen)));
}

/*
 * Set login name.
 */
/* ARGSUSED */
int
sys_setlogin(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_setlogin_args /* {
		syscallarg(const char *) namebuf;
	} */ *uap = v;
	int error;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);
	error = copyinstr(SCARG(uap, namebuf), p->p_pgrp->pg_session->s_login,
	    sizeof (p->p_pgrp->pg_session->s_login) - 1, (size_t *)0);
	if (error == ENAMETOOLONG)
		error = EINVAL;
	return (error);
}
