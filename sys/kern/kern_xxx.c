/*	$NetBSD: kern_xxx.c,v 1.25 1995/04/22 19:43:00 christos Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)kern_xxx.c	8.2 (Berkeley) 11/14/93
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <vm/vm.h>
#include <sys/sysctl.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

/* ARGSUSED */
int
reboot(p, uap, retval)
	struct proc *p;
	struct reboot_args /* {
		syscallarg(int) opt;
	} */ *uap;
	register_t *retval;
{
	int error;

	if (error = suser(p->p_ucred, &p->p_acflag))
		return (error);
	boot(SCARG(uap, opt));
	return (0);
}

#if defined(COMPAT_43) || defined(COMPAT_SUNOS) || defined(COMPAT_LINUX)

/* ARGSUSED */
int
compat_43_gethostname(p, uap, retval)
	struct proc *p;
	struct compat_43_gethostname_args /* {
		syscallarg(char *) hostname;
		syscallarg(u_int) len;
	} */ *uap;
	register_t *retval;
{
	int name;

	name = KERN_HOSTNAME;
	return (kern_sysctl(&name, 1, SCARG(uap, hostname), &SCARG(uap, len),
	    0, 0));
}

/* ARGSUSED */
int
compat_43_sethostname(p, uap, retval)
	struct proc *p;
	register struct compat_43_sethostname_args *uap;
	register_t *retval;
{
	int name;
	int error;

	if (error = suser(p->p_ucred, &p->p_acflag))
		return (error);
	name = KERN_HOSTNAME;
	return (kern_sysctl(&name, 1, 0, 0, SCARG(uap, hostname),
	    SCARG(uap, len)));
}

/* ARGSUSED */
int
compat_43_gethostid(p, uap, retval)
	struct proc *p;
	void *uap;
	register_t *retval;
{

	*(int32_t *)retval = hostid;
	return (0);
}
#endif /* COMPAT_43 || COMPAT_SUNOS || COMPAT_LINUX */

#ifdef COMPAT_43
/* ARGSUSED */
int
compat_43_sethostid(p, uap, retval)
	struct proc *p;
	struct compat_43_sethostid_args /* {
		syscallarg(int32_t) hostid;
	} */ *uap;
	register_t *retval;
{
	int error;

	if (error = suser(p->p_ucred, &p->p_acflag))
		return (error);
	hostid = SCARG(uap, hostid);
	return (0);
}

int
compat_43_quota(p, uap, retval)
	struct proc *p;
	void *uap;
	register_t *retval;
{

	return (ENOSYS);
}
#endif /* COMPAT_43 */

#if defined(COMPAT_09) || defined(COMPAT_SUNOS) ||defined(COMPAT_HPUX) \
  || defined(COMPAT_ULTRIX) || defined(COMPAT_LINUX)
/* ARGSUSED */
int
compat_09_getdomainname(p, uap, retval)
	struct proc *p;
	struct compat_09_getdomainname_args /* {
		syscallarg(char *) domainname;
		syscallarg(int) len;
	} */ *uap;
	register_t *retval;
{
	int name;

	name = KERN_DOMAINNAME;
	return (kern_sysctl(&name, 1, SCARG(uap, domainname),
	    &SCARG(uap, len), 0, 0));
}

/* ARGSUSED */
int
compat_09_setdomainname(p, uap, retval)
	struct proc *p;
	struct compat_09_setdomainname_args /* {
		syscallarg(char *) domainname;
		syscallarg(int) len;
	} */ *uap;
	register_t *retval;
{
	int name;
	int error;

	if (error = suser(p->p_ucred, &p->p_acflag))
		return (error);
	name = KERN_DOMAINNAME;
	return (kern_sysctl(&name, 1, 0, 0, SCARG(uap, domainname),
	    SCARG(uap, len)));
}
#endif /* COMPAT_09 || COMPAT_SUNOS || COMPAT_HPUX || COMPAT_ULTRIX 
	  COMPAT_LINUX */

#ifdef COMPAT_09
struct outsname {
	char	sysname[32];
	char	nodename[32];
	char	release[32];
	char	version[32];
	char	machine[32];
};

/* ARGSUSED */
int
compat_09_uname(p, uap, retval)
	struct proc *p;
	struct compat_09_uname_args /* {
		syscallarg(struct outsname *) name;
	} */ *uap;
	register_t *retval;
{
	struct outsname outsname;
	char *cp, *dp, *ep;
	extern char ostype[], osrelease[];

	strncpy(outsname.sysname, ostype, sizeof(outsname.sysname));
	strncpy(outsname.nodename, hostname, sizeof(outsname.nodename));
	strncpy(outsname.release, osrelease, sizeof(outsname.release));
	dp = outsname.version;
	ep = &outsname.version[sizeof(outsname.version) - 1];
	for (cp = version; *cp && *cp != '('; cp++)
		;
	for (cp++; *cp && *cp != ')' && dp < ep; cp++)
		*dp++ = *cp;
	for (; *cp && *cp != '#'; cp++)
		;
	for (; *cp && *cp != ':' && dp < ep; cp++)
		*dp++ = *cp;
	*dp = '\0';
	strncpy(outsname.machine, MACHINE, sizeof(outsname.machine));
	return (copyout((caddr_t)&outsname, (caddr_t)SCARG(uap, name),
	    sizeof(struct outsname)));
}
#endif /* COMPAT_09 */

#ifdef SYSCALL_DEBUG
#define	SCDEBUG_CALLS		0x0001	/* show calls */
#define	SCDEBUG_RETURNS		0x0002	/* show returns */
#define	SCDEBUG_ALL		0x0004	/* even syscalls that are implemented */
#define	SCDEBUG_SHOWARGS	0x0008	/* show arguments to calls */

int	scdebug = SCDEBUG_CALLS|SCDEBUG_RETURNS|SCDEBUG_SHOWARGS;

void
scdebug_call(p, code, args)
	struct proc *p;
	register_t code, args[];
{
	struct sysent *sy;
	struct emul *em;
	int i;

	if (!(scdebug & SCDEBUG_CALLS))
		return;

	em = p->p_emul;
	sy = &em->e_sysent[code];
	if (!(scdebug & SCDEBUG_ALL || code < 0 || code >= em->e_nsysent ||
	     sy->sy_call == nosys))
		return;
		
	printf("proc %d (%s): %s num ", p->p_pid, p->p_comm, em->e_name);
	if (code < 0 || code >= em->e_nsysent)
		printf("OUT OF RANGE (%d)", code);
	else {
		printf("%d call: %s", code, em->e_syscallnames[code]);
		if (scdebug & SCDEBUG_SHOWARGS) {
			printf("(");
			for (i = 0; i < sy->sy_argsize / sizeof(register_t);
			    i++)
				printf("%s0x%lx", i == 0 ? "" : ", ",
				    (long)args[i]);
			printf(")");
		}
	}
	printf("\n");
}

void
scdebug_ret(p, code, error, retval)
	struct proc *p;
	register_t code;
	int error;
	register_t retval[];
{
	struct sysent *sy;
	struct emul *em;

	if (!(scdebug & SCDEBUG_RETURNS))
		return;

	em = p->p_emul;
	sy = &em->e_sysent[code];
	if (!(scdebug & SCDEBUG_ALL || code < 0 || code >= *em->e_nsysent ||
	    sy->sy_call == nosys))
		return;
		
	printf("proc %d (%s): %s num ", p->p_pid, p->p_comm, em->e_name);
	if (code < 0 || code >= em->e_nsysent)
		printf("OUT OF RANGE (%d)", code);
	else
		printf("%d ret: err = %d, rv = 0x%lx,0x%lx", code,
		    error, (long)retval[0], (long)retval[1]);
	printf("\n");
}
#endif /* SYSCALL_DEBUG */
