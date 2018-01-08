/*	$NetBSD: crypto.c,v 1.103 2018/01/08 23:23:25 knakahara Exp $ */
/*	$FreeBSD: src/sys/opencrypto/crypto.c,v 1.4.2.5 2003/02/26 00:14:05 sam Exp $	*/
/*	$OpenBSD: crypto.c,v 1.41 2002/07/17 23:52:38 art Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Coyote Point Systems, Inc.
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
 * The author of this code is Angelos D. Keromytis (angelos@cis.upenn.edu)
 *
 * This code was written by Angelos D. Keromytis in Athens, Greece, in
 * February 2000. Network Security Technologies Inc. (NSTI) kindly
 * supported the development of this code.
 *
 * Copyright (c) 2000, 2001 Angelos D. Keromytis
 *
 * Permission to use, copy, and modify this software with or without fee
 * is hereby granted, provided that this entire notice is included in
 * all source code copies of any software which is or includes a copy or
 * modification of this software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTY. IN PARTICULAR, NONE OF THE AUTHORS MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE
 * MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
 * PURPOSE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: crypto.c,v 1.103 2018/01/08 23:23:25 knakahara Exp $");

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/kthread.h>
#include <sys/once.h>
#include <sys/sysctl.h>
#include <sys/intr.h>
#include <sys/errno.h>
#include <sys/module.h>
#include <sys/xcall.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/percpu.h>
#include <sys/kmem.h>

#if defined(_KERNEL_OPT)
#include "opt_ocf.h"
#endif

#include <opencrypto/cryptodev.h>
#include <opencrypto/xform.h>			/* XXX for M_XDATA */

/*
 * Crypto drivers register themselves by allocating a slot in the
 * crypto_drivers table with crypto_get_driverid() and then registering
 * each algorithm they support with crypto_register() and crypto_kregister().
 */
/* Don't directly access crypto_drivers[i], use crypto_checkdriver(i). */
static struct {
	kmutex_t mtx;
	int num;
	struct cryptocap *list;
} crypto_drv __cacheline_aligned;
#define crypto_drv_mtx		(crypto_drv.mtx)
#define crypto_drivers_num	(crypto_drv.num)
#define crypto_drivers		(crypto_drv.list)

static	void *crypto_q_si;
static	void *crypto_ret_si;

/*
 * There are two queues for crypto requests; one for symmetric (e.g.
 * cipher) operations and one for asymmetric (e.g. MOD) operations.
 * See below for how synchronization is handled.
 */
TAILQ_HEAD(crypto_crp_q, cryptop);
TAILQ_HEAD(crypto_crp_kq, cryptkop);
struct crypto_crp_qs {
	struct crypto_crp_q *crp_q;
	struct crypto_crp_kq *crp_kq;
};
static percpu_t *crypto_crp_qs_percpu;

static inline struct crypto_crp_qs *
crypto_get_crp_qs(int *s)
{

	KASSERT(s != NULL);

	*s = splsoftnet();
	return percpu_getref(crypto_crp_qs_percpu);
}

static inline void
crypto_put_crp_qs(int *s)
{

	KASSERT(s != NULL);

	percpu_putref(crypto_crp_qs_percpu);
	splx(*s);
}

static void
crypto_crp_q_is_busy_pc(void *p, void *arg, struct cpu_info *ci __unused)
{
	struct crypto_crp_qs *qs_pc = p;
	bool *isempty = arg;

	if (!TAILQ_EMPTY(qs_pc->crp_q) || !TAILQ_EMPTY(qs_pc->crp_kq))
		*isempty = true;
}

static void
crypto_crp_qs_init_pc(void *p, void *arg __unused, struct cpu_info *ci __unused)
{
	struct crypto_crp_qs *qs = p;

	qs->crp_q = kmem_alloc(sizeof(struct crypto_crp_q), KM_SLEEP);
	qs->crp_kq = kmem_alloc(sizeof(struct crypto_crp_kq), KM_SLEEP);

	TAILQ_INIT(qs->crp_q);
	TAILQ_INIT(qs->crp_kq);
}

/*
 * There are two queues for processing completed crypto requests; one
 * for the symmetric and one for the asymmetric ops.  We only need one
 * but have two to avoid type futzing (cryptop vs. cryptkop).  See below
 * for how synchronization is handled.
 */
TAILQ_HEAD(crypto_crp_ret_q, cryptop);
TAILQ_HEAD(crypto_crp_ret_kq, cryptkop);
struct crypto_crp_ret_qs {
	kmutex_t crp_ret_q_mtx;
	bool crp_ret_q_exit_flag;

	struct crypto_crp_ret_q crp_ret_q;
	int crp_ret_q_len;
	int crp_ret_q_maxlen; /* queue length limit. <=0 means unlimited. */
	int crp_ret_q_drops;

	struct crypto_crp_ret_kq crp_ret_kq;
	int crp_ret_kq_len;
	int crp_ret_kq_maxlen; /* queue length limit. <=0 means unlimited. */
	int crp_ret_kq_drops;
};
struct crypto_crp_ret_qs **crypto_crp_ret_qs_list;


static inline struct crypto_crp_ret_qs *
crypto_get_crp_ret_qs(struct cpu_info *ci)
{
	u_int cpuid;
	struct crypto_crp_ret_qs *qs;

	KASSERT(ci != NULL);

	cpuid = cpu_index(ci);
	qs = crypto_crp_ret_qs_list[cpuid];
	mutex_enter(&qs->crp_ret_q_mtx);
	return qs;
}

static inline void
crypto_put_crp_ret_qs(struct cpu_info *ci)
{
	u_int cpuid;
	struct crypto_crp_ret_qs *qs;

	KASSERT(ci != NULL);

	cpuid = cpu_index(ci);
	qs = crypto_crp_ret_qs_list[cpuid];
	mutex_exit(&qs->crp_ret_q_mtx);
}

#ifndef CRYPTO_RET_Q_MAXLEN
#define CRYPTO_RET_Q_MAXLEN 0
#endif
#ifndef CRYPTO_RET_KQ_MAXLEN
#define CRYPTO_RET_KQ_MAXLEN 0
#endif

static int
sysctl_opencrypto_q_len(SYSCTLFN_ARGS)
{
	int error, len = 0;
	struct sysctlnode node = *rnode;

	for (int i = 0; i < ncpu; i++) {
		struct crypto_crp_ret_qs *qs;
		struct cpu_info *ci = cpu_lookup(i);

		qs = crypto_get_crp_ret_qs(ci);
		len += qs->crp_ret_q_len;
		crypto_put_crp_ret_qs(ci);
	}

	node.sysctl_data = &len;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	return 0;
}

static int
sysctl_opencrypto_q_drops(SYSCTLFN_ARGS)
{
	int error, drops = 0;
	struct sysctlnode node = *rnode;

	for (int i = 0; i < ncpu; i++) {
		struct crypto_crp_ret_qs *qs;
		struct cpu_info *ci = cpu_lookup(i);

		qs = crypto_get_crp_ret_qs(ci);
		drops += qs->crp_ret_q_drops;
		crypto_put_crp_ret_qs(ci);
	}

	node.sysctl_data = &drops;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	return 0;
}

static int
sysctl_opencrypto_q_maxlen(SYSCTLFN_ARGS)
{
	int error, maxlen;
	struct crypto_crp_ret_qs *qs;
	struct sysctlnode node = *rnode;

	/* each crp_ret_kq_maxlen is the same. */
	qs = crypto_get_crp_ret_qs(curcpu());
	maxlen = qs->crp_ret_q_maxlen;
	crypto_put_crp_ret_qs(curcpu());

	node.sysctl_data = &maxlen;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	for (int i = 0; i < ncpu; i++) {
		struct cpu_info *ci = cpu_lookup(i);

		qs = crypto_get_crp_ret_qs(ci);
		qs->crp_ret_q_maxlen = maxlen;
		crypto_put_crp_ret_qs(ci);
	}

	return 0;
}

static int
sysctl_opencrypto_kq_len(SYSCTLFN_ARGS)
{
	int error, len = 0;
	struct sysctlnode node = *rnode;

	for (int i = 0; i < ncpu; i++) {
		struct crypto_crp_ret_qs *qs;
		struct cpu_info *ci = cpu_lookup(i);

		qs = crypto_get_crp_ret_qs(ci);
		len += qs->crp_ret_kq_len;
		crypto_put_crp_ret_qs(ci);
	}

	node.sysctl_data = &len;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	return 0;
}

static int
sysctl_opencrypto_kq_drops(SYSCTLFN_ARGS)
{
	int error, drops = 0;
	struct sysctlnode node = *rnode;

	for (int i = 0; i < ncpu; i++) {
		struct crypto_crp_ret_qs *qs;
		struct cpu_info *ci = cpu_lookup(i);

		qs = crypto_get_crp_ret_qs(ci);
		drops += qs->crp_ret_kq_drops;
		crypto_put_crp_ret_qs(ci);
	}

	node.sysctl_data = &drops;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	return 0;
}

static int
sysctl_opencrypto_kq_maxlen(SYSCTLFN_ARGS)
{
	int error, maxlen;
	struct crypto_crp_ret_qs *qs;
	struct sysctlnode node = *rnode;

	/* each crp_ret_kq_maxlen is the same. */
	qs = crypto_get_crp_ret_qs(curcpu());
	maxlen = qs->crp_ret_kq_maxlen;
	crypto_put_crp_ret_qs(curcpu());

	node.sysctl_data = &maxlen;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	for (int i = 0; i < ncpu; i++) {
		struct cpu_info *ci = cpu_lookup(i);

		qs = crypto_get_crp_ret_qs(ci);
		qs->crp_ret_kq_maxlen = maxlen;
		crypto_put_crp_ret_qs(ci);
	}

	return 0;
}

/*
 * Crypto op and descriptor data structures are allocated
 * from separate private zones(FreeBSD)/pools(netBSD/OpenBSD) .
 */
static pool_cache_t cryptop_cache;
static pool_cache_t cryptodesc_cache;
static pool_cache_t cryptkop_cache;

int	crypto_usercrypto = 1;		/* userland may open /dev/crypto */
int	crypto_userasymcrypto = 1;	/* userland may do asym crypto reqs */
/*
 * cryptodevallowsoft is (intended to be) sysctl'able, controlling
 * access to hardware versus software transforms as below:
 *
 * crypto_devallowsoft < 0:  Force userlevel requests to use software
 *                              transforms, always
 * crypto_devallowsoft = 0:  Use hardware if present, grant userlevel
 *                              requests for non-accelerated transforms
 *                              (handling the latter in software)
 * crypto_devallowsoft > 0:  Allow user requests only for transforms which
 *                               are hardware-accelerated.
 */
int	crypto_devallowsoft = 1;	/* only use hardware crypto */

static void
sysctl_opencrypto_setup(struct sysctllog **clog)
{
	const struct sysctlnode *ocnode;
	const struct sysctlnode *retqnode, *retkqnode;

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "usercrypto",
		       SYSCTL_DESCR("Enable/disable user-mode access to "
			   "crypto support"),
		       NULL, 0, &crypto_usercrypto, 0,
		       CTL_KERN, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "userasymcrypto",
		       SYSCTL_DESCR("Enable/disable user-mode access to "
			   "asymmetric crypto support"),
		       NULL, 0, &crypto_userasymcrypto, 0,
		       CTL_KERN, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "cryptodevallowsoft",
		       SYSCTL_DESCR("Enable/disable use of software "
			   "asymmetric crypto support"),
		       NULL, 0, &crypto_devallowsoft, 0,
		       CTL_KERN, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, &ocnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "opencrypto",
		       SYSCTL_DESCR("opencrypto related entries"),
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &ocnode, &retqnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "crypto_ret_q",
		       SYSCTL_DESCR("crypto_ret_q related entries"),
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &retqnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_INT, "len",
		       SYSCTL_DESCR("Current queue length"),
		       sysctl_opencrypto_q_len, 0,
		       NULL, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &retqnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_INT, "drops",
		       SYSCTL_DESCR("Crypto requests dropped due to full ret queue"),
		       sysctl_opencrypto_q_drops, 0,
		       NULL, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &retqnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "maxlen",
		       SYSCTL_DESCR("Maximum allowed queue length"),
		       sysctl_opencrypto_q_maxlen, 0,
		       NULL, 0,
		       CTL_CREATE, CTL_EOL);


	sysctl_createv(clog, 0, &ocnode, &retkqnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "crypto_ret_kq",
		       SYSCTL_DESCR("crypto_ret_kq related entries"),
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &retkqnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_INT, "len",
		       SYSCTL_DESCR("Current queue length"),
		       sysctl_opencrypto_kq_len, 0,
		       NULL, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &retkqnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_INT, "drops",
		       SYSCTL_DESCR("Crypto requests dropped due to full ret queue"),
		       sysctl_opencrypto_kq_drops, 0,
		       NULL, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &retkqnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "maxlen",
		       SYSCTL_DESCR("Maximum allowed queue length"),
		       sysctl_opencrypto_kq_maxlen, 0,
		       NULL, 0,
		       CTL_CREATE, CTL_EOL);
}

/*
 * Synchronization: read carefully, this is non-trivial.
 *
 * Crypto requests are submitted via crypto_dispatch.  Typically
 * these come in from network protocols at spl0 (output path) or
 * spl[,soft]net (input path).
 *
 * Requests are typically passed on the driver directly, but they
 * may also be queued for processing by a software interrupt thread,
 * cryptointr, that runs at splsoftcrypto.  This thread dispatches
 * the requests to crypto drivers (h/w or s/w) who call crypto_done
 * when a request is complete.  Hardware crypto drivers are assumed
 * to register their IRQ's as network devices so their interrupt handlers
 * and subsequent "done callbacks" happen at spl[imp,net].
 *
 * Completed crypto ops are queued for a separate kernel thread that
 * handles the callbacks at spl0.  This decoupling insures the crypto
 * driver interrupt service routine is not delayed while the callback
 * takes place and that callbacks are delivered after a context switch
 * (as opposed to a software interrupt that clients must block).
 *
 * This scheme is not intended for SMP machines.
 */
static	void cryptointr(void *);	/* swi thread to dispatch ops */
static	void cryptoret_softint(void *);	/* kernel thread for callbacks*/
static	int crypto_destroy(bool);
static	int crypto_invoke(struct cryptop *crp, int hint);
static	int crypto_kinvoke(struct cryptkop *krp, int hint);

static struct cryptocap *crypto_checkdriver_lock(u_int32_t);
static struct cryptocap *crypto_checkdriver_uninit(u_int32_t);
static struct cryptocap *crypto_checkdriver(u_int32_t);
static void crypto_driver_lock(struct cryptocap *);
static void crypto_driver_unlock(struct cryptocap *);
static void crypto_driver_clear(struct cryptocap *);

static int crypto_init_finalize(device_t);

static struct cryptostats cryptostats;
#ifdef CRYPTO_TIMING
static	int crypto_timing = 0;
#endif

static struct sysctllog *sysctl_opencrypto_clog;

static int
crypto_crp_ret_qs_init(void)
{
	int i, j;

	crypto_crp_ret_qs_list = kmem_alloc(sizeof(struct crypto_crp_ret_qs *) * ncpu,
	    KM_NOSLEEP);
	if (crypto_crp_ret_qs_list == NULL) {
		printf("crypto_init: crypto_crp_qs_list\n");
		return ENOMEM;
	}

	for (i = 0; i < ncpu; i++) {
		struct crypto_crp_ret_qs *qs;
		qs = kmem_alloc(sizeof(struct crypto_crp_ret_qs), KM_NOSLEEP);
		if (qs == NULL)
			break;

		mutex_init(&qs->crp_ret_q_mtx, MUTEX_DEFAULT, IPL_NET);
		qs->crp_ret_q_exit_flag = false;

		TAILQ_INIT(&qs->crp_ret_q);
		qs->crp_ret_q_len = 0;
		qs->crp_ret_q_maxlen = CRYPTO_RET_Q_MAXLEN;
		qs->crp_ret_q_drops = 0;

		TAILQ_INIT(&qs->crp_ret_kq);
		qs->crp_ret_kq_len = 0;
		qs->crp_ret_kq_maxlen = CRYPTO_RET_KQ_MAXLEN;
		qs->crp_ret_kq_drops = 0;

		crypto_crp_ret_qs_list[i] = qs;
	}
	if (i == ncpu)
		return 0;

	for (j = 0; j < i; j++) {
		struct crypto_crp_ret_qs *qs = crypto_crp_ret_qs_list[j];

		mutex_destroy(&qs->crp_ret_q_mtx);
		kmem_free(qs, sizeof(struct crypto_crp_ret_qs));
	}
	kmem_free(crypto_crp_ret_qs_list, sizeof(struct crypto_crp_ret_qs *) * ncpu);

	return ENOMEM;
}

static int
crypto_init0(void)
{
	int error;

	mutex_init(&crypto_drv_mtx, MUTEX_DEFAULT, IPL_NONE);
	cryptop_cache = pool_cache_init(sizeof(struct cryptop),
	    coherency_unit, 0, 0, "cryptop", NULL, IPL_NET, NULL, NULL, NULL);
	cryptodesc_cache = pool_cache_init(sizeof(struct cryptodesc),
	    coherency_unit, 0, 0, "cryptdesc", NULL, IPL_NET, NULL, NULL, NULL);
	cryptkop_cache = pool_cache_init(sizeof(struct cryptkop),
	    coherency_unit, 0, 0, "cryptkop", NULL, IPL_NET, NULL, NULL, NULL);

	crypto_crp_qs_percpu = percpu_alloc(sizeof(struct crypto_crp_qs));
	percpu_foreach(crypto_crp_qs_percpu, crypto_crp_qs_init_pc, NULL);

	error = crypto_crp_ret_qs_init();
	if (error) {
		printf("crypto_init: cannot malloc ret_q list\n");
		return ENOMEM;
	}

	crypto_drivers = kmem_zalloc(CRYPTO_DRIVERS_INITIAL *
	    sizeof(struct cryptocap), KM_NOSLEEP);
	if (crypto_drivers == NULL) {
		printf("crypto_init: cannot malloc driver table\n");
		return ENOMEM;
	}
	crypto_drivers_num = CRYPTO_DRIVERS_INITIAL;

	crypto_q_si = softint_establish(SOFTINT_NET|SOFTINT_MPSAFE, cryptointr, NULL);
	if (crypto_q_si == NULL) {
		printf("crypto_init: cannot establish request queue handler\n");
		return crypto_destroy(false);
	}

	/*
	 * Some encryption devices (such as mvcesa) are attached before
	 * ipi_sysinit(). That causes an assertion in ipi_register() as
	 * crypto_ret_si softint uses SOFTINT_RCPU.
	 */
	if (config_finalize_register(NULL, crypto_init_finalize) != 0) {
		printf("crypto_init: cannot register crypto_init_finalize\n");
		return crypto_destroy(false);
	}

	sysctl_opencrypto_setup(&sysctl_opencrypto_clog);

	return 0;
}

static int
crypto_init_finalize(device_t self __unused)
{

	crypto_ret_si = softint_establish(SOFTINT_NET|SOFTINT_MPSAFE|SOFTINT_RCPU,
	    &cryptoret_softint, NULL);
	KASSERT(crypto_ret_si != NULL);

	return 0;
}

int
crypto_init(void)
{
	static ONCE_DECL(crypto_init_once);

	return RUN_ONCE(&crypto_init_once, crypto_init0);
}

static int
crypto_destroy(bool exit_kthread)
{
	int i;

	if (exit_kthread) {
		struct cryptocap *cap = NULL;
		uint64_t where;
		bool is_busy = false;

		/* if we have any in-progress requests, don't unload */
		percpu_foreach(crypto_crp_qs_percpu, crypto_crp_q_is_busy_pc,
				   &is_busy);
		if (is_busy)
			return EBUSY;
		/* FIXME:
		 * prohibit enqueue to crp_q and crp_kq after here.
		 */

		mutex_enter(&crypto_drv_mtx);
		for (i = 0; i < crypto_drivers_num; i++) {
			cap = crypto_checkdriver(i);
			if (cap == NULL)
				continue;
			if (cap->cc_sessions != 0) {
				mutex_exit(&crypto_drv_mtx);
				return EBUSY;
			}
		}
		mutex_exit(&crypto_drv_mtx);
		/* FIXME:
		 * prohibit touch crypto_drivers[] and each element after here.
		 */

		/*
		 * Ensure cryptoret_softint() is never scheduled and then wait
		 * for last softint_execute().
		 */
		for (i = 0; i < ncpu; i++) {
			struct crypto_crp_ret_qs *qs;
			struct cpu_info *ci = cpu_lookup(i);

			qs = crypto_get_crp_ret_qs(ci);
			qs->crp_ret_q_exit_flag = true;
			crypto_put_crp_ret_qs(ci);
		}
		where = xc_broadcast(0, (xcfunc_t)nullop, NULL, NULL);
		xc_wait(where);
	}

	if (sysctl_opencrypto_clog != NULL)
		sysctl_teardown(&sysctl_opencrypto_clog);

	if (crypto_ret_si != NULL)
		softint_disestablish(crypto_ret_si);

	if (crypto_q_si != NULL)
		softint_disestablish(crypto_q_si);

	mutex_enter(&crypto_drv_mtx);
	if (crypto_drivers != NULL)
		kmem_free(crypto_drivers,
		    crypto_drivers_num * sizeof(struct cryptocap));
	mutex_exit(&crypto_drv_mtx);

	percpu_free(crypto_crp_qs_percpu, sizeof(struct crypto_crp_qs));

	pool_cache_destroy(cryptop_cache);
	pool_cache_destroy(cryptodesc_cache);
	pool_cache_destroy(cryptkop_cache);

	mutex_destroy(&crypto_drv_mtx);

	return 0;
}

static bool
crypto_driver_suitable(struct cryptocap *cap, struct cryptoini *cri)
{
	struct cryptoini *cr;

	for (cr = cri; cr; cr = cr->cri_next)
		if (cap->cc_alg[cr->cri_alg] == 0) {
			DPRINTF("alg %d not supported\n", cr->cri_alg);
			return false;
		}

	return true;
}

#define CRYPTO_ACCEPT_HARDWARE 0x1
#define CRYPTO_ACCEPT_SOFTWARE 0x2
/*
 * The algorithm we use here is pretty stupid; just use the
 * first driver that supports all the algorithms we need.
 * If there are multiple drivers we choose the driver with
 * the fewest active sessions. We prefer hardware-backed
 * drivers to software ones.
 *
 * XXX We need more smarts here (in real life too, but that's
 * XXX another story altogether).
 */
static struct cryptocap *
crypto_select_driver_lock(struct cryptoini *cri, int hard)
{
	u_int32_t hid;
	int accept;
	struct cryptocap *cap, *best;

	best = NULL;
	/*
	 * hard == 0 can use both hardware and software drivers.
	 * We use hardware drivers prior to software drivers, so search
	 * hardware drivers at first time.
	 */
	if (hard >= 0)
		accept = CRYPTO_ACCEPT_HARDWARE;
	else
		accept = CRYPTO_ACCEPT_SOFTWARE;
again:
	for (hid = 0; hid < crypto_drivers_num; hid++) {
		cap = crypto_checkdriver(hid);
		if (cap == NULL)
			continue;

		crypto_driver_lock(cap);

		/*
		 * If it's not initialized or has remaining sessions
		 * referencing it, skip.
		 */
		if (cap->cc_newsession == NULL ||
		    (cap->cc_flags & CRYPTOCAP_F_CLEANUP)) {
			crypto_driver_unlock(cap);
			continue;
		}

		/* Hardware required -- ignore software drivers. */
		if ((accept & CRYPTO_ACCEPT_SOFTWARE) == 0
		    && (cap->cc_flags & CRYPTOCAP_F_SOFTWARE)) {
			crypto_driver_unlock(cap);
			continue;
		}
		/* Software required -- ignore hardware drivers. */
		if ((accept & CRYPTO_ACCEPT_HARDWARE) == 0
		    && (cap->cc_flags & CRYPTOCAP_F_SOFTWARE) == 0) {
			crypto_driver_unlock(cap);
			continue;
		}

		/* See if all the algorithms are supported. */
		if (crypto_driver_suitable(cap, cri)) {
			if (best == NULL) {
				/* keep holding crypto_driver_lock(cap) */
				best = cap;
				continue;
			} else if (cap->cc_sessions < best->cc_sessions) {
				crypto_driver_unlock(best);
				/* keep holding crypto_driver_lock(cap) */
				best = cap;
				continue;
			}
		}

		crypto_driver_unlock(cap);
	}
	if (best == NULL && hard == 0
	    && (accept & CRYPTO_ACCEPT_SOFTWARE) == 0) {
		accept = CRYPTO_ACCEPT_SOFTWARE;
		goto again;
	}

	return best;
}

/*
 * Create a new session.
 */
int
crypto_newsession(u_int64_t *sid, struct cryptoini *cri, int hard)
{
	struct cryptocap *cap;
	int err = EINVAL;

	mutex_enter(&crypto_drv_mtx);

	cap = crypto_select_driver_lock(cri, hard);
	if (cap != NULL) {
		u_int32_t hid, lid;

		hid = cap - crypto_drivers;
		/*
		 * Can't do everything in one session.
		 *
		 * XXX Fix this. We need to inject a "virtual" session layer right
		 * XXX about here.
		 */

		/* Call the driver initialization routine. */
		lid = hid;		/* Pass the driver ID. */
		crypto_driver_unlock(cap);
		err = cap->cc_newsession(cap->cc_arg, &lid, cri);
		crypto_driver_lock(cap);
		if (err == 0) {
			(*sid) = hid;
			(*sid) <<= 32;
			(*sid) |= (lid & 0xffffffff);
			(cap->cc_sessions)++;
		} else {
			DPRINTF("crypto_drivers[%d].cc_newsession() failed. error=%d\n",
			    hid, err);
		}
		crypto_driver_unlock(cap);
	}

	mutex_exit(&crypto_drv_mtx);

	return err;
}

/*
 * Delete an existing session (or a reserved session on an unregistered
 * driver).
 */
int
crypto_freesession(u_int64_t sid)
{
	struct cryptocap *cap;
	int err = 0;

	/* Determine two IDs. */
	cap = crypto_checkdriver_lock(CRYPTO_SESID2HID(sid));
	if (cap == NULL)
		return ENOENT;

	if (cap->cc_sessions)
		(cap->cc_sessions)--;

	/* Call the driver cleanup routine, if available. */
	if (cap->cc_freesession)
		err = cap->cc_freesession(cap->cc_arg, sid);
	else
		err = 0;

	/*
	 * If this was the last session of a driver marked as invalid,
	 * make the entry available for reuse.
	 */
	if ((cap->cc_flags & CRYPTOCAP_F_CLEANUP) && cap->cc_sessions == 0)
		crypto_driver_clear(cap);

	crypto_driver_unlock(cap);
	return err;
}

static bool
crypto_checkdriver_initialized(const struct cryptocap *cap)
{

	return cap->cc_process != NULL ||
	    (cap->cc_flags & CRYPTOCAP_F_CLEANUP) != 0 ||
	    cap->cc_sessions != 0;
}

/*
 * Return an unused driver id.  Used by drivers prior to registering
 * support for the algorithms they handle.
 */
int32_t
crypto_get_driverid(u_int32_t flags)
{
	struct cryptocap *newdrv;
	struct cryptocap *cap = NULL;
	int i;

	(void)crypto_init();		/* XXX oh, this is foul! */

	mutex_enter(&crypto_drv_mtx);
	for (i = 0; i < crypto_drivers_num; i++) {
		cap = crypto_checkdriver_uninit(i);
		if (cap == NULL || crypto_checkdriver_initialized(cap))
			continue;
		break;
	}

	/* Out of entries, allocate some more. */
	if (cap == NULL) {
		/* Be careful about wrap-around. */
		if (2 * crypto_drivers_num <= crypto_drivers_num) {
			mutex_exit(&crypto_drv_mtx);
			printf("crypto: driver count wraparound!\n");
			return -1;
		}

		newdrv = kmem_zalloc(2 * crypto_drivers_num *
		    sizeof(struct cryptocap), KM_NOSLEEP);
		if (newdrv == NULL) {
			mutex_exit(&crypto_drv_mtx);
			printf("crypto: no space to expand driver table!\n");
			return -1;
		}

		memcpy(newdrv, crypto_drivers,
		    crypto_drivers_num * sizeof(struct cryptocap));
		kmem_free(crypto_drivers,
		    crypto_drivers_num * sizeof(struct cryptocap));

		crypto_drivers_num *= 2;
		crypto_drivers = newdrv;

		cap = crypto_checkdriver_uninit(i);
		KASSERT(cap != NULL);
	}

	/* NB: state is zero'd on free */
	cap->cc_sessions = 1;	/* Mark */
	cap->cc_flags = flags;
	mutex_init(&cap->cc_lock, MUTEX_DEFAULT, IPL_NET);

	if (bootverbose)
		printf("crypto: assign driver %u, flags %u\n", i, flags);

	mutex_exit(&crypto_drv_mtx);

	return i;
}

static struct cryptocap *
crypto_checkdriver_lock(u_int32_t hid)
{
	struct cryptocap *cap;

	KASSERT(crypto_drivers != NULL);

	if (hid >= crypto_drivers_num)
		return NULL;

	cap = &crypto_drivers[hid];
	mutex_enter(&cap->cc_lock);
	return cap;
}

/*
 * Use crypto_checkdriver_uninit() instead of crypto_checkdriver() below two
 * situations
 *     - crypto_drivers[] may not be allocated
 *     - crypto_drivers[hid] may not be initialized
 */
static struct cryptocap *
crypto_checkdriver_uninit(u_int32_t hid)
{

	KASSERT(mutex_owned(&crypto_drv_mtx));

	if (crypto_drivers == NULL)
		return NULL;

	return (hid >= crypto_drivers_num ? NULL : &crypto_drivers[hid]);
}

/*
 * Use crypto_checkdriver_uninit() instead of crypto_checkdriver() below two
 * situations
 *     - crypto_drivers[] may not be allocated
 *     - crypto_drivers[hid] may not be initialized
 */
static struct cryptocap *
crypto_checkdriver(u_int32_t hid)
{

	KASSERT(mutex_owned(&crypto_drv_mtx));

	if (crypto_drivers == NULL || hid >= crypto_drivers_num)
		return NULL;

	struct cryptocap *cap = &crypto_drivers[hid];
	return crypto_checkdriver_initialized(cap) ? cap : NULL;
}

static inline void
crypto_driver_lock(struct cryptocap *cap)
{

	KASSERT(cap != NULL);

	mutex_enter(&cap->cc_lock);
}

static inline void
crypto_driver_unlock(struct cryptocap *cap)
{

	KASSERT(cap != NULL);

	mutex_exit(&cap->cc_lock);
}

static void
crypto_driver_clear(struct cryptocap *cap)
{

	if (cap == NULL)
		return;

	KASSERT(mutex_owned(&cap->cc_lock));

	cap->cc_sessions = 0;
	memset(&cap->cc_max_op_len, 0, sizeof(cap->cc_max_op_len));
	memset(&cap->cc_alg, 0, sizeof(cap->cc_alg));
	memset(&cap->cc_kalg, 0, sizeof(cap->cc_kalg));
	cap->cc_flags = 0;
	cap->cc_qblocked = 0;
	cap->cc_kqblocked = 0;

	cap->cc_arg = NULL;
	cap->cc_newsession = NULL;
	cap->cc_process = NULL;
	cap->cc_freesession = NULL;
	cap->cc_kprocess = NULL;
}

/*
 * Register support for a key-related algorithm.  This routine
 * is called once for each algorithm supported a driver.
 */
int
crypto_kregister(u_int32_t driverid, int kalg, u_int32_t flags,
    int (*kprocess)(void *, struct cryptkop *, int),
    void *karg)
{
	struct cryptocap *cap;
	int err;

	mutex_enter(&crypto_drv_mtx);

	cap = crypto_checkdriver_lock(driverid);
	if (cap != NULL &&
	    (CRK_ALGORITM_MIN <= kalg && kalg <= CRK_ALGORITHM_MAX)) {
		/*
		 * XXX Do some performance testing to determine placing.
		 * XXX We probably need an auxiliary data structure that
		 * XXX describes relative performances.
		 */

		cap->cc_kalg[kalg] = flags | CRYPTO_ALG_FLAG_SUPPORTED;
		if (bootverbose) {
			printf("crypto: driver %u registers key alg %u "
			       " flags %u\n",
				driverid,
				kalg,
				flags
			);
		}

		if (cap->cc_kprocess == NULL) {
			cap->cc_karg = karg;
			cap->cc_kprocess = kprocess;
		}
		err = 0;
	} else
		err = EINVAL;

	mutex_exit(&crypto_drv_mtx);
	return err;
}

/*
 * Register support for a non-key-related algorithm.  This routine
 * is called once for each such algorithm supported by a driver.
 */
int
crypto_register(u_int32_t driverid, int alg, u_int16_t maxoplen,
    u_int32_t flags,
    int (*newses)(void *, u_int32_t*, struct cryptoini*),
    int (*freeses)(void *, u_int64_t),
    int (*process)(void *, struct cryptop *, int),
    void *arg)
{
	struct cryptocap *cap;
	int err;

	cap = crypto_checkdriver_lock(driverid);
	if (cap == NULL)
		return EINVAL;

	/* NB: algorithms are in the range [1..max] */
	if (CRYPTO_ALGORITHM_MIN <= alg && alg <= CRYPTO_ALGORITHM_MAX) {
		/*
		 * XXX Do some performance testing to determine placing.
		 * XXX We probably need an auxiliary data structure that
		 * XXX describes relative performances.
		 */

		cap->cc_alg[alg] = flags | CRYPTO_ALG_FLAG_SUPPORTED;
		cap->cc_max_op_len[alg] = maxoplen;
		if (bootverbose) {
			printf("crypto: driver %u registers alg %u "
				"flags %u maxoplen %u\n",
				driverid,
				alg,
				flags,
				maxoplen
			);
		}

		if (cap->cc_process == NULL) {
			cap->cc_arg = arg;
			cap->cc_newsession = newses;
			cap->cc_process = process;
			cap->cc_freesession = freeses;
			cap->cc_sessions = 0;		/* Unmark */
		}
		err = 0;
	} else
		err = EINVAL;

	crypto_driver_unlock(cap);

	return err;
}

static int
crypto_unregister_locked(struct cryptocap *cap, int alg, bool all)
{
	int i;
	u_int32_t ses;
	bool lastalg = true;

	KASSERT(cap != NULL);
	KASSERT(mutex_owned(&cap->cc_lock));

	if (alg < CRYPTO_ALGORITHM_MIN || CRYPTO_ALGORITHM_MAX < alg)
		return EINVAL;

	if (!all && cap->cc_alg[alg] == 0)
		return EINVAL;

	cap->cc_alg[alg] = 0;
	cap->cc_max_op_len[alg] = 0;

	if (all) {
		if (alg != CRYPTO_ALGORITHM_MAX)
			lastalg = false;
	} else {
		/* Was this the last algorithm ? */
		for (i = CRYPTO_ALGORITHM_MIN; i <= CRYPTO_ALGORITHM_MAX; i++)
			if (cap->cc_alg[i] != 0) {
				lastalg = false;
				break;
			}
	}
	if (lastalg) {
		ses = cap->cc_sessions;
		crypto_driver_clear(cap);
		if (ses != 0) {
			/*
			 * If there are pending sessions, just mark as invalid.
			 */
			cap->cc_flags |= CRYPTOCAP_F_CLEANUP;
			cap->cc_sessions = ses;
		}
	}

	return 0;
}

/*
 * Unregister a crypto driver. If there are pending sessions using it,
 * leave enough information around so that subsequent calls using those
 * sessions will correctly detect the driver has been unregistered and
 * reroute requests.
 */
int
crypto_unregister(u_int32_t driverid, int alg)
{
	int err;
	struct cryptocap *cap;

	cap = crypto_checkdriver_lock(driverid);
	err = crypto_unregister_locked(cap, alg, false);
	crypto_driver_unlock(cap);

	return err;
}

/*
 * Unregister all algorithms associated with a crypto driver.
 * If there are pending sessions using it, leave enough information
 * around so that subsequent calls using those sessions will
 * correctly detect the driver has been unregistered and reroute
 * requests.
 */
int
crypto_unregister_all(u_int32_t driverid)
{
	int err, i;
	struct cryptocap *cap;

	cap = crypto_checkdriver_lock(driverid);
	for (i = CRYPTO_ALGORITHM_MIN; i <= CRYPTO_ALGORITHM_MAX; i++) {
		err = crypto_unregister_locked(cap, i, true);
		if (err)
			break;
	}
	crypto_driver_unlock(cap);

	return err;
}

/*
 * Clear blockage on a driver.  The what parameter indicates whether
 * the driver is now ready for cryptop's and/or cryptokop's.
 */
int
crypto_unblock(u_int32_t driverid, int what)
{
	struct cryptocap *cap;
	int needwakeup = 0;

	cap = crypto_checkdriver_lock(driverid);
	if (cap == NULL)
		return EINVAL;

	if (what & CRYPTO_SYMQ) {
		needwakeup |= cap->cc_qblocked;
		cap->cc_qblocked = 0;
	}
	if (what & CRYPTO_ASYMQ) {
		needwakeup |= cap->cc_kqblocked;
		cap->cc_kqblocked = 0;
	}
	crypto_driver_unlock(cap);
	if (needwakeup) {
		kpreempt_disable();
		softint_schedule(crypto_q_si);
		kpreempt_enable();
	}

	return 0;
}

/*
 * Dispatch a crypto request to a driver or queue
 * it, to be processed by the kernel thread.
 */
int
crypto_dispatch(struct cryptop *crp)
{
	int result, s;
	struct cryptocap *cap;
	struct crypto_crp_qs *crp_qs;
	struct crypto_crp_q *crp_q;

	KASSERT(crp != NULL);

	DPRINTF("crp %p, alg %d\n", crp, crp->crp_desc->crd_alg);

	cryptostats.cs_ops++;

#ifdef CRYPTO_TIMING
	if (crypto_timing)
		nanouptime(&crp->crp_tstamp);
#endif

	if ((crp->crp_flags & CRYPTO_F_BATCH) != 0) {
		int wasempty;
		/*
		 * Caller marked the request as ``ok to delay'';
		 * queue it for the swi thread.  This is desirable
		 * when the operation is low priority and/or suitable
		 * for batching.
		 *
		 * don't care list order in batch job.
		 */
		crp_qs = crypto_get_crp_qs(&s);
		crp_q = crp_qs->crp_q;
		wasempty  = TAILQ_EMPTY(crp_q);
		TAILQ_INSERT_TAIL(crp_q, crp, crp_next);
		crypto_put_crp_qs(&s);
		crp_q = NULL;
		if (wasempty) {
			kpreempt_disable();
			softint_schedule(crypto_q_si);
			kpreempt_enable();
		}

		return 0;
	}

	crp_qs = crypto_get_crp_qs(&s);
	crp_q = crp_qs->crp_q;
	cap = crypto_checkdriver_lock(CRYPTO_SESID2HID(crp->crp_sid));
	/*
	 * TODO:
	 * If we can ensure the driver has been valid until the driver is
	 * done crypto_unregister(), this migrate operation is not required.
	 */
	if (cap == NULL) {
		/*
		 * The driver must be detached, so this request will migrate
		 * to other drivers in cryptointr() later.
		 */
		TAILQ_INSERT_TAIL(crp_q, crp, crp_next);
		result = 0;
		goto out;
	}

	if (cap->cc_qblocked != 0) {
		crypto_driver_unlock(cap);
		/*
		 * The driver is blocked, just queue the op until
		 * it unblocks and the swi thread gets kicked.
		 */
		TAILQ_INSERT_TAIL(crp_q, crp, crp_next);
		result = 0;
		goto out;
	}

	/*
	 * Caller marked the request to be processed
	 * immediately; dispatch it directly to the
	 * driver unless the driver is currently blocked.
	 */
	crypto_driver_unlock(cap);
	result = crypto_invoke(crp, 0);
	if (result == ERESTART) {
		/*
		 * The driver ran out of resources, mark the
		 * driver ``blocked'' for cryptop's and put
		 * the op on the queue.
		 */
		crypto_driver_lock(cap);
		cap->cc_qblocked = 1;
		crypto_driver_unlock(cap);
		TAILQ_INSERT_HEAD(crp_q, crp, crp_next);
		cryptostats.cs_blocks++;

		/*
		 * The crp is enqueued to crp_q, that is,
		 * no error occurs. So, this function should
		 * not return error.
		 */
		result = 0;
	}

out:
	crypto_put_crp_qs(&s);
	return result;
}

/*
 * Add an asymetric crypto request to a queue,
 * to be processed by the kernel thread.
 */
int
crypto_kdispatch(struct cryptkop *krp)
{
	int result, s;
	struct cryptocap *cap;
	struct crypto_crp_qs *crp_qs;
	struct crypto_crp_kq *crp_kq;

	KASSERT(krp != NULL);

	cryptostats.cs_kops++;

	crp_qs = crypto_get_crp_qs(&s);
	crp_kq = crp_qs->crp_kq;
	cap = crypto_checkdriver_lock(krp->krp_hid);
	/*
	 * TODO:
	 * If we can ensure the driver has been valid until the driver is
	 * done crypto_unregister(), this migrate operation is not required.
	 */
	if (cap == NULL) {
		TAILQ_INSERT_TAIL(crp_kq, krp, krp_next);
		result = 0;
		goto out;
	}

	if (cap->cc_kqblocked != 0) {
		crypto_driver_unlock(cap);
		/*
		 * The driver is blocked, just queue the op until
		 * it unblocks and the swi thread gets kicked.
		 */
		TAILQ_INSERT_TAIL(crp_kq, krp, krp_next);
		result = 0;
		goto out;
	}

	crypto_driver_unlock(cap);
	result = crypto_kinvoke(krp, 0);
	if (result == ERESTART) {
		/*
		 * The driver ran out of resources, mark the
		 * driver ``blocked'' for cryptop's and put
		 * the op on the queue.
		 */
		crypto_driver_lock(cap);
		cap->cc_kqblocked = 1;
		crypto_driver_unlock(cap);
		TAILQ_INSERT_HEAD(crp_kq, krp, krp_next);
		cryptostats.cs_kblocks++;

		/*
		 * The krp is enqueued to crp_kq, that is,
		 * no error occurs. So, this function should
		 * not return error.
		 */
		result = 0;
	}

out:
	crypto_put_crp_qs(&s);
	return result;
}

/*
 * Dispatch an assymetric crypto request to the appropriate crypto devices.
 */
static int
crypto_kinvoke(struct cryptkop *krp, int hint)
{
	struct cryptocap *cap = NULL;
	u_int32_t hid;
	int error;

	KASSERT(krp != NULL);

	/* Sanity checks. */
	if (krp->krp_callback == NULL) {
		cv_destroy(&krp->krp_cv);
		crypto_kfreereq(krp);
		return EINVAL;
	}

	mutex_enter(&crypto_drv_mtx);
	for (hid = 0; hid < crypto_drivers_num; hid++) {
		cap = crypto_checkdriver(hid);
		if (cap == NULL)
			continue;
		crypto_driver_lock(cap);
		if ((cap->cc_flags & CRYPTOCAP_F_SOFTWARE) &&
		    crypto_devallowsoft == 0) {
			crypto_driver_unlock(cap);
			continue;
		}
		if (cap->cc_kprocess == NULL) {
			crypto_driver_unlock(cap);
			continue;
		}
		if ((cap->cc_kalg[krp->krp_op] &
			CRYPTO_ALG_FLAG_SUPPORTED) == 0) {
			crypto_driver_unlock(cap);
			continue;
		}
		break;
	}
	mutex_exit(&crypto_drv_mtx);
	if (cap != NULL) {
		int (*process)(void *, struct cryptkop *, int);
		void *arg;

		process = cap->cc_kprocess;
		arg = cap->cc_karg;
		krp->krp_hid = hid;
		krp->reqcpu = curcpu();
		crypto_driver_unlock(cap);
		error = (*process)(arg, krp, hint);
	} else {
		error = ENODEV;
	}

	if (error) {
		krp->krp_status = error;
		crypto_kdone(krp);
	}
	return 0;
}

#ifdef CRYPTO_TIMING
static void
crypto_tstat(struct cryptotstat *ts, struct timespec *tv)
{
	struct timespec now, t;

	nanouptime(&now);
	t.tv_sec = now.tv_sec - tv->tv_sec;
	t.tv_nsec = now.tv_nsec - tv->tv_nsec;
	if (t.tv_nsec < 0) {
		t.tv_sec--;
		t.tv_nsec += 1000000000;
	}
	timespecadd(&ts->acc, &t, &t);
	if (timespeccmp(&t, &ts->min, <))
		ts->min = t;
	if (timespeccmp(&t, &ts->max, >))
		ts->max = t;
	ts->count++;

	*tv = now;
}
#endif

/*
 * Dispatch a crypto request to the appropriate crypto devices.
 */
static int
crypto_invoke(struct cryptop *crp, int hint)
{
	struct cryptocap *cap;

	KASSERT(crp != NULL);

#ifdef CRYPTO_TIMING
	if (crypto_timing)
		crypto_tstat(&cryptostats.cs_invoke, &crp->crp_tstamp);
#endif
	/* Sanity checks. */
	if (crp->crp_callback == NULL) {
		return EINVAL;
	}
	if (crp->crp_desc == NULL) {
		crp->crp_etype = EINVAL;
		crypto_done(crp);
		return 0;
	}

	cap = crypto_checkdriver_lock(CRYPTO_SESID2HID(crp->crp_sid));
	if (cap != NULL && (cap->cc_flags & CRYPTOCAP_F_CLEANUP) == 0) {
		int (*process)(void *, struct cryptop *, int);
		void *arg;

		process = cap->cc_process;
		arg = cap->cc_arg;
		crp->reqcpu = curcpu();

		/*
		 * Invoke the driver to process the request.
		 */
		DPRINTF("calling process for %p\n", crp);
		crypto_driver_unlock(cap);
		return (*process)(arg, crp, hint);
	} else {
		struct cryptodesc *crd;
		u_int64_t nid = 0;

		if (cap != NULL)
			crypto_driver_unlock(cap);

		/*
		 * Driver has unregistered; migrate the session and return
		 * an error to the caller so they'll resubmit the op.
		 */
		crypto_freesession(crp->crp_sid);

		for (crd = crp->crp_desc; crd->crd_next; crd = crd->crd_next)
			crd->CRD_INI.cri_next = &(crd->crd_next->CRD_INI);

		if (crypto_newsession(&nid, &(crp->crp_desc->CRD_INI), 0) == 0)
			crp->crp_sid = nid;

		crp->crp_etype = EAGAIN;

		crypto_done(crp);
		return 0;
	}
}

/*
 * Release a set of crypto descriptors.
 */
void
crypto_freereq(struct cryptop *crp)
{
	struct cryptodesc *crd;

	if (crp == NULL)
		return;
	DPRINTF("lid[%u]: crp %p\n", CRYPTO_SESID2LID(crp->crp_sid), crp);

	/* sanity check */
	if (crp->crp_flags & CRYPTO_F_ONRETQ) {
		panic("crypto_freereq() freeing crp on RETQ\n");
	}

	while ((crd = crp->crp_desc) != NULL) {
		crp->crp_desc = crd->crd_next;
		pool_cache_put(cryptodesc_cache, crd);
	}
	pool_cache_put(cryptop_cache, crp);
}

/*
 * Acquire a set of crypto descriptors.
 */
struct cryptop *
crypto_getreq(int num)
{
	struct cryptodesc *crd;
	struct cryptop *crp;
	struct crypto_crp_ret_qs *qs;

	/*
	 * When crp_ret_q is full, we restrict here to avoid crp_ret_q overflow
	 * by error callback.
	 */
	qs = crypto_get_crp_ret_qs(curcpu());
	if (qs->crp_ret_q_maxlen > 0
	    && qs->crp_ret_q_len > qs->crp_ret_q_maxlen) {
		qs->crp_ret_q_drops++;
		crypto_put_crp_ret_qs(curcpu());
		return NULL;
	}
	crypto_put_crp_ret_qs(curcpu());

	crp = pool_cache_get(cryptop_cache, PR_NOWAIT);
	if (crp == NULL) {
		return NULL;
	}
	memset(crp, 0, sizeof(struct cryptop));

	while (num--) {
		crd = pool_cache_get(cryptodesc_cache, PR_NOWAIT);
		if (crd == NULL) {
			crypto_freereq(crp);
			return NULL;
		}

		memset(crd, 0, sizeof(struct cryptodesc));
		crd->crd_next = crp->crp_desc;
		crp->crp_desc = crd;
	}

	return crp;
}

/*
 * Release a set of asymmetric crypto descriptors.
 * Currently, support one descriptor only.
 */
void
crypto_kfreereq(struct cryptkop *krp)
{

	if (krp == NULL)
		return;

	DPRINTF("krp %p\n", krp);

	/* sanity check */
	if (krp->krp_flags & CRYPTO_F_ONRETQ) {
		panic("crypto_kfreereq() freeing krp on RETQ\n");
	}

	pool_cache_put(cryptkop_cache, krp);
}

/*
 * Acquire a set of asymmetric crypto descriptors.
 * Currently, support one descriptor only.
 */
struct cryptkop *
crypto_kgetreq(int num __unused, int prflags)
{
	struct cryptkop *krp;
	struct crypto_crp_ret_qs *qs;

	/*
	 * When crp_ret_kq is full, we restrict here to avoid crp_ret_kq
	 * overflow by error callback.
	 */
	qs = crypto_get_crp_ret_qs(curcpu());
	if (qs->crp_ret_kq_maxlen > 0
	    && qs->crp_ret_kq_len > qs->crp_ret_kq_maxlen) {
		qs->crp_ret_kq_drops++;
		crypto_put_crp_ret_qs(curcpu());
		return NULL;
	}
	crypto_put_crp_ret_qs(curcpu());

	krp = pool_cache_get(cryptkop_cache, prflags);
	if (krp == NULL) {
		return NULL;
	}
	memset(krp, 0, sizeof(struct cryptkop));

	return krp;
}

/*
 * Invoke the callback on behalf of the driver.
 */
void
crypto_done(struct cryptop *crp)
{

	KASSERT(crp != NULL);

	if (crp->crp_etype != 0)
		cryptostats.cs_errs++;
#ifdef CRYPTO_TIMING
	if (crypto_timing)
		crypto_tstat(&cryptostats.cs_done, &crp->crp_tstamp);
#endif
	DPRINTF("lid[%u]: crp %p\n", CRYPTO_SESID2LID(crp->crp_sid), crp);

	/*
	 * Normal case; queue the callback for the thread.
	 *
	 * The return queue is manipulated by the swi thread
	 * and, potentially, by crypto device drivers calling
	 * back to mark operations completed.  Thus we need
	 * to mask both while manipulating the return queue.
	 */
  	if (crp->crp_flags & CRYPTO_F_CBIMM) {
		/*
	 	* Do the callback directly.  This is ok when the
  	 	* callback routine does very little (e.g. the
	 	* /dev/crypto callback method just does a wakeup).
	 	*/
		crp->crp_flags |= CRYPTO_F_DONE;

#ifdef CRYPTO_TIMING
		if (crypto_timing) {
			/*
		 	* NB: We must copy the timestamp before
		 	* doing the callback as the cryptop is
		 	* likely to be reclaimed.
		 	*/
			struct timespec t = crp->crp_tstamp;
			crypto_tstat(&cryptostats.cs_cb, &t);
			crp->crp_callback(crp);
			crypto_tstat(&cryptostats.cs_finis, &t);
		} else
#endif
		crp->crp_callback(crp);
	} else {
		crp->crp_flags |= CRYPTO_F_DONE;
#if 0
		if (crp->crp_flags & CRYPTO_F_USER) {
			/*
			 * TODO:
			 * If crp->crp_flags & CRYPTO_F_USER and the used
			 * encryption driver does all the processing in
			 * the same context, we can skip enqueueing crp_ret_q
			 * and softint_schedule(crypto_ret_si).
			 */
			DPRINTF("lid[%u]: crp %p CRYPTO_F_USER\n",
				CRYPTO_SESID2LID(crp->crp_sid), crp);
		} else
#endif
		{
			int wasempty;
			struct crypto_crp_ret_qs *qs;
			struct crypto_crp_ret_q *crp_ret_q;;

			qs = crypto_get_crp_ret_qs(crp->reqcpu);
			crp_ret_q = &qs->crp_ret_q;
			wasempty = TAILQ_EMPTY(crp_ret_q);
			DPRINTF("lid[%u]: queueing %p\n",
				CRYPTO_SESID2LID(crp->crp_sid), crp);
			crp->crp_flags |= CRYPTO_F_ONRETQ;
			TAILQ_INSERT_TAIL(crp_ret_q, crp, crp_next);
			qs->crp_ret_q_len++;
			if (wasempty && !qs->crp_ret_q_exit_flag) {
				DPRINTF("lid[%u]: waking cryptoret,"
					"crp %p hit empty queue\n.",
					CRYPTO_SESID2LID(crp->crp_sid), crp);
				softint_schedule_cpu(crypto_ret_si, crp->reqcpu);
			}
			crypto_put_crp_ret_qs(crp->reqcpu);
		}
	}
}

/*
 * Invoke the callback on behalf of the driver.
 */
void
crypto_kdone(struct cryptkop *krp)
{

	KASSERT(krp != NULL);

	if (krp->krp_status != 0)
		cryptostats.cs_kerrs++;
		
	krp->krp_flags |= CRYPTO_F_DONE;

	/*
	 * The return queue is manipulated by the swi thread
	 * and, potentially, by crypto device drivers calling
	 * back to mark operations completed.  Thus we need
	 * to mask both while manipulating the return queue.
	 */
	if (krp->krp_flags & CRYPTO_F_CBIMM) {
		krp->krp_callback(krp);
	} else {
		int wasempty;
		struct crypto_crp_ret_qs *qs;
		struct crypto_crp_ret_kq *crp_ret_kq;;

		qs = crypto_get_crp_ret_qs(krp->reqcpu);
		crp_ret_kq = &qs->crp_ret_kq;

		wasempty = TAILQ_EMPTY(crp_ret_kq);
		krp->krp_flags |= CRYPTO_F_ONRETQ;
		TAILQ_INSERT_TAIL(crp_ret_kq, krp, krp_next);
		qs->crp_ret_kq_len++;
		if (wasempty && !qs->crp_ret_q_exit_flag)
			softint_schedule_cpu(crypto_ret_si, krp->reqcpu);
		crypto_put_crp_ret_qs(krp->reqcpu);
	}
}

int
crypto_getfeat(int *featp)
{

	if (crypto_userasymcrypto == 0) {
		*featp = 0;
		return 0;
	}

	mutex_enter(&crypto_drv_mtx);

	int feat = 0;
	for (int hid = 0; hid < crypto_drivers_num; hid++) {
		struct cryptocap *cap;
		cap = crypto_checkdriver(hid);
		if (cap == NULL)
			continue;

		crypto_driver_lock(cap);

		if ((cap->cc_flags & CRYPTOCAP_F_SOFTWARE) &&
		    crypto_devallowsoft == 0)
			goto unlock;

		if (cap->cc_kprocess == NULL)
			goto unlock;

		for (int kalg = 0; kalg < CRK_ALGORITHM_MAX; kalg++)
			if ((cap->cc_kalg[kalg] &
			    CRYPTO_ALG_FLAG_SUPPORTED) != 0)
				feat |=  1 << kalg;

unlock:		crypto_driver_unlock(cap);
	}

	mutex_exit(&crypto_drv_mtx);
	*featp = feat;
	return (0);
}

/*
 * Software interrupt thread to dispatch crypto requests.
 */
static void
cryptointr(void *arg __unused)
{
	struct cryptop *crp, *submit, *cnext;
	struct cryptkop *krp, *knext;
	struct cryptocap *cap;
	struct crypto_crp_qs *crp_qs;
	struct crypto_crp_q *crp_q;
	struct crypto_crp_kq *crp_kq;
	int result, hint, s;

	cryptostats.cs_intrs++;
	crp_qs = crypto_get_crp_qs(&s);
	crp_q = crp_qs->crp_q;
	crp_kq = crp_qs->crp_kq;
	do {
		/*
		 * Find the first element in the queue that can be
		 * processed and look-ahead to see if multiple ops
		 * are ready for the same driver.
		 */
		submit = NULL;
		hint = 0;
		TAILQ_FOREACH_SAFE(crp, crp_q, crp_next, cnext) {
			u_int32_t hid = CRYPTO_SESID2HID(crp->crp_sid);
			cap = crypto_checkdriver_lock(hid);
			if (cap == NULL || cap->cc_process == NULL) {
				if (cap != NULL)
					crypto_driver_unlock(cap);
				/* Op needs to be migrated, process it. */
				submit = crp;
				break;
			}

			/*
			 * skip blocked crp regardless of CRYPTO_F_BATCH
			 */
			if (cap->cc_qblocked != 0) {
				crypto_driver_unlock(cap);
				continue;
			}
			crypto_driver_unlock(cap);

			/*
			 * skip batch crp until the end of crp_q
			 */
			if ((crp->crp_flags & CRYPTO_F_BATCH) != 0) {
				if (submit == NULL) {
					submit = crp;
				} else {
					if (CRYPTO_SESID2HID(submit->crp_sid)
					    == hid)
						hint = CRYPTO_HINT_MORE;
				}

				continue;
			}

			/*
			 * found first crp which is neither blocked nor batch.
			 */
			submit = crp;
			/*
			 * batch crp can be processed much later, so clear hint.
			 */
			hint = 0;
			break;
		}
		if (submit != NULL) {
			TAILQ_REMOVE(crp_q, submit, crp_next);
			result = crypto_invoke(submit, hint);
			/* we must take here as the TAILQ op or kinvoke
			   may need this mutex below.  sigh. */
			if (result == ERESTART) {
				/*
				 * The driver ran out of resources, mark the
				 * driver ``blocked'' for cryptop's and put
				 * the request back in the queue.  It would
				 * best to put the request back where we got
				 * it but that's hard so for now we put it
				 * at the front.  This should be ok; putting
				 * it at the end does not work.
				 */
				/* validate sid again */
				cap = crypto_checkdriver_lock(CRYPTO_SESID2HID(submit->crp_sid));
				if (cap == NULL) {
					/* migrate again, sigh... */
					TAILQ_INSERT_TAIL(crp_q, submit, crp_next);
				} else {
					cap->cc_qblocked = 1;
					crypto_driver_unlock(cap);
					TAILQ_INSERT_HEAD(crp_q, submit, crp_next);
					cryptostats.cs_blocks++;
				}
			}
		}

		/* As above, but for key ops */
		TAILQ_FOREACH_SAFE(krp, crp_kq, krp_next, knext) {
			cap = crypto_checkdriver_lock(krp->krp_hid);
			if (cap == NULL || cap->cc_kprocess == NULL) {
				if (cap != NULL)
					crypto_driver_unlock(cap);
				/* Op needs to be migrated, process it. */
				break;
			}
			if (!cap->cc_kqblocked) {
				crypto_driver_unlock(cap);
				break;
			}
			crypto_driver_unlock(cap);
		}
		if (krp != NULL) {
			TAILQ_REMOVE(crp_kq, krp, krp_next);
			result = crypto_kinvoke(krp, 0);
			/* the next iteration will want the mutex. :-/ */
			if (result == ERESTART) {
				/*
				 * The driver ran out of resources, mark the
				 * driver ``blocked'' for cryptkop's and put
				 * the request back in the queue.  It would
				 * best to put the request back where we got
				 * it but that's hard so for now we put it
				 * at the front.  This should be ok; putting
				 * it at the end does not work.
				 */
				/* validate sid again */
				cap = crypto_checkdriver_lock(krp->krp_hid);
				if (cap == NULL) {
					/* migrate again, sigh... */
					TAILQ_INSERT_TAIL(crp_kq, krp, krp_next);
				} else {
					cap->cc_kqblocked = 1;
					crypto_driver_unlock(cap);
					TAILQ_INSERT_HEAD(crp_kq, krp, krp_next);
					cryptostats.cs_kblocks++;
				}
			}
		}
	} while (submit != NULL || krp != NULL);
	crypto_put_crp_qs(&s);
}

/*
 * softint handler to do callbacks.
 */
static void
cryptoret_softint(void *arg __unused)
{
	struct crypto_crp_ret_qs *qs;
	struct crypto_crp_ret_q *crp_ret_q;;
	struct crypto_crp_ret_kq *crp_ret_kq;;

	qs = crypto_get_crp_ret_qs(curcpu());
	crp_ret_q = &qs->crp_ret_q;
	crp_ret_kq = &qs->crp_ret_kq;
	for (;;) {
		struct cryptop *crp;
		struct cryptkop *krp;

		crp = TAILQ_FIRST(crp_ret_q);
		if (crp != NULL) {
			TAILQ_REMOVE(crp_ret_q, crp, crp_next);
			qs->crp_ret_q_len--;
			crp->crp_flags &= ~CRYPTO_F_ONRETQ;
		}
		krp = TAILQ_FIRST(crp_ret_kq);
		if (krp != NULL) {
			TAILQ_REMOVE(crp_ret_kq, krp, krp_next);
			qs->crp_ret_q_len--;
			krp->krp_flags &= ~CRYPTO_F_ONRETQ;
		}

		/* drop before calling any callbacks. */
		if (crp == NULL && krp == NULL)
			break;

		mutex_spin_exit(&qs->crp_ret_q_mtx);
		if (crp != NULL) {
#ifdef CRYPTO_TIMING
			if (crypto_timing) {
				/*
				 * NB: We must copy the timestamp before
				 * doing the callback as the cryptop is
				 * likely to be reclaimed.
				 */
				struct timespec t = crp->crp_tstamp;
				crypto_tstat(&cryptostats.cs_cb, &t);
				crp->crp_callback(crp);
				crypto_tstat(&cryptostats.cs_finis, &t);
			} else
#endif
			{
				crp->crp_callback(crp);
			}
		}
		if (krp != NULL)
			krp->krp_callback(krp);

		mutex_spin_enter(&qs->crp_ret_q_mtx);
	}
	crypto_put_crp_ret_qs(curcpu());
}

/* NetBSD module interface */

MODULE(MODULE_CLASS_MISC, opencrypto, NULL);

static int
opencrypto_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = crypto_init();
#endif
		break;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = crypto_destroy(true);
#endif
		break;
	default:
		error = ENOTTY;
	}
	return error;
}
