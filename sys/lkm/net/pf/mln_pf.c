/*	$NetBSD: mln_pf.c,v 1.2 2004/06/22 18:04:05 christos Exp $	*/

/*
 * Copyright (C) 2003 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright (C) 1993-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mln_pf.c,v 1.2 2004/06/22 18:04:05 christos Exp $");

#include <sys/param.h>

/*
 * Post NetBSD 1.2 has the PFIL interface for packet filters.  This turns
 * on those hooks.  We don't need any special mods with this!
 */

#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/mbuf.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/route.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>
#include <sys/lkm.h>
#include <net/pfvar.h>

extern	int	lkmenodev __P((void));

int	pf_lkm_lkmentry __P((struct lkm_table *, int, int));
static	int	pf_unload __P((void));
static	int	pf_load __P((void));
static	int	pf_remove __P((void));
static	int	pfaction __P((struct lkm_table *, int));
static	char	*ipf_devfiles[] = { "/dev/pf" };

extern void pfattach(int);
extern int pfdetach(void);

#ifdef DEBUG
#define DPRINTF(a) printf a
#else
#define DPRINTF(a)
#endif

extern const struct cdevsw pf_cdevsw;

int	pf_major = 0;

MOD_DEV("pf", "pf", NULL, -1, &pf_cdevsw, -1);

extern int vd_unuseddev __P((void));
extern struct cdevsw cdevsw[];
extern int nchrdev;

int pf_lkm_lkmentry(lkmtp, cmd, ver)
	struct lkm_table *lkmtp;
	int cmd, ver;
{
	DISPATCH(lkmtp, cmd, ver, pfaction, pfaction, pfaction);
}


static int pfaction(lkmtp, cmd)
	struct lkm_table *lkmtp;
	int cmd;
{
	struct lkm_dev *args = lkmtp->private.lkm_dev;
	int error = 0;

	switch (cmd) {
	case LKM_E_LOAD :
		if (lkmexists(lkmtp)) {
			DPRINTF(("lkm exists\n"));
			return EEXIST;
		}

		error = devsw_attach(args->lkm_devname,
				   args->lkm_bdev, &args->lkm_bdevmaj,
				   args->lkm_cdev, &args->lkm_cdevmaj);
		if (error != 0) {
			DPRINTF(("devsw_attach(%s, %u, %u) failed %d\n",
			    args->lkm_devname, (unsigned)args->lkm_bdev,
			    (unsigned)args->lkm_cdev, error));
			return error;
		}
		if ((error = pf_load()) != 0) {
			DPRINTF(("pf_load failed %d\n", error));
			devsw_detach(args->lkm_bdev, args->lkm_cdev);
			return error;
		}
		pf_major = args->lkm_cdevmaj;
		printf("PF: loaded into slot %d\n", pf_major);
		break;
	case LKM_E_UNLOAD :
		devsw_detach(args->lkm_bdev, args->lkm_cdev);
		args->lkm_bdevmaj = -1;
		args->lkm_cdevmaj = -1;
		if ((error = pf_unload()) != 0) {
			DPRINTF(("unload failed %d\n", error));
			return error;
		}
		printf("PF: unloaded from slot %d\n", pf_major);
		break;
	case LKM_E_STAT :
		break;
	default:
		DPRINTF(("unknown command %d\n", cmd));
		error = EIO;
		break;
	}
	return 0;
}

static int pf_remove()
{
	char *name;
	struct nameidata nd;
	int error, i;

        for (i = 0; (name = ipf_devfiles[i]); i++) {
		NDINIT(&nd, DELETE, LOCKPARENT|LOCKLEAF, UIO_SYSSPACE,
		       name, curproc);
		if ((error = namei(&nd)) != 0) {
			DPRINTF(("namei failed %d\n", error));
			return error;
		}
		VOP_LEASE(nd.ni_dvp, curproc, curproc->p_ucred, LEASE_WRITE);
		VOP_LEASE(nd.ni_vp, curproc, curproc->p_ucred, LEASE_WRITE);
		(void) VOP_REMOVE(nd.ni_dvp, nd.ni_vp, &nd.ni_cnd);
	}
	return 0;
}

static int pf_unload()
{
	int error = 0;

	/*
	 * Unloading - remove the filter rule check from the IP
	 * input/output stream.
	 */
	if ((error = pfdetach()) != 0) {
		DPRINTF(("Detach failed %d\n", error));
		return error;
	}

	if ((error = pf_remove()) != 0) {
		DPRINTF(("Remove failed %d\n", error));
		return error;
	}
	printf("PF unloaded\n");
	return 0;
}

static int pf_load()
{
	struct nameidata nd;
	struct vattr vattr;
	int error = 0, fmode = S_IFCHR|0600, i;
	char *name;

	/*
	 * XXX Remove existing device nodes prior to creating new ones
	 * XXX using the assigned LKM device slot's major number.  In a
	 * XXX perfect world we could use the ones specified by cdevsw[].
	 */
	(void)pf_remove();

	pfattach(1);

	for (i = 0; (error == 0) && (name = ipf_devfiles[i]); i++) {
		NDINIT(&nd, CREATE, LOCKPARENT, UIO_SYSSPACE, name, curproc);
		if ((error = namei(&nd)) != 0)
			break;
		if (nd.ni_vp != NULL) {
			VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
			if (nd.ni_dvp == nd.ni_vp)
				vrele(nd.ni_dvp);
			else
				vput(nd.ni_dvp);
			vrele(nd.ni_vp);
			error = EEXIST;
			break;
		}
		VATTR_NULL(&vattr);
		vattr.va_type = VCHR;
		vattr.va_mode = (fmode & 07777);
		vattr.va_rdev = (pf_major << 8) | i;
		VOP_LEASE(nd.ni_dvp, curproc, curproc->p_ucred, LEASE_WRITE);
		error = VOP_MKNOD(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr);
		if (error == 0)
			vput(nd.ni_vp);
	}

	if (error == 0)
		printf("PF initialized.");
	return error;
}
