/*	$NetBSD: kern_prot.c,v 1.82 2004/04/25 16:42:41 simonb Exp $	*/

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
 *	@(#)kern_prot.c	8.9 (Berkeley) 2/14/95
 */

/*
 * System calls related to processes and protection
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_prot.c,v 1.82 2004/04/25 16:42:41 simonb Exp $");

#include "opt_compat_43.h"

#include <sys/param.h>
#include <sys/acct.h>
#include <sys/systm.h>
#include <sys/ucred.h>
#include <sys/proc.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/pool.h>
#include <sys/syslog.h>
#include <sys/resourcevar.h>

#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

POOL_INIT(cred_pool, sizeof(struct ucred), 0, 0, 0, "credpl",
    &pool_allocator_nointr);

int	sys_getpid(struct lwp *, void *, register_t *);
int	sys_getpid_with_ppid(struct lwp *, void *, register_t *);
int	sys_getuid(struct lwp *, void *, register_t *);
int	sys_getuid_with_euid(struct lwp *, void *, register_t *);
int	sys_getgid(struct lwp *, void *, register_t *);
int	sys_getgid_with_egid(struct lwp *, void *, register_t *);

/* ARGSUSED */
int
sys_getpid(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;

	*retval = p->p_pid;
	return (0);
}

/* ARGSUSED */
int
sys_getpid_with_ppid(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;

	retval[0] = p->p_pid;
	retval[1] = p->p_pptr->p_pid;
	return (0);
}

/* ARGSUSED */
int
sys_getppid(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;

	*retval = p->p_pptr->p_pid;
	return (0);
}

/* Get process group ID; note that POSIX getpgrp takes no parameter */
int
sys_getpgrp(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;

	*retval = p->p_pgrp->pg_id;
	return (0);
}

/*
 * Return the process group ID of the session leader (session ID)
 * for the specified process.
 */
int
sys_getsid(struct lwp *l, void *v, register_t *retval)
{
	struct sys_getsid_args /* {
		syscalldarg(pid_t) pid;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	if (SCARG(uap, pid) == 0)
		goto found;
	if ((p = pfind(SCARG(uap, pid))) == 0)
		return (ESRCH);
found:
	*retval = p->p_session->s_sid;
	return (0);
}

int
sys_getpgid(struct lwp *l, void *v, register_t *retval)
{
	struct sys_getpgid_args /* {
		syscallarg(pid_t) pid;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	if (SCARG(uap, pid) == 0)
		goto found;
	if ((p = pfind(SCARG(uap, pid))) == 0)
		return (ESRCH);
found:
	*retval = p->p_pgid;
	return (0);
}

/* ARGSUSED */
int
sys_getuid(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;

	*retval = p->p_cred->p_ruid;
	return (0);
}

/* ARGSUSED */
int
sys_getuid_with_euid(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;

	retval[0] = p->p_cred->p_ruid;
	retval[1] = p->p_ucred->cr_uid;
	return (0);
}

/* ARGSUSED */
int
sys_geteuid(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;

	*retval = p->p_ucred->cr_uid;
	return (0);
}

/* ARGSUSED */
int
sys_getgid(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;

	*retval = p->p_cred->p_rgid;
	return (0);
}

/* ARGSUSED */
int
sys_getgid_with_egid(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;

	retval[0] = p->p_cred->p_rgid;
	retval[1] = p->p_ucred->cr_gid;
	return (0);
}

/*
 * Get effective group ID.  The "egid" is groups[0], and could be obtained
 * via getgroups.  This syscall exists because it is somewhat painful to do
 * correctly in a library function.
 */
/* ARGSUSED */
int
sys_getegid(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;

	*retval = p->p_ucred->cr_gid;
	return (0);
}

int
sys_getgroups(struct lwp *l, void *v, register_t *retval)
{
	struct sys_getgroups_args /* {
		syscallarg(int) gidsetsize;
		syscallarg(gid_t *) gidset;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct pcred *pc = p->p_cred;
	u_int ngrp;
	int error;

	if (SCARG(uap, gidsetsize) == 0) {
		*retval = pc->pc_ucred->cr_ngroups;
		return (0);
	} else if (SCARG(uap, gidsetsize) < 0)
		return (EINVAL);
	ngrp = SCARG(uap, gidsetsize);
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
sys_setsid(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;

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
 *
 * Permission checks now in enterpgrp()
 */
/* ARGSUSED */
int
sys_setpgid(struct lwp *l, void *v, register_t *retval)
{
	struct sys_setpgid_args /* {
		syscallarg(int) pid;
		syscallarg(int) pgid;
	} */ *uap = v;
	struct proc *curp = l->l_proc;
	struct proc *targp;			/* target process */

	if (SCARG(uap, pgid) < 0)
		return EINVAL;

	/* XXX MP - there is a horrid race here with targp exiting! */
	if (SCARG(uap, pid) != 0 && SCARG(uap, pid) != curp->p_pid) {
		targp = pfind(SCARG(uap, pid));
		if (targp == NULL)
			return ESRCH;
	} else
		targp = curp;

	if (SCARG(uap, pgid) == 0)
		SCARG(uap, pgid) = targp->p_pid;
	return enterpgrp(targp, SCARG(uap, pgid), 0);
}

/*
 * Set real, effective and saved uids to the requested values.
 * non-root callers can only ever change uids to values that match
 * one of the processes current uid values.
 * This is further restricted by the flags argument.
 */

int
do_setresuid(struct lwp *l, uid_t r, uid_t e, uid_t sv, u_int flags)
{
	int error;
	struct proc *p = l->l_proc;
	struct pcred *pcred = p->p_cred;
	struct ucred *cred = pcred->pc_ucred;

	/* Superuser can do anything it wants to.... */
	error = suser(cred, &p->p_acflag);
	if (error) {
		/* Otherwise check new value is one of the allowed
		   existing values. */
		if (r != -1 && !((flags & ID_R_EQ_R) && r == pcred->p_ruid)
			    && !((flags & ID_R_EQ_E) && r == cred->cr_uid)
			    && !((flags & ID_R_EQ_S) && r == pcred->p_svuid))
			return error;
		if (e != -1 && !((flags & ID_E_EQ_R) && e == pcred->p_ruid)
			    && !((flags & ID_E_EQ_E) && e == cred->cr_uid)
			    && !((flags & ID_E_EQ_S) && e == pcred->p_svuid))
			return error;
		if (sv != -1 && !((flags & ID_S_EQ_R) && sv == pcred->p_ruid)
			    && !((flags & ID_S_EQ_E) && sv == cred->cr_uid)
			    && !((flags & ID_S_EQ_S) && sv == pcred->p_svuid))
			return error;
	}

	/* If nothing has changed, short circuit the request */
	if ((r == -1 || r == pcred->p_ruid)
	    && (e == -1 || e == cred->cr_uid)
	    && (sv == -1 || sv == pcred->p_svuid))
		/* nothing to do */
		return 0;

	/* The pcred structure is not actually shared... */
	if (r != -1 && r != pcred->p_ruid) {
		/* Update count of processes for this user */
		(void)chgproccnt(pcred->p_ruid, -1);
		(void)chgproccnt(r, 1);
		pcred->p_ruid = r;
	}
	if (sv != -1)
		pcred->p_svuid = sv;
	if (e != -1 && e != cred->cr_uid) {
		/* Update a clone of the current credentials */
		pcred->pc_ucred = cred = crcopy(cred);
		cred->cr_uid = e;
	}

	/* Mark process as having changed credentials, stops tracing etc */
	p_sugid(p);
	return 0;
}

/*
 * Set real, effective and saved gids to the requested values.
 * non-root callers can only ever change gids to values that match
 * one of the processes current gid values.
 * This is further restricted by the flags argument.
 */

int
do_setresgid(struct lwp *l, gid_t r, gid_t e, gid_t sv, u_int flags)
{
	int error;
	struct proc *p = l->l_proc;
	struct pcred *pcred = p->p_cred;
	struct ucred *cred = pcred->pc_ucred;

	/* Superuser can do anything it wants to.... */
	error = suser(cred, &p->p_acflag);
	if (error) {
		/* Otherwise check new value is one of the allowed
		   existing values. */
		if (r != -1 && !((flags & ID_R_EQ_R) && r == pcred->p_rgid)
			    && !((flags & ID_R_EQ_E) && r == cred->cr_gid)
			    && !((flags & ID_R_EQ_S) && r == pcred->p_svgid))
			return error;
		if (e != -1 && !((flags & ID_E_EQ_R) && e == pcred->p_rgid)
			    && !((flags & ID_E_EQ_E) && e == cred->cr_gid)
			    && !((flags & ID_E_EQ_S) && e == pcred->p_svgid))
			return error;
		if (sv != -1 && !((flags & ID_S_EQ_R) && sv == pcred->p_rgid)
			    && !((flags & ID_S_EQ_E) && sv == cred->cr_gid)
			    && !((flags & ID_S_EQ_S) && sv == pcred->p_svgid))
			return error;
	}

	/* If nothing has changed, short circuit the request */
	if ((r == -1 || r == pcred->p_rgid)
	    && (e == -1 || e == cred->cr_gid)
	    && (sv == -1 || sv == pcred->p_svgid))
		/* nothing to do */
		return 0;

	/* The pcred structure is not actually shared... */
	if (r != -1)
		pcred->p_rgid = r;
	if (sv != -1)
		pcred->p_svgid = sv;
	if (e != -1 && e != cred->cr_gid) {
		/* Update a clone of the current credentials */
		pcred->pc_ucred = cred = crcopy(cred);
		cred->cr_gid = e;
	}

	/* Mark process as having changed credentials, stops tracing etc */
	p_sugid(p);
	return 0;
}

/* ARGSUSED */
int
sys_setuid(struct lwp *l, void *v, register_t *retval)
{
	struct sys_setuid_args /* {
		syscallarg(uid_t) uid;
	} */ *uap = v;
	uid_t uid = SCARG(uap, uid);

	return do_setresuid(l, uid, uid, uid,
			    ID_R_EQ_R | ID_E_EQ_R | ID_S_EQ_R);
}

/* ARGSUSED */
int
sys_seteuid(struct lwp *l, void *v, register_t *retval)
{
	struct sys_seteuid_args /* {
		syscallarg(uid_t) euid;
	} */ *uap = v;

	return do_setresuid(l, -1, SCARG(uap, euid), -1, ID_E_EQ_R | ID_E_EQ_S);
}

int
sys_setreuid(struct lwp *l, void *v, register_t *retval)
{
	struct sys_setreuid_args /* {
		syscallarg(uid_t) ruid;
		syscallarg(uid_t) euid;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	uid_t ruid, euid, svuid;

	ruid = SCARG(uap, ruid);
	euid = SCARG(uap, euid);
	if (ruid == -1)
		ruid = p->p_cred->p_ruid;
	if (euid == -1)
		euid = p->p_ucred->cr_uid;
	/* Saved uid is set to the new euid if the ruid changed */
	svuid = (ruid == p->p_cred->p_ruid) ? -1 : euid;

	return do_setresuid(l, ruid, euid, svuid,
			    ID_R_EQ_R | ID_R_EQ_E |
			    ID_E_EQ_R | ID_E_EQ_E | ID_E_EQ_S |
			    ID_S_EQ_R | ID_S_EQ_E | ID_S_EQ_S);
}

/* ARGSUSED */
int
sys_setgid(struct lwp *l, void *v, register_t *retval)
{
	struct sys_setgid_args /* {
		syscallarg(gid_t) gid;
	} */ *uap = v;
	gid_t gid = SCARG(uap, gid);

	return do_setresgid(l, gid, gid, gid,
			    ID_R_EQ_R | ID_E_EQ_R | ID_S_EQ_R);
}

/* ARGSUSED */
int
sys_setegid(struct lwp *l, void *v, register_t *retval)
{
	struct sys_setegid_args /* {
		syscallarg(gid_t) egid;
	} */ *uap = v;

	return do_setresgid(l, -1, SCARG(uap, egid), -1, ID_E_EQ_R | ID_E_EQ_S);
}

int
sys_setregid(struct lwp *l, void *v, register_t *retval)
{
	struct sys_setregid_args /* {
		syscallarg(gid_t) rgid;
		syscallarg(gid_t) egid;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	gid_t rgid, egid, svgid;

	rgid = SCARG(uap, rgid);
	egid = SCARG(uap, egid);
	if (rgid == -1)
		rgid = p->p_cred->p_rgid;
	if (egid == -1)
		egid = p->p_ucred->cr_gid;
	/* Saved gid is set to the new egid if the rgid changed */
	svgid = rgid == p->p_cred->p_rgid ? -1 : egid;

	return do_setresgid(l, rgid, egid, svgid,
			ID_R_EQ_R | ID_R_EQ_E |
			ID_E_EQ_R | ID_E_EQ_E | ID_E_EQ_S |
			ID_S_EQ_R | ID_S_EQ_E | ID_S_EQ_S);
}

int
sys_issetugid(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;

	/*
	 * Note: OpenBSD sets a P_SUGIDEXEC flag set at execve() time,
	 * we use P_SUGID because we consider changing the owners as
	 * "tainting" as well.
	 * This is significant for procs that start as root and "become"
	 * a user without an exec - programs cannot know *everything*
	 * that libc *might* have put in their data segment.
	 */
	*retval = (p->p_flag & P_SUGID) != 0;
	return (0);
}

/* ARGSUSED */
int
sys_setgroups(struct lwp *l, void *v, register_t *retval)
{
	struct sys_setgroups_args /* {
		syscallarg(int) gidsetsize;
		syscallarg(const gid_t *) gidset;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct pcred *pc = p->p_cred;
	int ngrp;
	int error;
	gid_t grp[NGROUPS];
	size_t grsize;

	if ((error = suser(pc->pc_ucred, &p->p_acflag)) != 0)
		return (error);

	ngrp = SCARG(uap, gidsetsize);
	if ((u_int)ngrp > NGROUPS)
		return (EINVAL);

	grsize = ngrp * sizeof(gid_t);
	error = copyin(SCARG(uap, gidset), grp, grsize);
	if (error)
		return (error);
	/*
	 * Check if this is a no-op.
	 */
	if (pc->pc_ucred->cr_ngroups == (u_int) ngrp &&
	    memcmp(grp, pc->pc_ucred->cr_groups, grsize) == 0)
		return (0);

	pc->pc_ucred = crcopy(pc->pc_ucred);
	(void)memcpy(pc->pc_ucred->cr_groups, grp, grsize);
	pc->pc_ucred->cr_ngroups = ngrp;
	p_sugid(p);
	return (0);
}

/*
 * Check if gid is a member of the group set.
 */
int
groupmember(gid_t gid, const struct ucred *cred)
{
	const gid_t *gp;
	const gid_t *egp;

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
suser(const struct ucred *cred, u_short *acflag)
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
crget(void)
{
	struct ucred *cr;

	cr = pool_get(&cred_pool, PR_WAITOK);
	memset(cr, 0, sizeof(*cr));
	cr->cr_ref = 1;
	return (cr);
}

/*
 * Free a cred structure.
 * Throws away space when ref count gets to 0.
 */
void
crfree(struct ucred *cr)
{

	if (--cr->cr_ref == 0)
		pool_put(&cred_pool, cr);
}

/*
 * Compare cred structures and return 0 if they match
 */
int
crcmp(const struct ucred *cr1, const struct uucred *cr2)
{
	return cr1->cr_uid != cr2->cr_uid ||
	    cr1->cr_gid != cr2->cr_gid ||
	    cr1->cr_ngroups != (uint32_t)cr2->cr_ngroups ||
	    memcmp(cr1->cr_groups, cr2->cr_groups, cr1->cr_ngroups);
}

/*
 * Copy cred structure to a new one and free the old one.
 */
struct ucred *
crcopy(struct ucred *cr)
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
crdup(const struct ucred *cr)
{
	struct ucred *newcr;

	newcr = crget();
	*newcr = *cr;
	newcr->cr_ref = 1;
	return (newcr);
}

/*
 * convert from userland credentials to kernel one
 */
void
crcvt(struct ucred *uc, const struct uucred *uuc)
{

	uc->cr_ref = 0;
	uc->cr_uid = uuc->cr_uid;
	uc->cr_gid = uuc->cr_gid;
	uc->cr_ngroups = uuc->cr_ngroups;
	(void)memcpy(uc->cr_groups, uuc->cr_groups, sizeof(uuc->cr_groups));
}

/*
 * Get login name, if available.
 */
/* ARGSUSED */
int
sys___getlogin(struct lwp *l, void *v, register_t *retval)
{
	struct sys___getlogin_args /* {
		syscallarg(char *) namebuf;
		syscallarg(size_t) namelen;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	if (SCARG(uap, namelen) > sizeof(p->p_pgrp->pg_session->s_login))
		SCARG(uap, namelen) = sizeof(p->p_pgrp->pg_session->s_login);
	return (copyout((caddr_t) p->p_pgrp->pg_session->s_login,
	    (caddr_t) SCARG(uap, namebuf), SCARG(uap, namelen)));
}

/*
 * Set login name.
 */
/* ARGSUSED */
int
sys___setlogin(struct lwp *l, void *v, register_t *retval)
{
	struct sys___setlogin_args /* {
		syscallarg(const char *) namebuf;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct session *s = p->p_pgrp->pg_session;
	char newname[sizeof s->s_login + 1];
	int error;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);
	error = copyinstr(SCARG(uap, namebuf), &newname, sizeof newname, NULL);
	if (error != 0)
		return (error == ENAMETOOLONG ? EINVAL : error);

	if (s->s_flags & S_LOGIN_SET && p->p_pid != s->s_sid &&
	    strncmp(newname, s->s_login, sizeof s->s_login) != 0)
		log(LOG_WARNING, "%s (pid %d) changing logname from "
		    "%.*s to %s\n", p->p_comm, p->p_pid,
		    (int)sizeof s->s_login, s->s_login, newname);
	s->s_flags |= S_LOGIN_SET;
	strncpy(s->s_login, newname, sizeof s->s_login);
	return (0);
}
