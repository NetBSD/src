/*-
 * Copyright (c) 2010 Max Khon <fjoe@freebsd.org>
 * Copyright (c) 2012 Oleksandr Tymoshenko <gonzo@bluezbox.com>
 * Copyright (c) 2013 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef __VCHI_NETBSD_H__
#define __VCHI_NETBSD_H__

#include <sys/systm.h>
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/lock.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/ioccom.h>
#include <sys/atomic.h>
#include <sys/rwlock.h>
#include <sys/callout.h>

#include <linux/completion.h>
#include <asm/barrier.h>

/*
 * Copy from/to user API
 */
#define copy_from_user(to, from, n)	copyin((from), (to), (n))
#define copy_to_user(to, from, n)	copyout((from), (to), (n))

/*
 * Atomic API
 */
typedef volatile unsigned int atomic_t;

#define atomic_set(p, v)	(*(p) = (v))
#define atomic_read(p)		(*(volatile int *)(p))
#define atomic_inc(p)		atomic_inc_uint(p)
#define atomic_dec(p)		atomic_dec_uint(p)
#define atomic_dec_and_test(p)	(atomic_dec_uint_nv(p) == 0)
#define	atomic_inc_return(v)	atomic_inc_uint_nv(v)
#define	atomic_dec_return(v)	atomic_dec_uint_nv(v)
#define atomic_add(v, p)	atomic_add_int(p, v)
#define atomic_sub(v, p)	atomic_add_int(p, -(v))
#define atomic_add_return(v, p)	atomic_add_int_nv(p, v)
#define atomic_sub_return(v, p)	atomic_add_int_nv(p, -(v))
#define atomic_xchg(p, v)	atomic_swap_uint(p, v)
#define atomic_cmpxchg(p, oldv, newv) atomic_cas_uint(p, oldv, newv)

#define ATOMIC_INIT(v)		(v)

/*
 * Spinlock API
 */
typedef kmutex_t spinlock_t;

/*
 * NB: Need to initialize these at attach time!
 */
#define DEFINE_SPINLOCK(name)	kmutex_t name

#define spin_lock_init(lock)	mutex_init(lock, MUTEX_DEFAULT, IPL_VM)
#define spin_lock_destroy(lock)	mutex_destroy(lock)
#define spin_lock(lock)		mutex_spin_enter(lock)
#define spin_unlock(lock)	mutex_spin_exit(lock)

/*
 * Mutex API
 */
struct mutex {
	kmutex_t	mtx;
};

#define	lmutex_init(lock)	mutex_init(&(lock)->mtx, MUTEX_DEFAULT, IPL_NONE)
#define lmutex_destroy(lock)	mutex_destroy(&(lock)->mtx)
#define	lmutex_lock(lock)	mutex_enter(&(lock)->mtx)
#define	lmutex_lock_interruptible(lock)	(mutex_enter(&(lock)->mtx),0)
#define	lmutex_unlock(lock)	mutex_exit(&(lock)->mtx)

/*
 * Rwlock API
 */
typedef kmutex_t rwlock_t;

#define DEFINE_RWLOCK(name)	kmutex_t name

#define rwlock_init(rwlock)	mutex_init(rwlock, MUTEX_DEFAULT, IPL_VM)
#define read_lock(rwlock)	mutex_spin_enter(rwlock)
#define read_unlock(rwlock)	mutex_spin_exit(rwlock)

#define write_lock(rwlock)	mutex_spin_enter(rwlock)
#define write_unlock(rwlock)	mutex_spin_exit(rwlock)

#define read_lock_bh(rwlock)	read_lock(rwlock)
#define read_unlock_bh(rwlock)	read_unlock(rwlock)
#define write_lock_bh(rwlock)	write_lock(rwlock)
#define write_unlock_bh(rwlock)	write_unlock(rwlock)

/*
 * Timer API
 */
struct timer_list {
	kmutex_t mtx;
	callout_t callout;

	unsigned long expires;
	void (*function)(unsigned long);
	unsigned long data;
};

void init_timer(struct timer_list *t);
void setup_timer(struct timer_list *t, void (*function)(unsigned long), unsigned long data);
void mod_timer(struct timer_list *t, unsigned long expires);
void add_timer(struct timer_list *t);
int del_timer(struct timer_list *t);
int del_timer_sync(struct timer_list *t);

/*
 * Semaphore API
 */
struct semaphore {
	kmutex_t	mtx;
	kcondvar_t	cv;
	int		value;
	int		waiters;
};

/*
 * NB: Need to initialize these at attach time!
 */
#define	DEFINE_SEMAPHORE(name)	struct semaphore name

void sema_sysinit(void *arg);
void _sema_init(struct semaphore *s, int value);
void _sema_destroy(struct semaphore *s);
void down(struct semaphore *s);
int down_interruptible(struct semaphore *s);
int down_trylock(struct semaphore *s);
void up(struct semaphore *s);

/*
 * Logging and assertions API
 */
void rlprintf(int pps, const char *fmt, ...)
	__printflike(2, 3);

void
device_rlprintf(int pps, device_t dev, const char *fmt, ...)
	__printflike(3, 4);

#define might_sleep()

#define WARN(condition, msg)				\
({							\
	int __ret_warn_on = !!(condition);		\
	if (unlikely(__ret_warn_on))			\
		printf((msg));				\
	unlikely(__ret_warn_on);			\
})



#define WARN_ON(condition)				\
({							\
	int __ret_warn_on = !!(condition);		\
	if (unlikely(__ret_warn_on))			\
		printf("WARN_ON: " #condition "\n");	\
	unlikely(__ret_warn_on);			\
})

#define WARN_ON_ONCE(condition) ({			\
	static int __warned;				\
	int __ret_warn_once = !!(condition);		\
							\
	if (unlikely(__ret_warn_once))			\
		if (WARN_ON(!__warned))			\
			__warned = 1;			\
	unlikely(__ret_warn_once);			\
})

#define BUG_ON(cond)					\
	do {						\
		if (cond)				\
			panic("BUG_ON: " #cond);	\
	} while (0)

#define BUG()						\
	do {						\
		panic("BUG: %s:%d", __FILE__, __LINE__);	\
	} while (0)

#define vchiq_static_assert(cond) CTASSERT(cond)

/*
 * Kernel module API
 */
#define __init
#define __exit
#define __devinit
#define __devexit
#define __devinitdata

/*
 * Time API
 */
#if 1
/* emulate jiffies */
static inline unsigned long
_jiffies(void)
{
	struct timeval tv;

	microuptime(&tv);
	return tvtohz(&tv);
}

static inline unsigned long
msecs_to_jiffies(unsigned long msecs)
{
	struct timeval tv;

	tv.tv_sec = msecs / 1000000UL;
	tv.tv_usec = msecs % 1000000UL;
	return tvtohz(&tv);
}

#define jiffies			_jiffies()
#else
#define jiffies			ticks
#endif
#define HZ			hz

#define udelay(usec)		DELAY(usec)
#define mdelay(msec)		DELAY((msec) * 1000)

#define schedule_timeout(jiff)	kpause("dhdslp", false, jiff, NULL)

#if defined(msleep)
#undef msleep
#endif
#define msleep(msec)		mdelay(msec)

#define time_after(a, b)	((a) > (b))
#define time_after_eq(a, b)	((a) >= (b))
#define time_before(a, b)	time_after((b), (a))

/*
 * kthread API (we use lwp)
 */
typedef lwp_t * VCHIQ_THREAD_T;

VCHIQ_THREAD_T vchiq_thread_create(int (*threadfn)(void *data),
                                   void *data,
                                   const char namefmt[], ...);
void set_user_nice(VCHIQ_THREAD_T p, int nice);
void wake_up_process(VCHIQ_THREAD_T p);

/*
 * Proc APIs
 */
void flush_signals(VCHIQ_THREAD_T);
int fatal_signal_pending(VCHIQ_THREAD_T);

/*
 * Misc API
 */

#define __user

#define	current			curlwp
#define EXPORT_SYMBOL(x) 
#define PAGE_ALIGN(addr)	round_page(addr)

typedef	void	irqreturn_t;
typedef	off_t	loff_t;

#define BCM2835_MBOX_CHAN_VCHIQ	3
#define bcm_mbox_write	bcmmbox_write

#define dsb	membar_producer

#define device_print_prettyname(dev)	device_printf((dev), "")

#endif /* __VCHI_NETBSD_H__ */
