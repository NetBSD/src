/*	$NetBSD: kern_xxx.c,v 1.18 1994/06/29 06:32:50 cgd Exp $	*/

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

struct reboot_args {
	int	opt;
};
/* ARGSUSED */
int
reboot(p, uap, retval)
	struct proc *p;
	struct reboot_args *uap;
	int *retval;
{
	int error;

	if (error = suser(p->p_ucred, &p->p_acflag))
		return (error);
	boot(uap->opt);
	return (0);
}

#if defined(COMPAT_43) || defined(COMPAT_SUNOS)

struct ogethostname_args {
	char	*hostname;
	u_int	len;
};
/* ARGSUSED */
int
ogethostname(p, uap, retval)
	struct proc *p;
	struct ogethostname_args *uap;
	int *retval;
{
	int name;

	name = KERN_HOSTNAME;
	return (kern_sysctl(&name, 1, uap->hostname, &uap->len, 0, 0));
}

struct osethostname_args {
	char	*hostname;
	u_int	len;
};
/* ARGSUSED */
int
osethostname(p, uap, retval)
	struct proc *p;
	register struct osethostname_args *uap;
	int *retval;
{
	int name;
	int error;

	if (error = suser(p->p_ucred, &p->p_acflag))
		return (error);
	name = KERN_HOSTNAME;
	return (kern_sysctl(&name, 1, 0, 0, uap->hostname, uap->len));
}

/* ARGSUSED */
int
ogethostid(p, uap, retval)
	struct proc *p;
	void *uap;
	int *retval;
{

	*(long *)retval = hostid;
	return (0);
}
#endif /* COMPAT_43 || COMPAT_SUNOS */

#ifdef COMPAT_43
struct osethostid_args {
	long	hostid;
};
/* ARGSUSED */
int
osethostid(p, uap, retval)
	struct proc *p;
	struct osethostid_args *uap;
	int *retval;
{
	int error;

	if (error = suser(p->p_ucred, &p->p_acflag))
		return (error);
	hostid = uap->hostid;
	return (0);
}

int
oquota()
{

	return (ENOSYS);
}
#endif /* COMPAT_43 */

#if defined(COMPAT_09) || defined(COMPAT_SUNOS) || defined(COMPAT_HPUX)
struct ogetdomainname_args {
	char	*domainname;
	u_int	len;
};
/* ARGSUSED */
int
ogetdomainname(p, uap, retval)
	struct proc *p;
	struct ogetdomainname_args *uap;
	int *retval;
{
	int name;

	name = KERN_DOMAINNAME;
	return (kern_sysctl(&name, 1, uap->domainname, &uap->len, 0, 0));
}

struct osetdomainname_args {
	char	*domainname;
	u_int	len;
};
/* ARGSUSED */
int
osetdomainname(p, uap, retval)
	struct proc *p;
	struct osetdomainname_args *uap;
	int *retval;
{
	int name;
	int error;

	if (error = suser(p->p_ucred, &p->p_acflag))
		return (error);
	name = KERN_DOMAINNAME;
	return (kern_sysctl(&name, 1, 0, 0, uap->domainname, uap->len));
}
#endif /* COMPAT_09 || COMPAT_SUNOS || COMPAT_HPUX */

#ifdef COMPAT_09
struct outsname {
	char	sysname[32];
	char	nodename[32];
	char	release[32];
	char	version[32];
	char	machine[32];
};

struct ouname_args {
	struct outsname	*name;
};
/* ARGSUSED */
int
ouname(p, uap, retval)
	struct proc *p;
	struct ouname_args *uap;
	int *retval;
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
	return (copyout((caddr_t)&outsname, (caddr_t)uap->name,
	    sizeof(struct outsname)));
}
#endif /* COMPAT_09 */

#ifdef SYSCALL_DEBUG
int	scdebug = 1;	/* XXX */

void
scdebug_call(p, code, narg, args)
	struct proc *p;
	int code, narg, *args;
{
	int i;

	if (!scdebug)
		return;

	printf("proc %d: syscall ", p->p_pid);
	if (code < 0 || code >= nsysent) {
		printf("OUT OF RANGE (%d)", code);
		code = 0;
	} else
		printf("%d", code);
	printf(" called: %s(", syscallnames[code]);
	for (i = 0; i < narg; i++)
		printf("0x%x, ", args[i]);
	printf(")\n");
}

void
scdebug_ret(p, code, error, retval)
	struct proc *p;
	int code, error, retval;
{
	if (!scdebug)
		return;

	printf("proc %d: syscall ", p->p_pid);
	if (code < 0 || code >= nsysent) {
		printf("OUT OF RANGE (%d)", code);
		code = 0;
	} else
		printf("%d", code);
	printf(" return: error = %d, retval = %d\n", error, retval);
}
#endif /* SYSCALL_DEBUG */
