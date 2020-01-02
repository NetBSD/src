/*	$NetBSD: subr_cpu.c,v 1.4 2020/01/02 01:31:17 ad Exp $	*/

/*-
 * Copyright (c) 2007, 2008, 2009, 2010, 2012, 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*-
 * Copyright (c)2007 YAMAMOTO Takashi,
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

/*
 * CPU related routines shared with rump.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_cpu.c,v 1.4 2020/01/02 01:31:17 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sched.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

kmutex_t	cpu_lock		__cacheline_aligned;
int		ncpu			__read_mostly;
int		ncpuonline		__read_mostly;
bool		mp_online		__read_mostly;
static bool	cpu_topology_present	__read_mostly;
int64_t		cpu_counts[CPU_COUNT_MAX];

/* An array of CPUs.  There are ncpu entries. */
struct cpu_info **cpu_infos		__read_mostly;

/* Note: set on mi_cpu_attach() and idle_loop(). */
kcpuset_t *	kcpuset_attached	__read_mostly	= NULL;
kcpuset_t *	kcpuset_running		__read_mostly	= NULL;

static char cpu_model[128];

/*
 * mi_cpu_init: early initialisation of MI CPU related structures.
 *
 * Note: may not block and memory allocator is not yet available.
 */
void
mi_cpu_init(void)
{
	struct cpu_info *ci;

	mutex_init(&cpu_lock, MUTEX_DEFAULT, IPL_NONE);

	kcpuset_create(&kcpuset_attached, true);
	kcpuset_create(&kcpuset_running, true);
	kcpuset_set(kcpuset_running, 0);

	ci = curcpu();
	ci->ci_smt_primary = ci;
}

int
cpu_setmodel(const char *fmt, ...)
{
	int len;
	va_list ap;

	va_start(ap, fmt);
	len = vsnprintf(cpu_model, sizeof(cpu_model), fmt, ap);
	va_end(ap);
	return len;
}

const char *
cpu_getmodel(void)
{
	return cpu_model;
}

bool
cpu_softintr_p(void)
{

	return (curlwp->l_pflag & LP_INTR) != 0;
}

/*
 * Collect CPU topology information as each CPU is attached.  This can be
 * called early during boot, so we need to be careful what we do.
 */
void
cpu_topology_set(struct cpu_info *ci, u_int package_id, u_int core_id,
    u_int smt_id, u_int numa_id)
{
	enum cpu_rel rel;

	cpu_topology_present = true;
	ci->ci_package_id = package_id;
	ci->ci_core_id = core_id;
	ci->ci_smt_id = smt_id;
	ci->ci_numa_id = numa_id;
	for (rel = 0; rel < __arraycount(ci->ci_sibling); rel++) {
		ci->ci_sibling[rel] = ci;
		ci->ci_nsibling[rel] = 1;
	}
}

/*
 * Link a CPU into the given circular list.
 */
static void
cpu_topology_link(struct cpu_info *ci, struct cpu_info *ci2, enum cpu_rel rel)
{
	struct cpu_info *ci3;

	/* Walk to the end of the existing circular list and append. */
	for (ci3 = ci2;; ci3 = ci3->ci_sibling[rel]) {
		ci3->ci_nsibling[rel]++;
		if (ci3->ci_sibling[rel] == ci2) {
			break;
		}
	}
	ci->ci_sibling[rel] = ci2;
	ci3->ci_sibling[rel] = ci;
	ci->ci_nsibling[rel] = ci3->ci_nsibling[rel];
}

/*
 * Print out the topology lists.
 */
static void
cpu_topology_dump(void)
{
#if DEBUG 
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci, *ci2;
	const char *names[] = { "core", "package", "peer", "smt" };
	enum cpu_rel rel;
	int i;

	for (CPU_INFO_FOREACH(cii, ci)) {
		for (rel = 0; rel < __arraycount(ci->ci_sibling); rel++) {
			printf("%s has %d %s siblings:", cpu_name(ci),
			    ci->ci_nsibling[rel], names[rel]);
			ci2 = ci->ci_sibling[rel];
			i = 0;
			do {
				printf(" %s", cpu_name(ci2));
				ci2 = ci2->ci_sibling[rel];
			} while (++i < 64 && ci2 != ci->ci_sibling[rel]);
			if (i == 64) {
				printf(" GAVE UP");
			}
			printf("\n");
		}
	}
#endif	/* DEBUG */
}

/*
 * Fake up topology info if we have none, or if what we got was bogus.
 * Don't override ci_package_id, etc, if cpu_topology_present is set.
 * MD code also uses these.
 */
static void
cpu_topology_fake(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	enum cpu_rel rel;

	for (CPU_INFO_FOREACH(cii, ci)) {
		for (rel = 0; rel < __arraycount(ci->ci_sibling); rel++) {
			ci->ci_sibling[rel] = ci;
			ci->ci_nsibling[rel] = 1;
		}
		if (!cpu_topology_present) {
			ci->ci_package_id = cpu_index(ci);
		}
		ci->ci_smt_primary = ci;
		ci->ci_schedstate.spc_flags |= SPCF_SMTPRIMARY;
	}
	cpu_topology_dump();
}

/*
 * Fix up basic CPU topology info.  Right now that means attach each CPU to
 * circular lists of its siblings in the same core, and in the same package. 
 */
void
cpu_topology_init(void)
{
	CPU_INFO_ITERATOR cii, cii2;
	struct cpu_info *ci, *ci2, *ci3;
	u_int ncore, npackage, npeer, minsmt;
	bool symmetric;

	if (!cpu_topology_present) {
		cpu_topology_fake();
		return;
	}

	/* Find siblings in same core and package. */
	for (CPU_INFO_FOREACH(cii, ci)) {
		for (CPU_INFO_FOREACH(cii2, ci2)) {
			/* Avoid bad things happening. */
			if (ci2->ci_package_id == ci->ci_package_id &&
			    ci2->ci_core_id == ci->ci_core_id &&
			    ci2->ci_smt_id == ci->ci_smt_id &&
			    ci2 != ci) {
			    	printf("cpu_topology_init: info bogus, "
			    	    "faking it\n");
			    	cpu_topology_fake();
			    	return;
			}
			if (ci2 == ci ||
			    ci2->ci_package_id != ci->ci_package_id) {
				continue;
			}
			/* Find CPUs in the same core. */
			if (ci->ci_nsibling[CPUREL_CORE] == 1 &&
			    ci->ci_core_id == ci2->ci_core_id) {
			    	cpu_topology_link(ci, ci2, CPUREL_CORE);
			}
			/* Find CPUs in the same package. */
			if (ci->ci_nsibling[CPUREL_PACKAGE] == 1) {
			    	cpu_topology_link(ci, ci2, CPUREL_PACKAGE);
			}
			if (ci->ci_nsibling[CPUREL_CORE] > 1 &&
			    ci->ci_nsibling[CPUREL_PACKAGE] > 1) {
				break;
			}
		}
	}

	/* Find peers in other packages, and peer SMTs in same package. */
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci->ci_nsibling[CPUREL_PEER] <= 1) {
			for (CPU_INFO_FOREACH(cii2, ci2)) {
				if (ci != ci2 &&
				    ci->ci_package_id != ci2->ci_package_id &&
				    ci->ci_core_id == ci2->ci_core_id &&
				    ci->ci_smt_id == ci2->ci_smt_id) {
					cpu_topology_link(ci, ci2,
					    CPUREL_PEER);
					break;
				}
			}
		}
		if (ci->ci_nsibling[CPUREL_SMT] <= 1) {
			for (CPU_INFO_FOREACH(cii2, ci2)) {
				if (ci != ci2 &&
				    ci->ci_package_id == ci2->ci_package_id &&
				    ci->ci_core_id != ci2->ci_core_id &&
				    ci->ci_smt_id == ci2->ci_smt_id) {
					cpu_topology_link(ci, ci2,
					    CPUREL_SMT);
					break;
				}
			}
		}
	}

	/* Determine whether the topology is bogus/symmetric. */
	npackage = curcpu()->ci_nsibling[CPUREL_PACKAGE];
	ncore = curcpu()->ci_nsibling[CPUREL_CORE];
	npeer = curcpu()->ci_nsibling[CPUREL_PEER];
	symmetric = true;
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (npackage != ci->ci_nsibling[CPUREL_PACKAGE] ||
		    ncore != ci->ci_nsibling[CPUREL_CORE] ||
		    npeer != ci->ci_nsibling[CPUREL_PEER]) {
			symmetric = false;
		}
	}
	cpu_topology_dump();
	if (symmetric == false) {
		printf("cpu_topology_init: not symmetric, faking it\n");
		cpu_topology_fake();
		return;
	}

	/* Identify SMT primary in each core. */
	for (CPU_INFO_FOREACH(cii, ci)) {
		ci2 = ci3 = ci;
		minsmt = ci->ci_smt_id;
		do {
			if (ci2->ci_smt_id < minsmt) {
				ci3 = ci2;
				minsmt = ci2->ci_smt_id;
			}
			ci2 = ci2->ci_sibling[CPUREL_CORE];
		} while (ci2 != ci);

		/*
		 * Mark the SMT primary, and walk back over the list
		 * pointing secondaries to the primary.
		 */
		ci3->ci_schedstate.spc_flags |= SPCF_SMTPRIMARY;
		ci2 = ci;
		do {
			ci2->ci_smt_primary = ci3;
			ci2 = ci2->ci_sibling[CPUREL_CORE];
		} while (ci2 != ci);
	}
}

/*
 * Adjust one count, for a counter that's NOT updated from interrupt
 * context.  Hardly worth making an inline due to preemption stuff.
 */
void
cpu_count(enum cpu_count idx, int64_t delta)
{
	lwp_t *l = curlwp;
	KPREEMPT_DISABLE(l);
	l->l_cpu->ci_counts[idx] += delta;
	KPREEMPT_ENABLE(l);
}

/*
 * Fetch fresh sum total for all counts.  Expensive - don't call often.
 */
void
cpu_count_sync_all(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	int64_t sum[CPU_COUNT_MAX], *ptr;
	enum cpu_count i;
	int s;

	KASSERT(sizeof(ci->ci_counts) == sizeof(cpu_counts));

	if (__predict_true(mp_online)) {
		memset(sum, 0, sizeof(sum));
		/*
		 * We want this to be reasonably quick, so any value we get
		 * isn't totally out of whack, so don't let the current LWP
		 * get preempted.
		 */
		s = splvm();
		curcpu()->ci_counts[CPU_COUNT_SYNC_ALL]++;
		for (CPU_INFO_FOREACH(cii, ci)) {
			ptr = ci->ci_counts;
			for (i = 0; i < CPU_COUNT_MAX; i += 8) {
				sum[i+0] += ptr[i+0];
				sum[i+1] += ptr[i+1];
				sum[i+2] += ptr[i+2];
				sum[i+3] += ptr[i+3];
				sum[i+4] += ptr[i+4];
				sum[i+5] += ptr[i+5];
				sum[i+6] += ptr[i+6];
				sum[i+7] += ptr[i+7];
			}
			KASSERT(i == CPU_COUNT_MAX);
		}
		memcpy(cpu_counts, sum, sizeof(cpu_counts));
		splx(s);
	} else {
		memcpy(cpu_counts, curcpu()->ci_counts, sizeof(cpu_counts));
	}
}

/*
 * Fetch a fresh sum total for one single count.  Expensive - don't call often.
 */
int64_t
cpu_count_sync(enum cpu_count count)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	int64_t sum;
	int s;

	if (__predict_true(mp_online)) {
		s = splvm();
		curcpu()->ci_counts[CPU_COUNT_SYNC_ONE]++;
		sum = 0;
		for (CPU_INFO_FOREACH(cii, ci)) {
			sum += ci->ci_counts[count];
		}
		splx(s);
	} else {
		/* XXX Early boot, iterator might not be available. */
		sum = curcpu()->ci_counts[count];
	}
	return cpu_counts[count] = sum;
}
