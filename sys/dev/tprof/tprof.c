/*	$NetBSD: tprof.c,v 1.1.4.2 2008/01/09 01:54:36 matt Exp $	*/

/*-
 * Copyright (c)2008 YAMAMOTO Takashi,
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tprof.c,v 1.1.4.2 2008/01/09 01:54:36 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <sys/cpu.h>
#include <sys/conf.h>
#include <sys/callout.h>
#include <sys/kmem.h>
#include <sys/workqueue.h>
#include <sys/queue.h>

#include <dev/tprof/tprof.h>
#include <dev/tprof/tprof_ioctl.h>

#include <machine/db_machdep.h> /* PC_REGS */

typedef struct {
	uintptr_t s_pc;	/* program counter */
} tprof_sample_t;

typedef struct tprof_buf {
	u_int b_used;
	u_int b_size;
	u_int b_overflow;
	u_int b_unused;
	STAILQ_ENTRY(tprof_buf) b_list;
	tprof_sample_t b_data[];
} tprof_buf_t;
#define	TPROF_BUF_BYTESIZE(sz) \
	(sizeof(tprof_buf_t) + (sz) * sizeof(tprof_sample_t))
#define	TPROF_MAX_SAMPLES_PER_BUF	10000

#define	TPROF_MAX_BUF			100

typedef struct {
	tprof_buf_t *c_buf;
	struct work c_work;
	callout_t c_callout;
} __aligned(CACHE_LINE_SIZE) tprof_cpu_t;

static kmutex_t tprof_lock;
static bool tprof_running;
static u_int tprof_nworker;
static lwp_t *tprof_owner;
static STAILQ_HEAD(, tprof_buf) tprof_list;
static u_int tprof_nbuf_on_list;
static struct workqueue *tprof_wq;
static tprof_cpu_t tprof_cpus[MAXCPUS] __aligned(CACHE_LINE_SIZE);
static u_int tprof_samples_per_buf;

static kmutex_t tprof_reader_lock;
static kcondvar_t tprof_reader_cv;
static off_t tprof_reader_offset;

static kmutex_t tprof_startstop_lock;
static kcondvar_t tprof_cv;

static struct tprof_stat tprof_stat;

static tprof_cpu_t *
tprof_cpu(struct cpu_info *ci)
{

	return &tprof_cpus[cpu_index(ci)];
}

static tprof_cpu_t *
tprof_curcpu(void)
{

	return tprof_cpu(curcpu());
}

static tprof_buf_t *
tprof_buf_alloc(void)
{
	tprof_buf_t *new;
	u_int size = tprof_samples_per_buf;
	
	new = kmem_alloc(TPROF_BUF_BYTESIZE(size), KM_SLEEP);
	new->b_used = 0;
	new->b_size = size;
	new->b_overflow = 0;
	return new;
}

static void
tprof_buf_free(tprof_buf_t *buf)
{

	kmem_free(buf, TPROF_BUF_BYTESIZE(buf->b_size));
}

static tprof_buf_t *
tprof_buf_switch(tprof_cpu_t *c, tprof_buf_t *new)
{
	tprof_buf_t *old;

	old = c->c_buf;
	c->c_buf = new;
	return old;
}

static tprof_buf_t *
tprof_buf_refresh(void)
{
	tprof_cpu_t * const c = tprof_curcpu();
	tprof_buf_t *new;

	new = tprof_buf_alloc();
	return tprof_buf_switch(c, new);
}

static void
tprof_worker(struct work *wk, void *dummy)
{
	tprof_cpu_t * const c = tprof_curcpu();
	tprof_buf_t *buf;
	bool shouldstop;

	KASSERT(wk == &c->c_work);
	KASSERT(dummy == NULL);

	/*
	 * get a per cpu buffer.
	 */
	buf = tprof_buf_refresh();

	/*
	 * and put it on the global list for read(2).
	 */
	mutex_enter(&tprof_lock);
	shouldstop = !tprof_running;
	if (shouldstop) {
		KASSERT(tprof_nworker > 0);
		tprof_nworker--;
		cv_broadcast(&tprof_cv);
		cv_broadcast(&tprof_reader_cv);
	}
	if (buf->b_used == 0) {
		tprof_stat.ts_emptybuf++;
	} else if (tprof_nbuf_on_list < TPROF_MAX_BUF) {
		tprof_stat.ts_sample += buf->b_used;
		tprof_stat.ts_overflow += buf->b_overflow;
		tprof_stat.ts_buf++;
		STAILQ_INSERT_TAIL(&tprof_list, buf, b_list);
		tprof_nbuf_on_list++;
		buf = NULL;
		cv_broadcast(&tprof_reader_cv);
	} else {
		tprof_stat.ts_dropbuf_sample += buf->b_used;
		tprof_stat.ts_dropbuf++;
	}
	mutex_exit(&tprof_lock);
	if (buf) {
		tprof_buf_free(buf);
	}
	if (!shouldstop) {
		callout_schedule(&c->c_callout, hz);
	}
}

static void
tprof_kick(void *vp)
{
	struct cpu_info * const ci = vp;
	tprof_cpu_t * const c = tprof_cpu(ci);

	workqueue_enqueue(tprof_wq, &c->c_work, ci);
}

static void
tprof_stop1(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	KASSERT(mutex_owned(&tprof_startstop_lock));

	for (CPU_INFO_FOREACH(cii, ci)) {
		tprof_cpu_t * const c = tprof_cpu(ci);
		tprof_buf_t *old;

		old = tprof_buf_switch(c, NULL);
		if (old != NULL) {
			tprof_buf_free(old);
		}
		callout_destroy(&c->c_callout);
	}
	workqueue_destroy(tprof_wq);
}

static int
tprof_start(const struct tprof_param *param)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	int error;
	uint64_t freq;

	KASSERT(mutex_owned(&tprof_startstop_lock));
	if (tprof_running) {
		error = EBUSY;
		goto done;
	}

	freq = tprof_backend_estimate_freq();
	tprof_samples_per_buf = MIN(freq * 2, TPROF_MAX_SAMPLES_PER_BUF);

	error = workqueue_create(&tprof_wq, "tprofmv", tprof_worker, NULL,
	    PRI_NONE, PRI_SOFTCLOCK, WQ_MPSAFE | WQ_PERCPU);
	if (error != 0) {
		goto done;
	}

	for (CPU_INFO_FOREACH(cii, ci)) {
		tprof_cpu_t * const c = tprof_cpu(ci);
		tprof_buf_t *new;
		tprof_buf_t *old;

		new = tprof_buf_alloc();
		old = tprof_buf_switch(c, new);
		if (old != NULL) {
			tprof_buf_free(old);
		}
		callout_init(&c->c_callout, CALLOUT_MPSAFE);
		callout_setfunc(&c->c_callout, tprof_kick, ci);
	}

	error = tprof_backend_start();
	if (error != 0) {
		tprof_stop1();
		goto done;
	}

	mutex_enter(&tprof_lock);
	tprof_running = true;
	mutex_exit(&tprof_lock);
	for (CPU_INFO_FOREACH(cii, ci)) {
		tprof_cpu_t * const c = tprof_cpu(ci);

		mutex_enter(&tprof_lock);
		tprof_nworker++;
		mutex_exit(&tprof_lock);
		workqueue_enqueue(tprof_wq, &c->c_work, ci);
	}
done:
	return error;
}

static void
tprof_stop(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	KASSERT(mutex_owned(&tprof_startstop_lock));
	if (!tprof_running) {
		goto done;
	}

	tprof_backend_stop();

	mutex_enter(&tprof_lock);
	tprof_running = false;
	cv_broadcast(&tprof_reader_cv);
	mutex_exit(&tprof_lock);

	for (CPU_INFO_FOREACH(cii, ci)) {
		mutex_enter(&tprof_lock);
		while (tprof_nworker > 0) {
			cv_wait(&tprof_cv, &tprof_lock);
		}
		mutex_exit(&tprof_lock);
	}

	tprof_stop1();
done:
	;
}

static void
tprof_clear(void)
{
	tprof_buf_t *buf;

	mutex_enter(&tprof_reader_lock);
	mutex_enter(&tprof_lock);
	while ((buf = STAILQ_FIRST(&tprof_list)) != NULL) {
		if (buf != NULL) {
			STAILQ_REMOVE_HEAD(&tprof_list, b_list);
			KASSERT(tprof_nbuf_on_list > 0);
			tprof_nbuf_on_list--;
			mutex_exit(&tprof_lock);
			tprof_buf_free(buf);
			mutex_enter(&tprof_lock);
		}
	}
	KASSERT(tprof_nbuf_on_list == 0);
	mutex_exit(&tprof_lock);
	tprof_reader_offset = 0;
	mutex_exit(&tprof_reader_lock);

	memset(&tprof_stat, 0, sizeof(tprof_stat));
}

/* -------------------- backend interfaces */

/*
 * tprof_sample: record a sample on the per-cpu buffer.
 *
 * be careful; can be called in NMI context.
 * we are assuming that curcpu() is safe.
 */

void
tprof_sample(const struct trapframe *tf)
{
	tprof_cpu_t * const c = tprof_curcpu();
	tprof_buf_t * const buf = c->c_buf;
	const uintptr_t pc = PC_REGS(tf);
	u_int idx;

	idx = buf->b_used;
	if (__predict_false(idx >= buf->b_size)) {
		buf->b_overflow++;
		return;
	}
	buf->b_data[idx].s_pc = pc;
	buf->b_used = idx + 1;
}

/* -------------------- cdevsw interfaces */

void tprofattach(int);

static int
tprof_open(dev_t dev, int flags, int type, struct lwp *l)
{

	if (minor(dev) != 0) {
		return EXDEV;
	}
	mutex_enter(&tprof_lock);
	if (tprof_owner != NULL) {
		mutex_exit(&tprof_lock);
		return  EBUSY;
	}
	tprof_owner = curlwp;
	mutex_exit(&tprof_lock);

	return 0;
}

static int
tprof_close(dev_t dev, int flags, int type, struct lwp *l)
{

	KASSERT(minor(dev) == 0);

	mutex_enter(&tprof_startstop_lock);
	mutex_enter(&tprof_lock);
	tprof_owner = NULL;
	mutex_exit(&tprof_lock);
	tprof_stop();
	tprof_clear();
	mutex_exit(&tprof_startstop_lock);

	return 0;
}

static int
tprof_read(dev_t dev, struct uio *uio, int flags)
{
	tprof_buf_t *buf;
	size_t bytes;
	size_t resid;
	size_t done;
	int error = 0;

	KASSERT(minor(dev) == 0);
	mutex_enter(&tprof_reader_lock);
	while (uio->uio_resid > 0 && error == 0) {
		/*
		 * take the first buffer from the list.
		 */
		mutex_enter(&tprof_lock);
		buf = STAILQ_FIRST(&tprof_list);
		if (buf == NULL) {
			if (tprof_nworker == 0) {
				mutex_exit(&tprof_lock);
				error = 0;
				break;
			}
			mutex_exit(&tprof_reader_lock);
			error = cv_wait_sig(&tprof_reader_cv, &tprof_lock);
			mutex_exit(&tprof_lock);
			mutex_enter(&tprof_reader_lock);
			continue;
		}
		STAILQ_REMOVE_HEAD(&tprof_list, b_list);
		KASSERT(tprof_nbuf_on_list > 0);
		tprof_nbuf_on_list--;
		mutex_exit(&tprof_lock);

		/*
		 * copy it out.
		 */
		bytes = MIN(buf->b_used * sizeof(tprof_sample_t) -
		    tprof_reader_offset, uio->uio_resid);
		resid = uio->uio_resid;
		error = uiomove((char *)buf->b_data + tprof_reader_offset,
		    bytes, uio);
		done = resid - uio->uio_resid;
		tprof_reader_offset += done;

		/*
		 * if we didn't consume the whole buffer,
		 * put it back to the list.
		 */
		if (tprof_reader_offset <
		    buf->b_used * sizeof(tprof_sample_t)) {
			mutex_enter(&tprof_lock);
			STAILQ_INSERT_HEAD(&tprof_list, buf, b_list);
			tprof_nbuf_on_list++;
			cv_broadcast(&tprof_reader_cv);
			mutex_exit(&tprof_lock);
		} else {
			tprof_buf_free(buf);
			tprof_reader_offset = 0;
		}
	}
	mutex_exit(&tprof_reader_lock);

	return error;
}

static int
tprof_ioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	const struct tprof_param *param;
	int error = 0;

	KASSERT(minor(dev) == 0);

	switch (cmd) {
	case TPROF_IOC_GETVERSION:
		*(int *)data = TPROF_VERSION;
		break;
	case TPROF_IOC_START:
		param = data;
		mutex_enter(&tprof_startstop_lock);
		error = tprof_start(param);
		mutex_exit(&tprof_startstop_lock);
		break;
	case TPROF_IOC_STOP:
		mutex_enter(&tprof_startstop_lock);
		tprof_stop();
		mutex_exit(&tprof_startstop_lock);
		break;
	case TPROF_IOC_GETSTAT:
		mutex_enter(&tprof_lock);
		memcpy(data, &tprof_stat, sizeof(tprof_stat));
		mutex_exit(&tprof_lock);
		break;
	default:
		error = EINVAL;
		break;
	}

	return error;
}

const struct cdevsw tprof_cdevsw = {
	.d_open = tprof_open,
	.d_close = tprof_close,
	.d_read = tprof_read,
	.d_write = nowrite,
	.d_ioctl = tprof_ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_flag = D_OTHER | D_MPSAFE,
};

void
tprofattach(int nunits)
{

	mutex_init(&tprof_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&tprof_reader_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&tprof_startstop_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&tprof_cv, "tprof");
	cv_init(&tprof_reader_cv, "tprofread");
	STAILQ_INIT(&tprof_list);
}
