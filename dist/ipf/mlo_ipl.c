/*	$NetBSD: mlo_ipl.c,v 1.1.1.3 2007/04/14 20:17:25 martin Exp $	*/

/*
 * Copyright (C) 1993-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 */

#include <sys/param.h>
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
#include "ipl.h"
#include "ip_compat.h"
#include "ip_fil.h"

#define vn_lock(v,f) VOP_LOCK(v)

#if !defined(VOP_LEASE) && defined(LEASE_CHECK)
#define	VOP_LEASE	LEASE_CHECK
#endif


extern	int	lkmenodev __P((void));

#if OpenBSD >= 200311
int	if_ipl_lkmentry __P((struct lkm_table *, int, int));
#else
int	if_ipl __P((struct lkm_table *, int, int));
#endif
static	int	ipl_unload __P((void));
static	int	ipl_load __P((void));
static	int	ipl_remove __P((void));
static	int	iplaction __P((struct lkm_table *, int));
static	char	*ipf_devfiles[] = { IPL_NAME, IPNAT_NAME, IPSTATE_NAME,
				    IPAUTH_NAME, IPSYNC_NAME, IPSCAN_NAME,
				    IPLOOKUP_NAME, NULL };


struct	cdevsw	ipldevsw =
{
	iplopen,		/* open */
	iplclose,		/* close */
	iplread,		/* read */
	iplwrite,		/* write */
	iplioctl,		/* ioctl */
	(void *)nullop,		/* stop */
	(void *)NULL,		/* tty */
	(void *)nullop,		/* select */
	(void *)nullop,		/* mmap */
	NULL			/* strategy */
};

int	ipl_major = 0;

MOD_DEV(IPL_VERSION, LM_DT_CHAR, -1, &ipldevsw);

extern int vd_unuseddev __P((void));
extern struct cdevsw cdevsw[];
extern int nchrdev;


#if OpenBSD >= 200311
int if_ipl_lkmentry (lkmtp, cmd, ver)
#else
int if_ipl(lkmtp, cmd, ver)
#endif
struct lkm_table *lkmtp;
int cmd, ver;
{
	DISPATCH(lkmtp, cmd, ver, iplaction, iplaction, iplaction);
}

int lkmexists __P((struct lkm_table *)); /* defined in /sys/kern/kern_lkm.c */

static int iplaction(lkmtp, cmd)
struct lkm_table *lkmtp;
int cmd;
{
	int i;
	struct lkm_dev *args = lkmtp->private.lkm_dev;
	int err = 0;

	switch (cmd)
	{
	case LKM_E_LOAD :
		if (lkmexists(lkmtp))
			return EEXIST;

		for (i = 0; i < nchrdev; i++)
			if (cdevsw[i].d_open == (dev_type_open((*)))lkmenodev ||
			    cdevsw[i].d_open == iplopen)
				break;
		if (i == nchrdev) {
			printf("IP Filter: No free cdevsw slots\n");
			return ENODEV;
		}

		ipl_major = i;
		args->lkm_offset = i;   /* slot in cdevsw[] */
		printf("IP Filter: loaded into slot %d\n", ipl_major);
		return ipl_load();
	case LKM_E_UNLOAD :
		err = ipl_unload();
		if (!err)
			printf("IP Filter: unloaded from slot %d\n",
			       ipl_major);
		break;
	case LKM_E_STAT :
		break;
	default:
		err = EIO;
		break;
	}
	return err;
}


static int ipl_remove()
{
	struct nameidata nd;
	int error, i;
	char *name;

        for (i = 0; (name = ipf_devfiles[i]); i++) {
#if OpenBSD >= 200311
		NDINIT(&nd, DELETE, LOCKPARENT | LOCKLEAF, UIO_SYSSPACE,
		       name, curproc);
#else
		NDINIT(&nd, DELETE, LOCKPARENT, UIO_SYSSPACE, name, curproc);
#endif
		if ((error = namei(&nd)))
			return (error);
		VOP_LEASE(nd.ni_vp, curproc, curproc->p_ucred, LEASE_WRITE);
#if OpenBSD < 200311
		VOP_LOCK(nd.ni_vp, LK_EXCLUSIVE | LK_RETRY, curproc);
		VOP_LEASE(nd.ni_dvp, curproc, curproc->p_ucred, LEASE_WRITE);
#else
		(void)uvm_vnp_uncache(nd.ni_vp);

		VOP_LEASE(nd.ni_dvp, curproc, curproc->p_ucred, LEASE_WRITE);
		VOP_LEASE(nd.ni_vp, curproc, curproc->p_ucred, LEASE_WRITE);
#endif
		(void) VOP_REMOVE(nd.ni_dvp, nd.ni_vp, &nd.ni_cnd);
	}
	return 0;
}


static int ipl_unload()
{
	int error = 0;

	/*
	 * Unloading - remove the filter rule check from the IP
	 * input/output stream.
	 */
        if (fr_refcnt)
                error = EBUSY;
	else if (fr_running >= 0)
		error = ipfdetach();

	if (error == 0) {
		fr_running = -2;
		error = ipl_remove();
		printf("%s unloaded\n", ipfilter_version);
	}
	return error;
}


static int ipl_load()
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
	(void)ipl_remove();

	error = ipfattach();

	for (i = 0; (error == 0) && (name = ipf_devfiles[i]); i++) {
		NDINIT(&nd, CREATE, LOCKPARENT, UIO_SYSSPACE, name, curproc);
		if ((error = namei(&nd)))
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
		vattr.va_rdev = (ipl_major << 8) | i;
		VOP_LEASE(nd.ni_dvp, curproc, curproc->p_ucred, LEASE_WRITE);
		error = VOP_MKNOD(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr);
	}

	if (error == 0) {
		char *defpass;

		if (FR_ISPASS(fr_pass))
			defpass = "pass";
		else if (FR_ISBLOCK(fr_pass))
			defpass = "block";
		else
			defpass = "no-match -> block";

		printf("%s initialized.  Default = %s all, Logging = %s%s\n",
			ipfilter_version, defpass,
#ifdef IPFILTER_LOG
			"enabled",
#else
			"disabled",
#endif
#ifdef IPFILTER_COMPILED
			" (COMPILED)"
#else
			""
#endif
			);
		fr_running = 1;
	}
	return error;
}
