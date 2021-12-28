/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
 */
/*
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2011 Nexenta Systems, Inc.  All rights reserved.
 * Copyright (c) 2013 by Delphix. All rights reserved.
 */

#ifndef _SYS_ZFS_CONTEXT_H
#define	_SYS_ZFS_CONTEXT_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/param.h>
#include <sys/stdint.h>
#include <sys/note.h>
#include <sys/kernel.h>
#include <sys/debug.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/sysmacros.h>
#include <sys/bitmap.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/taskq.h>
#ifndef __NetBSD__
#include <sys/taskqueue.h>
#endif
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mutex.h>
#include <sys/rwlock.h>
#include <sys/kcondvar.h>
#include <sys/random.h>
#include <sys/byteorder.h>
#include <sys/systm.h>
#include <sys/list.h>
#include <sys/zfs_debug.h>
#include <sys/sysevent.h>
#include <sys/uio.h>
#include <sys/dirent.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/fcntl.h>
#ifndef __NetBSD__
#include <sys/limits.h>
#else
#include <sys/syslimits.h>
#endif
#include <sys/string.h>
#ifndef __NetBSD__
#include <sys/bio.h>
#endif
#include <sys/buf.h>
#include <sys/cred.h>
#include <sys/sdt.h>
#include <sys/file.h>
#include <sys/vfs.h>
#include <sys/sysctl.h>
#ifndef __NetBSD__
#include <sys/sbuf.h>
#include <sys/priv.h>
#include <sys/kdb.h>
#include <sys/ktr.h>
#include <sys/stack.h>
#endif
#include <sys/lockf.h>
#include <sys/pathname.h>
#include <sys/policy.h>
#include <sys/refstr.h>
#include <sys/zone.h>
#ifndef __NetBSD__
#include <sys/eventhandler.h>
#endif
#include <sys/extattr.h>
#include <sys/misc.h>
#ifndef __NetBSD__
#include <sys/sig.h>
#include <sys/osd.h>
#endif
#include <sys/sysevent/dev.h>
#include <sys/sysevent/eventdefs.h>
#include <sys/u8_textprep.h>
#include <sys/fm/util.h>
#include <sys/sunddi.h>
#if defined(illumos) || defined(__NetBSD__)
#include <sys/cyclic.h>
#endif
#ifndef __NetBSD__
#include <sys/callo.h>
#include <sys/disp.h>
#include <machine/stdarg.h>

#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_object.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
/* There is clash. vm_map.h defines the two below and vdev_cache.c use them. */
#ifdef min_offset
#undef min_offset
#endif
#ifdef max_offset
#undef max_offset
#endif
#include <vm/vm_extern.h>
#include <vm/vnode_pager.h>
#else /* !__NetBSD__ */
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/stdarg.h>

#include <miscfs/specfs/specdev.h>
#endif /* !__NetBSD__ */

#ifndef __NetBSD__

#define	CPU_SEQID	(curcpu)

#define	tsd_create(keyp, destructor)	do {				\
	*(keyp) = osd_thread_register((destructor));			\
	KASSERT(*(keyp) > 0, ("cannot register OSD"));			\
} while (0)
#define	tsd_destroy(keyp)		osd_thread_deregister(*(keyp))
#define	tsd_get(key)			osd_thread_get(curthread, (key))
#define	tsd_set(key, value)		osd_thread_set(curthread, (key), (value))

#else /* !__NetBSD__ */

#define ASSERT_VOP_LOCKED(vp, name)	KASSERT(VOP_ISLOCKED(vp) != 0)
#define ASSERT_VOP_ELOCKED(vp, name)	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE)
#define callout_drain(x)		callout_halt(x, NULL)
#define CPU_SEQID			(curcpu()->ci_data.cpu_index)
#define FIGNORECASE			0
#define fm_panic			panic
#define getf				fd_getfile
#define getminor(a)			minor(a)
#define GID_NOBODY			(39)
#define issig(x)			(sigispending(curlwp, 0))
#define kmem_debugging()		0
#define releasef			fd_putfile
#define strfree(str)			kmem_free((str), strlen(str)+1)
#define td_ru				l_ru
#define UID_NOBODY			(32767)
#define vnode_pager_setsize(vp, size)	zfs_netbsd_setsize(vp, size)
#define zone_get_hostid(a)		((unsigned)hostid)

extern struct utsname utsname;

void zfs_netbsd_setsize(vnode_t *, off_t);

static inline int
sprintf(char * __restrict buf, const char * __restrict fmt, ...)
{
	va_list ap;
	int rv;

	va_start(ap, fmt);
	rv = vsnprintf(buf, 1024, fmt, ap);
	va_end(ap);
	return rv;
}

static inline void
tsd_create(uint_t *keyp, void (*func)(void *))
{
	int error __unused;

	error = lwp_specific_key_create(keyp, func);
	KASSERT(error == 0);
}

static inline void
tsd_destroy(uint_t *keyp)
{

	lwp_specific_key_delete(*keyp);
}

static inline void *
tsd_get(uint_t key)
{

	return lwp_getspecific(key);
}

static inline int
tsd_set(uint_t key, void *value)
{

	lwp_setspecific(key, value);
	return 0;	
}

static inline int
vsprintf(char * __restrict buf, const char * __restrict fmt, va_list ap)
{

	return vsnprintf(buf, 1024, fmt, ap);
}

#define SYSCTL_DECL(...)
#define SYSCTL_NODE(...)
#define SYSCTL_INT(...)
#define SYSCTL_UINT(...)
#define SYSCTL_LONG(...)
#define SYSCTL_ULONG(...)
#define SYSCTL_QUAD(...)
#define SYSCTL_UQUAD(...)
#ifdef TUNABLE_INT
#undef TUNABLE_INT
#undef TUNABLE_ULONG
#undef TUNABLE_INT_FETCH
#endif
#define TUNABLE_INT(...)
#define TUNABLE_ULONG(...)
#define TUNABLE_INT_FETCH(...)		0

#endif /* !__NetBSD__ */

#ifdef	__cplusplus
}
#endif

extern int zfs_debug_level;
#ifndef __NetBSD__

extern struct mtx zfs_debug_mtx;
#define	ZFS_LOG(lvl, ...)	do {					\
	if (((lvl) & 0xff) <= zfs_debug_level) {			\
		mtx_lock(&zfs_debug_mtx);				\
		printf("%s:%u[%d]: ", __func__, __LINE__, (lvl));	\
		printf(__VA_ARGS__);					\
		printf("\n");						\
		if ((lvl) & 0x100)					\
			kdb_backtrace();				\
		mtx_unlock(&zfs_debug_mtx);				\
	}								\
} while (0)

#define	sys_shutdown	rebooting

#else /* !__NetBSD__ */

extern kmutex_t zfs_debug_mtx;
#define	ZFS_LOG(lvl, ...)	do {					\
	if (((lvl) & 0xff) <= zfs_debug_level) {			\
		mutex_enter(&zfs_debug_mtx);				\
		printf("%s:%u[%d]: ", __func__, __LINE__, (lvl));	\
		printf(__VA_ARGS__);					\
		printf("\n");						\
		if ((lvl) & 0x100)					\
			/* XXXNETBSD backtrace */;				\
		mutex_exit(&zfs_debug_mtx);				\
	}								\
} while (0)

#define	sys_shutdown	0

#endif /* !__NetBSD__ */

#endif	/* _SYS_ZFS_CONTEXT_H */
