/*	$NetBSD: systm.h,v 1.130 2001/07/06 15:59:23 perry Exp $	*/

/*-
 * Copyright (c) 1982, 1988, 1991, 1993
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
 *	@(#)systm.h	8.7 (Berkeley) 3/29/95
 */

/*
 * The `securelevel' variable controls the security level of the system.
 * It can only be decreased by process 1 (/sbin/init).
 *
 * Security levels are as follows:
 *   -1	permanently insecure mode - always run system in level 0 mode.
 *    0	insecure mode - immutable and append-only flags may be turned off.
 *	All devices may be read or written subject to permission modes.
 *    1	secure mode - immutable and append-only flags may not be changed;
 *	raw disks of mounted filesystems, /dev/mem, and /dev/kmem are
 *	read-only.
 *    2	highly secure mode - same as (1) plus raw disks are always
 *	read-only whether mounted or not. This level precludes tampering
 *	with filesystems by unmounting them, but also inhibits running
 *	newfs while the system is secured.
 *
 * In normal operation, the system runs in level 0 mode while single user
 * and in level 1 mode while multiuser. If level 2 mode is desired while
 * running multiuser, it can be set in the multiuser startup script
 * (/etc/rc.local) using sysctl(8). If it is desired to run the system
 * in level 0 mode while multiuser, initialize the variable securelevel
 * in /sys/kern/kern_sysctl.c to -1. Note that it is NOT initialized to
 * zero as that would allow the vmunix binary to be patched to -1.
 * Without initialization, securelevel loads in the BSS area which only
 * comes into existence when the kernel is loaded and hence cannot be
 * patched by a stalking hacker.
 */

#ifndef _SYS_SYSTM_H_
#define _SYS_SYSTM_H_

#if defined(_KERNEL_OPT)
#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#endif

#include <machine/endian.h>

struct clockframe;
struct device;
struct proc;
struct timeval;
struct tty;
struct uio;
struct vnode;

extern int securelevel;		/* system security level */
extern const char *panicstr;	/* panic message */
extern int doing_shutdown;	/* shutting down */

extern const char copyright[];	/* system copyright */
extern char cpu_model[];	/* machine/cpu model name */
extern char machine[];		/* machine type */
extern char machine_arch[];	/* machine architecture */
extern const char osrelease[];	/* short system version */
extern const char ostype[];	/* system type */
extern const char version[];	/* system version */

extern int autonicetime;        /* time (in seconds) before autoniceval */
extern int autoniceval;         /* proc priority after autonicetime */

extern int nblkdev;		/* number of entries in bdevsw */
extern int nchrdev;		/* number of entries in cdevsw */

extern int selwait;		/* select timeout address */

extern int maxmem;		/* max memory per process */
extern int physmem;		/* physical memory */

extern dev_t dumpdev;		/* dump device */
extern long dumplo;		/* offset into dumpdev */
extern int dumpsize;		/* size of dump in pages */
extern const char *dumpspec;	/* how dump device was specified */

extern dev_t rootdev;		/* root device */
extern struct vnode *rootvp;	/* vnode equivalent to above */
extern struct device *root_device; /* device equivalent to above */
extern const char *rootspec;	/* how root device was specified */

/*
 * These represent the swap pseudo-device (`sw').  This device
 * is used by the swap pager to indirect through the routines
 * in sys/vm/vm_swap.c.
 */
extern dev_t swapdev;		/* swapping device */
extern struct vnode *swapdev_vp;/* vnode equivalent to above */

typedef int	sy_call_t(struct proc *, void *, register_t *);

extern struct sysent {		/* system call table */
	short	sy_narg;	/* number of args */
	short	sy_argsize;	/* total size of arguments */
	int	sy_flags;	/* flags. see below */
	sy_call_t *sy_call;     /* implementing function */
} sysent[];
extern int nsysent;
#if	BYTE_ORDER == BIG_ENDIAN
#define	SCARG(p,k)	((p)->k.be.datum)	/* get arg from args pointer */
#elif	BYTE_ORDER == LITTLE_ENDIAN
#define	SCARG(p,k)	((p)->k.le.datum)	/* get arg from args pointer */
#else
#error	"what byte order is this machine?"
#endif

#define	SYCALL_MPSAFE	0x0001	/* syscall is MP-safe */

extern int boothowto;		/* reboot flags, from console subsystem */
#define	bootverbose	(boothowto & AB_VERBOSE)
#define	bootquiet	(boothowto & AB_QUIET)

extern void (*v_putc) __P((int)); /* Virtual console putc routine */

extern	void	_insque	__P((void *, void *));
extern	void	_remque	__P((void *));

/* casts to keep lint happy, but it should be happy with void * */
#define	insque(q,p)	_insque(q, p)
#define	remque(q)	_remque(q)

/*
 * General function declarations.
 */
int	nullop __P((void *));
int	enodev __P((void));
int	enosys __P((void));
int	enoioctl __P((void));
int	enxio __P((void));
int	eopnotsupp __P((void));

#if defined(LKM) || defined(_LKM)
int	lkmenodev __P((void));
#endif

enum hashtype {
	HASH_LIST,
	HASH_TAILQ
};

void	*hashinit __P((int, enum hashtype, int, int, u_long *));
void	hashdone __P((void *, int));
int	seltrue __P((dev_t dev, int events, struct proc *p));
int	sys_nosys __P((struct proc *, void *, register_t *));


void	printf __P((const char *, ...))
    __attribute__((__format__(__printf__,1,2)));
int	sprintf __P((char *buf, const char *, ...))
    __attribute__((__format__(__printf__,2,3)));
int	snprintf __P((char *buf, size_t, const char *, ...))
    __attribute__((__format__(__printf__,3,4)));
void	vprintf __P((const char *, _BSD_VA_LIST_));
int	vsprintf __P((char *buf, const char *, _BSD_VA_LIST_));
int	vsnprintf __P((char *buf, size_t, const char *, _BSD_VA_LIST_));

void	panic __P((const char *, ...))
    __attribute__((__noreturn__,__format__(__printf__,1,2)));
void	uprintf __P((const char *, ...))
    __attribute__((__format__(__printf__,1,2)));
void	ttyprintf __P((struct tty *, const char *, ...))
    __attribute__((__format__(__printf__,2,3)));

char	*bitmask_snprintf __P((u_quad_t, const char *, char *, size_t));

int	humanize_number __P((char *, size_t, u_int64_t, const char *, int));
int	format_bytes __P((char *, size_t, u_int64_t));

void	tablefull __P((const char *, const char *));

int	kcopy __P((const void *, void *, size_t));

#define bcopy(src, dst, len)	memmove((dst), (src), (len))
#define bzero(src, len)		memset((src), 0, (len))
#define bcmp(a, b, len)		memcmp((a), (b), (len))

int	copystr __P((const void *, void *, size_t, size_t *));
int	copyinstr __P((const void *, void *, size_t, size_t *));
int	copyoutstr __P((const void *, void *, size_t, size_t *));
int	copyin __P((const void *, void *, size_t));
int	copyout __P((const void *, void *, size_t));

int	subyte __P((void *, int));
int	suibyte __P((void *, int));
int	susword __P((void *, short));
int	suisword __P((void *, short));
int	suswintr __P((void *, short));
int	suword __P((void *, long));
int	suiword __P((void *, long));

int	fubyte __P((const void *));
int	fuibyte __P((const void *));
int	fusword __P((const void *));
int	fuisword __P((const void *));
int	fuswintr __P((const void *));
long	fuword __P((const void *));
long	fuiword __P((const void *));

int	hzto __P((struct timeval *tv));
void	realitexpire __P((void *));

void	hardclock __P((struct clockframe *frame));
#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
void	softclock __P((void *));
#endif
void	statclock __P((struct clockframe *frame));
#ifdef NTP
void	hardupdate __P((long offset));
#ifdef PPS_SYNC
void	hardpps __P((struct timeval *, long));
#endif
#endif

void	initclocks __P((void));
void	inittodr __P((time_t));
void	resettodr __P((void));
void	cpu_initclocks __P((void));

void	startprofclock __P((struct proc *));
void	stopprofclock __P((struct proc *));
void	setstatclockrate __P((int hzrate));

/*
 * Shutdown hooks.  Functions to be run with all interrupts disabled
 * immediately before the system is halted or rebooted.
 */
void	*shutdownhook_establish __P((void (*)(void *), void *));
void	shutdownhook_disestablish __P((void *));
void	doshutdownhooks __P((void));

/*
 * Power managment hooks.
 */
void	*powerhook_establish __P((void (*)(int, void *), void *));
void	powerhook_disestablish __P((void *));
void	dopowerhooks __P((int));
#define PWR_RESUME	0
#define PWR_SUSPEND	1
#define PWR_STANDBY	2
#define PWR_SOFTRESUME	3
#define PWR_SOFTSUSPEND	4
#define PWR_SOFTSTANDBY	5

/*
 * Mountroot hooks.  Device drivers establish these to be executed
 * just before (*mountroot)() if the passed device is selected
 * as the root device.
 */
void	*mountroothook_establish __P((void (*)(struct device *),
	    struct device *));
void	mountroothook_disestablish __P((void *));
void	mountroothook_destroy __P((void));
void	domountroothook __P((void));

/*
 * Exec hooks. Subsystems may want to do cleanup when a process
 * execs.
 */
void	*exechook_establish __P((void (*)(struct proc *, void *), void *));
void	exechook_disestablish __P((void *));
void	doexechooks __P((struct proc *));

int	uiomove __P((void *, int, struct uio *));

#ifdef _KERNEL
caddr_t	allocsys __P((caddr_t, caddr_t (*)(caddr_t)));
#define	ALLOCSYS(base, name, type, num) \
	    (name) = (type *)(base); (base) = (caddr_t)ALIGN((name)+(num))
#endif

#ifdef _KERNEL
int	setjmp	__P((label_t *));
void	longjmp	__P((label_t *));
#endif

void	consinit __P((void));

void	cpu_startup __P((void));
void	cpu_configure __P((void));
void	cpu_rootconf __P((void));
void	cpu_dumpconf __P((void));

#ifdef GPROF
void	kmstartup __P((void));
#endif

#ifdef _KERNEL
#include <lib/libkern/libkern.h>
#endif

#ifdef _KERNEL

/*
 * Stuff to handle debugger magic key sequences.
 */
#define CNS_LEN			128
#define CNS_MAGIC_VAL(x)	((x)&0x1ff)
#define CNS_MAGIC_NEXT(x)	(((x)>>9)&0x7f)
#define CNS_TERM		0x7f	/* End of sequence */

typedef struct cnm_state {
	int	cnm_state;
	u_short	*cnm_magic;
} cnm_state_t;

/* Override db_console() in MD headers */
#ifndef cn_trap
#define cn_trap()	console_debugger()
#endif
#ifndef cn_isconsole
#define cn_isconsole(d)	((d) == cn_tab->cn_dev)
#endif

void cn_init_magic __P((cnm_state_t *cnm));
void cn_destroy_magic __P((cnm_state_t *cnm));
int cn_set_magic __P((char *magic));
int cn_get_magic __P((char *magic, int len));
/* This should be called for each byte read */
#ifndef cn_check_magic
#define cn_check_magic(d, k, s)						\
	do {								\
		if (cn_isconsole(d)) {					\
			int v = (s).cnm_magic[(s).cnm_state];		\
			if ((k) == CNS_MAGIC_VAL(v)) {			\
				(s).cnm_state = CNS_MAGIC_NEXT(v);	\
				if ((s).cnm_state == CNS_TERM) {	\
					cn_trap();			\
					(s).cnm_state = 0;		\
				}					\
			} else {					\
				(s).cnm_state = 0;			\
			}						\
		}							\
	} while (/* CONSTCOND */ 0)
#endif

/* Encode out-of-band events this way when passing to cn_check_magic() */
#define	CNC_BREAK		0x100

#if defined(DDB) || defined(sun3) || defined(sun2)
/* note that cpu_Debugger() is always available on sun[23] */
void	cpu_Debugger __P((void));
#define Debugger	cpu_Debugger
#endif

#ifdef DDB
/*
 * Enter debugger(s) from console attention if enabled
 */
extern int db_fromconsole; /* XXX ddb/ddbvar.h */
#define console_debugger() if (db_fromconsole) Debugger()
#else
#define console_debugger() do {} while (/* CONSTCOND */ 0) /* NOP */
#endif
#endif /* _KERNEL */

#ifdef SYSCALL_DEBUG
void scdebug_call __P((struct proc *, register_t, register_t[]));
void scdebug_ret __P((struct proc *, register_t, int, register_t[]));
#endif /* SYSCALL_DEBUG */

#if defined(MULTIPROCESSOR)
#include <sys/lock.h>

extern struct lock kernel_lock;

#define	KERNEL_LOCK_INIT()	spinlockinit(&kernel_lock, "klock", 0)

/*
 * Acquire/release kernel lock.
 * Intended for use in the scheduler and the lower half of the kernel.
 */
#define	KERNEL_LOCK(flag)						\
do {									\
	SCHED_ASSERT_UNLOCKED();					\
	spinlockmgr(&kernel_lock, (flag), 0);				\
} while (/* CONSTCOND */ 0)

#define	KERNEL_UNLOCK()		spinlockmgr(&kernel_lock, LK_RELEASE, 0)

/*
 * Acquire/release kernel lock on behalf of a process.
 * Intended for use in the top half of the kernel.
 */
#define	KERNEL_PROC_LOCK(p)						\
do {									\
	KERNEL_LOCK(LK_EXCLUSIVE);					\
	(p)->p_flag |= P_BIGLOCK;					\
} while (/* CONSTCOND */ 0)

#define	KERNEL_PROC_UNLOCK(p)						\
do {									\
	p->p_flag &= ~P_BIGLOCK;					\
	KERNEL_UNLOCK();						\
} while (/* CONSTCOND */ 0)

#else /* ! MULTIPROCESSOR */

#define	KERNEL_LOCK_INIT()		/* nothing */
#define	KERNEL_LOCK(flag)		/* nothing */
#define	KERNEL_UNLOCK()			/* nothing */
#define	KERNEL_PROC_LOCK(p)		/* nothing */
#define	KERNEL_PROC_UNLOCK(p)		/* nothing */

#endif /* MULTIPROCESSOR */

#endif	/* !_SYS_SYSTM_H_ */
