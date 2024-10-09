/*	$NetBSD: kern_entropy.c,v 1.57.4.6 2024/10/09 13:25:10 martin Exp $	*/

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
 *	* Entropy depletion is available for testing (or if you're into
 *	  that sort of thing), with sysctl -w kern.entropy.depletion=1;
 *	  the logic to support it is small, to minimize chance of bugs.
 *
 *	* While cold, a single global entropy pool is available for
 *	  entering and extracting, serialized through splhigh/splx.
 *	  The per-CPU entropy pool data structures are initialized in
 *	  entropy_init and entropy_init_late (separated mainly for
 *	  hysterical raisins at this point), but are not used until the
 *	  system is warm, at which point access to the global entropy
 *	  pool is limited to thread and softint context and serialized
 *	  by E->lock.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_entropy.c,v 1.57.4.6 2024/10/09 13:25:10 martin Exp $");

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

#define	MINENTROPYBYTES	ENTROPY_CAPACITY
#define	MINENTROPYBITS	(MINENTROPYBYTES*NBBY)
#define	MINSAMPLES	(2*MINENTROPYBITS)

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
	unsigned		ec_bitspending;
	unsigned		ec_samplespending;
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
	rnd_delta_t		rc_timedelta;
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
	unsigned	bitsneeded;	/* (A) needed globally */
	unsigned	bitspending;	/* pending in per-CPU pools */
	unsigned	samplesneeded;	/* (A) needed globally */
	unsigned	samplespending;	/* pending in per-CPU pools */
	unsigned	timestamp;	/* (A) time of last consolidation */
	unsigned	epoch;		/* (A) changes when needed -> 0 */
	kcondvar_t	cv;		/* notifies state changes */
	struct selinfo	selq;		/* notifies needed -> 0 */
	struct lwp	*sourcelock;	/* lock on list of sources */
	kcondvar_t	sourcelock_cv;	/* notifies sourcelock release */
	LIST_HEAD(,krndsource) sources;	/* list of entropy sources */
	bool		consolidate;	/* kick thread to consolidate */
	bool		seed_rndsource;	/* true if seed source is attached */
	bool		seeded;		/* true if seed file already loaded */
} entropy_global __cacheline_aligned = {
	/* Fields that must be initialized when the kernel is loaded.  */
	.bitsneeded = MINENTROPYBITS,
	.samplesneeded = MINSAMPLES,
	.epoch = (unsigned)-1,	/* -1 means entropy never consolidated */
	.sources = LIST_HEAD_INITIALIZER(entropy_global.sources),
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
static void	entropy_enter(const void *, size_t, unsigned, bool);
static bool	entropy_enter_intr(const void *, size_t, unsigned, bool);
static void	entropy_softintr(void *);
static void	entropy_thread(void *);
static bool	entropy_pending(void);
static void	entropy_pending_cpu(void *, void *, struct cpu_info *);
static void	entropy_do_consolidate(void);
static void	entropy_consolidate_xc(void *, void *);
static void	entropy_notify(void);
static int	sysctl_entropy_consolidate(SYSCTLFN_ARGS);
static int	sysctl_entropy_gather(SYSCTLFN_ARGS);
static void	filt_entropy_read_detach(struct knote *);
static int	filt_entropy_read_event(struct knote *, long);
static int	entropy_request(size_t, int);
static void	rnd_add_data_internal(struct krndsource *, const void *,
		    uint32_t, uint32_t, bool);
static void	rnd_add_data_1(struct krndsource *, const void *, uint32_t,
		    uint32_t, bool, uint32_t, bool);
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

	KASSERT(!cpu_intr_p());
	KASSERT(!cpu_softintr_p());
	KASSERT(cold);

	/*
	 * First called no later than entropy_init, while we are still
	 * single-threaded, so no need for RUN_ONCE.
	 */
	if (E->seed_rndsource)
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
 *	Requires percpu(9) and sysctl(9) to be initialized.  Must run
 *	while cold.
 */
static void
entropy_init(void)
{
	uint32_t extra[2];
	struct krndsource *rs;
	unsigned i = 0;

	KASSERT(cold);

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
	    "needed",
	    SYSCTL_DESCR("Systemwide entropy deficit (bits of entropy)"),
	    NULL, 0, &E->bitsneeded, 0, CTL_CREATE, CTL_EOL);
	sysctl_createv(&entropy_sysctllog, 0, &entropy_sysctlroot, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY|CTLFLAG_PRIVATE, CTLTYPE_INT,
	    "pending",
	    SYSCTL_DESCR("Number of bits of entropy pending on CPUs"),
	    NULL, 0, &E->bitspending, 0, CTL_CREATE, CTL_EOL);
	sysctl_createv(&entropy_sysctllog, 0, &entropy_sysctlroot, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY|CTLFLAG_PRIVATE, CTLTYPE_INT,
	    "samplesneeded",
	    SYSCTL_DESCR("Systemwide entropy deficit (samples)"),
	    NULL, 0, &E->samplesneeded, 0, CTL_CREATE, CTL_EOL);
	sysctl_createv(&entropy_sysctllog, 0, &entropy_sysctlroot, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY|CTLFLAG_PRIVATE, CTLTYPE_INT,
	    "samplespending",
	    SYSCTL_DESCR("Number of samples pending on CPUs"),
	    NULL, 0, &E->samplespending, 0, CTL_CREATE, CTL_EOL);
	sysctl_createv(&entropy_sysctllog, 0, &entropy_sysctlroot, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY, CTLTYPE_INT,
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
	entropy_enter(extra, sizeof extra, /*nbits*/0, /*count*/false);
	explicit_memset(extra, 0, sizeof extra);
}

/*
 * entropy_init_late()
 *
 *	Late initialization.  Panic on failure.
 *
 *	Requires CPUs to have been detected and LWPs to have started.
 *	Must run while cold.
 */
static void
entropy_init_late(void)
{
	int error;

	KASSERT(cold);

	/*
	 * Establish the softint at the highest softint priority level.
	 * Must happen after CPU detection.
	 */
	entropy_sih = softint_establish(SOFTINT_SERIAL|SOFTINT_MPSAFE,
	    &entropy_softintr, NULL);
	if (entropy_sih == NULL)
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
	ec->ec_bitspending = 0;
	ec->ec_samplespending = 0;
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

	KASSERT(!cpu_intr_p());
	KASSERT(!cpu_softintr_p());
	KASSERT(cold);

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
	seeded = E->seeded;
	E->seeded = (seed->entropy > 0);

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
 *	complete and interrupts are running but we are still cold.
 */
void
entropy_bootrequest(void)
{
	int error;

	KASSERT(!cpu_intr_p());
	KASSERT(!cpu_softintr_p());
	KASSERT(cold);

	/*
	 * Request enough to satisfy the maximum entropy shortage.
	 * This is harmless overkill if the bootloader provided a seed.
	 */
	error = entropy_request(MINENTROPYBYTES, ENTROPY_WAIT);
	KASSERTMSG(error == 0, "error=%d", error);
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

	return atomic_load_relaxed(&E->bitsneeded) == 0;
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
	unsigned bitsdiff, samplesdiff;

	KASSERT(!cpu_intr_p());
	KASSERT(!cold);
	KASSERT(curlwp->l_pflag & LP_BOUND);

	/*
	 * If there's no entropy needed, and entropy has been
	 * consolidated in the last minute, do nothing.
	 */
	if (__predict_true(atomic_load_relaxed(&E->bitsneeded) == 0) &&
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

	if (ec->ec_bitspending == 0 && ec->ec_samplespending == 0) {
		/* Raced with consolidation xcall.  Nothing to do.  */
	} else if (E->bitsneeded != 0 && E->bitsneeded <= ec->ec_bitspending) {
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
		atomic_store_relaxed(&ec->ec_bitspending, 0);
		atomic_store_relaxed(&ec->ec_samplespending, 0);
		atomic_store_relaxed(&E->bitsneeded, 0);
		atomic_store_relaxed(&E->samplesneeded, 0);

		/* Notify waiters that we now have full entropy.  */
		entropy_notify();
		entropy_immediate_evcnt.ev_count++;
	} else {
		/* Determine how much we can add to the global pool.  */
		KASSERTMSG(E->bitspending <= MINENTROPYBITS,
		    "E->bitspending=%u", E->bitspending);
		bitsdiff = MIN(ec->ec_bitspending,
		    MINENTROPYBITS - E->bitspending);
		KASSERTMSG(E->samplespending <= MINSAMPLES,
		    "E->samplespending=%u", E->samplespending);
		samplesdiff = MIN(ec->ec_samplespending,
		    MINSAMPLES - E->samplespending);

		/*
		 * This should make a difference unless we are already
		 * saturated.
		 */
		KASSERTMSG((bitsdiff || samplesdiff ||
			E->bitspending == MINENTROPYBITS ||
			E->samplespending == MINSAMPLES),
		    "bitsdiff=%u E->bitspending=%u ec->ec_bitspending=%u"
		    "samplesdiff=%u E->samplespending=%u"
		    " ec->ec_samplespending=%u"
		    " minentropybits=%u minsamples=%u",
		    bitsdiff, E->bitspending, ec->ec_bitspending,
		    samplesdiff, E->samplespending, ec->ec_samplespending,
		    (unsigned)MINENTROPYBITS, (unsigned)MINSAMPLES);

		/* Add to the global, subtract from the local.  */
		E->bitspending += bitsdiff;
		KASSERTMSG(E->bitspending <= MINENTROPYBITS,
		    "E->bitspending=%u", E->bitspending);
		atomic_store_relaxed(&ec->ec_bitspending,
		    ec->ec_bitspending - bitsdiff);

		E->samplespending += samplesdiff;
		KASSERTMSG(E->samplespending <= MINSAMPLES,
		    "E->samplespending=%u", E->samplespending);
		atomic_store_relaxed(&ec->ec_samplespending,
		    ec->ec_samplespending - samplesdiff);

		/* One or the other must have gone up from zero.  */
		KASSERT(E->bitspending || E->samplespending);

		if (E->bitsneeded <= E->bitspending ||
		    E->samplesneeded <= E->samplespending) {
			/*
			 * Enough bits or at least samples between all
			 * the per-CPU pools.  Leave a note for the
			 * housekeeping thread to consolidate entropy
			 * next time it wakes up -- and wake it up if
			 * this is the first time, to speed things up.
			 *
			 * If we don't need any entropy, this doesn't
			 * mean much, but it is the only time we ever
			 * gather additional entropy in case the
			 * accounting has been overly optimistic.  This
			 * happens at most once a minute, so there's
			 * negligible performance cost.
			 */
			E->consolidate = true;
			if (E->epoch == (unsigned)-1)
				cv_broadcast(&E->cv);
			if (E->bitsneeded == 0)
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
	int s;

	KASSERT(cold);

	/*
	 * We're early at boot before multithreading and multi-CPU
	 * operation, and we don't have softints yet to defer
	 * processing from interrupt context, so we have to enter the
	 * samples directly into the global pool.  But interrupts may
	 * be enabled, and we enter this path from interrupt context,
	 * so block interrupts until we're done.
	 */
	s = splhigh();

	/* Enter it into the pool.  */
	entpool_enter(&E->pool, buf, len);

	/*
	 * Decide whether to notify reseed -- we will do so if either:
	 * (a) we transition from partial entropy to full entropy, or
	 * (b) we get a batch of full entropy all at once.
	 * We don't count timing samples because we assume, while cold,
	 * there's not likely to be much jitter yet.
	 */
	notify |= (E->bitsneeded && E->bitsneeded <= nbits);
	notify |= (nbits >= MINENTROPYBITS);

	/*
	 * Subtract from the needed count and notify if appropriate.
	 * We don't count samples here because entropy_timer might
	 * still be returning zero at this point if there's no CPU
	 * cycle counter.
	 */
	E->bitsneeded -= MIN(E->bitsneeded, nbits);
	if (notify) {
		entropy_notify();
		entropy_immediate_evcnt.ev_count++;
	}

	splx(s);
}

/*
 * entropy_enter(buf, len, nbits, count)
 *
 *	Enter len bytes of data from buf into the system's entropy
 *	pool, stirring as necessary when the internal buffer fills up.
 *	nbits is a lower bound on the number of bits of entropy in the
 *	process that led to this sample.
 */
static void
entropy_enter(const void *buf, size_t len, unsigned nbits, bool count)
{
	struct entropy_cpu_lock lock;
	struct entropy_cpu *ec;
	unsigned bitspending, samplespending;
	int bound;

	KASSERTMSG(!cpu_intr_p(),
	    "use entropy_enter_intr from interrupt context");
	KASSERTMSG(howmany(nbits, NBBY) <= len,
	    "impossible entropy rate: %u bits in %zu-byte string", nbits, len);

	/*
	 * If we're still cold, just use entropy_enter_early to put
	 * samples directly into the global pool.
	 */
	if (__predict_false(cold)) {
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
	 *
	 * We don't count samples while cold because entropy_timer
	 * might still be returning zero if there's no CPU cycle
	 * counter.
	 */
	ec = entropy_cpu_get(&lock);
	entpool_enter(ec->ec_pool, buf, len);
	bitspending = ec->ec_bitspending;
	bitspending += MIN(MINENTROPYBITS - bitspending, nbits);
	atomic_store_relaxed(&ec->ec_bitspending, bitspending);
	samplespending = ec->ec_samplespending;
	if (__predict_true(count)) {
		samplespending += MIN(MINSAMPLES - samplespending, 1);
		atomic_store_relaxed(&ec->ec_samplespending, samplespending);
	}
	entropy_cpu_put(&lock, ec);

	/* Consolidate globally if appropriate based on what we added.  */
	if (bitspending > 0 || samplespending >= MINSAMPLES)
		entropy_account_cpu(ec);

	curlwp_bindx(bound);
}

/*
 * entropy_enter_intr(buf, len, nbits, count)
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
 *	Using this in thread or softint context with no spin locks held
 *	will work, but you might as well use entropy_enter in that
 *	case.
 */
static bool
entropy_enter_intr(const void *buf, size_t len, unsigned nbits, bool count)
{
	struct entropy_cpu *ec;
	bool fullyused = false;
	uint32_t bitspending, samplespending;
	int s;

	KASSERTMSG(howmany(nbits, NBBY) <= len,
	    "impossible entropy rate: %u bits in %zu-byte string", nbits, len);

	/*
	 * If we're still cold, just use entropy_enter_early to put
	 * samples directly into the global pool.
	 */
	if (__predict_false(cold)) {
		entropy_enter_early(buf, len, nbits);
		return true;
	}

	/*
	 * In case we were called in thread or interrupt context with
	 * interrupts unblocked, block soft interrupts up to
	 * IPL_SOFTSERIAL.  This way logic that is safe in interrupt
	 * context or under a spin lock is also safe in less
	 * restrictive contexts.
	 */
	s = splsoftserial();

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
		if (__predict_true(!cold))
			softint_schedule(entropy_sih);
		ec->ec_evcnt->intrtrunc.ev_count++;
		goto out1;
	}
	fullyused = true;

	/*
	 * Count up what we can contribute.
	 *
	 * We don't count samples while cold because entropy_timer
	 * might still be returning zero if there's no CPU cycle
	 * counter.
	 */
	bitspending = ec->ec_bitspending;
	bitspending += MIN(MINENTROPYBITS - bitspending, nbits);
	atomic_store_relaxed(&ec->ec_bitspending, bitspending);
	if (__predict_true(count)) {
		samplespending = ec->ec_samplespending;
		samplespending += MIN(MINSAMPLES - samplespending, 1);
		atomic_store_relaxed(&ec->ec_samplespending, samplespending);
	}

	/* Schedule a softint if we added anything and it matters.  */
	if (__predict_false(atomic_load_relaxed(&E->bitsneeded) ||
		atomic_load_relaxed(&entropy_depletion)) &&
	    (nbits != 0 || count) &&
	    __predict_true(!cold))
		softint_schedule(entropy_sih);

out1:	/* Release the per-CPU state.  */
	KASSERT(ec->ec_locked);
	__insn_barrier();
	ec->ec_locked = false;
out0:	percpu_putref(entropy_percpu);
	splx(s);

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
	unsigned bitspending, samplespending;

	/*
	 * With the per-CPU state locked, stir the pool if necessary
	 * and determine if there's any pending entropy on this CPU to
	 * account globally.
	 */
	ec = entropy_cpu_get(&lock);
	ec->ec_evcnt->softint.ev_count++;
	entpool_stir(ec->ec_pool);
	bitspending = ec->ec_bitspending;
	samplespending = ec->ec_samplespending;
	entropy_cpu_put(&lock, ec);

	/* Consolidate globally if appropriate based on what we added.  */
	if (bitspending > 0 || samplespending >= MINSAMPLES)
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

#ifndef _RUMPKERNEL		/* XXX rump starts threads before cold */
	KASSERT(!cold);
#endif

	for (;;) {
		/*
		 * Wait until there's full entropy somewhere among the
		 * CPUs, as confirmed at most once per minute, or
		 * someone wants to consolidate.
		 */
		if (entropy_pending()) {
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

struct entropy_pending_count {
	uint32_t bitspending;
	uint32_t samplespending;
};

/*
 * entropy_pending()
 *
 *	True if enough bits or samples are pending on other CPUs to
 *	warrant consolidation.
 */
static bool
entropy_pending(void)
{
	struct entropy_pending_count count = { 0, 0 }, *C = &count;

	percpu_foreach(entropy_percpu, &entropy_pending_cpu, C);
	return C->bitspending >= MINENTROPYBITS ||
	    C->samplespending >= MINSAMPLES;
}

static void
entropy_pending_cpu(void *ptr, void *cookie, struct cpu_info *ci)
{
	struct entropy_cpu *ec = ptr;
	struct entropy_pending_count *C = cookie;
	uint32_t cpu_bitspending;
	uint32_t cpu_samplespending;

	cpu_bitspending = atomic_load_relaxed(&ec->ec_bitspending);
	cpu_samplespending = atomic_load_relaxed(&ec->ec_samplespending);
	C->bitspending += MIN(MINENTROPYBITS - C->bitspending,
	    cpu_bitspending);
	C->samplespending += MIN(MINSAMPLES - C->samplespending,
	    cpu_samplespending);
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
	unsigned bitsdiff, samplesdiff;
	uint64_t ticket;

	KASSERT(!cold);
	ASSERT_SLEEPABLE();

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
	bitsdiff = MIN(E->bitsneeded, E->bitspending);
	atomic_store_relaxed(&E->bitsneeded, E->bitsneeded - bitsdiff);
	E->bitspending -= bitsdiff;
	if (__predict_false(E->bitsneeded > 0) && bitsdiff != 0) {
		if ((boothowto & AB_DEBUG) != 0 &&
		    ratecheck(&lasttime, &interval)) {
			printf("WARNING:"
			    " consolidating less than full entropy\n");
		}
	}

	samplesdiff = MIN(E->samplesneeded, E->samplespending);
	atomic_store_relaxed(&E->samplesneeded,
	    E->samplesneeded - samplesdiff);
	E->samplespending -= samplesdiff;

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
	atomic_store_relaxed(&ec->ec_bitspending, 0);
	atomic_store_relaxed(&ec->ec_samplespending, 0);
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
 *	Caller must hold the global entropy lock.
 */
static void
entropy_notify(void)
{
	static const struct timeval interval = {.tv_sec = 60, .tv_usec = 0};
	static struct timeval lasttime; /* serialized by E->lock */
	static bool ready = false, besteffort = false;
	unsigned epoch;

	KASSERT(__predict_false(cold) || mutex_owned(&E->lock));

	/*
	 * If this is the first time, print a message to the console
	 * that we're ready so operators can compare it to the timing
	 * of other events.
	 *
	 * If we didn't get full entropy from reliable sources, report
	 * instead that we are running on fumes with best effort.  (If
	 * we ever do get full entropy after that, print the ready
	 * message once.)
	 */
	if (__predict_false(!ready)) {
		if (E->bitsneeded == 0) {
			printf("entropy: ready\n");
			ready = true;
		} else if (E->samplesneeded == 0 && !besteffort) {
			printf("entropy: best effort\n");
			besteffort = true;
		}
	}

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
	if (__predict_true(!cold)) {
		cv_broadcast(&E->cv);
		selnotify(&E->selq, POLLIN|POLLRDNORM, NOTE_SUBMIT);
	}

	/* Count another notification.  */
	entropy_notify_evcnt.ev_count++;
}

/*
 * entropy_consolidate()
 *
 *	Trigger entropy consolidation and wait for it to complete, or
 *	return early if interrupted by a signal.
 */
void
entropy_consolidate(void)
{

	(void)entropy_consolidate_sig();
}

/*
 * entropy_consolidate_sig()
 *
 *	Trigger entropy consolidation and wait for it to complete, or
 *	return EINTR if interrupted by a signal.
 *
 *	This should be used sparingly, not periodically -- requiring
 *	conscious intervention by the operator or a clear policy
 *	decision.  Otherwise, the kernel will automatically consolidate
 *	when enough entropy has been gathered into per-CPU pools to
 *	transition to full entropy.
 */
int
entropy_consolidate_sig(void)
{
	uint64_t ticket;
	int error;

	KASSERT(!cold);
	ASSERT_SLEEPABLE();

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

	return error;
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
	int arg = 0;
	int error;

	node.sysctl_data = &arg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;
	if (arg)
		error = entropy_consolidate_sig();

	return error;
}

/*
 * entropy_gather()
 *
 *	Trigger gathering entropy from all on-demand sources, and, if
 *	requested, wait for synchronous sources (but not asynchronous
 *	sources) to complete, or fail with EINTR if interrupted by a
 *	signal.
 */
int
entropy_gather(void)
{
	int error;

	mutex_enter(&E->lock);
	error = entropy_request(ENTROPY_CAPACITY, ENTROPY_WAIT|ENTROPY_SIG);
	mutex_exit(&E->lock);

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
	int arg = 0;
	int error;

	node.sysctl_data = &arg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;
	if (arg)
		error = entropy_gather();

	return error;
}

/*
 * entropy_extract(buf, len, flags)
 *
 *	Extract len bytes from the global entropy pool into buf.
 *
 *	Caller MUST NOT expose these bytes directly -- must use them
 *	ONLY to seed a cryptographic pseudorandom number generator
 *	(`CPRNG'), a.k.a. deterministic random bit generator (`DRBG'),
 *	and then erase them.  entropy_extract does not, on its own,
 *	provide backtracking resistance -- it must be combined with a
 *	PRNG/DRBG that does.
 *
 *	This may be used very early at boot, before even entropy_init
 *	has been called.
 *
 *	You generally shouldn't use this directly -- use cprng(9)
 *	instead.
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
 *	ENTROPY_WAIT is not set, allowed also in softint context -- may
 *	sleep on an adaptive lock up to IPL_SOFTSERIAL.  Forbidden in
 *	hard interrupt context.
 */
int
entropy_extract(void *buf, size_t len, int flags)
{
	static const struct timeval interval = {.tv_sec = 60, .tv_usec = 0};
	static struct timeval lasttime; /* serialized by E->lock */
	bool printed = false;
	int s = -1/*XXXGCC*/, error;

	if (ISSET(flags, ENTROPY_WAIT)) {
		ASSERT_SLEEPABLE();
		KASSERT(!cold);
	}

	/* Refuse to operate in interrupt context.  */
	KASSERT(!cpu_intr_p());

	/*
	 * If we're cold, we are only contending with interrupts on the
	 * current CPU, so block them.  Otherwise, we are _not_
	 * contending with interrupts on the current CPU, but we are
	 * contending with other threads, to exclude them with a mutex.
	 */
	if (__predict_false(cold))
		s = splhigh();
	else
		mutex_enter(&E->lock);

	/* Wait until there is enough entropy in the system.  */
	error = 0;
	if (E->bitsneeded > 0 && E->samplesneeded == 0) {
		/*
		 * We don't have full entropy from reliable sources,
		 * but we gathered a plausible number of samples from
		 * other sources such as timers.  Try asking for more
		 * from any sources we can, but don't worry if it
		 * fails -- best effort.
		 */
		(void)entropy_request(ENTROPY_CAPACITY, flags);
	} else while (E->bitsneeded > 0 && E->samplesneeded > 0) {
		/* Ask for more, synchronously if possible.  */
		error = entropy_request(len, flags);
		if (error)
			break;

		/* If we got enough, we're done.  */
		if (E->bitsneeded == 0 || E->samplesneeded == 0) {
			KASSERT(error == 0);
			break;
		}

		/* If not waiting, stop here.  */
		if (!ISSET(flags, ENTROPY_WAIT)) {
			error = EWOULDBLOCK;
			break;
		}

		/* Wait for some entropy to come in and try again.  */
		KASSERT(!cold);
		if (!printed) {
			printf("entropy: pid %d (%s) waiting for entropy(7)\n",
			    curproc->p_pid, curproc->p_comm);
			printed = true;
		}

		if (ISSET(flags, ENTROPY_SIG)) {
			error = cv_timedwait_sig(&E->cv, &E->lock, hz);
			if (error && error != EWOULDBLOCK)
				break;
		} else {
			cv_timedwait(&E->cv, &E->lock, hz);
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
	 * Report a warning if we haven't yet reached full entropy.
	 * This is the only case where we consider entropy to be
	 * `depleted' without kern.entropy.depletion enabled -- when we
	 * only have partial entropy, an adversary may be able to
	 * narrow the state of the pool down to a small number of
	 * possibilities; the output then enables them to confirm a
	 * guess, reducing its entropy from the adversary's perspective
	 * to zero.
	 *
	 * This should only happen if the operator has chosen to
	 * consolidate, either through sysctl kern.entropy.consolidate
	 * or by writing less than full entropy to /dev/random as root
	 * (which /dev/random promises will immediately affect
	 * subsequent output, for better or worse).
	 */
	if (E->bitsneeded > 0 && E->samplesneeded > 0) {
		if (__predict_false(E->epoch == (unsigned)-1) &&
		    ratecheck(&lasttime, &interval)) {
			printf("WARNING:"
			    " system needs entropy for security;"
			    " see entropy(7)\n");
		}
		atomic_store_relaxed(&E->bitsneeded, MINENTROPYBITS);
		atomic_store_relaxed(&E->samplesneeded, MINSAMPLES);
	}

	/* Extract data from the pool, and `deplete' if we're doing that.  */
	entpool_extract(&E->pool, buf, len);
	if (__predict_false(atomic_load_relaxed(&entropy_depletion)) &&
	    error == 0) {
		unsigned cost = MIN(len, ENTROPY_CAPACITY)*NBBY;
		unsigned bitsneeded = E->bitsneeded;
		unsigned samplesneeded = E->samplesneeded;

		bitsneeded += MIN(MINENTROPYBITS - bitsneeded, cost);
		samplesneeded += MIN(MINSAMPLES - samplesneeded, cost);

		atomic_store_relaxed(&E->bitsneeded, bitsneeded);
		atomic_store_relaxed(&E->samplesneeded, samplesneeded);
		entropy_deplete_evcnt.ev_count++;
	}

out:	/* Release the global lock and return the error.  */
	if (__predict_false(cold))
		splx(s);
	else
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

	KASSERT(!cold);

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
	if (__predict_true(atomic_load_relaxed(&E->bitsneeded) == 0 ||
		atomic_load_relaxed(&E->samplesneeded) == 0) &&
	    __predict_true(!atomic_load_relaxed(&entropy_depletion)))
		return revents | events;

	/*
	 * Otherwise, check whether we need entropy under the lock.  If
	 * we don't, we're ready; if we do, add ourselves to the queue.
	 */
	mutex_enter(&E->lock);
	if (E->bitsneeded == 0 || E->samplesneeded == 0)
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

	KASSERT(!cold);

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

	KASSERT(!cold);

	/* Acquire the lock, if caller is outside entropy subsystem.  */
	if (hint == NOTE_SUBMIT)
		KASSERT(mutex_owned(&E->lock));
	else
		mutex_enter(&E->lock);

	/*
	 * If we still need entropy, can't read anything; if not, can
	 * read arbitrarily much.
	 */
	if (E->bitsneeded != 0 && E->samplesneeded != 0) {
		ret = 0;
	} else {
		if (atomic_load_relaxed(&entropy_depletion))
			kn->kn_data = ENTROPY_CAPACITY; /* bytes */
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

	KASSERT(!cold);

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

	KASSERTMSG(name[0] != '\0', "rndsource must have nonempty name");

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
	if (entropy_percpu != NULL)
		rs->state = percpu_alloc(sizeof(struct rndsource_cpu));
	extra[i++] = entropy_timer();

	/* Wire it into the global list of random sources.  */
	if (__predict_true(!cold))
		mutex_enter(&E->lock);
	LIST_INSERT_HEAD(&E->sources, rs, list);
	if (__predict_true(!cold))
		mutex_exit(&E->lock);
	extra[i++] = entropy_timer();

	/* Request that it provide entropy ASAP, if we can.  */
	if (ISSET(flags, RND_FLAG_HASCB))
		(*rs->get)(ENTROPY_CAPACITY, rs->getarg);
	extra[i++] = entropy_timer();

	/* Mix the extra into the pool.  */
	KASSERT(i == __arraycount(extra));
	entropy_enter(extra, sizeof extra, 0, /*count*/__predict_true(!cold));
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
	if (__predict_false(cold) && entropy_percpu == NULL) {
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
 *	May be called very early at boot, before entropy_init.
 *
 *	If flags & ENTROPY_WAIT, wait for concurrent access to finish.
 *	If flags & ENTROPY_SIG, allow interruption by signal.
 */
static int __attribute__((warn_unused_result))
rnd_lock_sources(int flags)
{
	int error;

	KASSERT(__predict_false(cold) || mutex_owned(&E->lock));
	KASSERT(!cpu_intr_p());

	while (E->sourcelock) {
		KASSERT(!cold);
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
 *
 *	May be called very early at boot, before entropy_init.
 */
static void
rnd_unlock_sources(void)
{

	KASSERT(__predict_false(cold) || mutex_owned(&E->lock));
	KASSERT(!cpu_intr_p());

	KASSERTMSG(E->sourcelock == curlwp, "lwp %p releasing lock held by %p",
	    curlwp, E->sourcelock);
	E->sourcelock = NULL;
	if (__predict_true(!cold))
		cv_signal(&E->sourcelock_cv);
}

/*
 * rnd_sources_locked()
 *
 *	True if we hold the list of rndsources locked, for diagnostic
 *	assertions.
 *
 *	May be called very early at boot, before entropy_init.
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
 *	May be called very early at boot, before entropy_init.
 *
 *	If flags & ENTROPY_WAIT, wait for concurrent access to finish.
 *	If flags & ENTROPY_SIG, allow interruption by signal.
 */
static int
entropy_request(size_t nbytes, int flags)
{
	struct krndsource *rs;
	int error;

	KASSERT(__predict_false(cold) || mutex_owned(&E->lock));
	KASSERT(!cpu_intr_p());
	if ((flags & ENTROPY_WAIT) != 0 && __predict_false(!cold))
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
		if (__predict_true(!cold))
			mutex_exit(&E->lock);
		(*rs->get)(nbytes, rs->getarg);
		if (__predict_true(!cold))
			mutex_enter(&E->lock);
	}

	/* Request done; unlock the list of entropy sources.  */
	rnd_unlock_sources();
	return 0;
}

static inline uint32_t
rnd_delta_estimate(rnd_delta_t *d, uint32_t v, int32_t delta)
{
	int32_t delta2, delta3;

	/*
	 * Calculate the second and third order differentials
	 */
	delta2 = d->dx - delta;
	if (delta2 < 0)
		delta2 = -delta2; /* XXX arithmetic overflow */

	delta3 = d->d2x - delta2;
	if (delta3 < 0)
		delta3 = -delta3; /* XXX arithmetic overflow */

	d->x = v;
	d->dx = delta;
	d->d2x = delta2;

	/*
	 * If any delta is 0, we got no entropy.  If all are non-zero, we
	 * might have something.
	 */
	if (delta == 0 || delta2 == 0 || delta3 == 0)
		return 0;

	return 1;
}

static inline uint32_t
rnd_dt_estimate(struct krndsource *rs, uint32_t t)
{
	int32_t delta;
	uint32_t ret;
	rnd_delta_t *d;
	struct rndsource_cpu *rc;

	rc = percpu_getref(rs->state);
	d = &rc->rc_timedelta;

	if (t < d->x) {
		delta = UINT32_MAX - d->x + t;
	} else {
		delta = d->x - t;
	}

	if (delta < 0) {
		delta = -delta;	/* XXX arithmetic overflow */
	}

	ret = rnd_delta_estimate(d, t, delta);

	KASSERT(d->x == t);
	KASSERT(d->dx == delta);
	percpu_putref(rs->state);
	return ret;
}

/*
 * rnd_add_uint32(rs, value)
 *
 *	Enter 32 bits of data from an entropy source into the pool.
 *
 *	May be called from any context or with spin locks held, but may
 *	drop data.
 *
 *	This is meant for cheaply taking samples from devices that
 *	aren't designed to be hardware random number generators.
 */
void
rnd_add_uint32(struct krndsource *rs, uint32_t value)
{
	bool intr_p = true;

	rnd_add_data_internal(rs, &value, sizeof value, 0, intr_p);
}

void
_rnd_add_uint32(struct krndsource *rs, uint32_t value)
{
	bool intr_p = true;

	rnd_add_data_internal(rs, &value, sizeof value, 0, intr_p);
}

void
_rnd_add_uint64(struct krndsource *rs, uint64_t value)
{
	bool intr_p = true;

	rnd_add_data_internal(rs, &value, sizeof value, 0, intr_p);
}

/*
 * rnd_add_data(rs, buf, len, entropybits)
 *
 *	Enter data from an entropy source into the pool, with a
 *	driver's estimate of how much entropy the physical source of
 *	the data has.  If RND_FLAG_NO_ESTIMATE, we ignore the driver's
 *	estimate and treat it as zero.
 *
 *	rs MAY but SHOULD NOT be NULL.  If rs is NULL, MUST NOT be
 *	called from interrupt context or with spin locks held.
 *
 *	If rs is non-NULL, MAY but SHOULD NOT be called from interrupt
 *	context, in which case act like rnd_add_data_intr -- if the
 *	sample buffer is full, schedule a softint and drop any
 *	additional data on the floor.  (This may change later once we
 *	fix drivers that still call this from interrupt context to use
 *	rnd_add_data_intr instead.)  MUST NOT be called with spin locks
 *	held if not in hard interrupt context -- i.e., MUST NOT be
 *	called in thread context or softint context with spin locks
 *	held.
 */
void
rnd_add_data(struct krndsource *rs, const void *buf, uint32_t len,
    uint32_t entropybits)
{
	bool intr_p = cpu_intr_p(); /* XXX make this unconditionally false */

	/*
	 * Weird legacy exception that we should rip out and replace by
	 * creating new rndsources to attribute entropy to the callers:
	 * If there's no rndsource, just enter the data and time now.
	 */
	if (rs == NULL) {
		uint32_t extra;

		KASSERT(!intr_p);
		KASSERTMSG(howmany(entropybits, NBBY) <= len,
		    "%s: impossible entropy rate:"
		    " %"PRIu32" bits in %"PRIu32"-byte string",
		    rs ? rs->name : "(anonymous)", entropybits, len);
		entropy_enter(buf, len, entropybits, /*count*/false);
		extra = entropy_timer();
		entropy_enter(&extra, sizeof extra, 0, /*count*/false);
		explicit_memset(&extra, 0, sizeof extra);
		return;
	}

	rnd_add_data_internal(rs, buf, len, entropybits, intr_p);
}

/*
 * rnd_add_data_intr(rs, buf, len, entropybits)
 *
 *	Try to enter data from an entropy source into the pool, with a
 *	driver's estimate of how much entropy the physical source of
 *	the data has.  If RND_FLAG_NO_ESTIMATE, we ignore the driver's
 *	estimate and treat it as zero.  If the sample buffer is full,
 *	schedule a softint and drop any additional data on the floor.
 */
void
rnd_add_data_intr(struct krndsource *rs, const void *buf, uint32_t len,
    uint32_t entropybits)
{
	bool intr_p = true;

	rnd_add_data_internal(rs, buf, len, entropybits, intr_p);
}

/*
 * rnd_add_data_internal(rs, buf, len, entropybits, intr_p)
 *
 *	Internal subroutine to decide whether or not to enter data or
 *	timing for a particular rndsource, and if so, to enter it.
 *
 *	intr_p is true for callers from interrupt context or spin locks
 *	held, and false for callers from thread or soft interrupt
 *	context and no spin locks held.
 */
static void
rnd_add_data_internal(struct krndsource *rs, const void *buf, uint32_t len,
    uint32_t entropybits, bool intr_p)
{
	uint32_t flags;

	KASSERTMSG(howmany(entropybits, NBBY) <= len,
	    "%s: impossible entropy rate:"
	    " %"PRIu32" bits in %"PRIu32"-byte string",
	    rs ? rs->name : "(anonymous)", entropybits, len);

	/*
	 * Hold up the reset xcall before it zeroes the entropy counts
	 * on this CPU or globally.  Otherwise, we might leave some
	 * nonzero entropy attributed to an untrusted source in the
	 * event of a race with a change to flags.
	 */
	kpreempt_disable();

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
		goto out;

	/* If asked, ignore the estimate.  */
	if (ISSET(flags, RND_FLAG_NO_ESTIMATE))
		entropybits = 0;

	/* If we are collecting data, enter them.  */
	if (ISSET(flags, RND_FLAG_COLLECT_VALUE)) {
		rnd_add_data_1(rs, buf, len, entropybits, /*count*/false,
		    RND_FLAG_COLLECT_VALUE, intr_p);
	}

	/* If we are collecting timings, enter one.  */
	if (ISSET(flags, RND_FLAG_COLLECT_TIME)) {
		uint32_t extra;
		bool count;

		/* Sample a timer.  */
		extra = entropy_timer();

		/* If asked, do entropy estimation on the time.  */
		if ((flags & (RND_FLAG_ESTIMATE_TIME|RND_FLAG_NO_ESTIMATE)) ==
		    RND_FLAG_ESTIMATE_TIME && __predict_true(!cold))
			count = rnd_dt_estimate(rs, extra);
		else
			count = false;

		rnd_add_data_1(rs, &extra, sizeof extra, 0, count,
		    RND_FLAG_COLLECT_TIME, intr_p);
	}

out:	/* Allow concurrent changes to flags to finish.  */
	kpreempt_enable();
}

static unsigned
add_sat(unsigned a, unsigned b)
{
	unsigned c = a + b;

	return (c < a ? UINT_MAX : c);
}

/*
 * rnd_add_data_1(rs, buf, len, entropybits, count, flag)
 *
 *	Internal subroutine to call either entropy_enter_intr, if we're
 *	in interrupt context, or entropy_enter if not, and to count the
 *	entropy in an rndsource.
 */
static void
rnd_add_data_1(struct krndsource *rs, const void *buf, uint32_t len,
    uint32_t entropybits, bool count, uint32_t flag, bool intr_p)
{
	bool fullyused;

	/*
	 * For the interrupt-like path, use entropy_enter_intr and take
	 * note of whether it consumed the full sample; otherwise, use
	 * entropy_enter, which always consumes the full sample.
	 */
	if (intr_p) {
		fullyused = entropy_enter_intr(buf, len, entropybits, count);
	} else {
		entropy_enter(buf, len, entropybits, count);
		fullyused = true;
	}

	/*
	 * If we used the full sample, note how many bits were
	 * contributed from this source.
	 */
	if (fullyused) {
		if (__predict_false(cold)) {
			const int s = splhigh();
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
			splx(s);
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

	KASSERT(!cold);
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

	KASSERT(!cold);
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

	KASSERT(!cold);
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
	ec->ec_bitspending = 0;
	ec->ec_samplespending = 0;
	entpool_enter(ec->ec_pool, &extra, sizeof extra);
	entropy_cpu_put(&lock, ec);
}

/*
 * entropy_reset()
 *
 *	Assume the entropy pool has been exposed, e.g. because the VM
 *	has been cloned.  Nix all the pending entropy and set the
 *	needed to maximum.
 */
void
entropy_reset(void)
{

	xc_broadcast(0, &entropy_reset_xc, NULL, NULL);
	mutex_enter(&E->lock);
	E->bitspending = 0;
	E->samplespending = 0;
	atomic_store_relaxed(&E->bitsneeded, MINENTROPYBITS);
	atomic_store_relaxed(&E->samplesneeded, MINSAMPLES);
	E->consolidate = false;
	mutex_exit(&E->lock);
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

	KASSERT(!cold);

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
		*countp = MINENTROPYBITS - E->bitsneeded;
		mutex_exit(&E->lock);

		break;
	}
	case RNDGETPOOLSTAT: {	/* Get entropy pool statistics.  */
		rndpoolstat_t *pstat = data;

		mutex_enter(&E->lock);

		/* parameters */
		pstat->poolsize = ENTPOOL_SIZE/sizeof(uint32_t); /* words */
		pstat->threshold = MINENTROPYBITS/NBBY; /* bytes */
		pstat->maxentropy = ENTROPY_CAPACITY*NBBY; /* bits */

		/* state */
		pstat->added = 0; /* XXX total entropy_enter count */
		pstat->curentropy = MINENTROPYBITS - E->bitsneeded; /* bits */
		pstat->removed = 0; /* XXX total entropy_extract count */
		pstat->discarded = 0; /* XXX bits of entropy beyond capacity */

		/*
		 * This used to be bits of data fabricated in some
		 * sense; we'll take it to mean number of samples,
		 * excluding the bits of entropy from HWRNG or seed.
		 */
		pstat->generated = MINSAMPLES - E->samplesneeded;
		pstat->generated -= MIN(pstat->generated, pstat->curentropy);

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
			} else if (rndctl->name[0] != '\0') {
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
		if (reset)
			entropy_reset();

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
		if (request)
			error = entropy_gather();
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
		error = entropy_consolidate_sig();
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
