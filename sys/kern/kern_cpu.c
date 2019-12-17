/*	$NetBSD: kern_cpu.c,v 1.85 2019/12/17 00:59:14 ad Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_cpu.c,v 1.85 2019/12/17 00:59:14 ad Exp $");

#ifdef _KERNEL_OPT
#include "opt_cpu_ucode.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/idle.h>
#include <sys/sched.h>
#include <sys/intr.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/cpuio.h>
#include <sys/proc.h>
#include <sys/percpu.h>
#include <sys/kernel.h>
#include <sys/kauth.h>
#include <sys/xcall.h>
#include <sys/pool.h>
#include <sys/kmem.h>
#include <sys/select.h>
#include <sys/namei.h>
#include <sys/callout.h>
#include <sys/pcu.h>

#include <uvm/uvm_extern.h>

#include "ioconf.h"

/*
 * If the port has stated that cpu_data is the first thing in cpu_info,
 * verify that the claim is true. This will prevent them from getting out
 * of sync.
 */
#ifdef __HAVE_CPU_DATA_FIRST
CTASSERT(offsetof(struct cpu_info, ci_data) == 0);
#else
CTASSERT(offsetof(struct cpu_info, ci_data) != 0);
#endif

#ifndef _RUMPKERNEL /* XXX temporary */
static void	cpu_xc_online(struct cpu_info *, void *);
static void	cpu_xc_offline(struct cpu_info *, void *);

dev_type_ioctl(cpuctl_ioctl);

const struct cdevsw cpuctl_cdevsw = {
	.d_open = nullopen,
	.d_close = nullclose,
	.d_read = nullread,
	.d_write = nullwrite,
	.d_ioctl = cpuctl_ioctl,
	.d_stop = nullstop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};
#endif /* ifndef _RUMPKERNEL XXX */

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

int (*compat_cpuctl_ioctl)(struct lwp *, u_long, void *) = (void *)enosys;

static char cpu_model[128];

/*
 * mi_cpu_init: early initialisation of MI CPU related structures.
 *
 * Note: may not block and memory allocator is not yet available.
 */
void
mi_cpu_init(void)
{

	mutex_init(&cpu_lock, MUTEX_DEFAULT, IPL_NONE);

	kcpuset_create(&kcpuset_attached, true);
	kcpuset_create(&kcpuset_running, true);
	kcpuset_set(kcpuset_running, 0);
}

#ifndef _RUMPKERNEL /* XXX temporary */
int
mi_cpu_attach(struct cpu_info *ci)
{
	int error;

	KASSERT(maxcpus > 0);

	ci->ci_index = ncpu;
	kcpuset_set(kcpuset_attached, cpu_index(ci));

	/*
	 * Create a convenience cpuset of just ourselves.
	 */
	kcpuset_create(&ci->ci_data.cpu_kcpuset, true);
	kcpuset_set(ci->ci_data.cpu_kcpuset, cpu_index(ci));

	TAILQ_INIT(&ci->ci_data.cpu_ld_locks);
	__cpu_simple_lock_init(&ci->ci_data.cpu_ld_lock);

	/* This is useful for eg, per-cpu evcnt */
	snprintf(ci->ci_data.cpu_name, sizeof(ci->ci_data.cpu_name), "cpu%d",
	    cpu_index(ci));

	if (__predict_false(cpu_infos == NULL)) {
		size_t ci_bufsize = (maxcpus + 1) * sizeof(struct cpu_info *);
		cpu_infos = kmem_zalloc(ci_bufsize, KM_SLEEP);
	}
	cpu_infos[cpu_index(ci)] = ci;

	sched_cpuattach(ci);

	error = create_idle_lwp(ci);
	if (error != 0) {
		/* XXX revert sched_cpuattach */
		return error;
	}

	if (ci == curcpu())
		ci->ci_onproc = curlwp;
	else
		ci->ci_onproc = ci->ci_data.cpu_idlelwp;

	percpu_init_cpu(ci);
	softint_init(ci);
	callout_init_cpu(ci);
	xc_init_cpu(ci);
	pool_cache_cpu_init(ci);
	selsysinit(ci);
	cache_cpu_init(ci);
	TAILQ_INIT(&ci->ci_data.cpu_biodone);
	ncpu++;
	ncpuonline++;

	return 0;
}

void
cpuctlattach(int dummy __unused)
{

	KASSERT(cpu_infos != NULL);
}

int
cpuctl_ioctl(dev_t dev, u_long cmd, void *data, int flag, lwp_t *l)
{
	CPU_INFO_ITERATOR cii;
	cpustate_t *cs;
	struct cpu_info *ci;
	int error, i;
	u_int id;

	error = 0;

	mutex_enter(&cpu_lock);
	switch (cmd) {
	case IOC_CPU_SETSTATE:
		cs = data;
		error = kauth_authorize_system(l->l_cred,
		    KAUTH_SYSTEM_CPU, KAUTH_REQ_SYSTEM_CPU_SETSTATE, cs, NULL,
		    NULL);
		if (error != 0)
			break;
		if (cs->cs_id >= maxcpus ||
		    (ci = cpu_lookup(cs->cs_id)) == NULL) {
			error = ESRCH;
			break;
		}
		cpu_setintr(ci, cs->cs_intr);
		error = cpu_setstate(ci, cs->cs_online);
		break;

	case IOC_CPU_GETSTATE:
		cs = data;
		id = cs->cs_id;
		memset(cs, 0, sizeof(*cs));
		cs->cs_id = id;
		if (cs->cs_id >= maxcpus ||
		    (ci = cpu_lookup(id)) == NULL) {
			error = ESRCH;
			break;
		}
		if ((ci->ci_schedstate.spc_flags & SPCF_OFFLINE) != 0)
			cs->cs_online = false;
		else
			cs->cs_online = true;
		if ((ci->ci_schedstate.spc_flags & SPCF_NOINTR) != 0)
			cs->cs_intr = false;
		else
			cs->cs_intr = true;
		cs->cs_lastmod = (int32_t)ci->ci_schedstate.spc_lastmod;
		cs->cs_lastmodhi = (int32_t)
		    (ci->ci_schedstate.spc_lastmod >> 32);
		cs->cs_intrcnt = cpu_intr_count(ci) + 1;
		cs->cs_hwid = ci->ci_cpuid;
		break;

	case IOC_CPU_MAPID:
		i = 0;
		for (CPU_INFO_FOREACH(cii, ci)) {
			if (i++ == *(int *)data)
				break;
		}
		if (ci == NULL)
			error = ESRCH;
		else
			*(int *)data = cpu_index(ci);
		break;

	case IOC_CPU_GETCOUNT:
		*(int *)data = ncpu;
		break;

#ifdef CPU_UCODE
	case IOC_CPU_UCODE_GET_VERSION:
		error = cpu_ucode_get_version((struct cpu_ucode_version *)data);
		break;

	case IOC_CPU_UCODE_APPLY:
		error = kauth_authorize_machdep(l->l_cred,
		    KAUTH_MACHDEP_CPU_UCODE_APPLY,
		    NULL, NULL, NULL, NULL);
		if (error != 0)
			break;
		error = cpu_ucode_apply((const struct cpu_ucode *)data);
		break;
#endif

	default:
		error = (*compat_cpuctl_ioctl)(l, cmd, data);
		break;
	}
	mutex_exit(&cpu_lock);

	return error;
}

struct cpu_info *
cpu_lookup(u_int idx)
{
	struct cpu_info *ci;

	/*
	 * cpu_infos is a NULL terminated array of MAXCPUS + 1 entries,
	 * so an index of MAXCPUS here is ok.  See mi_cpu_attach.
	 */
	KASSERT(idx <= maxcpus);

	if (__predict_false(cpu_infos == NULL)) {
		KASSERT(idx == 0);
		return curcpu();
	}

	ci = cpu_infos[idx];
	KASSERT(ci == NULL || cpu_index(ci) == idx);
	KASSERTMSG(idx < maxcpus || ci == NULL, "idx %d ci %p", idx, ci);

	return ci;
}

static void
cpu_xc_offline(struct cpu_info *ci, void *unused)
{
	struct schedstate_percpu *spc, *mspc = NULL;
	struct cpu_info *target_ci;
	struct lwp *l;
	CPU_INFO_ITERATOR cii;
	int s;

	/*
	 * Thread that made the cross call (separate context) holds
	 * cpu_lock on our behalf.
	 */
	spc = &ci->ci_schedstate;
	s = splsched();
	spc->spc_flags |= SPCF_OFFLINE;
	splx(s);

	/* Take the first available CPU for the migration. */
	for (CPU_INFO_FOREACH(cii, target_ci)) {
		mspc = &target_ci->ci_schedstate;
		if ((mspc->spc_flags & SPCF_OFFLINE) == 0)
			break;
	}
	KASSERT(target_ci != NULL);

	/*
	 * Migrate all non-bound threads to the other CPU.  Note that this
	 * runs from the xcall thread, thus handling of LSONPROC is not needed.
	 */
	mutex_enter(proc_lock);
	LIST_FOREACH(l, &alllwp, l_list) {
		struct cpu_info *mci;

		lwp_lock(l);
		if (l->l_cpu != ci || (l->l_pflag & (LP_BOUND | LP_INTR))) {
			lwp_unlock(l);
			continue;
		}
		/* Regular case - no affinity. */
		if (l->l_affinity == NULL) {
			lwp_migrate(l, target_ci);
			continue;
		}
		/* Affinity is set, find an online CPU in the set. */
		for (CPU_INFO_FOREACH(cii, mci)) {
			mspc = &mci->ci_schedstate;
			if ((mspc->spc_flags & SPCF_OFFLINE) == 0 &&
			    kcpuset_isset(l->l_affinity, cpu_index(mci)))
				break;
		}
		if (mci == NULL) {
			lwp_unlock(l);
			mutex_exit(proc_lock);
			goto fail;
		}
		lwp_migrate(l, mci);
	}
	mutex_exit(proc_lock);

#if PCU_UNIT_COUNT > 0
	pcu_save_all_on_cpu();
#endif

#ifdef __HAVE_MD_CPU_OFFLINE
	cpu_offline_md();
#endif
	return;
fail:
	/* Just unset the SPCF_OFFLINE flag, caller will check */
	s = splsched();
	spc->spc_flags &= ~SPCF_OFFLINE;
	splx(s);
}

static void
cpu_xc_online(struct cpu_info *ci, void *unused)
{
	struct schedstate_percpu *spc;
	int s;

	spc = &ci->ci_schedstate;
	s = splsched();
	spc->spc_flags &= ~SPCF_OFFLINE;
	splx(s);
}

int
cpu_setstate(struct cpu_info *ci, bool online)
{
	struct schedstate_percpu *spc;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci2;
	uint64_t where;
	xcfunc_t func;
	int nonline;

	spc = &ci->ci_schedstate;

	KASSERT(mutex_owned(&cpu_lock));

	if (online) {
		if ((spc->spc_flags & SPCF_OFFLINE) == 0)
			return 0;
		func = (xcfunc_t)cpu_xc_online;
	} else {
		if ((spc->spc_flags & SPCF_OFFLINE) != 0)
			return 0;
		nonline = 0;
		/*
		 * Ensure that at least one CPU within the processor set
		 * stays online.  Revisit this later.
		 */
		for (CPU_INFO_FOREACH(cii, ci2)) {
			if ((ci2->ci_schedstate.spc_flags & SPCF_OFFLINE) != 0)
				continue;
			if (ci2->ci_schedstate.spc_psid != spc->spc_psid)
				continue;
			nonline++;
		}
		if (nonline == 1)
			return EBUSY;
		func = (xcfunc_t)cpu_xc_offline;
	}

	where = xc_unicast(0, func, ci, NULL, ci);
	xc_wait(where);
	if (online) {
		KASSERT((spc->spc_flags & SPCF_OFFLINE) == 0);
		ncpuonline++;
	} else {
		if ((spc->spc_flags & SPCF_OFFLINE) == 0) {
			/* If was not set offline, then it is busy */
			return EBUSY;
		}
		ncpuonline--;
	}

	spc->spc_lastmod = time_second;
	return 0;
}
#endif	/* ifndef _RUMPKERNEL XXX */

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

#if defined(__HAVE_INTR_CONTROL) && !defined(_RUMPKERNEL) /* XXX */
static void
cpu_xc_intr(struct cpu_info *ci, void *unused)
{
	struct schedstate_percpu *spc;
	int s;

	spc = &ci->ci_schedstate;
	s = splsched();
	spc->spc_flags &= ~SPCF_NOINTR;
	splx(s);
}

static void
cpu_xc_nointr(struct cpu_info *ci, void *unused)
{
	struct schedstate_percpu *spc;
	int s;

	spc = &ci->ci_schedstate;
	s = splsched();
	spc->spc_flags |= SPCF_NOINTR;
	splx(s);
}

int
cpu_setintr(struct cpu_info *ci, bool intr)
{
	struct schedstate_percpu *spc;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci2;
	uint64_t where;
	xcfunc_t func;
	int nintr;

	spc = &ci->ci_schedstate;

	KASSERT(mutex_owned(&cpu_lock));

	if (intr) {
		if ((spc->spc_flags & SPCF_NOINTR) == 0)
			return 0;
		func = (xcfunc_t)cpu_xc_intr;
	} else {
		if ((spc->spc_flags & SPCF_NOINTR) != 0)
			return 0;
		/*
		 * Ensure that at least one CPU within the system
		 * is handing device interrupts.
		 */
		nintr = 0;
		for (CPU_INFO_FOREACH(cii, ci2)) {
			if ((ci2->ci_schedstate.spc_flags & SPCF_NOINTR) != 0)
				continue;
			if (ci2 == ci)
				continue;
			nintr++;
		}
		if (nintr == 0)
			return EBUSY;
		func = (xcfunc_t)cpu_xc_nointr;
	}

	where = xc_unicast(0, func, ci, NULL, ci);
	xc_wait(where);
	if (intr) {
		KASSERT((spc->spc_flags & SPCF_NOINTR) == 0);
	} else if ((spc->spc_flags & SPCF_NOINTR) == 0) {
		/* If was not set offline, then it is busy */
		return EBUSY;
	}

	/* Direct interrupts away from the CPU and record the change. */
	cpu_intr_redistribute();
	spc->spc_lastmod = time_second;
	return 0;
}
#else	/* __HAVE_INTR_CONTROL */
int
cpu_setintr(struct cpu_info *ci, bool intr)
{

	return EOPNOTSUPP;
}

u_int
cpu_intr_count(struct cpu_info *ci)
{

	return 0;	/* 0 == "don't know" */
}
#endif	/* __HAVE_INTR_CONTROL */

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
cpu_topology_set(struct cpu_info *ci, int package_id, int core_id, int smt_id)
{
	enum cpu_rel rel;

	cpu_topology_present = true;
	ci->ci_package_id = package_id;
	ci->ci_core_id = core_id;
	ci->ci_smt_id = smt_id;
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
 * Find peer CPUs in other packages.
 */
static void
cpu_topology_peers(void)
{
	CPU_INFO_ITERATOR cii, cii2;
	struct cpu_info *ci, *ci2;

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci->ci_nsibling[CPUREL_PEER] > 1) {
			/* Already linked. */
			continue;
		}
		for (CPU_INFO_FOREACH(cii2, ci2)) {
			if (ci != ci2 &&
			    ci->ci_package_id != ci2->ci_package_id &&
			    ci->ci_core_id == ci2->ci_core_id &&
			    ci->ci_smt_id == ci2->ci_smt_id) {
				cpu_topology_link(ci, ci2, CPUREL_PEER);
				break;
			}
		}
	}
}

/*
 * Print out the topology lists.
 */
static void
cpu_topology_print(void)
{
#ifdef DEBUG
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci, *ci2;
	const char *names[] = { "core", "package", "peer" };
	enum cpu_rel rel;
	int i;

	for (CPU_INFO_FOREACH(cii, ci)) {
		for (rel = 0; rel < __arraycount(ci->ci_sibling); rel++) {
			printf("%s has %dx %s siblings: ", cpu_name(ci),
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
	}
	cpu_topology_print();
}

/*
 * Fix up basic CPU topology info.  Right now that means attach each CPU to
 * circular lists of its siblings in the same core, and in the same package. 
 */
void
cpu_topology_init(void)
{
	CPU_INFO_ITERATOR cii, cii2;
	struct cpu_info *ci, *ci2;
	int ncore, npackage, npeer;
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

	/* Find peers in other packages. */
	cpu_topology_peers();

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
	cpu_topology_print();
	if (symmetric == false) {
		printf("cpu_topology_init: not symmetric, faking it\n");
		cpu_topology_fake();
	}
}

#ifdef CPU_UCODE
int
cpu_ucode_load(struct cpu_ucode_softc *sc, const char *fwname)
{
	firmware_handle_t fwh;
	int error;

	if (sc->sc_blob != NULL) {
		firmware_free(sc->sc_blob, sc->sc_blobsize);
		sc->sc_blob = NULL;
		sc->sc_blobsize = 0;
	}

	error = cpu_ucode_md_open(&fwh, sc->loader_version, fwname);
	if (error != 0) {
#ifdef DEBUG
		printf("ucode: firmware_open(%s) failed: %i\n", fwname, error);
#endif
		goto err0;
	}

	sc->sc_blobsize = firmware_get_size(fwh);
	if (sc->sc_blobsize == 0) {
		error = EFTYPE;
		firmware_close(fwh);
		goto err0;
	}
	sc->sc_blob = firmware_malloc(sc->sc_blobsize);
	if (sc->sc_blob == NULL) {
		error = ENOMEM;
		firmware_close(fwh);
		goto err0;
	}

	error = firmware_read(fwh, 0, sc->sc_blob, sc->sc_blobsize);
	firmware_close(fwh);
	if (error != 0)
		goto err1;

	return 0;

err1:
	firmware_free(sc->sc_blob, sc->sc_blobsize);
	sc->sc_blob = NULL;
	sc->sc_blobsize = 0;
err0:
	return error;
}
#endif

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
