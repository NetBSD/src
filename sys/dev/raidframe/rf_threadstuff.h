/*	$NetBSD: rf_threadstuff.h,v 1.21.30.3 2007/05/13 17:36:28 ad Exp $	*/
/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Mark Holland, Daniel Stodolsky, Jim Zelenka
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * threadstuff.h -- definitions for threads, locks, and synchronization
 *
 * The purpose of this file is provide some illusion of portability.
 * If the functions below can be implemented with the same semantics on
 * some new system, then at least the synchronization and thread control
 * part of the code should not require modification to port to a new machine.
 * the only other place where the pthread package is explicitly used is
 * threadid.h
 *
 * this file should be included above stdio.h to get some necessary defines.
 *
 */

#ifndef _RF__RF_THREADSTUFF_H_
#define _RF__RF_THREADSTUFF_H_

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/lock.h>

#include <dev/raidframe/raidframevar.h>

#define decl_simple_lock_data(a,b) a struct simplelock b;

typedef struct proc *RF_Thread_t;
typedef void *RF_ThreadArg_t;

#define RF_DECLARE_MUTEX(_m_)           decl_simple_lock_data(,(_m_))
#define RF_DECLARE_STATIC_MUTEX(_m_)    decl_simple_lock_data(static,(_m_))
#define RF_DECLARE_EXTERN_MUTEX(_m_)    decl_simple_lock_data(extern,(_m_))

#define RF_DECLARE_COND(_c_)            int _c_;

#define RF_LOCK_MUTEX(_m_)              simple_lock(&(_m_))
#define RF_UNLOCK_MUTEX(_m_)            simple_unlock(&(_m_))


/* non-spinlock */
#define decl_lock_data(a,b) a struct lock b;

#define RF_DECLARE_LKMGR_MUTEX(_m_)           decl_lock_data(,(_m_))
#define RF_DECLARE_LKMGR_STATIC_MUTEX(_m_)    decl_lock_data(static,(_m_))
#define RF_DECLARE_LKMGR_EXTERN_MUTEX(_m_)    decl_lock_data(extern,(_m_))

#define RF_LOCK_LKMGR_MUTEX(_m_)        lockmgr(&(_m_),LK_EXCLUSIVE,NULL)
#define RF_UNLOCK_LKMGR_MUTEX(_m_)      lockmgr(&(_m_),LK_RELEASE,NULL)


/*
 * In NetBSD, kernel threads are simply processes which share several
 * substructures and never run in userspace.
 */
#define RF_WAIT_COND(_c_,_m_)		\
	ltsleep(&(_c_), PRIBIO, "rfwcond", 0, &(_m_))
#define RF_SIGNAL_COND(_c_)            wakeup_one(&(_c_))
#define RF_BROADCAST_COND(_c_)         wakeup(&(_c_))
#define	RF_CREATE_THREAD(_handle_, _func_, _arg_, _name_) \
	kthread_create(PRI_NONE, 0, NULL, (void (*)(void *))(_func_), \
	    (void *)(_arg_), (struct lwp **)&(_handle_), _name_)

#define	RF_CREATE_ENGINE_THREAD(_handle_, _func_, _arg_, _fmt_, _fmt_arg_) \
	kthread_create(PRI_NONE, 0, NULL, (void (*)(void *))(_func_), \
	    (void *)(_arg_), (struct lwp **)&(_handle_), _fmt_, _fmt_arg_)

#define rf_mutex_init(m) simple_lock_init(m)

#endif				/* !_RF__RF_THREADSTUFF_H_ */
