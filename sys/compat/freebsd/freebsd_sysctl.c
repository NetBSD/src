/*	$NetBSD: freebsd_sysctl.c,v 1.18.2.1 2015/04/06 15:18:05 skrll Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
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
 * FreeBSD compatibility module. Try to deal with various FreeBSD sysctl calls.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: freebsd_sysctl.c,v 1.18.2.1 2015/04/06 15:18:05 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <sys/ktrace.h>

#include <sys/syscallargs.h>

#include <compat/freebsd/freebsd_syscallargs.h>
#include <compat/common/compat_util.h>
#include <compat/freebsd/freebsd_rtprio.h>
#include <compat/freebsd/freebsd_timex.h>
#include <compat/freebsd/freebsd_signal.h>
#include <compat/freebsd/freebsd_mman.h>
#include <compat/freebsd/freebsd_sysctl.h>

static int freebsd_sysctl_name2oid(char *, int *, int *);

static struct sysctllog *freebsd_clog;

void
freebsd_sysctl_init(void)
{

        sysctl_createv(&freebsd_clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
			CTLTYPE_INT, "osreldate",
			SYSCTL_DESCR("Operating system revision"),
			NULL, __NetBSD_Version__, NULL, 0,
			CTL_KERN, CTL_CREATE, CTL_EOL);
}

void
freebsd_sysctl_fini(void)
{

	sysctl_teardown(&freebsd_clog);
}

int
freebsd_sys_sysctl(struct lwp *l, const struct freebsd_sys_sysctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int *) name;
		syscallarg(u_int) namelen;
		syscallarg(void *) old;
		syscallarg(size_t *) oldlenp;
		syscallarg(void *) new;
		syscallarg(size_t) newlen;
	} */
	int error;
	int name[CTL_MAXNAME];
	size_t newlen, *oldlenp, oldlen;
	u_int namelen;
	void *new, *old;

	namelen = SCARG(uap, namelen);

	if ((namelen > CTL_MAXNAME) || (namelen < 1))
		return EINVAL;

	if ((error = copyin(SCARG(uap, name), name, 
	    namelen * sizeof(*name))) != 0)
		return error;

	if (namelen > 0 && name[0] != 0)
		return(sys___sysctl(l, (const void *)uap, retval));

	ktrmib(name, namelen);

	/*
	 * FreeBSD sysctl uses an undocumented set of special OIDs in its
	 * sysctl MIB whose tree is rooted at oid 0.  These OIDs are
	 * interpreted by their sysctl to implement functions that NetBSD
	 * performs in libc, such as sysctlgetmibinfo.
	 *
	 * From the FreeBSD kern_sysctl.c, these OIDs are:
	 * {0,0}        printf the entire MIB-tree.
	 * {0,1,...}    return the name of the "..." OID.
	 * {0,2,...}    return the next OID.
	 * {0,3}        return the OID of the name in "new"
	 * {0,4,...}    return the kind & format info for the "..." OID.
	 * {0,5,...}    return the description the "..." OID.
	 *
	 * Only implement {0,3} for now.
	 */

	if (namelen < 2 || namelen > CTL_MAXNAME)
		return(EINVAL);

	if (name[1] == 3) {
		char *locnew;
		int oid[CTL_MAXNAME];
		u_int oidlen = CTL_MAXNAME;

		new = SCARG(uap, new);
		newlen = SCARG(uap, newlen);
		if (new == NULL || newlen < 1 ||
		    newlen > (SYSCTL_NAMELEN * CTL_MAXNAME))
			return(EINVAL);

		old = SCARG(uap, old);
		oldlenp = SCARG(uap, oldlenp);
		if (old == NULL || oldlenp == NULL)
			return(EINVAL);

		if ((error = copyin(oldlenp, &oldlen, sizeof(oldlen))))
			return (error);
		if (oldlen < sizeof(int))
			return (EINVAL);

		if ((locnew =
		     (char *) malloc(newlen + 1, M_TEMP, M_WAITOK)) == NULL)
			return(ENOMEM);

		if ((error = copyinstr(new, locnew, newlen + 1, NULL))) {
			free(locnew, M_TEMP);
			return(error);
		}

		ktrmibio(-1, UIO_WRITE, new, newlen + 1, error);
		sysctl_lock(new != NULL);
		error = freebsd_sysctl_name2oid(locnew, oid, &oidlen);
		sysctl_unlock();
		free(locnew, M_TEMP);
		if (error)
			return(error);

		oidlen *= sizeof(int);
		error = copyout(oid, SCARG(uap, old),
				MIN(oidlen, oldlen));
		if (error)
			return(error);
		ktrmibio(-1, UIO_READ, SCARG(uap, old),
		    MIN(oidlen, oldlen),  0);

		error = copyout(&oidlen, SCARG(uap, oldlenp), sizeof(u_int));

		return(error);
	}

	return(EOPNOTSUPP);
}

static int
freebsd_sysctl_name2oid(char *name, int *oid, int *oidlen)
{
	char *dot;
	int oi, ci;
	struct sysctlnode *node, *pnode;

	pnode = &sysctl_root;
	node = sysctl_root.sysctl_child;
	oi = 0;
	if ((dot = strchr(name, (int)'.')) != NULL)
		*dot++ = '\0';

next:
	while (*name != '\0' && node != NULL) {
		for (ci = 0; ci < pnode->sysctl_clen; ci++) {
			if (strcmp(name, node[ci].sysctl_name) == 0) {
				oid[oi++] = node[ci].sysctl_num;
				if ((name = dot) == NULL) {
					*oidlen = oi;
					return(0);
				}
				if ((dot = strchr(name, (int)'.')) != NULL)
					*dot++ = '\0';

				if (SYSCTL_TYPE(node[ci].sysctl_flags) !=
				    CTLTYPE_NODE)
					return(ENOTDIR);

				pnode = &node[ci];
				node = pnode->sysctl_child;
				goto next;
			}
		}

		/* no more nodes, it must not exist */
		break;
	}

	return(ENOENT);
}
