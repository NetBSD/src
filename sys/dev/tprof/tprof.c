/*	$NetBSD: tprof.c,v 1.22 2022/12/16 17:38:56 ryo Exp $	*/

/*-
 * Copyright (c)2008,2009,2010 YAMAMOTO Takashi,
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
__KERNEL_RCSID(0, "$NetBSD: tprof.c,v 1.22 2022/12/16 17:38:56 ryo Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <sys/callout.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/percpu.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/select.h>
#include <sys/workqueue.h>
#include <sys/xcall.h>

#include <dev/tprof/tprof.h>
#include <dev/tprof/tprof_ioctl.h>

#include "ioconf.h"

#ifndef TPROF_HZ
#define TPROF_HZ	10000
#endif

/*
 * locking order:
 *	tprof_reader_lock -> tprof_lock
 *	tprof_startstop_lock -> tprof_lock
 */

/*
 * protected by:
 *	L: tprof_lock
 *	R: tprof_reader_lock
 *	S: tprof_startstop_lock
 *	s: writer should hold tprof_startstop_lock and tprof_lock
 *	   reader should hold tprof_startstop_lock or tprof_lock
 */

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
#define	TPROF_MAX_SAMPLES_PER_BUF	TPROF_HZ

typedef struct {
	tprof_buf_t *c_buf;
	uint32_t c_cpuid;
	struct work c_work;
	callout_t c_callout;
} __aligned(CACHE_LINE_SIZE) tprof_cpu_t;

typedef struct tprof_backend {
	/*
	 * tprof_backend_softc_t must be passed as an argument to the interrupt
	 * handler, but since this is difficult to implement in armv7/v8. Then,
	 * tprof_backend is exposed. Additionally, softc must be placed at the
	 * beginning of struct tprof_backend.
	 */
	tprof_backend_softc_t tb_softc;

	const char *tb_name;
	const tprof_backend_ops_t *tb_ops;
	LIST_ENTRY(tprof_backend) tb_list;
} tprof_backend_t;

static kmutex_t tprof_lock;
static u_int tprof_nworker;		/* L: # of running worker LWPs */
static lwp_t *tprof_owner;
static STAILQ_HEAD(, tprof_buf) tprof_list; /* L: global buffer list */
static u_int tprof_nbuf_on_list;	/* L: # of buffers on tprof_list */
static struct workqueue *tprof_wq;
static struct percpu *tprof_cpus __read_mostly;	/* tprof_cpu_t * */
static u_int tprof_samples_per_buf;
static u_int tprof_max_buf;

tprof_backend_t *tprof_backend;	/* S: */
static LIST_HEAD(, tprof_backend) tprof_backends =
    LIST_HEAD_INITIALIZER(tprof_backend); /* S: */

static kmutex_t tprof_reader_lock;
static kcondvar_t tprof_reader_cv;	/* L: */
static off_t tprof_reader_offset;	/* R: */

static kmutex_t tprof_startstop_lock;
static kcondvar_t tprof_cv;		/* L: */
static struct selinfo tprof_selp;	/* L: */

static struct tprof_stat tprof_stat;	/* L: */

static tprof_cpu_t *
tprof_cpu_direct(struct cpu_info *ci)
{
	tprof_cpu_t **cp;

	cp = percpu_getptr_remote(tprof_cpus, ci);
	return *cp;
}

static tprof_cpu_t *
tprof_cpu(struct cpu_info *ci)
{
	tprof_cpu_t *c;

	/*
	 * As long as xcalls are blocked -- e.g., by kpreempt_disable
	 * -- the percpu object will not be swapped and destroyed.  We
	 * can't write to it, because the data may have already been
	 * moved to a new buffer, but we can safely read from it.
	 */
	kpreempt_disable();
	c = tprof_cpu_direct(ci);
	kpreempt_enable();

	return c;
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
	tprof_backend_t *tb;
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
	tb = tprof_backend;
	shouldstop = (tb == NULL || tb->tb_softc.sc_ctr_running_mask == 0);
	if (shouldstop) {
		KASSERT(tprof_nworker > 0);
		tprof_nworker--;
		cv_broadcast(&tprof_cv);
		cv_broadcast(&tprof_reader_cv);
	}
	if (buf->b_used == 0) {
		tprof_stat.ts_emptybuf++;
	} else if (tprof_nbuf_on_list < tprof_max_buf) {
		tprof_stat.ts_sample += buf->b_used;
		tprof_stat.ts_overflow += buf->b_overflow;
		tprof_stat.ts_buf++;
		STAILQ_INSERT_TAIL(&tprof_list, buf, b_list);
		tprof_nbuf_on_list++;
		buf = NULL;
		selnotify(&tprof_selp, 0, NOTE_SUBMIT);
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
		callout_schedule(&c->c_callout, hz / 8);
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
	KASSERT(tprof_nworker == 0);

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

static void
tprof_getinfo(struct tprof_info *info)
{
	tprof_backend_t *tb;

	KASSERT(mutex_owned(&tprof_startstop_lock));

	memset(info, 0, sizeof(*info));
	info->ti_version = TPROF_VERSION;
	if ((tb = tprof_backend) != NULL) {
		info->ti_ident = tb->tb_ops->tbo_ident();
	}
}

static int
tprof_getncounters(u_int *ncounters)
{
	tprof_backend_t *tb;

	tb = tprof_backend;
	if (tb == NULL)
		return ENOENT;

	*ncounters = tb->tb_ops->tbo_ncounters();
	return 0;
}

static void
tprof_start_cpu(void *arg1, void *arg2)
{
	tprof_backend_t *tb = arg1;
	tprof_countermask_t runmask = (uintptr_t)arg2;

	tb->tb_ops->tbo_start(runmask);
}

static void
tprof_stop_cpu(void *arg1, void *arg2)
{
	tprof_backend_t *tb = arg1;
	tprof_countermask_t stopmask = (uintptr_t)arg2;

	tb->tb_ops->tbo_stop(stopmask);
}

static int
tprof_start(tprof_countermask_t runmask)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	tprof_backend_t *tb;
	uint64_t xc;
	int error;
	bool firstrun;

	KASSERT(mutex_owned(&tprof_startstop_lock));

	tb = tprof_backend;
	if (tb == NULL) {
		error = ENOENT;
		goto done;
	}

	runmask &= ~tb->tb_softc.sc_ctr_running_mask;
	runmask &= tb->tb_softc.sc_ctr_configured_mask;
	if (runmask == 0) {
		/*
		 * targets are already running.
		 * unconfigured counters are ignored.
		 */
		error = 0;
		goto done;
	}

	firstrun = (tb->tb_softc.sc_ctr_running_mask == 0);
	if (firstrun) {
		if (tb->tb_ops->tbo_establish != NULL) {
			error = tb->tb_ops->tbo_establish(&tb->tb_softc);
			if (error != 0)
				goto done;
		}

		tprof_samples_per_buf = TPROF_MAX_SAMPLES_PER_BUF;
		tprof_max_buf = ncpu * 3;
		error = workqueue_create(&tprof_wq, "tprofmv", tprof_worker,
		    NULL, PRI_NONE, IPL_SOFTCLOCK, WQ_MPSAFE | WQ_PERCPU);
		if (error != 0) {
			if (tb->tb_ops->tbo_disestablish != NULL)
				tb->tb_ops->tbo_disestablish(&tb->tb_softc);
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
	}

	runmask &= tb->tb_softc.sc_ctr_configured_mask;
	xc = xc_broadcast(0, tprof_start_cpu, tb, (void *)(uintptr_t)runmask);
	xc_wait(xc);
	mutex_enter(&tprof_lock);
	tb->tb_softc.sc_ctr_running_mask |= runmask;
	mutex_exit(&tprof_lock);

	if (firstrun) {
		for (CPU_INFO_FOREACH(cii, ci)) {
			tprof_cpu_t * const c = tprof_cpu(ci);

			mutex_enter(&tprof_lock);
			tprof_nworker++;
			mutex_exit(&tprof_lock);
			workqueue_enqueue(tprof_wq, &c->c_work, ci);
		}
	}
	error = 0;

done:
	return error;
}

static void
tprof_stop(tprof_countermask_t stopmask)
{
	tprof_backend_t *tb;
	uint64_t xc;

	tb = tprof_backend;
	if (tb == NULL)
		return;

	KASSERT(mutex_owned(&tprof_startstop_lock));
	stopmask &= tb->tb_softc.sc_ctr_running_mask;
	if (stopmask == 0) {
		/* targets are not running */
		goto done;
	}

	xc = xc_broadcast(0, tprof_stop_cpu, tb, (void *)(uintptr_t)stopmask);
	xc_wait(xc);
	mutex_enter(&tprof_lock);
	tb->tb_softc.sc_ctr_running_mask &= ~stopmask;
	mutex_exit(&tprof_lock);

	/* all counters have stopped? */
	if (tb->tb_softc.sc_ctr_running_mask == 0) {
		mutex_enter(&tprof_lock);
		cv_broadcast(&tprof_reader_cv);
		while (tprof_nworker > 0) {
			cv_wait(&tprof_cv, &tprof_lock);
		}
		mutex_exit(&tprof_lock);

		tprof_stop1();
		if (tb->tb_ops->tbo_disestablish != NULL)
			tb->tb_ops->tbo_disestablish(&tb->tb_softc);
	}
done:
	;
}

static void
tprof_init_percpu_counters_offset(void *vp, void *vp2, struct cpu_info *ci)
{
	uint64_t *counters_offset = vp;
	u_int counter = (uintptr_t)vp2;

	tprof_backend_t *tb = tprof_backend;
	tprof_param_t *param = &tb->tb_softc.sc_count[counter].ctr_param;
	counters_offset[counter] = param->p_value;
}

static void
tprof_configure_event_cpu(void *arg1, void *arg2)
{
	tprof_backend_t *tb = arg1;
	u_int counter = (uintptr_t)arg2;
	tprof_param_t *param = &tb->tb_softc.sc_count[counter].ctr_param;

	tb->tb_ops->tbo_configure_event(counter, param);
}

static int
tprof_configure_event(const tprof_param_t *param)
{
	tprof_backend_t *tb;
	tprof_backend_softc_t *sc;
	tprof_param_t *sc_param;
	uint64_t xc;
	int c, error;

	if ((param->p_flags & (TPROF_PARAM_USER | TPROF_PARAM_KERN)) == 0) {
		error = EINVAL;
		goto done;
	}

	tb = tprof_backend;
	if (tb == NULL) {
		error = ENOENT;
		goto done;
	}
	sc = &tb->tb_softc;

	c = param->p_counter;
	if (c >= tb->tb_softc.sc_ncounters) {
		error = EINVAL;
		goto done;
	}

	if (tb->tb_ops->tbo_valid_event != NULL) {
		error = tb->tb_ops->tbo_valid_event(param->p_counter, param);
		if (error != 0)
			goto done;
	}

	/* if already running, stop the counter */
	if (ISSET(c, tb->tb_softc.sc_ctr_running_mask))
		tprof_stop(__BIT(c));

	sc->sc_count[c].ctr_bitwidth =
	    tb->tb_ops->tbo_counter_bitwidth(param->p_counter);

	sc_param = &sc->sc_count[c].ctr_param;
	memcpy(sc_param, param, sizeof(*sc_param));	/* save copy of param */

	if (ISSET(param->p_flags, TPROF_PARAM_PROFILE)) {
		uint64_t freq, inum, dnum;

		freq = tb->tb_ops->tbo_counter_estimate_freq(c);
		sc->sc_count[c].ctr_counter_val = freq / TPROF_HZ;
		if (sc->sc_count[c].ctr_counter_val == 0) {
			printf("%s: counter#%d frequency (%"PRIu64") is"
			    " very low relative to TPROF_HZ (%u)\n", __func__,
			    c, freq, TPROF_HZ);
			sc->sc_count[c].ctr_counter_val =
			    4000000000ULL / TPROF_HZ;
		}

		switch (param->p_flags & TPROF_PARAM_VALUE2_MASK) {
		case TPROF_PARAM_VALUE2_SCALE:
			if (sc_param->p_value2 == 0)
				break;
			/*
			 * p_value2 is 64-bit fixed-point
			 * upper 32 bits are the integer part
			 * lower 32 bits are the decimal part
			 */
			inum = sc_param->p_value2 >> 32;
			dnum = sc_param->p_value2 & __BITS(31, 0);
			sc->sc_count[c].ctr_counter_val =
			    sc->sc_count[c].ctr_counter_val * inum +
			    (sc->sc_count[c].ctr_counter_val * dnum >> 32);
			if (sc->sc_count[c].ctr_counter_val == 0)
				sc->sc_count[c].ctr_counter_val = 1;
			break;
		case TPROF_PARAM_VALUE2_TRIGGERCOUNT:
			if (sc_param->p_value2 == 0)
				sc_param->p_value2 = 1;
			if (sc_param->p_value2 >
			    __BITS(sc->sc_count[c].ctr_bitwidth - 1, 0)) {
				sc_param->p_value2 =
				    __BITS(sc->sc_count[c].ctr_bitwidth - 1, 0);
			}
			sc->sc_count[c].ctr_counter_val = sc_param->p_value2;
			break;
		default:
			break;
		}
		sc->sc_count[c].ctr_counter_reset_val =
		    -sc->sc_count[c].ctr_counter_val;
		sc->sc_count[c].ctr_counter_reset_val &=
		    __BITS(sc->sc_count[c].ctr_bitwidth - 1, 0);
	} else {
		sc->sc_count[c].ctr_counter_val = 0;
		sc->sc_count[c].ctr_counter_reset_val = 0;
	}

	/* At this point, p_value is used as an initial value */
	percpu_foreach(tb->tb_softc.sc_ctr_offset_percpu,
	    tprof_init_percpu_counters_offset, (void *)(uintptr_t)c);
	/* On the backend side, p_value is used as the reset value */
	sc_param->p_value = tb->tb_softc.sc_count[c].ctr_counter_reset_val;

	xc = xc_broadcast(0, tprof_configure_event_cpu,
	    tb, (void *)(uintptr_t)c);
	xc_wait(xc);

	mutex_enter(&tprof_lock);
	/* update counters bitmasks */
	SET(tb->tb_softc.sc_ctr_configured_mask, __BIT(c));
	CLR(tb->tb_softc.sc_ctr_prof_mask, __BIT(c));
	CLR(tb->tb_softc.sc_ctr_ovf_mask, __BIT(c));
	/* profiled counter requires overflow handling */
	if (ISSET(param->p_flags, TPROF_PARAM_PROFILE)) {
		SET(tb->tb_softc.sc_ctr_prof_mask, __BIT(c));
		SET(tb->tb_softc.sc_ctr_ovf_mask, __BIT(c));
	}
	/* counters with less than 64bits also require overflow handling */
	if (sc->sc_count[c].ctr_bitwidth != 64)
		SET(tb->tb_softc.sc_ctr_ovf_mask, __BIT(c));
	mutex_exit(&tprof_lock);

	error = 0;

 done:
	return error;
}

static void
tprof_getcounts_cpu(void *arg1, void *arg2)
{
	tprof_backend_t *tb = arg1;
	tprof_backend_softc_t *sc = &tb->tb_softc;
	uint64_t *counters = arg2;
	uint64_t *counters_offset;
	unsigned int c;

	tprof_countermask_t configmask = sc->sc_ctr_configured_mask;
	counters_offset = percpu_getref(sc->sc_ctr_offset_percpu);
	for (c = 0; c < sc->sc_ncounters; c++) {
		if (ISSET(configmask, __BIT(c))) {
			uint64_t ctr = tb->tb_ops->tbo_counter_read(c);
			counters[c] = counters_offset[c] +
			    ((ctr - sc->sc_count[c].ctr_counter_reset_val) &
			    __BITS(sc->sc_count[c].ctr_bitwidth - 1, 0));
		} else {
			counters[c] = 0;
		}
	}
	percpu_putref(sc->sc_ctr_offset_percpu);
}

static int
tprof_getcounts(tprof_counts_t *counts)
{
	struct cpu_info *ci;
	tprof_backend_t *tb;
	uint64_t xc;

	tb = tprof_backend;
	if (tb == NULL)
		return ENOENT;

	if (counts->c_cpu >= ncpu)
		return ESRCH;
	ci = cpu_lookup(counts->c_cpu);
	if (ci == NULL)
		return ESRCH;

	xc = xc_unicast(0, tprof_getcounts_cpu, tb, counts->c_count, ci);
	xc_wait(xc);

	counts->c_ncounters = tb->tb_softc.sc_ncounters;
	counts->c_runningmask = tb->tb_softc.sc_ctr_running_mask;
	return 0;
}

/*
 * tprof_clear: drain unread samples.
 */

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

static tprof_backend_t *
tprof_backend_lookup(const char *name)
{
	tprof_backend_t *tb;

	KASSERT(mutex_owned(&tprof_startstop_lock));

	LIST_FOREACH(tb, &tprof_backends, tb_list) {
		if (!strcmp(tb->tb_name, name)) {
			return tb;
		}
	}
	return NULL;
}

/* -------------------- backend interfaces */

/*
 * tprof_sample: record a sample on the per-cpu buffer.
 *
 * be careful; can be called in NMI context.
 * we are bluntly assuming the followings are safe.
 *	curcpu()
 *	curlwp->l_lid
 *	curlwp->l_proc->p_pid
 */

void
tprof_sample(void *unused, const tprof_frame_info_t *tfi)
{
	tprof_cpu_t * const c = tprof_cpu_direct(curcpu());
	tprof_buf_t * const buf = c->c_buf;
	tprof_sample_t *sp;
	const uintptr_t pc = tfi->tfi_pc;
	const lwp_t * const l = curlwp;
	u_int idx;

	idx = buf->b_used;
	if (__predict_false(idx >= buf->b_size)) {
		buf->b_overflow++;
		return;
	}
	sp = &buf->b_data[idx];
	sp->s_pid = l->l_proc->p_pid;
	sp->s_lwpid = l->l_lid;
	sp->s_cpuid = c->c_cpuid;
	sp->s_flags = ((tfi->tfi_inkernel) ? TPROF_SAMPLE_INKERNEL : 0) |
	    __SHIFTIN(tfi->tfi_counter, TPROF_SAMPLE_COUNTER_MASK);
	sp->s_pc = pc;
	buf->b_used = idx + 1;
}

/*
 * tprof_backend_register:
 */

int
tprof_backend_register(const char *name, const tprof_backend_ops_t *ops,
    int vers)
{
	tprof_backend_t *tb;

	if (vers != TPROF_BACKEND_VERSION) {
		return EINVAL;
	}

	mutex_enter(&tprof_startstop_lock);
	tb = tprof_backend_lookup(name);
	if (tb != NULL) {
		mutex_exit(&tprof_startstop_lock);
		return EEXIST;
	}
#if 1 /* XXX for now */
	if (!LIST_EMPTY(&tprof_backends)) {
		mutex_exit(&tprof_startstop_lock);
		return ENOTSUP;
	}
#endif
	tb = kmem_zalloc(sizeof(*tb), KM_SLEEP);
	tb->tb_name = name;
	tb->tb_ops = ops;
	LIST_INSERT_HEAD(&tprof_backends, tb, tb_list);
#if 1 /* XXX for now */
	if (tprof_backend == NULL) {
		tprof_backend = tb;
	}
#endif
	mutex_exit(&tprof_startstop_lock);

	/* init backend softc */
	tb->tb_softc.sc_ncounters = tb->tb_ops->tbo_ncounters();
	tb->tb_softc.sc_ctr_offset_percpu_size =
	    sizeof(uint64_t) * tb->tb_softc.sc_ncounters;
	tb->tb_softc.sc_ctr_offset_percpu =
	    percpu_alloc(tb->tb_softc.sc_ctr_offset_percpu_size);

	return 0;
}

/*
 * tprof_backend_unregister:
 */

int
tprof_backend_unregister(const char *name)
{
	tprof_backend_t *tb;

	mutex_enter(&tprof_startstop_lock);
	tb = tprof_backend_lookup(name);
#if defined(DIAGNOSTIC)
	if (tb == NULL) {
		mutex_exit(&tprof_startstop_lock);
		panic("%s: not found '%s'", __func__, name);
	}
#endif /* defined(DIAGNOSTIC) */
	if (tb->tb_softc.sc_ctr_running_mask != 0) {
		mutex_exit(&tprof_startstop_lock);
		return EBUSY;
	}
#if 1 /* XXX for now */
	if (tprof_backend == tb) {
		tprof_backend = NULL;
	}
#endif
	LIST_REMOVE(tb, tb_list);
	mutex_exit(&tprof_startstop_lock);

	/* fini backend softc */
	percpu_free(tb->tb_softc.sc_ctr_offset_percpu,
	    tb->tb_softc.sc_ctr_offset_percpu_size);

	/* free backend */
	kmem_free(tb, sizeof(*tb));

	return 0;
}

/* -------------------- cdevsw interfaces */

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
	tprof_stop(TPROF_COUNTERMASK_ALL);
	tprof_clear();

	tprof_backend_t *tb = tprof_backend;
	if (tb != NULL) {
		KASSERT(tb->tb_softc.sc_ctr_running_mask == 0);
		tb->tb_softc.sc_ctr_configured_mask = 0;
		tb->tb_softc.sc_ctr_prof_mask = 0;
		tb->tb_softc.sc_ctr_ovf_mask = 0;
	}

	mutex_exit(&tprof_startstop_lock);

	return 0;
}

static int
tprof_poll(dev_t dev, int events, struct lwp *l)
{
	int revents;

	revents = events & (POLLIN | POLLRDNORM);
	if (revents == 0)
		return 0;

	mutex_enter(&tprof_lock);
	if (STAILQ_EMPTY(&tprof_list)) {
		revents = 0;
		selrecord(l, &tprof_selp);
	}
	mutex_exit(&tprof_lock);

	return revents;
}

static void
filt_tprof_read_detach(struct knote *kn)
{
	mutex_enter(&tprof_lock);
	selremove_knote(&tprof_selp, kn);
	mutex_exit(&tprof_lock);
}

static int
filt_tprof_read_event(struct knote *kn, long hint)
{
	int rv = 0;

	if ((hint & NOTE_SUBMIT) == 0)
		mutex_enter(&tprof_lock);

	if (!STAILQ_EMPTY(&tprof_list)) {
		tprof_buf_t *buf;
		int64_t n = 0;

		STAILQ_FOREACH(buf, &tprof_list, b_list) {
			n += buf->b_used;
		}
		kn->kn_data = n * sizeof(tprof_sample_t);

		rv = 1;
	}

	if ((hint & NOTE_SUBMIT) == 0)
		mutex_exit(&tprof_lock);

	return rv;
}

static const struct filterops tprof_read_filtops = {
	.f_flags = FILTEROP_ISFD | FILTEROP_MPSAFE,
	.f_attach = NULL,
	.f_detach = filt_tprof_read_detach,
	.f_event = filt_tprof_read_event,
};

static int
tprof_kqfilter(dev_t dev, struct knote *kn)
{
	switch (kn->kn_filter) {
	case EVFILT_READ:
		kn->kn_fop = &tprof_read_filtops;
		mutex_enter(&tprof_lock);
		selrecord_knote(&tprof_selp, kn);
		mutex_exit(&tprof_lock);
		break;
	default:
		return EINVAL;
	}

	return 0;
}

static int
tprof_read(dev_t dev, struct uio *uio, int flags)
{
	tprof_buf_t *buf;
	size_t bytes;
	size_t resid;
	size_t done = 0;
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
			if (tprof_nworker == 0 || done != 0) {
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
	const tprof_param_t *param;
	tprof_counts_t *counts;
	int error = 0;

	KASSERT(minor(dev) == 0);

	switch (cmd) {
	case TPROF_IOC_GETINFO:
		mutex_enter(&tprof_startstop_lock);
		tprof_getinfo(data);
		mutex_exit(&tprof_startstop_lock);
		break;
	case TPROF_IOC_GETNCOUNTERS:
		mutex_enter(&tprof_lock);
		error = tprof_getncounters((u_int *)data);
		mutex_exit(&tprof_lock);
		break;
	case TPROF_IOC_START:
		mutex_enter(&tprof_startstop_lock);
		error = tprof_start(*(tprof_countermask_t *)data);
		mutex_exit(&tprof_startstop_lock);
		break;
	case TPROF_IOC_STOP:
		mutex_enter(&tprof_startstop_lock);
		tprof_stop(*(tprof_countermask_t *)data);
		mutex_exit(&tprof_startstop_lock);
		break;
	case TPROF_IOC_GETSTAT:
		mutex_enter(&tprof_lock);
		memcpy(data, &tprof_stat, sizeof(tprof_stat));
		mutex_exit(&tprof_lock);
		break;
	case TPROF_IOC_CONFIGURE_EVENT:
		param = data;
		mutex_enter(&tprof_startstop_lock);
		error = tprof_configure_event(param);
		mutex_exit(&tprof_startstop_lock);
		break;
	case TPROF_IOC_GETCOUNTS:
		counts = data;
		mutex_enter(&tprof_startstop_lock);
		error = tprof_getcounts(counts);
		mutex_exit(&tprof_startstop_lock);
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
	.d_poll = tprof_poll,
	.d_mmap = nommap,
	.d_kqfilter = tprof_kqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};

void
tprofattach(int nunits)
{

	/* nothing */
}

MODULE(MODULE_CLASS_DRIVER, tprof, NULL);

static void
tprof_cpu_init(void *vcp, void *vcookie, struct cpu_info *ci)
{
	tprof_cpu_t **cp = vcp, *c;

	c = kmem_zalloc(sizeof(*c), KM_SLEEP);
	c->c_buf = NULL;
	c->c_cpuid = cpu_index(ci);
	*cp = c;
}

static void
tprof_cpu_fini(void *vcp, void *vcookie, struct cpu_info *ci)
{
	tprof_cpu_t **cp = vcp, *c;

	c = *cp;
	KASSERT(c->c_cpuid == cpu_index(ci));
	KASSERT(c->c_buf == NULL);
	kmem_free(c, sizeof(*c));
	*cp = NULL;
}

static void
tprof_driver_init(void)
{

	mutex_init(&tprof_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&tprof_reader_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&tprof_startstop_lock, MUTEX_DEFAULT, IPL_NONE);
	selinit(&tprof_selp);
	cv_init(&tprof_cv, "tprof");
	cv_init(&tprof_reader_cv, "tprof_rd");
	STAILQ_INIT(&tprof_list);
	tprof_cpus = percpu_create(sizeof(tprof_cpu_t *),
	    tprof_cpu_init, tprof_cpu_fini, NULL);
}

static void
tprof_driver_fini(void)
{

	percpu_free(tprof_cpus, sizeof(tprof_cpu_t *));
	mutex_destroy(&tprof_lock);
	mutex_destroy(&tprof_reader_lock);
	mutex_destroy(&tprof_startstop_lock);
	seldestroy(&tprof_selp);
	cv_destroy(&tprof_cv);
	cv_destroy(&tprof_reader_cv);
}

static int
tprof_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		tprof_driver_init();
#if defined(_MODULE)
		{
			devmajor_t bmajor = NODEVMAJOR;
			devmajor_t cmajor = NODEVMAJOR;
			int error;

			error = devsw_attach("tprof", NULL, &bmajor,
			    &tprof_cdevsw, &cmajor);
			if (error) {
				tprof_driver_fini();
				return error;
			}
		}
#endif /* defined(_MODULE) */
		return 0;

	case MODULE_CMD_FINI:
#if defined(_MODULE)
		devsw_detach(NULL, &tprof_cdevsw);
#endif /* defined(_MODULE) */
		tprof_driver_fini();
		return 0;

	default:
		return ENOTTY;
	}
}
