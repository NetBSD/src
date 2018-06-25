/*	$NetBSD: zfs_context.h,v 1.18.12.1 2018/06/25 07:25:26 pgoyette Exp $	*/

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SYS_ZFS_CONTEXT_H
#define	_SYS_ZFS_CONTEXT_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/types.h>

#ifndef _KERNEL

#define _SYS_MUTEX_H
#define _SYS_MUTEX_H_
#define _SYS_RWLOCK_H
#define _SYS_RWLOCK_H_
#define _SYS_CONDVAR_H
#define _SYS_CONDVAR_H_
#define	_SYS_SYSTM_H
#define	_SYS_SYSTM_H_
#define	_SYS_T_LOCK_H
#define	_SYS_VNODE_H
#define	_SYS_VFS_H
#define	_SYS_SUNDDI_H
#define	_SYS_CALLB_H

#include <solaris.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <thread.h>
#include <assert.h>
#include <dirent.h>
#include <time.h>
#include <math.h>
#include <umem.h>
#include <inttypes.h>
#include <fsshare.h>
#include <pthread.h>
#include <paths.h>

#include <sys/debug.h>
#include <sys/proc.h>
#include <sys/kstat.h>
#include <sys/param.h>
#include <sys/stdint.h>
#include <sys/note.h>
#include <sys/uio.h>
#include <sys/fcntl.h>
#include <sys/atomic.h>
#include <sys/misc.h>
#include <sys/sdt.h>
#include <sys/list.h>
#include <sys/ccompile.h>
#include <sys/byteorder.h>
#include <sys/utsname.h>
#include <sys/cred.h>
#include <sys/u8_textprep.h>
#include <sys/zone.h>
#include <sys/pathname.h>
#include <sys/sysevent.h>
#include <sys/zfs_debug.h>
#include <sys/cyclic.h>

extern void panic(const char *, ...);

#include <sys/cmn_err.h>

#include <sys/sysevent/eventdefs.h>

#ifndef ABS
#define	ABS(a) ((a) < 0 ? -(a) : (a))
#endif

extern int aok;

/*
 * Debugging
 */

/*
 * Note that we are not using the debugging levels.
 */

#define	CE_CONT		0	/* continuation		*/
#define	CE_NOTE		1	/* notice		*/
#define	CE_WARN		2	/* warning		*/
#define	CE_PANIC	3	/* panic		*/
#define	CE_IGNORE	4	/* print nothing	*/

/*
 * ZFS debugging
 */

#define	ZFS_LOG(...)	do {  } while (0)

typedef u_longlong_t      rlim64_t;
#define	RLIM64_INFINITY	((rlim64_t)-3)

#ifdef ZFS_DEBUG
extern void dprintf_setup(int *argc, char **argv);
#endif /* ZFS_DEBUG */

/* extern void cmn_err(int, const char *, ...);
extern void vcmn_err(int, const char *, va_list); */
/* extern void vpanic(const char *, va_list); */

#define	fm_panic	panic

/*
 * Dtrace SDT probes have different signatures in userland than they do in
 * kernel.  If they're being used in kernel code, re-define them out of
 * existence for their counterparts in libzpool.
 */

#ifdef DTRACE_PROBE1
#undef	DTRACE_PROBE1
#define	DTRACE_PROBE1(a, b, c)	((void)0)
#endif	/* DTRACE_PROBE1 */

#ifdef DTRACE_PROBE2
#undef	DTRACE_PROBE2
#define	DTRACE_PROBE2(a, b, c, d, e)	((void)0)
#endif	/* DTRACE_PROBE2 */

#ifdef DTRACE_PROBE3
#undef	DTRACE_PROBE3
#define	DTRACE_PROBE3(a, b, c, d, e, f, g)	((void)0)
#endif	/* DTRACE_PROBE3 */

#ifdef DTRACE_PROBE4
#undef	DTRACE_PROBE4
#define	DTRACE_PROBE4(a, b, c, d, e, f, g, h, i)	((void)0)
#endif	/* DTRACE_PROBE4 */

/*
 * Threads
 */
#define	curthread	((void *)(uintptr_t)thr_self())

#define	kpreempt(x)	yield()

typedef struct kthread kthread_t;

#define	thread_create(stk, stksize, func, arg, len, pp, state, pri)	\
	zk_thread_create(func, arg)
#define	thread_exit() thr_exit(NULL)
#define	thread_join(t)	panic("libzpool cannot join threads")

#define	newproc(f, a, cid, pri, ctp, pid)	(ENOSYS)

/* in libzpool, p0 exists only to have its address taken */
struct proc {
	uintptr_t	this_is_never_used_dont_dereference_it;
};

extern struct proc p0;
#define	curproc		(&p0)

#define	PS_NONE		-1

extern kthread_t *zk_thread_create(void (*func)(), void *arg);

#define	issig(why)	(FALSE)
#define	ISSIG(thr, why)	(FALSE)

/*
 * Mutexes
 */

typedef struct kmutex {
	void		*m_owner;
	boolean_t	initialized;
	mutex_t		m_lock;
} kmutex_t;

#define	MUTEX_DEFAULT	USYNC_THREAD
#undef	MUTEX_HELD
#undef	MUTEX_NOT_HELD
#define	MUTEX_HELD(m)	((m)->m_owner == curthread)
#define	MUTEX_NOT_HELD(m) (!MUTEX_HELD(m))
#define	_mutex_held(m)	pthread_mutex_isowned_np(m)

/*
 * Argh -- we have to get cheesy here because the kernel and userland
 * have different signatures for the same routine.
 */
//extern int _mutex_init(mutex_t *mp, int type, void *arg);
//extern int _mutex_destroy(mutex_t *mp);
//extern int _mutex_owned(mutex_t *mp);

#define	mutex_init(mp, b, c, d)		zmutex_init((kmutex_t *)(mp))
#define	mutex_destroy(mp)		zmutex_destroy((kmutex_t *)(mp))
#define	mutex_enter(mp)			zmutex_enter(mp)
#define	mutex_exit(mp)			zmutex_exit(mp)
#define	mutex_owned(mp)			zmutex_owned((kmutex_t *)(mp))

extern void zmutex_init(kmutex_t *mp);
extern void zmutex_destroy(kmutex_t *mp);
extern void zmutex_enter(kmutex_t *mp);
extern void zmutex_exit(kmutex_t *mp);
extern int mutex_tryenter(kmutex_t *mp);
extern void *mutex_owner(kmutex_t *mp);

/*
 * RW locks
 */
typedef struct krwlock {
        int             rw_count;
	void		*rw_owner;
	boolean_t	initialized;
	rwlock_t	rw_lock;
} krwlock_t;

typedef int krw_t;

#define	RW_READER	0
#define	RW_WRITER	1
#define	RW_DEFAULT	USYNC_THREAD

#define	RW_LOCK_HELD(x)		rw_lock_held(x)
#define	RW_WRITE_HELD(x)	rw_write_held(x)
#define	RW_READ_HELD(x)		rw_read_held(x)

#undef RW_READ_HELD
#define RW_READ_HELD(x)		((x)->rw_owner == NULL && (x)->rw_count > 0)

#undef RW_WRITE_HELD
#define	RW_WRITE_HELD(x)	((x)->rw_owner == curthread)

#undef RW_LOCK_HELD
#define	RW_LOCK_HELD(x)		(RW_READ_HELD(x) || RW_WRITE_HELD(x))

extern void rw_init(krwlock_t *rwlp, char *name, int type, void *arg);
extern void rw_destroy(krwlock_t *rwlp);
extern void rw_enter(krwlock_t *rwlp, krw_t rw);
extern int rw_tryenter(krwlock_t *rwlp, krw_t rw);
extern int rw_tryupgrade(krwlock_t *rwlp);
extern void rw_exit(krwlock_t *rwlp);
extern int rw_lock_held(krwlock_t *rwlp);
extern int rw_write_held(krwlock_t *rwlp);
extern int rw_read_held(krwlock_t *rwlp);
#define	rw_downgrade(rwlp) do { } while (0)

extern uid_t crgetuid(cred_t *cr);
extern gid_t crgetgid(cred_t *cr);
extern int crgetngroups(cred_t *cr);
extern gid_t *crgetgroups(cred_t *cr);
                
/*
 * Condition variables
 */
typedef cond_t kcondvar_t;

#define	CV_DEFAULT	USYNC_THREAD
#define	CALLOUT_FLAG_ABSOLUTE	0x2

extern void cv_init(kcondvar_t *cv, char *name, int type, void *arg);
extern void cv_destroy(kcondvar_t *cv);
extern void cv_wait(kcondvar_t *cv, kmutex_t *mp);
extern clock_t cv_timedwait(kcondvar_t *cv, kmutex_t *mp, clock_t abstime);
extern clock_t cv_timedwait_hires(kcondvar_t *cvp, kmutex_t *mp, hrtime_t tim,
    hrtime_t res, int flag);
extern void cv_signal(kcondvar_t *cv);
extern void cv_broadcast(kcondvar_t *cv);

/*
 * Thread-specific data
 */
#define	tsd_get(k) pthread_getspecific(k)
#define	tsd_set(k, v) pthread_setspecific(k, v)
#define	tsd_create(kp, d) pthread_key_create(kp, d)
#define	tsd_destroy(kp) /* nothing */

/*
 * Kernel memory
 */
#define	KM_SLEEP		UMEM_NOFAIL
#define	KM_PUSHPAGE		KM_SLEEP
#define	KM_NOSLEEP		UMEM_DEFAULT
#define	KMC_NODEBUG		UMC_NODEBUG
#define	KMC_NOTOUCH		0	/* not needed for userland caches */
#define	KM_NODEBUG		0
#define	kmem_alloc(_s, _f)	umem_alloc(_s, _f)
#define	kmem_zalloc(_s, _f)	umem_zalloc(_s, _f)
#define	kmem_free(_b, _s)	umem_free(_b, _s)
#define	kmem_size()		((uint64_t)physmem * PAGESIZE)
#define	kmem_cache_create(_a, _b, _c, _d, _e, _f, _g, _h, _i) \
	umem_cache_create(_a, _b, _c, _d, _e, _f, _g, _h, _i)
#define	kmem_cache_destroy(_c)	umem_cache_destroy(_c)
#define	kmem_cache_alloc(_c, _f) umem_cache_alloc(_c, _f)
#define	kmem_cache_free(_c, _b)	umem_cache_free(_c, _b)
#define	kmem_debugging()	0
#define	kmem_cache_reap_now(_c)		/* nothing */
#define	kmem_cache_set_move(_c, _cb)	/* nothing */
#define	vmem_qcache_reap(_v)		/* nothing */
#define	POINTER_INVALIDATE(_pp)		/* nothing */
#define	POINTER_IS_VALID(_p)	0

typedef umem_cache_t kmem_cache_t;
typedef struct vmem vmem_t;

/*
 * Task queues
 */
typedef struct taskq taskq_t;
typedef uintptr_t taskqid_t;
typedef void (task_func_t)(void *);

typedef struct taskq_ent {
	struct taskq_ent	*tqent_next;
	struct taskq_ent	*tqent_prev;
	task_func_t		*tqent_func;
	void			*tqent_arg;
	uintptr_t		tqent_flags;
} taskq_ent_t;

#define	TQENT_FLAG_PREALLOC	0x1	/* taskq_dispatch_ent used */

#define TASKQ_PREPOPULATE   0x0001
#define TASKQ_CPR_SAFE      0x0002  /* Use CPR safe protocol */
#define TASKQ_DYNAMIC       0x0004  /* Use dynamic thread scheduling */
#define TASKQ_THREADS_CPU_PCT   0x0008  /* Scale # threads by # cpus */
#define TASKQ_DC_BATCH      0x0010  /* Mark threads as batch */

#define	TQ_SLEEP	KM_SLEEP	/* Can block for memory */
#define	TQ_NOSLEEP	KM_NOSLEEP	/* cannot block for memory; may fail */
#define	TQ_NOQUEUE	0x02		/* Do not enqueue if can't dispatch */
#define	TQ_FRONT	0x08		/* Queue in front */


extern taskq_t *system_taskq;

extern taskq_t  *taskq_create(const char *, int, pri_t, int, int, uint_t);
#define taskq_create_proc(a, b, c, d, e, p, f) \
	(taskq_create(a, b, c, d, e, f))
#define taskq_create_sysdc(a, b, d, e, p, dc, f) \
	(taskq_create(a, b, maxclsyspri, d, e, f))
extern taskqid_t taskq_dispatch(taskq_t *, task_func_t, void *, uint_t);
extern void taskq_destroy(taskq_t *);
extern void taskq_wait(taskq_t *);
extern int  taskq_member(taskq_t *, void *);
extern void system_taskq_init(void);
extern void system_taskq_fini(void);

#define	XVA_MAPSIZE	3
#define	XVA_MAGIC	0x78766174

/*
 * vnodes
 */
typedef struct vnode {
	uint64_t	v_size;
	int		v_fd;
	char		*v_path;
	int		v_dump_fd;
} vnode_t;

extern char *vn_dumpdir;
#define	AV_SCANSTAMP_SZ	32		/* length of anti-virus scanstamp */

typedef struct vattr {
	uint_t		va_mask;	/* bit-mask of attributes */
	u_offset_t	va_size;	/* file size in bytes */
} vattr_t;

#define	AT_TYPE		0x0001
#define	AT_MODE		0x0002
#define	AT_UID		0x0004
#define	AT_GID		0x0008
#define	AT_FSID		0x0010
#define	AT_NODEID	0x0020
#define	AT_NLINK	0x0040
#define	AT_SIZE		0x0080
#define	AT_ATIME	0x0100
#define	AT_MTIME	0x0200
#define	AT_CTIME	0x0400
#define	AT_RDEV		0x0800
#define	AT_BLKSIZE	0x1000
#define	AT_NBLOCKS	0x2000
#define	AT_SEQ		0x8000

#define	CRCREAT		0

#define	VOP_CLOSE(vp, f, c, o, cr, unk)	0
#define	VOP_PUTPAGE(vp, of, sz, fl, cr, unk)	0
#define	VOP_GETATTR(vp, vap, fl, cr, unk) fop_getattr(vp, vap)
#define	VOP_FSYNC(vp, f, cr, unk)	fsync((vp)->v_fd)

#define	VN_RELE(vp)	vn_close(vp)
#define	VN_RELE_ASYNC(vp)	vn_close(vp)

#define vn_lock(vp, type)
#define VOP_UNLOCK(vp, type)

extern int vn_open(char *path, int x1, int oflags, int mode, vnode_t **vpp,
    int x2, int x3);
extern int vn_openat(char *path, int x1, int oflags, int mode, vnode_t **vpp,
    int x2, int x3, vnode_t *vp, int unk);
extern int vn_rdwr(int uio, vnode_t *vp, void *addr, ssize_t len,
    offset_t offset, int x1, int x2, rlim64_t x3, void *x4, ssize_t *residp);
extern void vn_close(vnode_t *vp, int, cred_t *, kthread_t *);
extern int vn_getattr(vnode_t *vp, vattr_t *va);

#define	vn_remove(path, x1, x2)		remove(path)
#define	vn_rename(from, to, seg)	rename((from), (to))
#define	vn_is_readonly(vp)		B_FALSE

extern vnode_t *rootdir;

#include <sys/file.h>		/* for FREAD, FWRITE, etc */
#define	FTRUNC	O_TRUNC

/*
 * Random stuff
 */
#define	lbolt	(gethrtime() >> 23)
#define	lbolt64	(gethrtime() >> 23)
#define	hz	119	/* frequency when using gethrtime() >> 23 for lbolt */

extern void delay(clock_t ticks);

#define	gethrestime_sec() time(NULL)
#define gethrestime(t) \
	do {\
	(t)->tv_sec = gethrestime_sec();\
	(t)->tv_nsec = 0;\
	} while (0);

#define	max_ncpus	64

#define	minclsyspri	60
#define	maxclsyspri	99

#define	CPU_SEQID	(thr_self() & (max_ncpus - 1))

#define	kcred		NULL
#define	CRED()		NULL

#ifdef ptob
#undef ptob
#endif
#define	ptob(x)		((x) * PAGESIZE)

extern uint64_t physmem;

extern int random_get_bytes(uint8_t *ptr, size_t len);
extern int random_get_pseudo_bytes(uint8_t *ptr, size_t len);

extern void kernel_init(int);
extern void kernel_fini(void);

struct spa;
extern void nicenum(uint64_t, char *);
extern void show_pool_stats(struct spa *);

typedef struct callb_cpr {
	kmutex_t	*cc_lockp;
} callb_cpr_t;

#define	CALLB_CPR_INIT(cp, lockp, func, name)	{		\
	(cp)->cc_lockp = lockp;					\
}

#define	CALLB_CPR_SAFE_BEGIN(cp) {				\
	ASSERT(MUTEX_HELD((cp)->cc_lockp));			\
}

#define	CALLB_CPR_SAFE_END(cp, lockp) {				\
	ASSERT(MUTEX_HELD((cp)->cc_lockp));			\
}

#define	CALLB_CPR_EXIT(cp) {					\
	ASSERT(MUTEX_HELD((cp)->cc_lockp));			\
	mutex_exit((cp)->cc_lockp);				\
}

#define	zone_dataset_visible(x, y)	(1)
#define	INGLOBALZONE(z)			(1)

/*
 * Hostname information
 */
extern struct utsname utsname;
extern char hw_serial[];
extern int ddi_strtoul(const char *str, char **nptr, int base,
    unsigned long *result);
#define	ddi_get_lbolt()	(gethrtime() >> 23)
#define	ddi_get_lbolt64() (gethrtime() >> 23)

/* ZFS Boot Related stuff. */

struct _buf {
	intptr_t	_fd;
};

struct bootstat {
	uint64_t st_size;
};

extern int zfs_secpolicy_snapshot_perms(const char *name, cred_t *cr);
extern int zfs_secpolicy_rename_perms(const char *from, const char *to,
    cred_t *cr);
extern int zfs_secpolicy_destroy_perms(const char *name, cred_t *cr);

extern struct _buf *kobj_open_file(char *name);
extern int kobj_read_file(struct _buf *file, char *buf, unsigned size,
    unsigned off);
extern void kobj_close_file(struct _buf *file);
extern int kobj_get_filesize(struct _buf *file, uint64_t *size);

/* Random compatibility stuff. */
#define	lbolt	(gethrtime() >> 23)
#define	lbolt64	(gethrtime() >> 23)

extern uint64_t physmem;

#define	gethrestime_sec()	time(NULL)

#define	pwrite64(d, p, n, o)	pwrite(d, p, n, o)
#define	readdir64(d)		readdir(d)
#define	sigispending(td, sig)	(0)
#define	root_mount_wait()	do { } while (0)
#define	root_mounted()		(1)

struct file {
	void *dummy;
};

#define	FCREAT	O_CREAT
#define	FOFFMAX	0x0

/* Errors */

#ifndef	ERESTART
#define	ERESTART	(-1)
#endif

/* SID stuff */

typedef struct ksiddomain {
	uint_t	kd_ref;
	uint_t	kd_len;
	char	*kd_name;
} ksiddomain_t;

ksiddomain_t *ksid_lookupdomain(const char *);
void ksiddomain_rele(ksiddomain_t *);

#define	DDI_SLEEP	KM_SLEEP
#define	ddi_log_sysevent(_a, _b, _c, _d, _e, _f, _g)	(0)

typedef struct ace_object {
 	uid_t		a_who;
 	uint32_t	a_access_mask;
 	uint16_t	a_flags;
 	uint16_t	a_type;
 	uint8_t		a_obj_type[16];
 	uint8_t		a_inherit_obj_type[16];
} ace_object_t;
 
#define	ACE_ACCESS_ALLOWED_OBJECT_ACE_TYPE	0x05
#define	ACE_ACCESS_DENIED_OBJECT_ACE_TYPE	0x06
#define	ACE_SYSTEM_AUDIT_OBJECT_ACE_TYPE	0x07
#define	ACE_SYSTEM_ALARM_OBJECT_ACE_TYPE	0x08

/* libzfs_dataset.c */
#define	priv_allocset()		(NULL)
#define	getppriv(a, b)		(0)
#define	priv_isfullset(a)	(B_FALSE)
#define	priv_freeset(a)		(0)

#define	di_devlink_init(a, b)	(NULL)
#define	di_devlink_fini(a)	(0)
typedef void			*di_devlink_handle_t;

extern char *kmem_asprintf(const char *fmt, ...);
#define strfree(str) kmem_free((str), strlen(str)+1)

#define DEV_PHYS_PATH "phys_path"

typedef	uint32_t	idmap_rid_t;

#define	ZFS_EXPORTS_PATH	"/etc/zfs/exports"

#else	/* _KERNEL */

#include <sys/systm.h>
#include <sys/syslimits.h>
#include <sys/kmem.h>
#include <sys/param.h>
#include <sys/stdint.h>
#include <sys/note.h>
#include <sys/kernel.h>
#include <sys/kstat.h>
#include <sys/debug.h>
#include <sys/proc.h>
#include <sys/sysmacros.h>
#include <sys/bitmap.h>
#include <sys/cmn_err.h>
#include <sys/taskq.h>
#include <sys/taskq_impl.h>
#include <sys/conf.h>
#include <sys/mutex.h>
#include <sys/rwlock.h>
#include <sys/kcondvar.h>
#include <sys/list.h>
#include <sys/uio.h>
#include <sys/dirent.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/fcntl.h>
#include <sys/string.h>
#include <sys/buf.h>
#include <sys/cred.h>
#include <sys/sdt.h>
#include <sys/file.h>
#include <sys/vfs.h>
#include <sys/lockf.h>
#include <sys/refstr.h>
#include <sys/zone.h>
#include <sys/misc.h>
#include <sys/zfs_debug.h>
#include <sys/byteorder.h>
#include <sys/mman.h>
#include <sys/random.h>
#include <sys/pset.h>
#include <sys/u8_textprep.h>
#include <sys/sysevent.h>
#include <sys/sysevent/eventdefs.h>
#include <sys/u8_textprep.h>
#include <sys/sunddi.h>
#include <sys/cyclic.h>

#include <miscfs/specfs/specdev.h>
#include <uvm/uvm_extern.h>

#define	CPU_SEQID	(curcpu()->ci_data.cpu_index)
#define	lbolt		hardclock_ticks
#define	lbolt64		lbolt

#ifdef	__cplusplus
}
#endif

extern int zfs_debug_level;
extern kmutex_t zfs_debug_mtx;
#define	ZFS_LOG(lvl, ...)	do {					\
	if (((lvl) & 0xff) <= zfs_debug_level) {			\
		mutex_enter(&zfs_debug_mtx);				\
		printf("%s:%u[%d]: ", __func__, __LINE__, (lvl));	\
		printf(__VA_ARGS__);					\
		printf("\n");						\
		if ((lvl) & 0x100)					\
			/* XXXNETBSD backtrace */			\
		mutex_exit(&zfs_debug_mtx);				\
	}								\
} while (0)

extern struct utsname utsname;

#define	makedevice(a, b)	makedev(a, b)
#define	getmajor(a)		major(a)
#define	getminor(a)		minor(a)
#define issig(x)		(sigispending(curlwp, 0))
#define	ISSIG(thr, why)		(sigispending(thr, 0))
#define	fm_panic		panic

#ifdef ptob
#undef ptob
#endif
#define ptob(x)			((x) * PAGE_SIZE)

#define	strncat(a, b, c)	strlcat(a, b, c)
#define	kmem_debugging()	0

#define zone_get_hostid(a)      0

extern char *kmem_asprintf(const char *fmt, ...);
#define strfree(str) kmem_free((str), strlen(str)+1)

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

#define td_ru l_ru

/* NetBSD doesn't need this routines in zfs code, yet */
#define	taskq_create_proc(a, b, c, d, e, p, f) \
	(taskq_create(a, b, c, d, e, f))
#define	taskq_create_sysdc(a, b, d, e, p, dc, f) \
	(taskq_create(a, b, maxclsyspri, d, e, f))

#define	FIGNORECASE		0

#define DEV_PHYS_PATH "phys_path"

#define sys_shutdown	0

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

static inline int
vsprintf(char * __restrict buf, const char * __restrict fmt, va_list ap)
{

	return vsnprintf(buf, 1024, fmt, ap);
}

/*
 * FreeBSD interfaces
 */

void zfs_netbsd_setsize(vnode_t *, off_t);
#define vnode_pager_setsize(vp, size) zfs_netbsd_setsize(vp, size)
#define vn_pages_remove(a, b, c)

#define getf		fd_getfile
#define releasef	fd_putfile

#define callout_drain(x) callout_stop(x)

#endif	/* _KERNEL */

#define	SYSCTL_DECL(...)
#define	SYSCTL_NODE(...)
#define	SYSCTL_INT(...)
#define	SYSCTL_UINT(...)
#define	SYSCTL_LONG(...)
#define	SYSCTL_ULONG(...)
#define	SYSCTL_QUAD(...)
#define	SYSCTL_UQUAD(...)
#ifdef TUNABLE_INT
#undef TUNABLE_INT
#undef TUNABLE_ULONG
#undef TUNABLE_INT_FETCH
#endif
#define	TUNABLE_INT(...)
#define	TUNABLE_ULONG(...)
#define	TUNABLE_INT_FETCH(...)		0

#define ASSERT_VOP_LOCKED(vp, name) KASSERT(VOP_ISLOCKED(vp) != 0)
#define ASSERT_VOP_ELOCKED(vp, name) KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE)

#define UID_NOBODY (-2)
#define GID_NOBODY (-2)

#endif	/* _SYS_ZFS_CONTEXT_H */
