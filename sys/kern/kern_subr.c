/*	$NetBSD: kern_subr.c,v 1.51 1999/06/07 20:16:09 thorpej Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Luke Mewburn.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Copyright (c) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)kern_subr.c	8.4 (Berkeley) 2/14/95
 */

#include "opt_md.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/disklabel.h>
#include <sys/queue.h>

#include <dev/cons.h>

#include <net/if.h>

/* XXX these should eventually move to subr_autoconf.c */
static int findblkmajor __P((const char *));
static const char *findblkname __P((int));
static struct device *getdisk __P((char *, int, int, dev_t *, int));
static struct device *parsedisk __P((char *, int, int, dev_t *));
static int getstr __P((char *, int));

int
uiomove(buf, n, uio)
	register void *buf;
	register int n;
	register struct uio *uio;
{
	register struct iovec *iov;
	u_int cnt;
	int error = 0;
	char *cp = buf;

#ifdef DIAGNOSTIC
	if (uio->uio_rw != UIO_READ && uio->uio_rw != UIO_WRITE)
		panic("uiomove: mode");
	if (uio->uio_segflg == UIO_USERSPACE && uio->uio_procp != curproc)
		panic("uiomove proc");
#endif
	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;
		switch (uio->uio_segflg) {

		case UIO_USERSPACE:
			if (uio->uio_rw == UIO_READ)
				error = copyout(cp, iov->iov_base, cnt);
			else
				error = copyin(iov->iov_base, cp, cnt);
			if (error)
				return (error);
			break;

		case UIO_SYSSPACE:
			if (uio->uio_rw == UIO_READ)
				error = kcopy(cp, iov->iov_base, cnt);
			else
				error = kcopy(iov->iov_base, cp, cnt);
			if (error)
				return(error);
			break;
		}
		iov->iov_base = (caddr_t)iov->iov_base + cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp += cnt;
		n -= cnt;
	}
	return (error);
}

/*
 * Give next character to user as result of read.
 */
int
ureadc(c, uio)
	register int c;
	register struct uio *uio;
{
	register struct iovec *iov;

	if (uio->uio_resid <= 0)
		panic("ureadc: non-positive resid");
again:
	if (uio->uio_iovcnt <= 0)
		panic("ureadc: non-positive iovcnt");
	iov = uio->uio_iov;
	if (iov->iov_len <= 0) {
		uio->uio_iovcnt--;
		uio->uio_iov++;
		goto again;
	}
	switch (uio->uio_segflg) {

	case UIO_USERSPACE:
		if (subyte(iov->iov_base, c) < 0)
			return (EFAULT);
		break;

	case UIO_SYSSPACE:
		*(char *)iov->iov_base = c;
		break;
	}
	iov->iov_base = (caddr_t)iov->iov_base + 1;
	iov->iov_len--;
	uio->uio_resid--;
	uio->uio_offset++;
	return (0);
}

/*
 * General routine to allocate a hash table.
 * Allocate enough memory to hold at least `elements' list-head pointers.
 * Return a pointer to the allocated space and set *hashmask to a pattern
 * suitable for masking a value to use as an index into the returned array.
 */
void *
hashinit(elements, type, flags, hashmask)
	int elements, type, flags;
	u_long *hashmask;
{
	long hashsize;
	LIST_HEAD(generic, generic) *hashtbl;
	int i;

	if (elements <= 0)
		panic("hashinit: bad cnt");
	for (hashsize = 1; hashsize < elements; hashsize <<= 1)
		continue;
	hashtbl = malloc((u_long)hashsize * sizeof(*hashtbl), type, flags);
	for (i = 0; i < hashsize; i++)
		LIST_INIT(&hashtbl[i]);
	*hashmask = hashsize - 1;
	return (hashtbl);
}

/*
 * "Shutdown hook" types, functions, and variables.
 */

struct shutdownhook_desc {
	LIST_ENTRY(shutdownhook_desc) sfd_list;
	void	(*sfd_fn) __P((void *));
	void	*sfd_arg;
};

LIST_HEAD(, shutdownhook_desc) shutdownhook_list;

void *
shutdownhook_establish(fn, arg)
	void (*fn) __P((void *));
	void *arg;
{
	struct shutdownhook_desc *ndp;

	ndp = (struct shutdownhook_desc *)
	    malloc(sizeof(*ndp), M_DEVBUF, M_NOWAIT);
	if (ndp == NULL)
		return NULL;

	ndp->sfd_fn = fn;
	ndp->sfd_arg = arg;
	LIST_INSERT_HEAD(&shutdownhook_list, ndp, sfd_list);

	return (ndp);
}

void
shutdownhook_disestablish(vhook)
	void *vhook;
{
#ifdef DIAGNOSTIC
	struct shutdownhook_desc *dp;

	for (dp = shutdownhook_list.lh_first; dp != NULL;
	    dp = dp->sfd_list.le_next)
                if (dp == vhook)
			break;
	if (dp == NULL)
		panic("shutdownhook_disestablish: hook not established");
#endif

	LIST_REMOVE((struct shutdownhook_desc *)vhook, sfd_list);
	free(vhook, M_DEVBUF);
}

/*
 * Run shutdown hooks.  Should be invoked immediately before the
 * system is halted or rebooted, i.e. after file systems unmounted,
 * after crash dump done, etc.
 *
 * Each shutdown hook is removed from the list before it's run, so that
 * it won't be run again.
 */
void
doshutdownhooks()
{
	struct shutdownhook_desc *dp;

	while ((dp = shutdownhook_list.lh_first) != NULL) {
		LIST_REMOVE(dp, sfd_list);
		(*dp->sfd_fn)(dp->sfd_arg);
#if 0
		/*
		 * Don't bother freeing the hook structure,, since we may
		 * be rebooting because of a memory corruption problem,
		 * and this might only make things worse.  It doesn't
		 * matter, anyway, since the system is just about to
		 * reboot.
		 */
		free(dp, M_DEVBUF);
#endif
	}
}

/*
 * "Mountroot hook" types, functions, and variables.
 */

struct mountroothook_desc {
	LIST_ENTRY(mountroothook_desc) mrd_list;
	struct	device *mrd_device;
	void 	(*mrd_func) __P((struct device *));
};

LIST_HEAD(, mountroothook_desc) mountroothook_list;

void *
mountroothook_establish(func, dev)
	void (*func) __P((struct device *));
	struct device *dev;
{
	struct mountroothook_desc *mrd;

	mrd = (struct mountroothook_desc *)
	    malloc(sizeof(*mrd), M_DEVBUF, M_NOWAIT);
	if (mrd == NULL)
		return (NULL);

	mrd->mrd_device = dev;
	mrd->mrd_func = func;
	LIST_INSERT_HEAD(&mountroothook_list, mrd, mrd_list);

	return (mrd);
}

void
mountroothook_disestablish(vhook)
	void *vhook;
{
#ifdef DIAGNOSTIC
	struct mountroothook_desc *mrd;

	for (mrd = mountroothook_list.lh_first; mrd != NULL;
	    mrd = mrd->mrd_list.le_next)
                if (mrd == vhook)
			break;
	if (mrd == NULL)
		panic("mountroothook_disestablish: hook not established");
#endif

	LIST_REMOVE((struct mountroothook_desc *)vhook, mrd_list);
	free(vhook, M_DEVBUF);
}

void
mountroothook_destroy()
{
	struct mountroothook_desc *mrd;

	while ((mrd = mountroothook_list.lh_first) != NULL) {
		LIST_REMOVE(mrd, mrd_list);
		free(mrd, M_DEVBUF);
	}
}

void
domountroothook()
{
	struct mountroothook_desc *mrd;

	for (mrd = mountroothook_list.lh_first; mrd != NULL;
	    mrd = mrd->mrd_list.le_next) {
		if (mrd->mrd_device == root_device) {
			(*mrd->mrd_func)(root_device);
			return;
		}
	}
}

/*
 * Determine the root device and, if instructed to, the root file system.
 */

#include "md.h"
#if NMD == 0
#undef MEMORY_DISK_HOOKS
#endif

#ifdef MEMORY_DISK_HOOKS
static struct device fakemdrootdev[NMD];
#endif

void
setroot(bootdv, bootpartition)
	struct device *bootdv;
	int bootpartition;
{
	struct device *dv;
	int len, print_newline = 0;
#ifdef MEMORY_DISK_HOOKS
	int i;
#endif
	dev_t nrootdev;
	dev_t ndumpdev = NODEV;
	char buf[128];
	const char *rootdevname;
	const char *dumpdevname;
	struct device *rootdv = NULL;		/* XXX gcc -Wuninitialized */
	struct device *dumpdv = NULL;
	struct ifnet *ifp;
	const char *deffsname;
	struct vfsops *vops;
	extern int (*mountroot) __P((void));

#ifdef MEMORY_DISK_HOOKS
	for (i = 0; i < NMD; i++) {
		fakemdrootdev[i].dv_class  = DV_DISK;
		fakemdrootdev[i].dv_cfdata = NULL;
		fakemdrootdev[i].dv_unit   = i;
		fakemdrootdev[i].dv_parent = NULL;
		sprintf(fakemdrootdev[i].dv_xname, "md%d", i);
	}
#endif /* MEMORY_DISK_HOOKS */

#ifdef MEMORY_DISK_IS_ROOT
	bootdv = &fakemdrootdev[0];
	bootpartition = 0;
#endif

	/*
	 * If NFS is specified as the file system, and we found
	 * a DV_DISK boot device (or no boot device at all), then
	 * find a reasonable network interface for "rootspec".
	 */
	vops = vfs_getopsbyname("nfs");
	if (vops != NULL && vops->vfs_mountroot == mountroot &&
	    rootspec == NULL &&
	    (bootdv == NULL || bootdv->dv_class != DV_IFNET)) {
		for (ifp = ifnet.tqh_first; ifp != NULL;
		    ifp = ifp->if_list.tqe_next)
			if ((ifp->if_flags &
			     (IFF_LOOPBACK|IFF_POINTOPOINT)) == 0)
				break;
		if (ifp == NULL) {
			/*
			 * Can't find a suitable interface; ask the
			 * user.
			 */
			boothowto |= RB_ASKNAME;
		} else {
			/*
			 * Have a suitable interface; behave as if
			 * the user specified this interface.
			 */
			rootspec = (const char *)ifp->if_xname;
		}
	}

	/*
	 * If wildcarded root and we the boot device wasn't determined,
	 * ask the user.
	 */
	if (rootspec == NULL && bootdv == NULL)
		boothowto |= RB_ASKNAME;

 top:
	if (boothowto & RB_ASKNAME) {
		struct device *defdumpdv;

		for (;;) {
			printf("root device");
			if (bootdv != NULL) {
				printf(" (default %s", bootdv->dv_xname);
				if (bootdv->dv_class == DV_DISK)
					printf("%c", bootpartition + 'a');
				printf(")");
			}
			printf(": ");
			len = getstr(buf, sizeof(buf));
			if (len == 0 && bootdv != NULL) {
				strcpy(buf, bootdv->dv_xname);
				len = strlen(buf);
			}
			if (len > 0 && buf[len - 1] == '*') {
				buf[--len] = '\0';
				dv = getdisk(buf, len, 1, &nrootdev, 0);
				if (dv != NULL) {
					rootdv = dv;
					break;
				}
			}
			dv = getdisk(buf, len, bootpartition, &nrootdev, 0);
			if (dv != NULL) {
				rootdv = dv;
				break;
			}
		}

		/*
		 * Set up the default dump device.  If root is on
		 * a network device, there is no default dump
		 * device, since we don't support dumps to the
		 * network.
		 */
		if (rootdv->dv_class == DV_IFNET)
			defdumpdv = NULL;
		else
			defdumpdv = rootdv;

		for (;;) {
			printf("dump device");
			if (defdumpdv != NULL) {
				/*
				 * Note, we know it's a disk if we get here.
				 */
				printf(" (default %sb)", defdumpdv->dv_xname);
			}
			printf(": ");
			len = getstr(buf, sizeof(buf));
			if (len == 0) {
				if (defdumpdv != NULL) {
					ndumpdev = MAKEDISKDEV(major(nrootdev),
					    DISKUNIT(nrootdev), 1);
				}
				if (rootdv->dv_class == DV_IFNET)
					dumpdv = NULL;
				else
					dumpdv = rootdv;
				break;
			}
			if (len == 4 && strcmp(buf, "none") == 0) {
				dumpspec = "none";
				goto havedump;
			}
			dv = getdisk(buf, len, 1, &ndumpdev, 1);
			if (dv) {
				dumpdv = dv;
				break;
			}
		}

 havedump:
		rootdev = nrootdev;
		dumpdev = ndumpdev;

		for (vops = LIST_FIRST(&vfs_list); vops != NULL;
		     vops = LIST_NEXT(vops, vfs_list)) {
			if (vops->vfs_mountroot != NULL &&
			    vops->vfs_mountroot == mountroot)
			break;
		}

		if (vops == NULL) {
			mountroot = NULL;
			deffsname = "generic";
		} else
			deffsname = vops->vfs_name;

		for (;;) {
			printf("file system (default %s): ", deffsname);
			len = getstr(buf, sizeof(buf));
			if (len == 0)
				break;
			if (len == 4 && strcmp(buf, "halt") == 0)
				cpu_reboot(RB_HALT, NULL);
			else if (len == 7 && strcmp(buf, "generic") == 0) {
				mountroot = NULL;
				break;
			}
			vops = vfs_getopsbyname(buf);
			if (vops == NULL || vops->vfs_mountroot == NULL) {
				printf("use one of: generic");
				for (vops = LIST_FIRST(&vfs_list);
				     vops != NULL;
				     vops = LIST_NEXT(vops, vfs_list)) {
					if (vops->vfs_mountroot != NULL)
						printf(" %s", vops->vfs_name);
				}
				printf(" halt\n");
			} else {
				mountroot = vops->vfs_mountroot;
				break;
			}
		}

	} else if (rootspec == NULL) {
		int majdev;

		/*
		 * Wildcarded root; use the boot device.
		 */
		rootdv = bootdv;

		majdev = findblkmajor(bootdv->dv_xname);
		if (majdev >= 0) {
			/*
			 * Root is on a disk.  `bootpartition' is root.
			 */
			rootdev = MAKEDISKDEV(majdev, bootdv->dv_unit,
			    bootpartition);
		}
	} else {

		/*
		 * `root on <dev> ...'
		 */

		/*
		 * If it's a network interface, we can bail out
		 * early.
		 */
		for (dv = alldevs.tqh_first; dv != NULL;
		    dv = dv->dv_list.tqe_next)
			if (strcmp(dv->dv_xname, rootspec) == 0)
				break;
		if (dv != NULL && dv->dv_class == DV_IFNET) {
			rootdv = dv;
			goto haveroot;
		}

		rootdevname = findblkname(major(rootdev));
		if (rootdevname == NULL) {
			printf("unknown device major 0x%x\n", rootdev);
			boothowto |= RB_ASKNAME;
			goto top;
		}
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%s%d", rootdevname, DISKUNIT(rootdev));

		for (dv = alldevs.tqh_first; dv != NULL;
		    dv = dv->dv_list.tqe_next) {
			if (strcmp(buf, dv->dv_xname) == 0) {
				rootdv = dv;
				break;
			}
		}
		if (rootdv == NULL) {
			printf("device %s (0x%x) not configured\n",
			    buf, rootdev);
			boothowto |= RB_ASKNAME;
			goto top;
		}
	}

 haveroot:

	root_device = rootdv;

	switch (rootdv->dv_class) {
	case DV_IFNET:
		/* Nothing. */
		break;

	case DV_DISK:
		printf("root on %s%c", rootdv->dv_xname,
		    DISKPART(rootdev) + 'a');
		print_newline = 1;
		break;

	default:
		printf("can't determine root device\n");
		boothowto |= RB_ASKNAME;
		goto top;
	}

	/*
	 * Now configure the dump device.
	 */

	if (dumpspec != NULL && strcmp(dumpspec, "none") == 0) {
		/*
		 * Operator doesn't want a dump device.
		 */
		goto nodumpdev;
	}

	/*
	 * If we haven't figured out the dump device, do so, with
	 * the following rules:
	 *
	 *	(a) We already know dumpdv in the RB_ASKNAME case.
	 *
	 *	(b) If dumpspec is set, try to use it.  If the device
	 *	    is not available, punt.
	 *
	 *	(c) If dumpspec is not set, the dump device is
	 *	    wildcarded or unspecified.  If the root device
	 *	    is DV_IFNET, punt.  Otherwise, use partition b
	 *	    of the root device.
	 */

	if (boothowto & RB_ASKNAME) {
		if (dumpdv == NULL) {
			/*
			 * Just return; dumpdev is already set to NODEV
			 * and we don't want to print a newline in this
			 * case.
			 */
			return;
		}
		goto out;
	}

	if (dumpspec != NULL) {
		if (dumpdev == NODEV) {
			/*
			 * Looks like they tried to pick a network
			 * device.  Oops.
			 */
			goto nodumpdev;
		}

		dumpdevname = findblkname(major(dumpdev));
		if (dumpdevname == NULL)
			goto nodumpdev;
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%s%d", dumpdevname, DISKUNIT(dumpdev));

		for (dv = alldevs.tqh_first; dv != NULL;
		    dv = dv->dv_list.tqe_next) {
			if (strcmp(buf, dv->dv_xname) == 0) {
				dumpdv = dv;
				break;
			}
		}
		if (dv == NULL) {
			/*
			 * Device not configured.
			 */
			goto nodumpdev;
		}
	} else if (rootdv->dv_class == DV_IFNET)
		goto nodumpdev;
	else {
		dumpdv = rootdv;
		dumpdev = MAKEDISKDEV(major(rootdev), dumpdv->dv_unit, 1);
	}

 out:
	printf(" dumps on %s%c\n", dumpdv->dv_xname, DISKPART(dumpdev) + 'a');
	return;

 nodumpdev:
	dumpdev = NODEV;
	if (print_newline)
		printf("\n");
}

static int
findblkmajor(name)
	const char *name;
{
	int i;

	for (i = 0; dev_name2blk[i].d_name != NULL; i++)
		if (strncmp(name, dev_name2blk[i].d_name,
		    strlen(dev_name2blk[i].d_name)) == 0)
			return (dev_name2blk[i].d_maj);
	return (-1);
}

const char *
findblkname(maj)
	int maj;
{
	int i;

	for (i = 0; dev_name2blk[i].d_name != NULL; i++)
		if (dev_name2blk[i].d_maj == maj)
			return (dev_name2blk[i].d_name);
	return (NULL);
}

static struct device *
getdisk(str, len, defpart, devp, isdump)
	char *str;
	int len, defpart;
	dev_t *devp;
	int isdump;
{
	struct device	*dv;
#ifdef MEMORY_DISK_HOOKS
	int		i;
#endif

	if ((dv = parsedisk(str, len, defpart, devp)) == NULL) {
		printf("use one of:");
#ifdef MEMORY_DISK_HOOKS
		if (isdump == 0)
			for (i = 0; i < NMD; i++)
				printf(" %s[a-%c]", fakemdrootdev[i].dv_xname,
				    'a' + MAXPARTITIONS - 1);
#endif
		for (dv = alldevs.tqh_first; dv != NULL;
		    dv = dv->dv_list.tqe_next) {
			if (dv->dv_class == DV_DISK)
				printf(" %s[a-%c]", dv->dv_xname,
				    'a' + MAXPARTITIONS - 1);
			if (isdump == 0 && dv->dv_class == DV_IFNET)
				printf(" %s", dv->dv_xname);
		}
		if (isdump)
			printf(" none");
		printf(" halt\n");
	}
	return (dv);
}

static struct device *
parsedisk(str, len, defpart, devp)
	char *str;
	int len, defpart;
	dev_t *devp;
{
	struct device *dv;
	char *cp, c;
	int majdev, part;
#ifdef MEMORY_DISK_HOOKS
	int i;
#endif

	if (len == 0)
		return (NULL);

	if (len == 4 && strcmp(str, "halt") == 0)
		cpu_reboot(RB_HALT, NULL);

	cp = str + len - 1;
	c = *cp;
	if (c >= 'a' && c <= ('a' + MAXPARTITIONS - 1)) {
		part = c - 'a';
		*cp = '\0';
	} else
		part = defpart;

#ifdef MEMORY_DISK_HOOKS
	for (i = 0; i < NMD; i++)
		if (strcmp(str, fakemdrootdev[i].dv_xname) == 0) {
			dv = &fakemdrootdev[i];
			goto gotdisk;
		}
#endif

	for (dv = alldevs.tqh_first; dv != NULL; dv = dv->dv_list.tqe_next) {
		if (dv->dv_class == DV_DISK &&
		    strcmp(str, dv->dv_xname) == 0) {
#ifdef MEMORY_DISK_HOOKS
 gotdisk:
#endif
			majdev = findblkmajor(dv->dv_xname);
			if (majdev < 0)
				panic("parsedisk");
			*devp = MAKEDISKDEV(majdev, dv->dv_unit, part);
			break;
		}

		if (dv->dv_class == DV_IFNET &&
		    strcmp(str, dv->dv_xname) == 0) {
			*devp = NODEV;
			break;
		}
	}

	*cp = c;
	return (dv);
}

/*
 * XXX shouldn't this be a common function?
 */
static int
getstr(cp, size)
	char *cp;
	int size;
{
	char *lp;
	int c, len;

	cnpollc(1);

	lp = cp;
	len = 0;
	for (;;) {
		c = cngetc();
		switch (c) {
		case '\n':
		case '\r':
			printf("\n");
			*lp++ = '\0';
			cnpollc(0);
			return (len);
		case '\b':
		case '\177':
		case '#':
			if (len) {
				--len;
				--lp;
				printf("\b \b");
			}
			continue;
		case '@':
		case 'u'&037:
			len = 0;
			lp = cp;
			printf("\n");
			continue;
		default:
			if (len + 1 >= size || c < ' ') {
				printf("\007");
				continue;
			}
			printf("%c", c);
			++len;
			*lp++ = c;
		}
	}
}

/*
 * snprintf() `bytes' into `buf', reformatting it so that the number,
 * plus a possible `x' + suffix extension) fits into len bytes (including
 * the terminating NUL).
 * Returns the number of bytes stored in buf, or -1 * if there was a problem.
 * E.g, given a len of 9 and a suffix of `B': 
 *	bytes		result
 *	-----		------
 *	99999		`99999 B'
 *	100000		`97 KB'
 *	66715648	`65152 KB'
 *	252215296	`240 MB'
 */
int
humanize_number(buf, len, bytes, suffix)
	char		*buf;
	size_t		 len;
	u_int64_t	 bytes;
	const char	*suffix;
{
		/* prefixes are: (none), Kilo, Mega, Giga, Tera, Peta, Exa */
	static const char prefixes[] = " KMGTPE";

	int		i, r;
	u_int64_t	max;
	size_t		suffixlen;

	if (buf == NULL || suffix == NULL)
		return (-1);
	if (len > 0)
		buf[0] = '\0';
	suffixlen = strlen(suffix);
			/* check if enough room for `x y' + suffix + `\0' */
	if (len < 4 + suffixlen)
		return (-1);

	max = 1;
	for (i = 0; i < len - suffixlen - 3; i++)
		max *= 10;
	for (i = 0; bytes >= max && i < sizeof(prefixes); i++)
		bytes /= 1024;

	r = snprintf(buf, len, "%qu%s%c%s", (unsigned long long)bytes,
	    i == 0 ? "" : " ", prefixes[i], suffix);

	return (r);
}

int
format_bytes(buf, len, bytes)
	char		*buf;
	size_t		 len;
	u_int64_t	 bytes;
{
	int	rv;
	size_t	nlen;

	rv = humanize_number(buf, len, bytes, "B");
	if (rv != -1) {
			/* nuke the trailing ` B' if it exists */
		nlen = strlen(buf) - 2;
		if (strcmp(&buf[nlen], " B") == 0)
			buf[nlen] = '\0';
	}
	return (rv);
}
