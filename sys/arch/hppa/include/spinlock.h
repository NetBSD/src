/*	$NetBSD: spinlock.h,v 1.1 2002/06/05 01:04:23 fredette Exp $	*/

/*	$OpenBSD: spinlock.h,v 1.1 1999/01/08 08:25:34 d Exp $	*/

#ifndef _HPPA_SPINLOCK_H_
#define _HPPA_SPINLOCK_H_

#define _SPINLOCK_UNLOCKED	(1)
#define _SPINLOCK_LOCKED	(0)
typedef int _spinlock_lock_t;

#endif
