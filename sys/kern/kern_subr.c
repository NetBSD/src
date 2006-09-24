/*	$NetBSD: kern_subr.c,v 1.145 2006/09/24 06:51:39 dogcow Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 1999, 2002 The NetBSD Foundation, Inc.
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
 *	@(#)kern_subr.c	8.4 (Berkeley) 2/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_subr.c,v 1.145 2006/09/24 06:51:39 dogcow Exp $");

#include "opt_ddb.h"
#include "opt_md.h"
#include "opt_syscall_debug.h"
#include "opt_ktrace.h"
#include "opt_ptrace.h"
#include "opt_systrace.h"
#include "opt_powerhook.h"

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
#include <sys/systrace.h>
#include <sys/ktrace.h>
#include <sys/ptrace.h>
#include <sys/fcntl.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <net/if.h>

/* XXX these should eventually move to subr_autoconf.c */
static struct device *finddevice(const char *);
static struct device *getdisk(char *, int, int, dev_t *, int);
static struct device *parsedisk(char *, int, int, dev_t *);

/*
 * A generic linear hook.
 */
struct hook_desc {
	LIST_ENTRY(hook_desc) hk_list;
	void	(*hk_fn)(void *);
	void	*hk_arg;
};
typedef LIST_HEAD(, hook_desc) hook_list_t;

MALLOC_DEFINE(M_IOV, "iov", "large iov's");

void
uio_setup_sysspace(struct uio *uio)
{

	uio->uio_vmspace = vmspace_kernel();
}

int
uiomove(void *buf, size_t n, struct uio *uio)
{
	struct vmspace *vm = uio->uio_vmspace;
	struct iovec *iov;
	u_int cnt;
	int error = 0;
	char *cp = buf;
	int hold_count;

	hold_count = KERNEL_LOCK_RELEASE_ALL();

	ASSERT_SLEEPABLE(NULL, "uiomove");

#ifdef DIAGNOSTIC
	if (uio->uio_rw != UIO_READ && uio->uio_rw != UIO_WRITE)
		panic("uiomove: mode");
#endif
	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			KASSERT(uio->uio_iovcnt > 0);
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;
		if (!VMSPACE_IS_KERNEL_P(vm)) {
			if (curcpu()->ci_schedstate.spc_flags &
			    SPCF_SHOULDYIELD)
				preempt(1);
		}

		if (uio->uio_rw == UIO_READ) {
			error = copyout_vmspace(vm, cp, iov->iov_base,
			    cnt);
		} else {
			error = copyin_vmspace(vm, iov->iov_base, cp,
			    cnt);
		}
		if (error) {
			break;
		}
		iov->iov_base = (caddr_t)iov->iov_base + cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp += cnt;
		KDASSERT(cnt <= n);
		n -= cnt;
	}
	KERNEL_LOCK_ACQUIRE_COUNT(hold_count);
	return (error);
}

/*
 * Wrapper for uiomove() that validates the arguments against a known-good
 * kernel buffer.
 */
int
uiomove_frombuf(void *buf, size_t buflen, struct uio *uio)
{
	size_t offset;

	if (uio->uio_offset < 0 || /* uio->uio_resid < 0 || */
	    (offset = uio->uio_offset) != uio->uio_offset)
		return (EINVAL);
	if (offset >= buflen)
		return (0);
	return (uiomove((char *)buf + offset, buflen - offset, uio));
}

/*
 * Give next character to user as result of read.
 */
int
ureadc(int c, struct uio *uio)
{
	struct iovec *iov;

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
	if (!VMSPACE_IS_KERNEL_P(uio->uio_vmspace)) {
		if (subyte(iov->iov_base, c) < 0)
			return (EFAULT);
	} else {
		*(char *)iov->iov_base = c;
	}
	iov->iov_base = (caddr_t)iov->iov_base + 1;
	iov->iov_len--;
	uio->uio_resid--;
	uio->uio_offset++;
	return (0);
}

/*
 * Like copyin(), but operates on an arbitrary vmspace.
 */
int
copyin_vmspace(struct vmspace *vm, const void *uaddr, void *kaddr, size_t len)
{
	struct iovec iov;
	struct uio uio;
	int error;

	if (len == 0)
		return (0);

	if (VMSPACE_IS_KERNEL_P(vm)) {
		return kcopy(uaddr, kaddr, len);
	}
	if (__predict_true(vm == curproc->p_vmspace)) {
		return copyin(uaddr, kaddr, len);
	}

	iov.iov_base = kaddr;
	iov.iov_len = len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = (off_t)(intptr_t)uaddr;
	uio.uio_resid = len;
	uio.uio_rw = UIO_READ;
	UIO_SETUP_SYSSPACE(&uio);
	error = uvm_io(&vm->vm_map, &uio);

	return (error);
}

/*
 * Like copyout(), but operates on an arbitrary vmspace.
 */
int
copyout_vmspace(struct vmspace *vm, const void *kaddr, void *uaddr, size_t len)
{
	struct iovec iov;
	struct uio uio;
	int error;

	if (len == 0)
		return (0);

	if (VMSPACE_IS_KERNEL_P(vm)) {
		return kcopy(kaddr, uaddr, len);
	}
	if (__predict_true(vm == curproc->p_vmspace)) {
		return copyout(kaddr, uaddr, len);
	}

	iov.iov_base = __UNCONST(kaddr); /* XXXUNCONST cast away const */
	iov.iov_len = len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = (off_t)(intptr_t)uaddr;
	uio.uio_resid = len;
	uio.uio_rw = UIO_WRITE;
	UIO_SETUP_SYSSPACE(&uio);
	error = uvm_io(&vm->vm_map, &uio);

	return (error);
}

/*
 * Like copyin(), but operates on an arbitrary process.
 */
int
copyin_proc(struct proc *p, const void *uaddr, void *kaddr, size_t len)
{
	struct vmspace *vm;
	int error;

	error = proc_vmspace_getref(p, &vm);
	if (error) {
		return error;
	}
	error = copyin_vmspace(vm, uaddr, kaddr, len);
	uvmspace_free(vm);

	return error;
}

/*
 * Like copyout(), but operates on an arbitrary process.
 */
int
copyout_proc(struct proc *p, const void *kaddr, void *uaddr, size_t len)
{
	struct vmspace *vm;
	int error;

	error = proc_vmspace_getref(p, &vm);
	if (error) {
		return error;
	}
	error = copyout_vmspace(vm, kaddr, uaddr, len);
	uvmspace_free(vm);

	return error;
}

/*
 * Like copyin(), except it operates on kernel addresses when the FKIOCTL
 * flag is passed in `ioctlflags' from the ioctl call.
 */
int
ioctl_copyin(int ioctlflags, const void *src, void *dst, size_t len)
{
	if (ioctlflags & FKIOCTL)
		return kcopy(src, dst, len);
	return copyin(src, dst, len);
}

/*
 * Like copyout(), except it operates on kernel addresses when the FKIOCTL
 * flag is passed in `ioctlflags' from the ioctl call.
 */
int
ioctl_copyout(int ioctlflags, const void *src, void *dst, size_t len)
{
	if (ioctlflags & FKIOCTL)
		return kcopy(src, dst, len);
	return copyout(src, dst, len);
}

/*
 * General routine to allocate a hash table.
 * Allocate enough memory to hold at least `elements' list-head pointers.
 * Return a pointer to the allocated space and set *hashmask to a pattern
 * suitable for masking a value to use as an index into the returned array.
 */
void *
hashinit(u_int elements, enum hashtype htype, struct malloc_type *mtype,
    int mflags, u_long *hashmask)
{
	u_long hashsize, i;
	LIST_HEAD(, generic) *hashtbl_list;
	TAILQ_HEAD(, generic) *hashtbl_tailq;
	size_t esize;
	void *p;

	if (elements == 0)
		panic("hashinit: bad cnt");
	for (hashsize = 1; hashsize < elements; hashsize <<= 1)
		continue;

	switch (htype) {
	case HASH_LIST:
		esize = sizeof(*hashtbl_list);
		break;
	case HASH_TAILQ:
		esize = sizeof(*hashtbl_tailq);
		break;
	default:
#ifdef DIAGNOSTIC
		panic("hashinit: invalid table type");
#else
		return NULL;
#endif
	}

	if ((p = malloc(hashsize * esize, mtype, mflags)) == NULL)
		return (NULL);

	switch (htype) {
	case HASH_LIST:
		hashtbl_list = p;
		for (i = 0; i < hashsize; i++)
			LIST_INIT(&hashtbl_list[i]);
		break;
	case HASH_TAILQ:
		hashtbl_tailq = p;
		for (i = 0; i < hashsize; i++)
			TAILQ_INIT(&hashtbl_tailq[i]);
		break;
	}
	*hashmask = hashsize - 1;
	return (p);
}

/*
 * Free memory from hash table previosly allocated via hashinit().
 */
void
hashdone(void *hashtbl, struct malloc_type *mtype)
{

	free(hashtbl, mtype);
}


static void *
hook_establish(hook_list_t *list, void (*fn)(void *), void *arg)
{
	struct hook_desc *hd;

	hd = malloc(sizeof(*hd), M_DEVBUF, M_NOWAIT);
	if (hd == NULL)
		return (NULL);

	hd->hk_fn = fn;
	hd->hk_arg = arg;
	LIST_INSERT_HEAD(list, hd, hk_list);

	return (hd);
}

static void
hook_disestablish(hook_list_t *list, void *vhook)
{
#ifdef DIAGNOSTIC
	struct hook_desc *hd;

	LIST_FOREACH(hd, list, hk_list) {
                if (hd == vhook)
			break;
	}

	if (hd == NULL)
		panic("hook_disestablish: hook %p not established", vhook);
#endif
	LIST_REMOVE((struct hook_desc *)vhook, hk_list);
	free(vhook, M_DEVBUF);
}

static void
hook_destroy(hook_list_t *list)
{
	struct hook_desc *hd;

	while ((hd = LIST_FIRST(list)) != NULL) {
		LIST_REMOVE(hd, hk_list);
		free(hd, M_DEVBUF);
	}
}

static void
hook_proc_run(hook_list_t *list, struct proc *p)
{
	struct hook_desc *hd;

	for (hd = LIST_FIRST(list); hd != NULL; hd = LIST_NEXT(hd, hk_list)) {
		((void (*)(struct proc *, void *))*hd->hk_fn)(p,
		    hd->hk_arg);
	}
}

/*
 * "Shutdown hook" types, functions, and variables.
 *
 * Should be invoked immediately before the
 * system is halted or rebooted, i.e. after file systems unmounted,
 * after crash dump done, etc.
 *
 * Each shutdown hook is removed from the list before it's run, so that
 * it won't be run again.
 */

static hook_list_t shutdownhook_list;

void *
shutdownhook_establish(void (*fn)(void *), void *arg)
{
	return hook_establish(&shutdownhook_list, fn, arg);
}

void
shutdownhook_disestablish(void *vhook)
{
	hook_disestablish(&shutdownhook_list, vhook);
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
doshutdownhooks(void)
{
	struct hook_desc *dp;

	while ((dp = LIST_FIRST(&shutdownhook_list)) != NULL) {
		LIST_REMOVE(dp, hk_list);
		(*dp->hk_fn)(dp->hk_arg);
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

static hook_list_t mountroothook_list;

void *
mountroothook_establish(void (*fn)(struct device *), struct device *dev)
{
	return hook_establish(&mountroothook_list, (void (*)(void *))fn, dev);
}

void
mountroothook_disestablish(void *vhook)
{
	hook_disestablish(&mountroothook_list, vhook);
}

void
mountroothook_destroy(void)
{
	hook_destroy(&mountroothook_list);
}

void
domountroothook(void)
{
	struct hook_desc *hd;

	LIST_FOREACH(hd, &mountroothook_list, hk_list) {
		if (hd->hk_arg == (void *)root_device) {
			(*hd->hk_fn)(hd->hk_arg);
			return;
		}
	}
}

static hook_list_t exechook_list;

void *
exechook_establish(void (*fn)(struct proc *, void *), void *arg)
{
	return hook_establish(&exechook_list, (void (*)(void *))fn, arg);
}

void
exechook_disestablish(void *vhook)
{
	hook_disestablish(&exechook_list, vhook);
}

/*
 * Run exec hooks.
 */
void
doexechooks(struct proc *p)
{
	hook_proc_run(&exechook_list, p);
}

static hook_list_t exithook_list;

void *
exithook_establish(void (*fn)(struct proc *, void *), void *arg)
{
	return hook_establish(&exithook_list, (void (*)(void *))fn, arg);
}

void
exithook_disestablish(void *vhook)
{
	hook_disestablish(&exithook_list, vhook);
}

/*
 * Run exit hooks.
 */
void
doexithooks(struct proc *p)
{
	hook_proc_run(&exithook_list, p);
}

static hook_list_t forkhook_list;

void *
forkhook_establish(void (*fn)(struct proc *, struct proc *))
{
	return hook_establish(&forkhook_list, (void (*)(void *))fn, NULL);
}

void
forkhook_disestablish(void *vhook)
{
	hook_disestablish(&forkhook_list, vhook);
}

/*
 * Run fork hooks.
 */
void
doforkhooks(struct proc *p2, struct proc *p1)
{
	struct hook_desc *hd;

	LIST_FOREACH(hd, &forkhook_list, hk_list) {
		((void (*)(struct proc *, struct proc *))*hd->hk_fn)
		    (p2, p1);
	}
}

/*
 * "Power hook" types, functions, and variables.
 * The list of power hooks is kept ordered with the last registered hook
 * first.
 * When running the hooks on power down the hooks are called in reverse
 * registration order, when powering up in registration order.
 */
struct powerhook_desc {
	CIRCLEQ_ENTRY(powerhook_desc) sfd_list;
	void	(*sfd_fn)(int, void *);
	void	*sfd_arg;
	char	sfd_name[16];
};

static CIRCLEQ_HEAD(, powerhook_desc) powerhook_list =
    CIRCLEQ_HEAD_INITIALIZER(powerhook_list);

void *
powerhook_establish(const char *name, void (*fn)(int, void *), void *arg)
{
	struct powerhook_desc *ndp;

	ndp = (struct powerhook_desc *)
	    malloc(sizeof(*ndp), M_DEVBUF, M_NOWAIT);
	if (ndp == NULL)
		return (NULL);

	ndp->sfd_fn = fn;
	ndp->sfd_arg = arg;
	strlcpy(ndp->sfd_name, name, sizeof(ndp->sfd_name));
	CIRCLEQ_INSERT_HEAD(&powerhook_list, ndp, sfd_list);

	return (ndp);
}

void
powerhook_disestablish(void *vhook)
{
#ifdef DIAGNOSTIC
	struct powerhook_desc *dp;

	CIRCLEQ_FOREACH(dp, &powerhook_list, sfd_list)
                if (dp == vhook)
			goto found;
	panic("powerhook_disestablish: hook %p not established", vhook);
 found:
#endif

	CIRCLEQ_REMOVE(&powerhook_list, (struct powerhook_desc *)vhook,
	    sfd_list);
	free(vhook, M_DEVBUF);
}

/*
 * Run power hooks.
 */
void
dopowerhooks(int why)
{
	struct powerhook_desc *dp;

#ifdef POWERHOOK_DEBUG
	printf("dopowerhooks ");
	switch (why) {
	case PWR_RESUME:
		printf("resume");
		break;
	case PWR_SOFTRESUME:
		printf("softresume");
		break;
	case PWR_SUSPEND:
		printf("suspend");
		break;
	case PWR_SOFTSUSPEND:
		printf("softsuspend");
		break;
	case PWR_STANDBY:
		printf("standby");
		break;
	}
	printf(":");
#endif

	if (why == PWR_RESUME || why == PWR_SOFTRESUME) {
		CIRCLEQ_FOREACH_REVERSE(dp, &powerhook_list, sfd_list) {
#ifdef POWERHOOK_DEBUG
			printf(" %s", dp->sfd_name);
#endif
			(*dp->sfd_fn)(why, dp->sfd_arg);
		}
	} else {
		CIRCLEQ_FOREACH(dp, &powerhook_list, sfd_list) {
#ifdef POWERHOOK_DEBUG
			printf(" %s", dp->sfd_name);
#endif
			(*dp->sfd_fn)(why, dp->sfd_arg);
		}
	}

#ifdef POWERHOOK_DEBUG
	printf(".\n");
#endif
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
extern struct cfdriver md_cd;
#endif

#ifdef MEMORY_DISK_IS_ROOT
#define BOOT_FROM_MEMORY_HOOKS 1
#endif

#include "raid.h"
#if NRAID == 1
#define BOOT_FROM_RAID_HOOKS 1
#endif

#ifdef BOOT_FROM_RAID_HOOKS
extern int numraid;
extern struct device *raidrootdev;
#endif

/*
 * The device and wedge that we booted from.  If booted_wedge is NULL,
 * the we might consult booted_partition.
 */
struct device *booted_device;
struct device *booted_wedge;
int booted_partition;

/*
 * Use partition letters if it's a disk class but not a wedge.
 * XXX Check for wedge is kinda gross.
 */
#define	DEV_USES_PARTITIONS(dv)						\
	(device_class((dv)) == DV_DISK &&				\
	 !device_is_a((dv), "dk"))

void
setroot(struct device *bootdv, int bootpartition)
{
	struct device *dv;
	int len;
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

#ifdef MEMORY_DISK_HOOKS
	for (i = 0; i < NMD; i++) {
		fakemdrootdev[i].dv_class  = DV_DISK;
		fakemdrootdev[i].dv_cfdata = NULL;
		fakemdrootdev[i].dv_cfdriver = &md_cd;
		fakemdrootdev[i].dv_unit   = i;
		fakemdrootdev[i].dv_parent = NULL;
		snprintf(fakemdrootdev[i].dv_xname,
		    sizeof(fakemdrootdev[i].dv_xname), "md%d", i);
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
	    (bootdv == NULL || device_class(bootdv) != DV_IFNET)) {
		IFNET_FOREACH(ifp) {
			if ((ifp->if_flags &
			     (IFF_LOOPBACK|IFF_POINTOPOINT)) == 0)
				break;
		}
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
				if (DEV_USES_PARTITIONS(bootdv))
					printf("%c", bootpartition + 'a');
				printf(")");
			}
			printf(": ");
			len = cngetsn(buf, sizeof(buf));
			if (len == 0 && bootdv != NULL) {
				strlcpy(buf, bootdv->dv_xname, sizeof(buf));
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
		if (DEV_USES_PARTITIONS(rootdv) == 0)
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
			len = cngetsn(buf, sizeof(buf));
			if (len == 0) {
				if (defdumpdv != NULL) {
					ndumpdev = MAKEDISKDEV(major(nrootdev),
					    DISKUNIT(nrootdev), 1);
				}
				dumpdv = defdumpdv;
				break;
			}
			if (len == 4 && strcmp(buf, "none") == 0) {
				dumpdv = NULL;
				break;
			}
			dv = getdisk(buf, len, 1, &ndumpdev, 1);
			if (dv != NULL) {
				dumpdv = dv;
				break;
			}
		}

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
			len = cngetsn(buf, sizeof(buf));
			if (len == 0)
				break;
			if (len == 4 && strcmp(buf, "halt") == 0)
				cpu_reboot(RB_HALT, NULL);
			else if (len == 6 && strcmp(buf, "reboot") == 0)
				cpu_reboot(0, NULL);
#if defined(DDB)
			else if (len == 3 && strcmp(buf, "ddb") == 0) {
				console_debugger();
			}
#endif
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
#if defined(DDB)
				printf(" ddb");
#endif
				printf(" halt reboot\n");
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

		majdev = devsw_name2blk(bootdv->dv_xname, NULL, 0);
		if (majdev >= 0) {
			/*
			 * Root is on a disk.  `bootpartition' is root,
			 * unless the device does not use partitions.
			 */
			if (DEV_USES_PARTITIONS(bootdv))
				rootdev = MAKEDISKDEV(majdev,
						      device_unit(bootdv),
						      bootpartition);
			else
				rootdev = makedev(majdev, device_unit(bootdv));
		}
	} else {

		/*
		 * `root on <dev> ...'
		 */

		/*
		 * If it's a network interface, we can bail out
		 * early.
		 */
		dv = finddevice(rootspec);
		if (dv != NULL && device_class(dv) == DV_IFNET) {
			rootdv = dv;
			goto haveroot;
		}

		rootdevname = devsw_blk2name(major(rootdev));
		if (rootdevname == NULL) {
			printf("unknown device major 0x%x\n", rootdev);
			boothowto |= RB_ASKNAME;
			goto top;
		}
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "%s%d", rootdevname,
		    DISKUNIT(rootdev));

		rootdv = finddevice(buf);
		if (rootdv == NULL) {
			printf("device %s (0x%x) not configured\n",
			    buf, rootdev);
			boothowto |= RB_ASKNAME;
			goto top;
		}
	}

 haveroot:

	root_device = rootdv;

	switch (device_class(rootdv)) {
	case DV_IFNET:
	case DV_DISK:
		aprint_normal("root on %s", rootdv->dv_xname);
		if (DEV_USES_PARTITIONS(rootdv))
			aprint_normal("%c", DISKPART(rootdev) + 'a');
		break;

	default:
		printf("can't determine root device\n");
		boothowto |= RB_ASKNAME;
		goto top;
	}

	/*
	 * Now configure the dump device.
	 *
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

	if (boothowto & RB_ASKNAME) {		/* (a) */
		if (dumpdv == NULL)
			goto nodumpdev;
	} else if (dumpspec != NULL) {		/* (b) */
		if (strcmp(dumpspec, "none") == 0 || dumpdev == NODEV) {
			/*
			 * Operator doesn't want a dump device.
			 * Or looks like they tried to pick a network
			 * device.  Oops.
			 */
			goto nodumpdev;
		}

		dumpdevname = devsw_blk2name(major(dumpdev));
		if (dumpdevname == NULL)
			goto nodumpdev;
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "%s%d", dumpdevname,
		    DISKUNIT(dumpdev));

		dumpdv = finddevice(buf);
		if (dumpdv == NULL) {
			/*
			 * Device not configured.
			 */
			goto nodumpdev;
		}
	} else {				/* (c) */
		if (DEV_USES_PARTITIONS(rootdv) == 0)
			goto nodumpdev;
		else {
			dumpdv = rootdv;
			dumpdev = MAKEDISKDEV(major(rootdev),
			    device_unit(dumpdv), 1);
		}
	}

	aprint_normal(" dumps on %s", dumpdv->dv_xname);
	if (DEV_USES_PARTITIONS(dumpdv))
		aprint_normal("%c", DISKPART(dumpdev) + 'a');
	aprint_normal("\n");
	return;

 nodumpdev:
	dumpdev = NODEV;
	aprint_normal("\n");
}

static struct device *
finddevice(const char *name)
{
	struct device *dv;
#if defined(BOOT_FROM_RAID_HOOKS) || defined(BOOT_FROM_MEMORY_HOOKS)
	int j;
#endif /* BOOT_FROM_RAID_HOOKS || BOOT_FROM_MEMORY_HOOKS */

#ifdef BOOT_FROM_RAID_HOOKS
	for (j = 0; j < numraid; j++) {
		if (strcmp(name, raidrootdev[j].dv_xname) == 0) {
			dv = &raidrootdev[j];
			return (dv);
		}
	}
#endif /* BOOT_FROM_RAID_HOOKS */

#ifdef BOOT_FROM_MEMORY_HOOKS
	for (j = 0; j < NMD; j++) {
		if (strcmp(name, fakemdrootdev[j].dv_xname) == 0) {
			dv = &fakemdrootdev[j];
			return (dv);
		}
	}
#endif /* BOOT_FROM_MEMORY_HOOKS */

	for (dv = TAILQ_FIRST(&alldevs); dv != NULL;
	    dv = TAILQ_NEXT(dv, dv_list))
		if (strcmp(dv->dv_xname, name) == 0)
			break;
	return (dv);
}

static struct device *
getdisk(char *str, int len, int defpart, dev_t *devp, int isdump)
{
	struct device	*dv;
#ifdef MEMORY_DISK_HOOKS
	int		i;
#endif
#ifdef BOOT_FROM_RAID_HOOKS
	int 		j;
#endif

	if ((dv = parsedisk(str, len, defpart, devp)) == NULL) {
		printf("use one of:");
#ifdef MEMORY_DISK_HOOKS
		if (isdump == 0)
			for (i = 0; i < NMD; i++)
				printf(" %s[a-%c]", fakemdrootdev[i].dv_xname,
				    'a' + MAXPARTITIONS - 1);
#endif
#ifdef BOOT_FROM_RAID_HOOKS
		if (isdump == 0)
			for (j = 0; j < numraid; j++)
				printf(" %s[a-%c]", raidrootdev[j].dv_xname,
				    'a' + MAXPARTITIONS - 1);
#endif
		TAILQ_FOREACH(dv, &alldevs, dv_list) {
			if (DEV_USES_PARTITIONS(dv))
				printf(" %s[a-%c]", dv->dv_xname,
				    'a' + MAXPARTITIONS - 1);
			else if (device_class(dv) == DV_DISK)
				printf(" %s", dv->dv_xname);
			if (isdump == 0 && device_class(dv) == DV_IFNET)
				printf(" %s", dv->dv_xname);
		}
		if (isdump)
			printf(" none");
#if defined(DDB)
		printf(" ddb");
#endif
		printf(" halt reboot\n");
	}
	return (dv);
}

static struct device *
parsedisk(char *str, int len, int defpart, dev_t *devp)
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
	else if (len == 6 && strcmp(str, "reboot") == 0)
		cpu_reboot(0, NULL);
#if defined(DDB)
	else if (len == 3 && strcmp(str, "ddb") == 0)
		console_debugger();
#endif

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

	dv = finddevice(str);
	if (dv != NULL) {
		if (device_class(dv) == DV_DISK) {
#ifdef MEMORY_DISK_HOOKS
 gotdisk:
#endif
			majdev = devsw_name2blk(dv->dv_xname, NULL, 0);
			if (majdev < 0)
				panic("parsedisk");
			if (DEV_USES_PARTITIONS(dv))
				*devp = MAKEDISKDEV(majdev, device_unit(dv),
						    part);
			else
				*devp = makedev(majdev, device_unit(dv));
		}

		if (device_class(dv) == DV_IFNET)
			*devp = NODEV;
	}

	*cp = c;
	return (dv);
}

/*
 * snprintf() `bytes' into `buf', reformatting it so that the number,
 * plus a possible `x' + suffix extension) fits into len bytes (including
 * the terminating NUL).
 * Returns the number of bytes stored in buf, or -1 if there was a problem.
 * E.g, given a len of 9 and a suffix of `B':
 *	bytes		result
 *	-----		------
 *	99999		`99999 B'
 *	100000		`97 kB'
 *	66715648	`65152 kB'
 *	252215296	`240 MB'
 */
int
humanize_number(char *buf, size_t len, uint64_t bytes, const char *suffix,
    int divisor)
{
       	/* prefixes are: (none), kilo, Mega, Giga, Tera, Peta, Exa */
	const char *prefixes;
	int		r;
	uint64_t	umax;
	size_t		i, suffixlen;

	if (buf == NULL || suffix == NULL)
		return (-1);
	if (len > 0)
		buf[0] = '\0';
	suffixlen = strlen(suffix);
	/* check if enough room for `x y' + suffix + `\0' */
	if (len < 4 + suffixlen)
		return (-1);

	if (divisor == 1024) {
		/*
		 * binary multiplies
		 * XXX IEC 60027-2 recommends Ki, Mi, Gi...
		 */
		prefixes = " KMGTPE";
	} else
		prefixes = " kMGTPE"; /* SI for decimal multiplies */

	umax = 1;
	for (i = 0; i < len - suffixlen - 3; i++)
		umax *= 10;
	for (i = 0; bytes >= umax && prefixes[i + 1]; i++)
		bytes /= divisor;

	r = snprintf(buf, len, "%qu%s%c%s", (unsigned long long)bytes,
	    i == 0 ? "" : " ", prefixes[i], suffix);

	return (r);
}

int
format_bytes(char *buf, size_t len, uint64_t bytes)
{
	int	rv;
	size_t	nlen;

	rv = humanize_number(buf, len, bytes, "B", 1024);
	if (rv != -1) {
			/* nuke the trailing ` B' if it exists */
		nlen = strlen(buf) - 2;
		if (strcmp(&buf[nlen], " B") == 0)
			buf[nlen] = '\0';
	}
	return (rv);
}

/*
 * Return TRUE if system call tracing is enabled for the specified process.
 */
boolean_t
trace_is_enabled(struct proc *p)
{
#ifdef SYSCALL_DEBUG
	return (TRUE);
#endif
#ifdef KTRACE
	if (ISSET(p->p_traceflag, (KTRFAC_SYSCALL | KTRFAC_SYSRET)))
		return (TRUE);
#endif
#ifdef SYSTRACE
	if (ISSET(p->p_flag, P_SYSTRACE))
		return (TRUE);
#endif
#ifdef PTRACE
	if (ISSET(p->p_flag, P_SYSCALL))
		return (TRUE);
#endif

	return (FALSE);
}

/*
 * Start trace of particular system call. If process is being traced,
 * this routine is called by MD syscall dispatch code just before
 * a system call is actually executed.
 * MD caller guarantees the passed 'code' is within the supported
 * system call number range for emulation the process runs under.
 */
int
trace_enter(struct lwp *l, register_t code,
    register_t realcode, const struct sysent *callp, void *args)
{
#if defined(SYSCALL_DEBUG) || defined(KTRACE) || defined(PTRACE) || defined(SYSTRACE)
	struct proc *p = l->l_proc;

#ifdef SYSCALL_DEBUG
	scdebug_call(l, code, args);
#endif /* SYSCALL_DEBUG */

#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSCALL))
		ktrsyscall(l, code, realcode, callp, args);
#endif /* KTRACE */

#ifdef PTRACE
	if ((p->p_flag & (P_SYSCALL|P_TRACED)) == (P_SYSCALL|P_TRACED))
		process_stoptrace(l);
#endif

#ifdef SYSTRACE
	if (ISSET(p->p_flag, P_SYSTRACE))
		return systrace_enter(l, code, args);
#endif
#endif /* SYSCALL_DEBUG || {K,P,SYS}TRACE */
	return 0;
}

/*
 * End trace of particular system call. If process is being traced,
 * this routine is called by MD syscall dispatch code just after
 * a system call finishes.
 * MD caller guarantees the passed 'code' is within the supported
 * system call number range for emulation the process runs under.
 */
void
trace_exit(struct lwp *l, register_t code, void *args, register_t rval[],
    int error)
{
#if defined(SYSCALL_DEBUG) || defined(KTRACE) || defined(PTRACE) || defined(SYSTRACE)
	struct proc *p = l->l_proc;

#ifdef SYSCALL_DEBUG
	scdebug_ret(l, code, error, rval);
#endif /* SYSCALL_DEBUG */

#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET)) {
		KERNEL_PROC_LOCK(l);
		ktrsysret(l, code, error, rval);
		KERNEL_PROC_UNLOCK(l);
	}
#endif /* KTRACE */
	
#ifdef PTRACE
	if ((p->p_flag & (P_SYSCALL|P_TRACED)) == (P_SYSCALL|P_TRACED))
		process_stoptrace(l);
#endif

#ifdef SYSTRACE
	if (ISSET(p->p_flag, P_SYSTRACE)) {
		KERNEL_PROC_LOCK(l);
		systrace_exit(l, code, args, rval, error);
		KERNEL_PROC_UNLOCK(l);
	}
#endif
#endif /* SYSCALL_DEBUG || {K,P,SYS}TRACE */
}
