/*-
 * Copyright (c) 2010 Max Khon <fjoe@freebsd.org>
 * All rights reserved.
 *
 * This software was developed by Max Khon under sponsorship from
 * the FreeBSD Foundation and Ethon Technologies GmbH.
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
 *
 * $Id: vchi_bsd.c,v 1.2 2013/03/24 14:26:16 jmcneill Exp $
 */

#include <sys/types.h>
#include <sys/bus.h>
#include <sys/callout.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <interface/compat/vchi_bsd.h>

MALLOC_DEFINE(M_VCHI, "VCHI", "VCHI");

/*
 * Timer API
 */
static void
run_timer(void *arg)
{
	struct timer_list *t = (struct timer_list *) arg;
	void (*function)(unsigned long);

	spin_lock(&t->mtx);
	if (callout_pending(&t->callout)) {
		/* callout was reset */
		spin_unlock(&t->mtx);
		return;
	}
	if (!callout_active(&t->callout)) {
		/* callout was stopped */
		spin_unlock(&t->mtx);
		return;
	}
	callout_ack(&t->callout);

	function = t->function;
	spin_unlock(&t->mtx);

	function(t->data);
}

void
init_timer(struct timer_list *t)
{
	spin_lock_init(&t->mtx);
	callout_init(&t->callout, CALLOUT_MPSAFE);
	t->expires = 0;
	/*
	 * function and data are not initialized intentionally:
	 * they are not initialized by Linux implementation too
	 */
}

void
setup_timer(struct timer_list *t, void (*function)(unsigned long), unsigned long data)
{
	t->function = function;
	t->data = data;
	init_timer(t);
}

void
mod_timer(struct timer_list *t, unsigned long expires)
{
	spin_lock(&t->mtx);
	callout_reset(&t->callout, expires - jiffies, run_timer, t);
	spin_unlock(&t->mtx);
}

void
add_timer(struct timer_list *t)
{
	mod_timer(t, t->expires);
}

int
del_timer_sync(struct timer_list *t)
{
	spin_lock(&t->mtx);
	callout_stop(&t->callout);
	spin_unlock(&t->mtx);

	spin_lock_destroy(&t->mtx);
	return 0;
}

int
del_timer(struct timer_list *t)
{
	del_timer_sync(t);
	return 0;
}

/*
 * Completion API
 */
void
init_completion(struct completion *c)
{
	cv_init(&c->cv, "VCHI completion cv");
	mutex_init(&c->lock, MUTEX_DEFAULT, IPL_NONE);
	c->done = 0;
}

void
destroy_completion(struct completion *c)
{
	cv_destroy(&c->cv);
	mutex_destroy(&c->lock);
}

void
wait_for_completion(struct completion *c)
{
	mutex_enter(&c->lock);
	if (!c->done)
		cv_wait(&c->cv, &c->lock);
	c->done--;
	mutex_exit(&c->lock);
}

int
try_wait_for_completion(struct completion *c)
{
	int res = 0;

	mutex_enter(&c->lock);
	if (!c->done)
		c->done--;
	else
		res = 1;
	mutex_exit(&c->lock);
	return res != 0;
}

int
wait_for_completion_timeout(struct completion *c, unsigned long timeout)
{
	int res = 0;

	mutex_enter(&c->lock);
	if (!c->done)
		res = cv_timedwait(&c->cv, &c->lock, timeout);
	if (res == 0)
		c->done--;
	mutex_exit(&c->lock);
	return res != 0;
}

int
wait_for_completion_interruptible_timeout(struct completion *c, unsigned long timeout)
{
	int res = 0;

	mutex_enter(&c->lock);
	if (!c->done)
		res = cv_timedwait_sig(&c->cv, &c->lock, timeout);
	if (res == 0)
		c->done--;
	mutex_exit(&c->lock);
	return res != 0;
}

int
wait_for_completion_interruptible(struct completion *c)
{
	int res = 0;

	mutex_enter(&c->lock);
	if (!c->done)
		res = cv_wait_sig(&c->cv, &c->lock);
	if (res == 0)
		c->done--;
	mutex_exit(&c->lock);
	return res != 0;
}

int
wait_for_completion_killable(struct completion *c)
{
	int res = 0;

	mutex_enter(&c->lock);
	if (!c->done)
		res = cv_wait_sig(&c->cv, &c->lock);
	/* TODO: check actual signals here ? */
	if (res == 0)
		c->done--;
	mutex_exit(&c->lock);
	return res != 0;
}

void
complete(struct completion *c)
{
	mutex_enter(&c->lock);
	c->done++;
	cv_signal(&c->cv);
	mutex_exit(&c->lock);
}

void
complete_all(struct completion *c)
{
	mutex_enter(&c->lock);
	c->done++;
	cv_broadcast(&c->cv);
	mutex_exit(&c->lock);
}

/*
 * Semaphore API
 */

void sema_sysinit(void *arg)
{
	struct semaphore *s = arg;

	printf("sema_sysinit\n");
	_sema_init(s, 1);
}

void
_sema_init(struct semaphore *s, int value)
{
	bzero(s, sizeof(*s));
	mutex_init(&s->mtx, MUTEX_DEFAULT, IPL_VM);
	cv_init(&s->cv, "sema cv");
	s->value = value;
}

void
_sema_destroy(struct semaphore *s)
{
	mutex_destroy(&s->mtx);
	cv_destroy(&s->cv);
}

void
down(struct semaphore *s)
{

	mutex_enter(&s->mtx);
	while (s->value == 0) {
		s->waiters++;
		cv_wait(&s->cv, &s->mtx);
		s->waiters--;
	}

	s->value--;
	mutex_exit(&s->mtx);
}

int
down_interruptible(struct semaphore *s)
{
	int ret ;

	ret = 0;

	mutex_enter(&s->mtx);

	while (s->value == 0) {
		s->waiters++;
		ret = cv_wait_sig(&s->cv, &s->mtx);
		s->waiters--;

		if (ret == EINTR || ret == ERESTART) {
			mutex_exit(&s->mtx);
			return (-EINTR);
		}
	}

	s->value--;
	mutex_exit(&s->mtx);

	return (0);
}

int
down_trylock(struct semaphore *s)
{
	int ret;

	ret = 0;

	mutex_enter(&s->mtx);

	if (s->value > 0) {
		/* Success. */
		s->value--;
		ret = 0;
	} else {
		ret = -EAGAIN;
	}

	mutex_exit(&s->mtx);

	return (ret);
}

void
up(struct semaphore *s)
{
	mutex_enter(&s->mtx);
	s->value++;
	if (s->waiters && s->value > 0)
		cv_signal(&s->cv);

	mutex_exit(&s->mtx);
}

/*
 * Logging API
 */
void
rlprintf(int pps, const char *fmt, ...)
{
	va_list ap;
	static struct timeval last_printf;
	static int count;

	if (ppsratecheck(&last_printf, &count, pps)) {
		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
	}
}

void
device_rlprintf(int pps, device_t dev, const char *fmt, ...)
{
	va_list ap;
	static struct timeval last_printf;
	static int count;

	if (ppsratecheck(&last_printf, &count, pps)) {
		va_start(ap, fmt);
		device_print_prettyname(dev);
		vprintf(fmt, ap);
		va_end(ap);
	}
}

/*
 * Signals API
 */

void
flush_signals(VCHIQ_THREAD_T thr)
{
	printf("Implement ME: %s\n", __func__);
}

int
fatal_signal_pending(VCHIQ_THREAD_T thr)
{
	printf("Implement ME: %s\n", __func__);
	return (0);
}

/*
 * kthread API
 */

/*
 *  This is a hack to avoid memory leak
 */
#define MAX_THREAD_DATA_SLOTS	32
static int thread_data_slot = 0;

struct thread_data {
	void *data;
	int (*threadfn)(void *);
};

static struct thread_data thread_slots[MAX_THREAD_DATA_SLOTS];

static void 
kthread_wrapper(void *data)
{
	struct thread_data *slot;

	slot = data;
	slot->threadfn(slot->data);
}

VCHIQ_THREAD_T
vchiq_thread_create(int (*threadfn)(void *data),
	void *data,
	const char namefmt[], ...)
{
	VCHIQ_THREAD_T newt;
	va_list ap;
	char name[MAXCOMLEN+1];
	struct thread_data *slot;

	if (thread_data_slot >= MAX_THREAD_DATA_SLOTS) {
		printf("kthread_create: out of thread data slots\n");
		return (NULL);
	}

	slot = &thread_slots[thread_data_slot];
	slot->data = data;
	slot->threadfn = threadfn;

	va_start(ap, namefmt);
	vsnprintf(name, sizeof(name), namefmt, ap);
	va_end(ap);
	
	newt = NULL;
	if (kthread_create(PRI_NONE, 0, NULL, kthread_wrapper, slot, &newt,
	    "%s", name) != 0) {
		/* Just to be sure */
		newt = NULL;
	} else {
		thread_data_slot++;
	}

	return newt;
}

void
set_user_nice(VCHIQ_THREAD_T thr, int nice)
{
	/* NOOP */
}

void
wake_up_process(VCHIQ_THREAD_T thr)
{
	/* NOOP */
}
