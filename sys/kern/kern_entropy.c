/*	$NetBSD: kern_entropy.c,v 1.54 2022/03/24 12:58:56 riastradh Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
 * Entropy subsystem
 *
 *	* Each CPU maintains a per-CPU entropy pool so that gathering
 *	  entropy requires no interprocessor synchronization, except
 *	  early at boot when we may be scrambling to gather entropy as
 *	  soon as possible.
 *
 *	  - entropy_enter gathers entropy and never drops it on the
 *	    floor, at the cost of sometimes having to do cryptography.
 *
 *	  - entropy_enter_intr gathers entropy or drops it on the
 *	    floor, with low latency.  Work to stir the pool or kick the
 *	    housekeeping thread is scheduled in soft interrupts.
 *
 *	* entropy_enter immediately enters into the global pool if it
 *	  can transition to full entropy in one swell foop.  Otherwise,
 *	  it defers to a housekeeping thread that consolidates entropy,
 *	  but only when the CPUs collectively have full entropy, in
 *	  order to mitigate iterative-guessing attacks.
 *
 *	* The entropy housekeeping thread continues to consolidate
 *	  entropy even after we think we have full entropy, in case we
 *	  are wrong, but is limited to one discretionary consolidation
 *	  per minute, and only when new entropy is actually coming in,
 *	  to limit performance impact.
 *
 *	* The entropy epoch is the number that changes when we
 *	  transition from partial entropy to full entropy, so that
 *	  users can easily determine when to reseed.  This also
 *	  facilitates an operator explicitly causing everything to
 *	  reseed by sysctl -w kern.entropy.consolidate=1.
 *
 *	* No entropy estimation based on the sample values, which is a
 *	  contradiction in terms and a potential source of side
 *	  channels.  It is the responsibility of the driver author to
 *	  study how predictable the physical source of input can ever
 *	  be, and to furnish a lower bound on the amount of entropy it
 *	  has.
 *
 *	* Entropy depletion is available for testing (or if you're into
 *	  that sort of thing), with sysctl -w kern.entropy.depletion=1;
 *	  the logic to support it is small, to minimize chance of bugs.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_entropy.c,v 1.54 2022/03/24 12:58:56 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/compat_stub.h>
#include <sys/condvar.h>
#include <sys/cpu.h>
#include <sys/entropy.h>
#include <sys/errno.h>
#include <sys/evcnt.h>
#include <sys/event.h>
#include <sys/file.h>
#include <sys/intr.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/lwp.h>
#include <sys/module_hook.h>
#include <sys/mutex.h>
#include <sys/percpu.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/reboot.h>
#include <sys/rnd.h>		/* legacy kernel API */
#include <sys/rndio.h>		/* userland ioctl interface */
#include <sys/rndsource.h>	/* kernel rndsource driver API */
#include <sys/select.h>
#include <sys/selinfo.h>
#include <sys/sha1.h>		/* for boot seed checksum */
#include <sys/stdint.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/xcall.h>

#include <lib/libkern/entpool.h>

#include <machine/limits.h>

#ifdef __HAVE_CPU_COUNTER
#include <machine/cpu_counter.h>
#endif

/*
 * struct entropy_cpu
 *
 *	Per-CPU entropy state.  The pool is allocated separately
 *	because percpu(9) sometimes moves per-CPU objects around
 *	without zeroing them, which would lead to unwanted copies of
 *	sensitive secrets.  The evcnt is allocated separately because
 *	evcnt(9) assumes it stays put in memory.
 */
struct entropy_cpu {
	struct entropy_cpu_evcnt {
		struct evcnt		softint;
		struct evcnt		intrdrop;
		struct evcnt		intrtrunc;
	}			*ec_evcnt;
	struct entpool		*ec_pool;
	unsigned		ec_pending;
	bool			ec_locked;
};

/*
 * struct entropy_cpu_lock
 *
 *	State for locking the per-CPU entropy state.
 */
struct entropy_cpu_lock {
	int		ecl_s;
	uint64_t	ecl_ncsw;
};

/*
 * struct rndsource_cpu
 *
 *	Per-CPU rndsource state.
 */
struct rndsource_cpu {
	unsigned		rc_entropybits;
	unsigned		rc_timesamples;
	unsigned		rc_datasamples;
};

/*
 * entropy_global (a.k.a. E for short in this file)
 *
 *	Global entropy state.  Writes protected by the global lock.
 *	Some fields, marked (A), can be read outside the lock, and are
 *	maintained with atomic_load/store_relaxed.
 */
struct {
	kmutex_t	lock;		/* covers all global state */
	struct entpool	pool;		/* global pool for extraction */
	unsigned	needed;		/* (A) needed globally */
	unsigned	pending;	/* (A) pending in per-CPU pools */
	unsigned	timestamp;	/* (A) time of last consolidation */
	unsigned	epoch;		/* (A) changes when needed -> 0 */
	kcondvar_t	cv;		/* notifies state changes */
	struct selinfo	selq;		/* notifies needed -> 0 */
	struct lwp	*sourcelock;	/* lock on list of sources */
	kcondvar_t	sourcelock_cv;	/* notifies sourcelock release */
	LIST_HEAD(,krndsource) sources;	/* list of entropy sources */
	enum entropy_stage {
		ENTROPY_COLD = 0, /* single-threaded */
		ENTROPY_WARM,	  /* multi-threaded at boot before CPUs */
		ENTROPY_HOT,	  /* multi-threaded multi-CPU */
	}		stage;
	bool		consolidate;	/* kick thread to consolidate */
	bool		seed_rndsource;	/* true if seed source is attached */
	bool		seeded;		/* true if seed file already loaded */
} entropy_global __cacheline_aligned = {
	/* Fields that must be initialized when the kernel is loaded.  */
	.needed = ENTROPY_CAPACITY*NBBY,
	.epoch = (unsigned)-1,	/* -1 means entropy never consolidated */
	.sources = LIST_HEAD_INITIALIZER(entropy_global.sources),
	.stage = ENTROPY_COLD,
};

#define	E	(&entropy_global)	/* declutter */

/* Read-mostly globals */
static struct percpu	*entropy_percpu __read_mostly; /* struct entropy_cpu */
static void		*entropy_sih __read_mostly; /* softint handler */
static struct lwp	*entropy_lwp __read_mostly; /* housekeeping thread */

static struct krndsource seed_rndsource __read_mostly;

/*
 * Event counters
 *
 *	Must be careful with adding these because they can serve as
 *	side channels.
 */
static struct evcnt entropy_discretionary_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "entropy", "discretionary");
EVCNT_ATTACH_STATIC(entropy_discretionary_evcnt);
static struct evcnt entropy_immediate_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "entropy", "immediate");
EVCNT_ATTACH_STATIC(entropy_immediate_evcnt);
static struct evcnt entropy_partial_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "entropy", "partial");
EVCNT_ATTACH_STATIC(entropy_partial_evcnt);
static struct evcnt entropy_consolidate_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "entropy", "consolidate");
EVCNT_ATTACH_STATIC(entropy_consolidate_evcnt);
static struct evcnt entropy_extract_fail_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "entropy", "extract fail");
EVCNT_ATTACH_STATIC(entropy_extract_fail_evcnt);
static struct evcnt entropy_request_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "entropy", "request");
EVCNT_ATTACH_STATIC(entropy_request_evcnt);
static struct evcnt entropy_deplete_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "entropy", "deplete");
EVCNT_ATTACH_STATIC(entropy_deplete_evcnt);
static struct evcnt entropy_notify_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "entropy", "notify");
EVCNT_ATTACH_STATIC(entropy_notify_evcnt);

/* Sysctl knobs */
static bool	entropy_collection = 1;
static bool	entropy_depletion = 0; /* Silly!  */

static const struct sysctlnode	*entropy_sysctlroot;
static struct sysctllog		*entropy_sysctllog;

/* Forward declarations */
static void	entropy_init_cpu(void *, void *, struct cpu_info *);
static void	entropy_fini_cpu(void *, void *, struct cpu_info *);
static void	entropy_account_cpu(struct entropy_cpu *);
static void	entropy_enter(const void *, size_t, unsigned);
static bool	entropy_enter_intr(const void *, size_t, unsigned);
static void	entropy_softintr(void *);
static void	entropy_thread(void *);
static uint32_t	entropy_pending(void);
static void	entropy_pending_cpu(void *, void *, struct cpu_info *);
static void	entropy_do_consolidate(void);
static void	entropy_consolidate_xc(void *, void *);
static void	entropy_notify(void);
static int	sysctl_entropy_consolidate(SYSCTLFN_ARGS);
static int	sysctl_entropy_gather(SYSCTLFN_ARGS);
static void	filt_entropy_read_detach(struct knote *);
static int	filt_entropy_read_event(struct knote *, long);
static int	entropy_request(size_t, int);
static void	rnd_add_data_1(struct krndsource *, const void *, uint32_t,
		    uint32_t, uint32_t);
static unsigned	rndsource_entropybits(struct krndsource *);
static void	rndsource_entropybits_cpu(void *, void *, struct cpu_info *);
static void	rndsource_to_user(struct krndsource *, rndsource_t *);
static void	rndsource_to_user_est(struct krndsource *, rndsource_est_t *);
static void	rndsource_to_user_est_cpu(void *, void *, struct cpu_info *);

/*
 * entropy_timer()
 *
 *	Cycle counter, time counter, or anything that changes a wee bit
 *	unpredictably.
 */
static inline uint32_t
entropy_timer(void)
{
	struct bintime bt;
	uint32_t v;

	/* If we have a CPU cycle counter, use the low 32 bits.  */
#ifdef __HAVE_CPU_COUNTER
	if (__predict_true(cpu_hascounter()))
		return cpu_counter32();
#endif	/* __HAVE_CPU_COUNTER */

	/* If we're cold, tough.  Can't binuptime while cold.  */
	if (__predict_false(cold))
		return 0;

	/* Fold the 128 bits of binuptime into 32 bits.  */
	binuptime(&bt);
	v = bt.frac;
	v ^= bt.frac >> 32;
	v ^= bt.sec;
	v ^= bt.sec >> 32;
	return v;
}

static void
attach_seed_rndsource(void)
{

	/*
	 * First called no later than entropy_init, while we are still
	 * single-threaded, so no need for RUN_ONCE.
	 */
	if (E->stage >= ENTROPY_WARM || E->seed_rndsource)
		return;
	rnd_attach_source(&seed_rndsource, "seed", RND_TYPE_UNKNOWN,
	    RND_FLAG_COLLECT_VALUE);
	E->seed_rndsource = true;
}

/*
 * entropy_init()
 *
 *	Initialize the entropy subsystem.  Panic on failure.
 *
 *	Requires percpu(9) and sysctl(9) to be initialized.
 */
static void
entropy_init(void)
{
	uint32_t extra[2];
	struct krndsource *rs;
	unsigned i = 0;

	KASSERT(E->stage == ENTROPY_COLD);

	/* Grab some cycle counts early at boot.  */
	extra[i++] = entropy_timer();

	/* Run the entropy pool cryptography self-test.  */
	if (entpool_selftest() == -1)
		panic("entropy pool crypto self-test failed");

	/* Create the sysctl directory.  */
	sysctl_createv(&entropy_sysctllog, 0, NULL, &entropy_sysctlroot,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "entropy",
	    SYSCTL_DESCR("Entropy (random number sources) options"),
	    NULL, 0, NULL, 0,
	    CTL_KERN, CTL_CREATE, CTL_EOL);

	/* Create the sysctl knobs.  */
	/* XXX These shouldn't be writable at securelevel>0.  */
	sysctl_createv(&entropy_sysctllog, 0, &entropy_sysctlroot, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_BOOL, "collection",
	    SYSCTL_DESCR("Automatically collect entropy from hardware"),
	    NULL, 0, &entropy_collection, 0, CTL_CREATE, CTL_EOL);
	sysctl_createv(&entropy_sysctllog, 0, &entropy_sysctlroot, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_BOOL, "depletion",
	    SYSCTL_DESCR("`Deplete' entropy pool when observed"),
	    NULL, 0, &entropy_depletion, 0, CTL_CREATE, CTL_EOL);
	sysctl_createv(&entropy_sysctllog, 0, &entropy_sysctlroot, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "consolidate",
	    SYSCTL_DESCR("Trigger entropy consolidation now"),
	    sysctl_entropy_consolidate, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	sysctl_createv(&entropy_sysctllog, 0, &entropy_sysctlroot, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT, "gather",
	    SYSCTL_DESCR("Trigger entropy gathering from sources now"),
	    sysctl_entropy_gather, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	/* XXX These should maybe not be readable at securelevel>0.  */
	sysctl_createv(&entropy_sysctllog, 0, &entropy_sysctlroot, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY|CTLFLAG_PRIVATE, CTLTYPE_INT,
	    "needed", SYSCTL_DESCR("Systemwide entropy deficit"),
	    NULL, 0, &E->needed, 0, CTL_CREATE, CTL_EOL);
	sysctl_createv(&entropy_sysctllog, 0, &entropy_sysctlroot, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY|CTLFLAG_PRIVATE, CTLTYPE_INT,
	    "pending", SYSCTL_DESCR("Entropy pending on CPUs"),
	    NULL, 0, &E->pending, 0, CTL_CREATE, CTL_EOL);
	sysctl_createv(&entropy_sysctllog, 0, &entropy_sysctlroot, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY|CTLFLAG_PRIVATE, CTLTYPE_INT,
	    "epoch", SYSCTL_DESCR("Entropy epoch"),
	    NULL, 0, &E->epoch, 0, CTL_CREATE, CTL_EOL);

	/* Initialize the global state for multithreaded operation.  */
	mutex_init(&E->lock, MUTEX_DEFAULT, IPL_SOFTSERIAL);
	cv_init(&E->cv, "entropy");
	selinit(&E->selq);
	cv_init(&E->sourcelock_cv, "entsrclock");

	/* Make sure the seed source is attached.  */
	attach_seed_rndsource();

	/* Note if the bootloader didn't provide a seed.  */
	if (!E->seeded)
		aprint_debug("entropy: no seed from bootloader\n");

	/* Allocate the per-CPU records for all early entropy sources.  */
	LIST_FOREACH(rs, &E->sources, list)
		rs->state = percpu_alloc(sizeof(struct rndsource_cpu));

	/* Allocate and initialize the per-CPU state.  */
	entropy_percpu = percpu_create(sizeof(struct entropy_cpu),
	    entropy_init_cpu, entropy_fini_cpu, NULL);

	/* Enter the boot cycle count to get started.  */
	extra[i++] = entropy_timer();
	KASSERT(i == __arraycount(extra));
	entropy_enter(extra, sizeof extra, 0);
	explicit_memset(extra, 0, sizeof extra);

	/* We are now ready for multi-threaded operation.  */
	E->stage = ENTROPY_WARM;
}

static void
entropy_init_late_cpu(void *a, void *b)
{
	int bound;

	/*
	 * We're not necessarily in a softint lwp here (xc_broadcast
	 * triggers softint on other CPUs, but calls directly on this
	 * CPU), so explicitly bind to the current CPU to invoke the
	 * softintr -- this lets us have a simpler assertion in
	 * entropy_account_cpu.  Not necessary to avoid migration
	 * because xc_broadcast disables kpreemption anyway, but it
	 * doesn't hurt.
	 */
	bound = curlwp_bind();
	entropy_softintr(NULL);
	curlwp_bindx(bound);
}

/*
 * entropy_init_late()
 *
 *	Late initialization.  Panic on failure.
 *
 *	Requires CPUs to have been detected and LWPs to have started.
 */
static void
entropy_init_late(void)
{
	void *sih;
	int error;

	KASSERT(E->stage == ENTROPY_WARM);

	/*
	 * Establish the softint at the highest softint priority level.
	 * Must happen after CPU detection.
	 */
	sih = softint_establish(SOFTINT_SERIAL|SOFTINT_MPSAFE,
	    &entropy_softintr, NULL);
	if (sih == NULL)
		panic("unable to establish entropy softint");

	/*
	 * Create the entropy housekeeping thread.  Must happen after
	 * lwpinit.
	 */
	error = kthread_create(PRI_NONE, KTHREAD_MPSAFE|KTHREAD_TS, NULL,
	    entropy_thread, NULL, &entropy_lwp, "entbutler");
	if (error)
		panic("unable to create entropy housekeeping thread: %d",
		    error);

	/*
	 * Wait until the per-CPU initialization has hit all CPUs
	 * before proceeding to mark the entropy system hot and
	 * enabling use of the softint.
	 */
	xc_barrier(XC_HIGHPRI);
	E->stage = ENTROPY_HOT;
	atomic_store_relaxed(&entropy_sih, sih);

	/*
	 * At this point, entering new samples from interrupt handlers
	 * will trigger the softint to process them.  But there may be
	 * some samples that were entered from interrupt handlers
	 * before the softint was available.  Make sure we process
	 * those samples on all CPUs by running the softint logic on
	 * all CPUs.
	 */
	xc_wait(xc_broadcast(XC_HIGHPRI, entropy_init_late_cpu, NULL, NULL));
}

/*
 * entropy_init_cpu(ptr, cookie, ci)
 *
 *	percpu(9) constructor for per-CPU entropy pool.
 */
static void
entropy_init_cpu(void *ptr, void *cookie, struct cpu_info *ci)
{
	struct entropy_cpu *ec = ptr;
	const char *cpuname;

	ec->ec_evcnt = kmem_alloc(sizeof(*ec->ec_evcnt), KM_SLEEP);
	ec->ec_pool = kmem_zalloc(sizeof(*ec->ec_pool), KM_SLEEP);
	ec->ec_pending = 0;
	ec->ec_locked = false;

	/* XXX ci_cpuname may not be initialized early enough.  */
	cpuname = ci->ci_cpuname[0] == '\0' ? "cpu0" : ci->ci_cpuname;
	evcnt_attach_dynamic(&ec->ec_evcnt->softint, EVCNT_TYPE_MISC, NULL,
	    cpuname, "entropy softint");
	evcnt_attach_dynamic(&ec->ec_evcnt->intrdrop, EVCNT_TYPE_MISC, NULL,
	    cpuname, "entropy intrdrop");
	evcnt_attach_dynamic(&ec->ec_evcnt->intrtrunc, EVCNT_TYPE_MISC, NULL,
	    cpuname, "entropy intrtrunc");
}

/*
 * entropy_fini_cpu(ptr, cookie, ci)
 *
 *	percpu(9) destructor for per-CPU entropy pool.
 */
static void
entropy_fini_cpu(void *ptr, void *cookie, struct cpu_info *ci)
{
	struct entropy_cpu *ec = ptr;

	/*
	 * Zero any lingering data.  Disclosure of the per-CPU pool
	 * shouldn't retroactively affect the security of any keys
	 * generated, because entpool(9) erases whatever we have just
	 * drawn out of any pool, but better safe than sorry.
	 */
	explicit_memset(ec->ec_pool, 0, sizeof(*ec->ec_pool));

	evcnt_detach(&ec->ec_evcnt->intrtrunc);
	evcnt_detach(&ec->ec_evcnt->intrdrop);
	evcnt_detach(&ec->ec_evcnt->softint);

	kmem_free(ec->ec_pool, sizeof(*ec->ec_pool));
	kmem_free(ec->ec_evcnt, sizeof(*ec->ec_evcnt));
}

/*
 * ec = entropy_cpu_get(&lock)
 * entropy_cpu_put(&lock, ec)
 *
 *	Lock and unlock the per-CPU entropy state.  This only prevents
 *	access on the same CPU -- by hard interrupts, by soft
 *	interrupts, or by other threads.
 *
 *	Blocks soft interrupts and preemption altogether; doesn't block
 *	hard interrupts, but causes samples in hard interrupts to be
 *	dropped.
 */
static struct entropy_cpu *
entropy_cpu_get(struct entropy_cpu_lock *lock)
{
	struct entropy_cpu *ec;

	ec = percpu_getref(entropy_percpu);
	lock->ecl_s = splsoftserial();
	KASSERT(!ec->ec_locked);
	ec->ec_locked = true;
	lock->ecl_ncsw = curlwp->l_ncsw;
	__insn_barrier();

	return ec;
}

static void
entropy_cpu_put(struct entropy_cpu_lock *lock, struct entropy_cpu *ec)
{

	KASSERT(ec == percpu_getptr_remote(entropy_percpu, curcpu()));
	KASSERT(ec->ec_locked);

	__insn_barrier();
	KASSERT(lock->ecl_ncsw == curlwp->l_ncsw);
	ec->ec_locked = false;
	splx(lock->ecl_s);
	percpu_putref(entropy_percpu);
}

/*
 * entropy_seed(seed)
 *
 *	Seed the entropy pool with seed.  Meant to be called as early
 *	as possible by the bootloader; may be called before or after
 *	entropy_init.  Must be called before system reaches userland.
 *	Must be called in thread or soft interrupt context, not in hard
 *	interrupt context.  Must be called at most once.
 *
 *	Overwrites the seed in place.  Caller may then free the memory.
 */
static void
entropy_seed(rndsave_t *seed)
{
	SHA1_CTX ctx;
	uint8_t digest[SHA1_DIGEST_LENGTH];
	bool seeded;

	/*
	 * Verify the checksum.  If the checksum fails, take the data
	 * but ignore the entropy estimate -- the file may have been
	 * incompletely written with garbage, which is harmless to add
	 * but may not be as unpredictable as alleged.
	 */
	SHA1Init(&ctx);
	SHA1Update(&ctx, (const void *)&seed->entropy, sizeof(seed->entropy));
	SHA1Update(&ctx, seed->data, sizeof(seed->data));
	SHA1Final(digest, &ctx);
	CTASSERT(sizeof(seed->digest) == sizeof(digest));
	if (!consttime_memequal(digest, seed->digest, sizeof(digest))) {
		printf("entropy: invalid seed checksum\n");
		seed->entropy = 0;
	}
	explicit_memset(&ctx, 0, sizeof ctx);
	explicit_memset(digest, 0, sizeof digest);

	/*
	 * If the entropy is insensibly large, try byte-swapping.
	 * Otherwise assume the file is corrupted and act as though it
	 * has zero entropy.
	 */
	if (howmany(seed->entropy, NBBY) > sizeof(seed->data)) {
		seed->entropy = bswap32(seed->entropy);
		if (howmany(seed->entropy, NBBY) > sizeof(seed->data))
			seed->entropy = 0;
	}

	/* Make sure the seed source is attached.  */
	attach_seed_rndsource();

	/* Test and set E->seeded.  */
	if (E->stage >= ENTROPY_WARM)
		mutex_enter(&E->lock);
	seeded = E->seeded;
	E->seeded = (seed->entropy > 0);
	if (E->stage >= ENTROPY_WARM)
		mutex_exit(&E->lock);

	/*
	 * If we've been seeded, may be re-entering the same seed
	 * (e.g., bootloader vs module init, or something).  No harm in
	 * entering it twice, but it contributes no additional entropy.
	 */
	if (seeded) {
		printf("entropy: double-seeded by bootloader\n");
		seed->entropy = 0;
	} else {
		printf("entropy: entering seed from bootloader"
		    " with %u bits of entropy\n", (unsigned)seed->entropy);
	}

	/* Enter it into the pool and promptly zero it.  */
	rnd_add_data(&seed_rndsource, seed->data, sizeof(seed->data),
	    seed->entropy);
	explicit_memset(seed, 0, sizeof(*seed));
}

/*
 * entropy_bootrequest()
 *
 *	Request entropy from all sources at boot, once config is
 *	complete and interrupts are running.
 */
void
entropy_bootrequest(void)
{
	int error;

	KASSERT(E->stage >= ENTROPY_WARM);

	/*
	 * Request enough to satisfy the maximum entropy shortage.
	 * This is harmless overkill if the bootloader provided a seed.
	 */
	mutex_enter(&E->lock);
	error = entropy_request(ENTROPY_CAPACITY, ENTROPY_WAIT);
	KASSERT(error == 0);
	mutex_exit(&E->lock);
}

/*
 * entropy_epoch()
 *
 *	Returns the current entropy epoch.  If this changes, you should
 *	reseed.  If -1, means system entropy has not yet reached full
 *	entropy or been explicitly consolidated; never reverts back to
 *	-1.  Never zero, so you can always use zero as an uninitialized
 *	sentinel value meaning `reseed ASAP'.
 *
 *	Usage model:
 *
 *		struct foo {
 *			struct crypto_prng prng;
 *			unsigned epoch;
 *		} *foo;
 *
 *		unsigned epoch = entropy_epoch();
 *		if (__predict_false(epoch != foo->epoch)) {
 *			uint8_t seed[32];
 *			if (entropy_extract(seed, sizeof seed, 0) != 0)
 *				warn("no entropy");
 *			crypto_prng_reseed(&foo->prng, seed, sizeof seed);
 *			foo->epoch = epoch;
 *		}
 */
unsigned
entropy_epoch(void)
{

	/*
	 * Unsigned int, so no need for seqlock for an atomic read, but
	 * make sure we read it afresh each time.
	 */
	return atomic_load_relaxed(&E->epoch);
}

/*
 * entropy_ready()
 *
 *	True if the entropy pool has full entropy.
 */
bool
entropy_ready(void)
{

	return atomic_load_relaxed(&E->needed) == 0;
}

/*
 * entropy_account_cpu(ec)
 *
 *	Consider whether to consolidate entropy into the global pool
 *	after we just added some into the current CPU's pending pool.
 *
 *	- If this CPU can provide enough entropy now, do so.
 *
 *	- If this and whatever else is available on other CPUs can
 *	  provide enough entropy, kick the consolidation thread.
 *
 *	- Otherwise, do as little as possible, except maybe consolidate
 *	  entropy at most once a minute.
 *
 *	Caller must be bound to a CPU and therefore have exclusive
 *	access to ec.  Will acquire and release the global lock.
 */
static void
entropy_account_cpu(struct entropy_cpu *ec)
{
	struct entropy_cpu_lock lock;
	struct entropy_cpu *ec0;
	unsigned diff;

	KASSERT(E->stage >= ENTROPY_WARM);
	KASSERT(curlwp->l_pflag & LP_BOUND);

	/*
	 * If there's no entropy needed, and entropy has been
	 * consolidated in the last minute, do nothing.
	 */
	if (__predict_true(atomic_load_relaxed(&E->needed) == 0) &&
	    __predict_true(!atomic_load_relaxed(&entropy_depletion)) &&
	    __predict_true((time_uptime - E->timestamp) <= 60))
		return;

	/*
	 * Consider consolidation, under the global lock and with the
	 * per-CPU state locked.
	 */
	mutex_enter(&E->lock);
	ec0 = entropy_cpu_get(&lock);
	KASSERT(ec0 == ec);
	if (ec->ec_pending == 0) {
		/* Raced with consolidation xcall.  Nothing to do.  */
	} else if (E->needed != 0 && E->needed <= ec->ec_pending) {
		/*
		 * If we have not yet attained full entropy but we can
		 * now, do so.  This way we disseminate entropy
		 * promptly when it becomes available early at boot;
		 * otherwise we leave it to the entropy consolidation
		 * thread, which is rate-limited to mitigate side
		 * channels and abuse.
		 */
		uint8_t buf[ENTPOOL_CAPACITY];

		/* Transfer from the local pool to the global pool.  */
		entpool_extract(ec->ec_pool, buf, sizeof buf);
		entpool_enter(&E->pool, buf, sizeof buf);
		atomic_store_relaxed(&ec->ec_pending, 0);
		atomic_store_relaxed(&E->needed, 0);

		/* Notify waiters that we now have full entropy.  */
		entropy_notify();
		entropy_immediate_evcnt.ev_count++;
	} else {
		/* Determine how much we can add to the global pool.  */
		KASSERTMSG(E->pending <= ENTROPY_CAPACITY*NBBY,
		    "E->pending=%u", E->pending);
		diff = MIN(ec->ec_pending, ENTROPY_CAPACITY*NBBY - E->pending);

		/*
		 * This should make a difference unless we are already
		 * saturated.
		 */
		KASSERTMSG(diff || E->pending == ENTROPY_CAPACITY*NBBY,
		    "diff=%u E->pending=%u ec->ec_pending=%u cap=%u",
		    diff, E->pending, ec->ec_pending,
		    (unsigned)ENTROPY_CAPACITY*NBBY);

		/* Add to the global, subtract from the local.  */
		E->pending += diff;
		KASSERT(E->pending);
		KASSERTMSG(E->pending <= ENTROPY_CAPACITY*NBBY,
		    "E->pending=%u", E->pending);
		atomic_store_relaxed(&ec->ec_pending, ec->ec_pending - diff);

		if (E->needed <= E->pending) {
			/*
			 * Enough entropy between all the per-CPU
			 * pools.  Wake up the housekeeping thread.
			 *
			 * If we don't need any entropy, this doesn't
			 * mean much, but it is the only time we ever
			 * gather additional entropy in case the
			 * accounting has been overly optimistic.  This
			 * happens at most once a minute, so there's
			 * negligible performance cost.
			 */
			E->consolidate = true;
			cv_broadcast(&E->cv);
			if (E->needed == 0)
				entropy_discretionary_evcnt.ev_count++;
		} else {
			/* Can't get full entropy.  Keep gathering.  */
			entropy_partial_evcnt.ev_count++;
		}
	}
	entropy_cpu_put(&lock, ec);
	mutex_exit(&E->lock);
}

/*
 * entropy_enter_early(buf, len, nbits)
 *
 *	Do entropy bookkeeping globally, before we have established
 *	per-CPU pools.  Enter directly into the global pool in the hope
 *	that we enter enough before the first entropy_extract to thwart
 *	iterative-guessing attacks; entropy_extract will warn if not.
 */
static void
entropy_enter_early(const void *buf, size_t len, unsigned nbits)
{
	bool notify = false;

	KASSERT(E->stage == ENTROPY_COLD);

	/* Enter it into the pool.  */
	entpool_enter(&E->pool, buf, len);

	/*
	 * Decide whether to notify reseed -- we will do so if either:
	 * (a) we transition from partial entropy to full entropy, or
	 * (b) we get a batch of full entropy all at once.
	 */
	notify |= (E->needed && E->needed <= nbits);
	notify |= (nbits >= ENTROPY_CAPACITY*NBBY);

	/* Subtract from the needed count and notify if appropriate.  */
	E->needed -= MIN(E->needed, nbits);
	if (notify) {
		entropy_notify();
		entropy_immediate_evcnt.ev_count++;
	}
}

/*
 * entropy_enter(buf, len, nbits)
 *
 *	Enter len bytes of data from buf into the system's entropy
 *	pool, stirring as necessary when the internal buffer fills up.
 *	nbits is a lower bound on the number of bits of entropy in the
 *	process that led to this sample.
 */
static void
entropy_enter(const void *buf, size_t len, unsigned nbits)
{
	struct entropy_cpu_lock lock;
	struct entropy_cpu *ec;
	unsigned pending;
	int bound;

	KASSERTMSG(!cpu_intr_p(),
	    "use entropy_enter_intr from interrupt context");
	KASSERTMSG(howmany(nbits, NBBY) <= len,
	    "impossible entropy rate: %u bits in %zu-byte string", nbits, len);

	/* If it's too early after boot, just use entropy_enter_early.  */
	if (__predict_false(E->stage == ENTROPY_COLD)) {
		entropy_enter_early(buf, len, nbits);
		return;
	}

	/*
	 * Bind ourselves to the current CPU so we don't switch CPUs
	 * between entering data into the current CPU's pool (and
	 * updating the pending count) and transferring it to the
	 * global pool in entropy_account_cpu.
	 */
	bound = curlwp_bind();

	/*
	 * With the per-CPU state locked, enter into the per-CPU pool
	 * and count up what we can add.
	 */
	ec = entropy_cpu_get(&lock);
	entpool_enter(ec->ec_pool, buf, len);
	pending = ec->ec_pending;
	pending += MIN(ENTROPY_CAPACITY*NBBY - pending, nbits);
	atomic_store_relaxed(&ec->ec_pending, pending);
	entropy_cpu_put(&lock, ec);

	/* Consolidate globally if appropriate based on what we added.  */
	if (pending)
		entropy_account_cpu(ec);

	curlwp_bindx(bound);
}

/*
 * entropy_enter_intr(buf, len, nbits)
 *
 *	Enter up to len bytes of data from buf into the system's
 *	entropy pool without stirring.  nbits is a lower bound on the
 *	number of bits of entropy in the process that led to this
 *	sample.  If the sample could be entered completely, assume
 *	nbits of entropy pending; otherwise assume none, since we don't
 *	know whether some parts of the sample are constant, for
 *	instance.  Schedule a softint to stir the entropy pool if
 *	needed.  Return true if used fully, false if truncated at all.
 *
 *	Using this in thread context will work, but you might as well
 *	use entropy_enter in that case.
 */
static bool
entropy_enter_intr(const void *buf, size_t len, unsigned nbits)
{
	struct entropy_cpu *ec;
	bool fullyused = false;
	uint32_t pending;
	void *sih;

	KASSERT(cpu_intr_p());
	KASSERTMSG(howmany(nbits, NBBY) <= len,
	    "impossible entropy rate: %u bits in %zu-byte string", nbits, len);

	/* If it's too early after boot, just use entropy_enter_early.  */
	if (__predict_false(E->stage == ENTROPY_COLD)) {
		entropy_enter_early(buf, len, nbits);
		return true;
	}

	/*
	 * Acquire the per-CPU state.  If someone is in the middle of
	 * using it, drop the sample.  Otherwise, take the lock so that
	 * higher-priority interrupts will drop their samples.
	 */
	ec = percpu_getref(entropy_percpu);
	if (ec->ec_locked) {
		ec->ec_evcnt->intrdrop.ev_count++;
		goto out0;
	}
	ec->ec_locked = true;
	__insn_barrier();

	/*
	 * Enter as much as we can into the per-CPU pool.  If it was
	 * truncated, schedule a softint to stir the pool and stop.
	 */
	if (!entpool_enter_nostir(ec->ec_pool, buf, len)) {
		sih = atomic_load_relaxed(&entropy_sih);
		if (__predict_true(sih != NULL))
			softint_schedule(sih);
		ec->ec_evcnt->intrtrunc.ev_count++;
		goto out1;
	}
	fullyused = true;

	/* Count up what we can contribute.  */
	pending = ec->ec_pending;
	pending += MIN(ENTROPY_CAPACITY*NBBY - pending, nbits);
	atomic_store_relaxed(&ec->ec_pending, pending);

	/* Schedule a softint if we added anything and it matters.  */
	if (__predict_false((atomic_load_relaxed(&E->needed) != 0) ||
		atomic_load_relaxed(&entropy_depletion)) &&
	    nbits != 0) {
		sih = atomic_load_relaxed(&entropy_sih);
		if (__predict_true(sih != NULL))
			softint_schedule(sih);
	}

out1:	/* Release the per-CPU state.  */
	KASSERT(ec->ec_locked);
	__insn_barrier();
	ec->ec_locked = false;
out0:	percpu_putref(entropy_percpu);

	return fullyused;
}

/*
 * entropy_softintr(cookie)
 *
 *	Soft interrupt handler for entering entropy.  Takes care of
 *	stirring the local CPU's entropy pool if it filled up during
 *	hard interrupts, and promptly crediting entropy from the local
 *	CPU's entropy pool to the global entropy pool if needed.
 */
static void
entropy_softintr(void *cookie)
{
	struct entropy_cpu_lock lock;
	struct entropy_cpu *ec;
	unsigned pending;

	/*
	 * With the per-CPU state locked, stir the pool if necessary
	 * and determine if there's any pending entropy on this CPU to
	 * account globally.
	 */
	ec = entropy_cpu_get(&lock);
	ec->ec_evcnt->softint.ev_count++;
	entpool_stir(ec->ec_pool);
	pending = ec->ec_pending;
	entropy_cpu_put(&lock, ec);

	/* Consolidate globally if appropriate based on what we added.  */
	if (pending)
		entropy_account_cpu(ec);
}

/*
 * entropy_thread(cookie)
 *
 *	Handle any asynchronous entropy housekeeping.
 */
static void
entropy_thread(void *cookie)
{
	bool consolidate;

	for (;;) {
		/*
		 * Wait until there's full entropy somewhere among the
		 * CPUs, as confirmed at most once per minute, or
		 * someone wants to consolidate.
		 */
		if (entropy_pending() >= ENTROPY_CAPACITY*NBBY) {
			consolidate = true;
		} else {
			mutex_enter(&E->lock);
			if (!E->consolidate)
				cv_timedwait(&E->cv, &E->lock, 60*hz);
			consolidate = E->consolidate;
			E->consolidate = false;
			mutex_exit(&E->lock);
		}

		if (consolidate) {
			/* Do it.  */
			entropy_do_consolidate();

			/* Mitigate abuse.  */
			kpause("entropy", false, hz, NULL);
		}
	}
}

/*
 * entropy_pending()
 *
 *	Count up the amount of entropy pending on other CPUs.
 */
static uint32_t
entropy_pending(void)
{
	uint32_t pending = 0;

	percpu_foreach(entropy_percpu, &entropy_pending_cpu, &pending);
	return pending;
}

static void
entropy_pending_cpu(void *ptr, void *cookie, struct cpu_info *ci)
{
	struct entropy_cpu *ec = ptr;
	uint32_t *pendingp = cookie;
	uint32_t cpu_pending;

	cpu_pending = atomic_load_relaxed(&ec->ec_pending);
	*pendingp += MIN(ENTROPY_CAPACITY*NBBY - *pendingp, cpu_pending);
}

/*
 * entropy_do_consolidate()
 *
 *	Issue a cross-call to gather entropy on all CPUs and advance
 *	the entropy epoch.
 */
static void
entropy_do_consolidate(void)
{
	static const struct timeval interval = {.tv_sec = 60, .tv_usec = 0};
	static struct timeval lasttime; /* serialized by E->lock */
	struct entpool pool;
	uint8_t buf[ENTPOOL_CAPACITY];
	unsigned diff;
	uint64_t ticket;

	/* Gather entropy on all CPUs into a temporary pool.  */
	memset(&pool, 0, sizeof pool);
	ticket = xc_broadcast(0, &entropy_consolidate_xc, &pool, NULL);
	xc_wait(ticket);

	/* Acquire the lock to notify waiters.  */
	mutex_enter(&E->lock);

	/* Count another consolidation.  */
	entropy_consolidate_evcnt.ev_count++;

	/* Note when we last consolidated, i.e. now.  */
	E->timestamp = time_uptime;

	/* Mix what we gathered into the global pool.  */
	entpool_extract(&pool, buf, sizeof buf);
	entpool_enter(&E->pool, buf, sizeof buf);
	explicit_memset(&pool, 0, sizeof pool);

	/* Count the entropy that was gathered.  */
	diff = MIN(E->needed, E->pending);
	atomic_store_relaxed(&E->needed, E->needed - diff);
	E->pending -= diff;
	if (__predict_false(E->needed > 0)) {
		if ((boothowto & AB_DEBUG) != 0 &&
		    ratecheck(&lasttime, &interval)) {
			printf("WARNING:"
			    " consolidating less than full entropy\n");
		}
	}

	/* Advance the epoch and notify waiters.  */
	entropy_notify();

	/* Release the lock.  */
	mutex_exit(&E->lock);
}

/*
 * entropy_consolidate_xc(vpool, arg2)
 *
 *	Extract output from the local CPU's input pool and enter it
 *	into a temporary pool passed as vpool.
 */
static void
entropy_consolidate_xc(void *vpool, void *arg2 __unused)
{
	struct entpool *pool = vpool;
	struct entropy_cpu_lock lock;
	struct entropy_cpu *ec;
	uint8_t buf[ENTPOOL_CAPACITY];
	uint32_t extra[7];
	unsigned i = 0;

	/* Grab CPU number and cycle counter to mix extra into the pool.  */
	extra[i++] = cpu_number();
	extra[i++] = entropy_timer();

	/*
	 * With the per-CPU state locked, extract from the per-CPU pool
	 * and count it as no longer pending.
	 */
	ec = entropy_cpu_get(&lock);
	extra[i++] = entropy_timer();
	entpool_extract(ec->ec_pool, buf, sizeof buf);
	atomic_store_relaxed(&ec->ec_pending, 0);
	extra[i++] = entropy_timer();
	entropy_cpu_put(&lock, ec);
	extra[i++] = entropy_timer();

	/*
	 * Copy over statistics, and enter the per-CPU extract and the
	 * extra timing into the temporary pool, under the global lock.
	 */
	mutex_enter(&E->lock);
	extra[i++] = entropy_timer();
	entpool_enter(pool, buf, sizeof buf);
	explicit_memset(buf, 0, sizeof buf);
	extra[i++] = entropy_timer();
	KASSERT(i == __arraycount(extra));
	entpool_enter(pool, extra, sizeof extra);
	explicit_memset(extra, 0, sizeof extra);
	mutex_exit(&E->lock);
}

/*
 * entropy_notify()
 *
 *	Caller just contributed entropy to the global pool.  Advance
 *	the entropy epoch and notify waiters.
 *
 *	Caller must hold the global entropy lock.  Except for the
 *	`sysctl -w kern.entropy.consolidate=1` trigger, the caller must
 *	have just have transitioned from partial entropy to full
 *	entropy -- E->needed should be zero now.
 */
static void
entropy_notify(void)
{
	static const struct timeval interval = {.tv_sec = 60, .tv_usec = 0};
	static struct timeval lasttime; /* serialized by E->lock */
	unsigned epoch;

	KASSERT(E->stage == ENTROPY_COLD || mutex_owned(&E->lock));

	/*
	 * If this is the first time, print a message to the console
	 * that we're ready so operators can compare it to the timing
	 * of other events.
	 */
	if (__predict_false(E->epoch == (unsigned)-1) && E->needed == 0)
		printf("entropy: ready\n");

	/* Set the epoch; roll over from UINTMAX-1 to 1.  */
	if (__predict_true(!atomic_load_relaxed(&entropy_depletion)) ||
	    ratecheck(&lasttime, &interval)) {
		epoch = E->epoch + 1;
		if (epoch == 0 || epoch == (unsigned)-1)
			epoch = 1;
		atomic_store_relaxed(&E->epoch, epoch);
	}
	KASSERT(E->epoch != (unsigned)-1);

	/* Notify waiters.  */
	if (E->stage >= ENTROPY_WARM) {
		cv_broadcast(&E->cv);
		selnotify(&E->selq, POLLIN|POLLRDNORM, NOTE_SUBMIT);
	}

	/* Count another notification.  */
	entropy_notify_evcnt.ev_count++;
}

/*
 * entropy_consolidate()
 *
 *	Trigger entropy consolidation and wait for it to complete.
 *
 *	This should be used sparingly, not periodically -- requiring
 *	conscious intervention by the operator or a clear policy
 *	decision.  Otherwise, the kernel will automatically consolidate
 *	when enough entropy has been gathered into per-CPU pools to
 *	transition to full entropy.
 */
void
entropy_consolidate(void)
{
	uint64_t ticket;
	int error;

	KASSERT(E->stage == ENTROPY_HOT);

	mutex_enter(&E->lock);
	ticket = entropy_consolidate_evcnt.ev_count;
	E->consolidate = true;
	cv_broadcast(&E->cv);
	while (ticket == entropy_consolidate_evcnt.ev_count) {
		error = cv_wait_sig(&E->cv, &E->lock);
		if (error)
			break;
	}
	mutex_exit(&E->lock);
}

/*
 * sysctl -w kern.entropy.consolidate=1
 *
 *	Trigger entropy consolidation and wait for it to complete.
 *	Writable only by superuser.  This, writing to /dev/random, and
 *	ioctl(RNDADDDATA) are the only ways for the system to
 *	consolidate entropy if the operator knows something the kernel
 *	doesn't about how unpredictable the pending entropy pools are.
 */
static int
sysctl_entropy_consolidate(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	int arg;
	int error;

	KASSERT(E->stage == ENTROPY_HOT);

	node.sysctl_data = &arg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;
	if (arg)
		entropy_consolidate();

	return error;
}

/*
 * sysctl -w kern.entropy.gather=1
 *
 *	Trigger gathering entropy from all on-demand sources, and wait
 *	for synchronous sources (but not asynchronous sources) to
 *	complete.  Writable only by superuser.
 */
static int
sysctl_entropy_gather(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	int arg;
	int error;

	KASSERT(E->stage == ENTROPY_HOT);

	node.sysctl_data = &arg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;
	if (arg) {
		mutex_enter(&E->lock);
		error = entropy_request(ENTROPY_CAPACITY,
		    ENTROPY_WAIT|ENTROPY_SIG);
		mutex_exit(&E->lock);
	}

	return 0;
}

/*
 * entropy_extract(buf, len, flags)
 *
 *	Extract len bytes from the global entropy pool into buf.
 *
 *	Flags may have:
 *
 *		ENTROPY_WAIT	Wait for entropy if not available yet.
 *		ENTROPY_SIG	Allow interruption by a signal during wait.
 *		ENTROPY_HARDFAIL Either fill the buffer with full entropy,
 *				or fail without filling it at all.
 *
 *	Return zero on success, or error on failure:
 *
 *		EWOULDBLOCK	No entropy and ENTROPY_WAIT not set.
 *		EINTR/ERESTART	No entropy, ENTROPY_SIG set, and interrupted.
 *
 *	If ENTROPY_WAIT is set, allowed only in thread context.  If
 *	ENTROPY_WAIT is not set, allowed up to IPL_VM.  (XXX That's
 *	awfully high...  Do we really need it in hard interrupts?  This
 *	arises from use of cprng_strong(9).)
 */
int
entropy_extract(void *buf, size_t len, int flags)
{
	static const struct timeval interval = {.tv_sec = 60, .tv_usec = 0};
	static struct timeval lasttime; /* serialized by E->lock */
	int error;

	if (ISSET(flags, ENTROPY_WAIT)) {
		ASSERT_SLEEPABLE();
		KASSERTMSG(E->stage >= ENTROPY_WARM,
		    "can't wait for entropy until warm");
	}

	/* Refuse to operate in interrupt context.  */
	KASSERT(!cpu_intr_p());

	/* Acquire the global lock to get at the global pool.  */
	if (E->stage >= ENTROPY_WARM)
		mutex_enter(&E->lock);

	/* Wait until there is enough entropy in the system.  */
	error = 0;
	while (E->needed) {
		/* Ask for more, synchronously if possible.  */
		error = entropy_request(len, flags);
		if (error)
			break;

		/* If we got enough, we're done.  */
		if (E->needed == 0) {
			KASSERT(error == 0);
			break;
		}

		/* If not waiting, stop here.  */
		if (!ISSET(flags, ENTROPY_WAIT)) {
			error = EWOULDBLOCK;
			break;
		}

		/* Wait for some entropy to come in and try again.  */
		KASSERT(E->stage >= ENTROPY_WARM);
		printf("entropy: pid %d (%s) blocking due to lack of entropy\n",
		       curproc->p_pid, curproc->p_comm);

		if (ISSET(flags, ENTROPY_SIG)) {
			error = cv_wait_sig(&E->cv, &E->lock);
			if (error)
				break;
		} else {
			cv_wait(&E->cv, &E->lock);
		}
	}

	/*
	 * Count failure -- but fill the buffer nevertheless, unless
	 * the caller specified ENTROPY_HARDFAIL.
	 */
	if (error) {
		if (ISSET(flags, ENTROPY_HARDFAIL))
			goto out;
		entropy_extract_fail_evcnt.ev_count++;
	}

	/*
	 * Report a warning if we have never yet reached full entropy.
	 * This is the only case where we consider entropy to be
	 * `depleted' without kern.entropy.depletion enabled -- when we
	 * only have partial entropy, an adversary may be able to
	 * narrow the state of the pool down to a small number of
	 * possibilities; the output then enables them to confirm a
	 * guess, reducing its entropy from the adversary's perspective
	 * to zero.
	 */
	if (__predict_false(E->epoch == (unsigned)-1)) {
		if (ratecheck(&lasttime, &interval))
			printf("WARNING:"
			    " system needs entropy for security;"
			    " see entropy(7)\n");
		atomic_store_relaxed(&E->needed, ENTROPY_CAPACITY*NBBY);
	}

	/* Extract data from the pool, and `deplete' if we're doing that.  */
	entpool_extract(&E->pool, buf, len);
	if (__predict_false(atomic_load_relaxed(&entropy_depletion)) &&
	    error == 0) {
		unsigned cost = MIN(len, ENTROPY_CAPACITY)*NBBY;

		atomic_store_relaxed(&E->needed,
		    E->needed + MIN(ENTROPY_CAPACITY*NBBY - E->needed, cost));
		entropy_deplete_evcnt.ev_count++;
	}

out:	/* Release the global lock and return the error.  */
	if (E->stage >= ENTROPY_WARM)
		mutex_exit(&E->lock);
	return error;
}

/*
 * entropy_poll(events)
 *
 *	Return the subset of events ready, and if it is not all of
 *	events, record curlwp as waiting for entropy.
 */
int
entropy_poll(int events)
{
	int revents = 0;

	KASSERT(E->stage >= ENTROPY_WARM);

	/* Always ready for writing.  */
	revents |= events & (POLLOUT|POLLWRNORM);

	/* Narrow it down to reads.  */
	events &= POLLIN|POLLRDNORM;
	if (events == 0)
		return revents;

	/*
	 * If we have reached full entropy and we're not depleting
	 * entropy, we are forever ready.
	 */
	if (__predict_true(atomic_load_relaxed(&E->needed) == 0) &&
	    __predict_true(!atomic_load_relaxed(&entropy_depletion)))
		return revents | events;

	/*
	 * Otherwise, check whether we need entropy under the lock.  If
	 * we don't, we're ready; if we do, add ourselves to the queue.
	 */
	mutex_enter(&E->lock);
	if (E->needed == 0)
		revents |= events;
	else
		selrecord(curlwp, &E->selq);
	mutex_exit(&E->lock);

	return revents;
}

/*
 * filt_entropy_read_detach(kn)
 *
 *	struct filterops::f_detach callback for entropy read events:
 *	remove kn from the list of waiters.
 */
static void
filt_entropy_read_detach(struct knote *kn)
{

	KASSERT(E->stage >= ENTROPY_WARM);

	mutex_enter(&E->lock);
	selremove_knote(&E->selq, kn);
	mutex_exit(&E->lock);
}

/*
 * filt_entropy_read_event(kn, hint)
 *
 *	struct filterops::f_event callback for entropy read events:
 *	poll for entropy.  Caller must hold the global entropy lock if
 *	hint is NOTE_SUBMIT, and must not if hint is not NOTE_SUBMIT.
 */
static int
filt_entropy_read_event(struct knote *kn, long hint)
{
	int ret;

	KASSERT(E->stage >= ENTROPY_WARM);

	/* Acquire the lock, if caller is outside entropy subsystem.  */
	if (hint == NOTE_SUBMIT)
		KASSERT(mutex_owned(&E->lock));
	else
		mutex_enter(&E->lock);

	/*
	 * If we still need entropy, can't read anything; if not, can
	 * read arbitrarily much.
	 */
	if (E->needed != 0) {
		ret = 0;
	} else {
		if (atomic_load_relaxed(&entropy_depletion))
			kn->kn_data = ENTROPY_CAPACITY*NBBY;
		else
			kn->kn_data = MIN(INT64_MAX, SSIZE_MAX);
		ret = 1;
	}

	/* Release the lock, if caller is outside entropy subsystem.  */
	if (hint == NOTE_SUBMIT)
		KASSERT(mutex_owned(&E->lock));
	else
		mutex_exit(&E->lock);

	return ret;
}

/* XXX Makes sense only for /dev/u?random.  */
static const struct filterops entropy_read_filtops = {
	.f_flags = FILTEROP_ISFD | FILTEROP_MPSAFE,
	.f_attach = NULL,
	.f_detach = filt_entropy_read_detach,
	.f_event = filt_entropy_read_event,
};

/*
 * entropy_kqfilter(kn)
 *
 *	Register kn to receive entropy event notifications.  May be
 *	EVFILT_READ or EVFILT_WRITE; anything else yields EINVAL.
 */
int
entropy_kqfilter(struct knote *kn)
{

	KASSERT(E->stage >= ENTROPY_WARM);

	switch (kn->kn_filter) {
	case EVFILT_READ:
		/* Enter into the global select queue.  */
		mutex_enter(&E->lock);
		kn->kn_fop = &entropy_read_filtops;
		selrecord_knote(&E->selq, kn);
		mutex_exit(&E->lock);
		return 0;
	case EVFILT_WRITE:
		/* Can always dump entropy into the system.  */
		kn->kn_fop = &seltrue_filtops;
		return 0;
	default:
		return EINVAL;
	}
}

/*
 * rndsource_setcb(rs, get, getarg)
 *
 *	Set the request callback for the entropy source rs, if it can
 *	provide entropy on demand.  Must precede rnd_attach_source.
 */
void
rndsource_setcb(struct krndsource *rs, void (*get)(size_t, void *),
    void *getarg)
{

	rs->get = get;
	rs->getarg = getarg;
}

/*
 * rnd_attach_source(rs, name, type, flags)
 *
 *	Attach the entropy source rs.  Must be done after
 *	rndsource_setcb, if any, and before any calls to rnd_add_data.
 */
void
rnd_attach_source(struct krndsource *rs, const char *name, uint32_t type,
    uint32_t flags)
{
	uint32_t extra[4];
	unsigned i = 0;

	/* Grab cycle counter to mix extra into the pool.  */
	extra[i++] = entropy_timer();

	/*
	 * Apply some standard flags:
	 *
	 * - We do not bother with network devices by default, for
	 *   hysterical raisins (perhaps: because it is often the case
	 *   that an adversary can influence network packet timings).
	 */
	switch (type) {
	case RND_TYPE_NET:
		flags |= RND_FLAG_NO_COLLECT;
		break;
	}

	/* Sanity-check the callback if RND_FLAG_HASCB is set.  */
	KASSERT(!ISSET(flags, RND_FLAG_HASCB) || rs->get != NULL);

	/* Initialize the random source.  */
	memset(rs->name, 0, sizeof(rs->name)); /* paranoia */
	strlcpy(rs->name, name, sizeof(rs->name));
	memset(&rs->time_delta, 0, sizeof(rs->time_delta));
	memset(&rs->value_delta, 0, sizeof(rs->value_delta));
	rs->total = 0;
	rs->type = type;
	rs->flags = flags;
	if (E->stage >= ENTROPY_WARM)
		rs->state = percpu_alloc(sizeof(struct rndsource_cpu));
	extra[i++] = entropy_timer();

	/* Wire it into the global list of random sources.  */
	if (E->stage >= ENTROPY_WARM)
		mutex_enter(&E->lock);
	LIST_INSERT_HEAD(&E->sources, rs, list);
	if (E->stage >= ENTROPY_WARM)
		mutex_exit(&E->lock);
	extra[i++] = entropy_timer();

	/* Request that it provide entropy ASAP, if we can.  */
	if (ISSET(flags, RND_FLAG_HASCB))
		(*rs->get)(ENTROPY_CAPACITY, rs->getarg);
	extra[i++] = entropy_timer();

	/* Mix the extra into the pool.  */
	KASSERT(i == __arraycount(extra));
	entropy_enter(extra, sizeof extra, 0);
	explicit_memset(extra, 0, sizeof extra);
}

/*
 * rnd_detach_source(rs)
 *
 *	Detach the entropy source rs.  May sleep waiting for users to
 *	drain.  Further use is not allowed.
 */
void
rnd_detach_source(struct krndsource *rs)
{

	/*
	 * If we're cold (shouldn't happen, but hey), just remove it
	 * from the list -- there's nothing allocated.
	 */
	if (E->stage == ENTROPY_COLD) {
		LIST_REMOVE(rs, list);
		return;
	}

	/* We may have to wait for entropy_request.  */
	ASSERT_SLEEPABLE();

	/* Wait until the source list is not in use, and remove it.  */
	mutex_enter(&E->lock);
	while (E->sourcelock)
		cv_wait(&E->sourcelock_cv, &E->lock);
	LIST_REMOVE(rs, list);
	mutex_exit(&E->lock);

	/* Free the per-CPU data.  */
	percpu_free(rs->state, sizeof(struct rndsource_cpu));
}

/*
 * rnd_lock_sources(flags)
 *
 *	Lock the list of entropy sources.  Caller must hold the global
 *	entropy lock.  If successful, no rndsource will go away until
 *	rnd_unlock_sources even while the caller releases the global
 *	entropy lock.
 *
 *	If flags & ENTROPY_WAIT, wait for concurrent access to finish.
 *	If flags & ENTROPY_SIG, allow interruption by signal.
 */
static int __attribute__((warn_unused_result))
rnd_lock_sources(int flags)
{
	int error;

	KASSERT(E->stage == ENTROPY_COLD || mutex_owned(&E->lock));

	while (E->sourcelock) {
		KASSERT(E->stage >= ENTROPY_WARM);
		if (!ISSET(flags, ENTROPY_WAIT))
			return EWOULDBLOCK;
		if (ISSET(flags, ENTROPY_SIG)) {
			error = cv_wait_sig(&E->sourcelock_cv, &E->lock);
			if (error)
				return error;
		} else {
			cv_wait(&E->sourcelock_cv, &E->lock);
		}
	}

	E->sourcelock = curlwp;
	return 0;
}

/*
 * rnd_unlock_sources()
 *
 *	Unlock the list of sources after rnd_lock_sources.  Caller must
 *	hold the global entropy lock.
 */
static void
rnd_unlock_sources(void)
{

	KASSERT(E->stage == ENTROPY_COLD || mutex_owned(&E->lock));

	KASSERTMSG(E->sourcelock == curlwp, "lwp %p releasing lock held by %p",
	    curlwp, E->sourcelock);
	E->sourcelock = NULL;
	if (E->stage >= ENTROPY_WARM)
		cv_signal(&E->sourcelock_cv);
}

/*
 * rnd_sources_locked()
 *
 *	True if we hold the list of rndsources locked, for diagnostic
 *	assertions.
 */
static bool __diagused
rnd_sources_locked(void)
{

	return E->sourcelock == curlwp;
}

/*
 * entropy_request(nbytes, flags)
 *
 *	Request nbytes bytes of entropy from all sources in the system.
 *	OK if we overdo it.  Caller must hold the global entropy lock;
 *	will release and re-acquire it.
 *
 *	If flags & ENTROPY_WAIT, wait for concurrent access to finish.
 *	If flags & ENTROPY_SIG, allow interruption by signal.
 */
static int
entropy_request(size_t nbytes, int flags)
{
	struct krndsource *rs;
	int error;

	KASSERT(E->stage == ENTROPY_COLD || mutex_owned(&E->lock));
	if (flags & ENTROPY_WAIT)
		ASSERT_SLEEPABLE();

	/*
	 * Lock the list of entropy sources to block rnd_detach_source
	 * until we're done, and to serialize calls to the entropy
	 * callbacks as guaranteed to drivers.
	 */
	error = rnd_lock_sources(flags);
	if (error)
		return error;
	entropy_request_evcnt.ev_count++;

	/* Clamp to the maximum reasonable request.  */
	nbytes = MIN(nbytes, ENTROPY_CAPACITY);

	/* Walk the list of sources.  */
	LIST_FOREACH(rs, &E->sources, list) {
		/* Skip sources without callbacks.  */
		if (!ISSET(rs->flags, RND_FLAG_HASCB))
			continue;

		/*
		 * Skip sources that are disabled altogether -- we
		 * would just ignore their samples anyway.
		 */
		if (ISSET(rs->flags, RND_FLAG_NO_COLLECT))
			continue;

		/* Drop the lock while we call the callback.  */
		if (E->stage >= ENTROPY_WARM)
			mutex_exit(&E->lock);
		(*rs->get)(nbytes, rs->getarg);
		if (E->stage >= ENTROPY_WARM)
			mutex_enter(&E->lock);
	}

	/* Request done; unlock the list of entropy sources.  */
	rnd_unlock_sources();
	return 0;
}

/*
 * rnd_add_uint32(rs, value)
 *
 *	Enter 32 bits of data from an entropy source into the pool.
 *
 *	If rs is NULL, may not be called from interrupt context.
 *
 *	If rs is non-NULL, may be called from any context.  May drop
 *	data if called from interrupt context.
 */
void
rnd_add_uint32(struct krndsource *rs, uint32_t value)
{

	rnd_add_data(rs, &value, sizeof value, 0);
}

void
_rnd_add_uint32(struct krndsource *rs, uint32_t value)
{

	rnd_add_data(rs, &value, sizeof value, 0);
}

void
_rnd_add_uint64(struct krndsource *rs, uint64_t value)
{

	rnd_add_data(rs, &value, sizeof value, 0);
}

/*
 * rnd_add_data(rs, buf, len, entropybits)
 *
 *	Enter data from an entropy source into the pool, with a
 *	driver's estimate of how much entropy the physical source of
 *	the data has.  If RND_FLAG_NO_ESTIMATE, we ignore the driver's
 *	estimate and treat it as zero.
 *
 *	If rs is NULL, may not be called from interrupt context.
 *
 *	If rs is non-NULL, may be called from any context.  May drop
 *	data if called from interrupt context.
 */
void
rnd_add_data(struct krndsource *rs, const void *buf, uint32_t len,
    uint32_t entropybits)
{
	uint32_t extra;
	uint32_t flags;

	KASSERTMSG(howmany(entropybits, NBBY) <= len,
	    "%s: impossible entropy rate:"
	    " %"PRIu32" bits in %"PRIu32"-byte string",
	    rs ? rs->name : "(anonymous)", entropybits, len);

	/* If there's no rndsource, just enter the data and time now.  */
	if (rs == NULL) {
		entropy_enter(buf, len, entropybits);
		extra = entropy_timer();
		entropy_enter(&extra, sizeof extra, 0);
		explicit_memset(&extra, 0, sizeof extra);
		return;
	}

	/* Load a snapshot of the flags.  Ioctl may change them under us.  */
	flags = atomic_load_relaxed(&rs->flags);

	/*
	 * Skip if:
	 * - we're not collecting entropy, or
	 * - the operator doesn't want to collect entropy from this, or
	 * - neither data nor timings are being collected from this.
	 */
	if (!atomic_load_relaxed(&entropy_collection) ||
	    ISSET(flags, RND_FLAG_NO_COLLECT) ||
	    !ISSET(flags, RND_FLAG_COLLECT_VALUE|RND_FLAG_COLLECT_TIME))
		return;

	/* If asked, ignore the estimate.  */
	if (ISSET(flags, RND_FLAG_NO_ESTIMATE))
		entropybits = 0;

	/* If we are collecting data, enter them.  */
	if (ISSET(flags, RND_FLAG_COLLECT_VALUE))
		rnd_add_data_1(rs, buf, len, entropybits,
		    RND_FLAG_COLLECT_VALUE);

	/* If we are collecting timings, enter one.  */
	if (ISSET(flags, RND_FLAG_COLLECT_TIME)) {
		extra = entropy_timer();
		rnd_add_data_1(rs, &extra, sizeof extra, 0,
		    RND_FLAG_COLLECT_TIME);
	}
}

static unsigned
add_sat(unsigned a, unsigned b)
{
	unsigned c = a + b;

	return (c < a ? UINT_MAX : c);
}

/*
 * rnd_add_data_1(rs, buf, len, entropybits, flag)
 *
 *	Internal subroutine to call either entropy_enter_intr, if we're
 *	in interrupt context, or entropy_enter if not, and to count the
 *	entropy in an rndsource.
 */
static void
rnd_add_data_1(struct krndsource *rs, const void *buf, uint32_t len,
    uint32_t entropybits, uint32_t flag)
{
	bool fullyused;

	/*
	 * If we're in interrupt context, use entropy_enter_intr and
	 * take note of whether it consumed the full sample; if not,
	 * use entropy_enter, which always consumes the full sample.
	 */
	if (curlwp && cpu_intr_p()) {
		fullyused = entropy_enter_intr(buf, len, entropybits);
	} else {
		entropy_enter(buf, len, entropybits);
		fullyused = true;
	}

	/*
	 * If we used the full sample, note how many bits were
	 * contributed from this source.
	 */
	if (fullyused) {
		if (__predict_false(E->stage == ENTROPY_COLD)) {
			rs->total = add_sat(rs->total, entropybits);
			switch (flag) {
			case RND_FLAG_COLLECT_TIME:
				rs->time_delta.insamples =
				    add_sat(rs->time_delta.insamples, 1);
				break;
			case RND_FLAG_COLLECT_VALUE:
				rs->value_delta.insamples =
				    add_sat(rs->value_delta.insamples, 1);
				break;
			}
		} else {
			struct rndsource_cpu *rc = percpu_getref(rs->state);

			atomic_store_relaxed(&rc->rc_entropybits,
			    add_sat(rc->rc_entropybits, entropybits));
			switch (flag) {
			case RND_FLAG_COLLECT_TIME:
				atomic_store_relaxed(&rc->rc_timesamples,
				    add_sat(rc->rc_timesamples, 1));
				break;
			case RND_FLAG_COLLECT_VALUE:
				atomic_store_relaxed(&rc->rc_datasamples,
				    add_sat(rc->rc_datasamples, 1));
				break;
			}
			percpu_putref(rs->state);
		}
	}
}

/*
 * rnd_add_data_sync(rs, buf, len, entropybits)
 *
 *	Same as rnd_add_data.  Originally used in rndsource callbacks,
 *	to break an unnecessary cycle; no longer really needed.
 */
void
rnd_add_data_sync(struct krndsource *rs, const void *buf, uint32_t len,
    uint32_t entropybits)
{

	rnd_add_data(rs, buf, len, entropybits);
}

/*
 * rndsource_entropybits(rs)
 *
 *	Return approximately the number of bits of entropy that have
 *	been contributed via rs so far.  Approximate if other CPUs may
 *	be calling rnd_add_data concurrently.
 */
static unsigned
rndsource_entropybits(struct krndsource *rs)
{
	unsigned nbits = rs->total;

	KASSERT(E->stage >= ENTROPY_WARM);
	KASSERT(rnd_sources_locked());
	percpu_foreach(rs->state, rndsource_entropybits_cpu, &nbits);
	return nbits;
}

static void
rndsource_entropybits_cpu(void *ptr, void *cookie, struct cpu_info *ci)
{
	struct rndsource_cpu *rc = ptr;
	unsigned *nbitsp = cookie;
	unsigned cpu_nbits;

	cpu_nbits = atomic_load_relaxed(&rc->rc_entropybits);
	*nbitsp += MIN(UINT_MAX - *nbitsp, cpu_nbits);
}

/*
 * rndsource_to_user(rs, urs)
 *
 *	Copy a description of rs out to urs for userland.
 */
static void
rndsource_to_user(struct krndsource *rs, rndsource_t *urs)
{

	KASSERT(E->stage >= ENTROPY_WARM);
	KASSERT(rnd_sources_locked());

	/* Avoid kernel memory disclosure.  */
	memset(urs, 0, sizeof(*urs));

	CTASSERT(sizeof(urs->name) == sizeof(rs->name));
	strlcpy(urs->name, rs->name, sizeof(urs->name));
	urs->total = rndsource_entropybits(rs);
	urs->type = rs->type;
	urs->flags = atomic_load_relaxed(&rs->flags);
}

/*
 * rndsource_to_user_est(rs, urse)
 *
 *	Copy a description of rs and estimation statistics out to urse
 *	for userland.
 */
static void
rndsource_to_user_est(struct krndsource *rs, rndsource_est_t *urse)
{

	KASSERT(E->stage >= ENTROPY_WARM);
	KASSERT(rnd_sources_locked());

	/* Avoid kernel memory disclosure.  */
	memset(urse, 0, sizeof(*urse));

	/* Copy out the rndsource description.  */
	rndsource_to_user(rs, &urse->rt);

	/* Gather the statistics.  */
	urse->dt_samples = rs->time_delta.insamples;
	urse->dt_total = 0;
	urse->dv_samples = rs->value_delta.insamples;
	urse->dv_total = urse->rt.total;
	percpu_foreach(rs->state, rndsource_to_user_est_cpu, urse);
}

static void
rndsource_to_user_est_cpu(void *ptr, void *cookie, struct cpu_info *ci)
{
	struct rndsource_cpu *rc = ptr;
	rndsource_est_t *urse = cookie;

	urse->dt_samples = add_sat(urse->dt_samples,
	    atomic_load_relaxed(&rc->rc_timesamples));
	urse->dv_samples = add_sat(urse->dv_samples,
	    atomic_load_relaxed(&rc->rc_datasamples));
}

/*
 * entropy_reset_xc(arg1, arg2)
 *
 *	Reset the current CPU's pending entropy to zero.
 */
static void
entropy_reset_xc(void *arg1 __unused, void *arg2 __unused)
{
	uint32_t extra = entropy_timer();
	struct entropy_cpu_lock lock;
	struct entropy_cpu *ec;

	/*
	 * With the per-CPU state locked, zero the pending count and
	 * enter a cycle count for fun.
	 */
	ec = entropy_cpu_get(&lock);
	ec->ec_pending = 0;
	entpool_enter(ec->ec_pool, &extra, sizeof extra);
	entropy_cpu_put(&lock, ec);
}

/*
 * entropy_ioctl(cmd, data)
 *
 *	Handle various /dev/random ioctl queries.
 */
int
entropy_ioctl(unsigned long cmd, void *data)
{
	struct krndsource *rs;
	bool privileged;
	int error;

	KASSERT(E->stage >= ENTROPY_WARM);

	/* Verify user's authorization to perform the ioctl.  */
	switch (cmd) {
	case RNDGETENTCNT:
	case RNDGETPOOLSTAT:
	case RNDGETSRCNUM:
	case RNDGETSRCNAME:
	case RNDGETESTNUM:
	case RNDGETESTNAME:
		error = kauth_authorize_device(kauth_cred_get(),
		    KAUTH_DEVICE_RND_GETPRIV, NULL, NULL, NULL, NULL);
		break;
	case RNDCTL:
		error = kauth_authorize_device(kauth_cred_get(),
		    KAUTH_DEVICE_RND_SETPRIV, NULL, NULL, NULL, NULL);
		break;
	case RNDADDDATA:
		error = kauth_authorize_device(kauth_cred_get(),
		    KAUTH_DEVICE_RND_ADDDATA, NULL, NULL, NULL, NULL);
		/* Ascertain whether the user's inputs should be counted.  */
		if (kauth_authorize_device(kauth_cred_get(),
			KAUTH_DEVICE_RND_ADDDATA_ESTIMATE,
			NULL, NULL, NULL, NULL) == 0)
			privileged = true;
		break;
	default: {
		/*
		 * XXX Hack to avoid changing module ABI so this can be
		 * pulled up.  Later, we can just remove the argument.
		 */
		static const struct fileops fops = {
			.fo_ioctl = rnd_system_ioctl,
		};
		struct file f = {
			.f_ops = &fops,
		};
		MODULE_HOOK_CALL(rnd_ioctl_50_hook, (&f, cmd, data),
		    enosys(), error);
#if defined(_LP64)
		if (error == ENOSYS)
			MODULE_HOOK_CALL(rnd_ioctl32_50_hook, (&f, cmd, data),
			    enosys(), error);
#endif
		if (error == ENOSYS)
			error = ENOTTY;
		break;
	}
	}

	/* If anything went wrong with authorization, stop here.  */
	if (error)
		return error;

	/* Dispatch on the command.  */
	switch (cmd) {
	case RNDGETENTCNT: {	/* Get current entropy count in bits.  */
		uint32_t *countp = data;

		mutex_enter(&E->lock);
		*countp = ENTROPY_CAPACITY*NBBY - E->needed;
		mutex_exit(&E->lock);

		break;
	}
	case RNDGETPOOLSTAT: {	/* Get entropy pool statistics.  */
		rndpoolstat_t *pstat = data;

		mutex_enter(&E->lock);

		/* parameters */
		pstat->poolsize = ENTPOOL_SIZE/sizeof(uint32_t); /* words */
		pstat->threshold = ENTROPY_CAPACITY*1; /* bytes */
		pstat->maxentropy = ENTROPY_CAPACITY*NBBY; /* bits */

		/* state */
		pstat->added = 0; /* XXX total entropy_enter count */
		pstat->curentropy = ENTROPY_CAPACITY*NBBY - E->needed;
		pstat->removed = 0; /* XXX total entropy_extract count */
		pstat->discarded = 0; /* XXX bits of entropy beyond capacity */
		pstat->generated = 0; /* XXX bits of data...fabricated? */

		mutex_exit(&E->lock);
		break;
	}
	case RNDGETSRCNUM: {	/* Get entropy sources by number.  */
		rndstat_t *stat = data;
		uint32_t start = 0, i = 0;

		/* Skip if none requested; fail if too many requested.  */
		if (stat->count == 0)
			break;
		if (stat->count > RND_MAXSTATCOUNT)
			return EINVAL;

		/*
		 * Under the lock, find the first one, copy out as many
		 * as requested, and report how many we copied out.
		 */
		mutex_enter(&E->lock);
		error = rnd_lock_sources(ENTROPY_WAIT|ENTROPY_SIG);
		if (error) {
			mutex_exit(&E->lock);
			return error;
		}
		LIST_FOREACH(rs, &E->sources, list) {
			if (start++ == stat->start)
				break;
		}
		while (i < stat->count && rs != NULL) {
			mutex_exit(&E->lock);
			rndsource_to_user(rs, &stat->source[i++]);
			mutex_enter(&E->lock);
			rs = LIST_NEXT(rs, list);
		}
		KASSERT(i <= stat->count);
		stat->count = i;
		rnd_unlock_sources();
		mutex_exit(&E->lock);
		break;
	}
	case RNDGETESTNUM: {	/* Get sources and estimates by number.  */
		rndstat_est_t *estat = data;
		uint32_t start = 0, i = 0;

		/* Skip if none requested; fail if too many requested.  */
		if (estat->count == 0)
			break;
		if (estat->count > RND_MAXSTATCOUNT)
			return EINVAL;

		/*
		 * Under the lock, find the first one, copy out as many
		 * as requested, and report how many we copied out.
		 */
		mutex_enter(&E->lock);
		error = rnd_lock_sources(ENTROPY_WAIT|ENTROPY_SIG);
		if (error) {
			mutex_exit(&E->lock);
			return error;
		}
		LIST_FOREACH(rs, &E->sources, list) {
			if (start++ == estat->start)
				break;
		}
		while (i < estat->count && rs != NULL) {
			mutex_exit(&E->lock);
			rndsource_to_user_est(rs, &estat->source[i++]);
			mutex_enter(&E->lock);
			rs = LIST_NEXT(rs, list);
		}
		KASSERT(i <= estat->count);
		estat->count = i;
		rnd_unlock_sources();
		mutex_exit(&E->lock);
		break;
	}
	case RNDGETSRCNAME: {	/* Get entropy sources by name.  */
		rndstat_name_t *nstat = data;
		const size_t n = sizeof(rs->name);

		CTASSERT(sizeof(rs->name) == sizeof(nstat->name));

		/*
		 * Under the lock, search by name.  If found, copy it
		 * out; if not found, fail with ENOENT.
		 */
		mutex_enter(&E->lock);
		error = rnd_lock_sources(ENTROPY_WAIT|ENTROPY_SIG);
		if (error) {
			mutex_exit(&E->lock);
			return error;
		}
		LIST_FOREACH(rs, &E->sources, list) {
			if (strncmp(rs->name, nstat->name, n) == 0)
				break;
		}
		if (rs != NULL) {
			mutex_exit(&E->lock);
			rndsource_to_user(rs, &nstat->source);
			mutex_enter(&E->lock);
		} else {
			error = ENOENT;
		}
		rnd_unlock_sources();
		mutex_exit(&E->lock);
		break;
	}
	case RNDGETESTNAME: {	/* Get sources and estimates by name.  */
		rndstat_est_name_t *enstat = data;
		const size_t n = sizeof(rs->name);

		CTASSERT(sizeof(rs->name) == sizeof(enstat->name));

		/*
		 * Under the lock, search by name.  If found, copy it
		 * out; if not found, fail with ENOENT.
		 */
		mutex_enter(&E->lock);
		error = rnd_lock_sources(ENTROPY_WAIT|ENTROPY_SIG);
		if (error) {
			mutex_exit(&E->lock);
			return error;
		}
		LIST_FOREACH(rs, &E->sources, list) {
			if (strncmp(rs->name, enstat->name, n) == 0)
				break;
		}
		if (rs != NULL) {
			mutex_exit(&E->lock);
			rndsource_to_user_est(rs, &enstat->source);
			mutex_enter(&E->lock);
		} else {
			error = ENOENT;
		}
		rnd_unlock_sources();
		mutex_exit(&E->lock);
		break;
	}
	case RNDCTL: {		/* Modify entropy source flags.  */
		rndctl_t *rndctl = data;
		const size_t n = sizeof(rs->name);
		uint32_t resetflags = RND_FLAG_NO_ESTIMATE|RND_FLAG_NO_COLLECT;
		uint32_t flags;
		bool reset = false, request = false;

		CTASSERT(sizeof(rs->name) == sizeof(rndctl->name));

		/* Whitelist the flags that user can change.  */
		rndctl->mask &= RND_FLAG_NO_ESTIMATE|RND_FLAG_NO_COLLECT;

		/*
		 * For each matching rndsource, either by type if
		 * specified or by name if not, set the masked flags.
		 */
		mutex_enter(&E->lock);
		LIST_FOREACH(rs, &E->sources, list) {
			if (rndctl->type != 0xff) {
				if (rs->type != rndctl->type)
					continue;
			} else {
				if (strncmp(rs->name, rndctl->name, n) != 0)
					continue;
			}
			flags = rs->flags & ~rndctl->mask;
			flags |= rndctl->flags & rndctl->mask;
			if ((rs->flags & resetflags) == 0 &&
			    (flags & resetflags) != 0)
				reset = true;
			if ((rs->flags ^ flags) & resetflags)
				request = true;
			atomic_store_relaxed(&rs->flags, flags);
		}
		mutex_exit(&E->lock);

		/*
		 * If we disabled estimation or collection, nix all the
		 * pending entropy and set needed to the maximum.
		 */
		if (reset) {
			xc_broadcast(0, &entropy_reset_xc, NULL, NULL);
			mutex_enter(&E->lock);
			E->pending = 0;
			atomic_store_relaxed(&E->needed,
			    ENTROPY_CAPACITY*NBBY);
			mutex_exit(&E->lock);
		}

		/*
		 * If we changed any of the estimation or collection
		 * flags, request new samples from everyone -- either
		 * to make up for what we just lost, or to get new
		 * samples from what we just added.
		 *
		 * Failing on signal, while waiting for another process
		 * to finish requesting entropy, is OK here even though
		 * we have committed side effects, because this ioctl
		 * command is idempotent, so repeating it is safe.
		 */
		if (request) {
			mutex_enter(&E->lock);
			error = entropy_request(ENTROPY_CAPACITY,
			    ENTROPY_WAIT|ENTROPY_SIG);
			mutex_exit(&E->lock);
		}
		break;
	}
	case RNDADDDATA: {	/* Enter seed into entropy pool.  */
		rnddata_t *rdata = data;
		unsigned entropybits = 0;

		if (!atomic_load_relaxed(&entropy_collection))
			break;	/* thanks but no thanks */
		if (rdata->len > MIN(sizeof(rdata->data), UINT32_MAX/NBBY))
			return EINVAL;

		/*
		 * This ioctl serves as the userland alternative a
		 * bootloader-provided seed -- typically furnished by
		 * /etc/rc.d/random_seed.  We accept the user's entropy
		 * claim only if
		 *
		 * (a) the user is privileged, and
		 * (b) we have not entered a bootloader seed.
		 *
		 * under the assumption that the user may use this to
		 * load a seed from disk that we have already loaded
		 * from the bootloader, so we don't double-count it.
		 */
		if (privileged && rdata->entropy && rdata->len) {
			mutex_enter(&E->lock);
			if (!E->seeded) {
				entropybits = MIN(rdata->entropy,
				    MIN(rdata->len, ENTROPY_CAPACITY)*NBBY);
				E->seeded = true;
			}
			mutex_exit(&E->lock);
		}

		/* Enter the data and consolidate entropy.  */
		rnd_add_data(&seed_rndsource, rdata->data, rdata->len,
		    entropybits);
		entropy_consolidate();
		break;
	}
	default:
		error = ENOTTY;
	}

	/* Return any error that may have come up.  */
	return error;
}

/* Legacy entry points */

void
rnd_seed(void *seed, size_t len)
{

	if (len != sizeof(rndsave_t)) {
		printf("entropy: invalid seed length: %zu,"
		    " expected sizeof(rndsave_t) = %zu\n",
		    len, sizeof(rndsave_t));
		return;
	}
	entropy_seed(seed);
}

void
rnd_init(void)
{

	entropy_init();
}

void
rnd_init_softint(void)
{

	entropy_init_late();
	entropy_bootrequest();
}

int
rnd_system_ioctl(struct file *fp, unsigned long cmd, void *data)
{

	return entropy_ioctl(cmd, data);
}
