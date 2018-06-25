/* $NetBSD: balloon.c,v 1.18.14.1 2018/06/25 07:25:48 pgoyette Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cherry G. Mathew <cherry@zyx.in> and
 * Jean-Yves Migeon <jym@NetBSD.org> 
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
 * The Xen balloon driver enables growing and shrinking PV domains
 * memory on the fly, by allocating and freeing memory pages directly.
 * This management needs domain cooperation to work properly, especially
 * during balloon_inflate() operation where a domain gives back memory to
 * the hypervisor.
 *
 * Shrinking memory on a live system is a difficult task, and may render
 * it unstable or lead to crash. The driver takes a conservative approach
 * there by doing memory operations in smal steps of a few MiB each time. It
 * will also refuse to decrease reservation below a certain threshold
 * (XEN_RESERVATION_MIN), so as to avoid a complete kernel memory exhaustion.
 *
 * The user can intervene at two different levels to manage the ballooning
 * of a domain:
 * - directly within the domain using a sysctl(9) interface.
 * - through the Xentools, by modifying the memory/target entry associated
 *   to a domain. This is usually done in dom0.
 *
 * Modification of the reservation is signaled by writing inside the 
 * memory/target node in Xenstore. Writing new values will fire the xenbus
 * watcher, and wakeup the balloon thread to inflate or deflate balloon.
 *
 * Both sysctl(9) nodes and memory/target entry assume that the values passed
 * to them are in KiB. Internally, the driver will convert this value in
 * pages (assuming a page is PAGE_SIZE bytes), and issue the correct hypercalls
 * to decrease/increase domain's reservation accordingly.
 *
 * XXX Pages used by balloon are tracked through entries stored in a SLIST.
 * This allows driver to conveniently add/remove wired pages from memory
 * without the need to support these "memory gaps" inside uvm(9). Still, the
 * driver does not currently "plug" new pages into uvm(9) when more memory
 * is available than originally managed by balloon. For example, deflating
 * balloon with a total number of pages above physmem is not supported for
 * now. See balloon_deflate() for more details.
 *
 */

#define BALLOONDEBUG 0

#if defined(_KERNEL_OPT)
#include "opt_uvm_hotplug.h"
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: balloon.c,v 1.18.14.1 2018/06/25 07:25:48 pgoyette Exp $");

#include <sys/inttypes.h>
#include <sys/device.h>
#include <sys/param.h>

#include <sys/atomic.h>
#include <sys/condvar.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/queue.h>
#include <sys/sysctl.h>

#include <xen/xen.h>
#include <xen/xenbus.h>
#include <xen/balloon.h>

#include <uvm/uvm.h>
#include <uvm/uvm.h>
#include <uvm/uvm_physseg.h>
#include <xen/xenpmap.h>

#include "locators.h"

/*
 * Number of MFNs stored in the array passed back and forth between domain
 * and balloon/hypervisor, during balloon_inflate() / balloon_deflate(). These
 * should fit in a page, for performance reasons.
 */
#define BALLOON_DELTA (PAGE_SIZE / sizeof(xen_pfn_t))

/*
 * Safeguard value. Refuse to go below this threshold, so that domain
 * can keep some free pages for its own use. Value is arbitrary, and may
 * evolve with time.
 */
#define BALLOON_BALLAST 256 /* In pages - 1MiB */
#define XEN_RESERVATION_MIN (uvmexp.freemin + BALLOON_BALLAST) /* In pages */

/* KB <-> PAGEs */
#define PAGE_SIZE_KB (PAGE_SIZE >> 10) /* page size in KB */
#define BALLOON_PAGES_TO_KB(_pg) ((uint64_t)_pg * PAGE_SIZE_KB)
#define BALLOON_KB_TO_PAGES(_kb) (roundup(_kb, PAGE_SIZE_KB) / PAGE_SIZE_KB)

/*
 * A balloon page entry. Needed to track pages put/reclaimed from balloon
 */
struct balloon_page_entry {
	struct vm_page *pg;
	SLIST_ENTRY(balloon_page_entry) entry;
};

struct balloon_xenbus_softc {
	device_t sc_dev;
	struct sysctllog *sc_log;

	kmutex_t balloon_mtx;   /* Protects condvar, target and res_min (below) */
	kcondvar_t balloon_cv;  /* Condvar variable for target (below) */
	size_t balloon_target;  /* Target domain reservation size in pages. */
	/* Minimum amount of memory reserved by domain, in KiB */
	uint64_t balloon_res_min;

	xen_pfn_t *sc_mfn_list; /* List of MFNs passed from/to balloon */
	pool_cache_t bpge_pool; /* pool cache for balloon page entries */
	/* linked list for tracking pages used by balloon */
	SLIST_HEAD(, balloon_page_entry) balloon_page_entries;
	size_t balloon_num_page_entries;
};

static size_t xenmem_get_currentreservation(void);
static size_t xenmem_get_maxreservation(void);

static int  bpge_ctor(void *, void *, int);
static void bpge_dtor(void *, void *);

static void   balloon_thread(void *);
static size_t balloon_deflate(struct balloon_xenbus_softc*, size_t);
static size_t balloon_inflate(struct balloon_xenbus_softc*, size_t);

static void sysctl_kern_xen_balloon_setup(struct balloon_xenbus_softc *);
static void balloon_xenbus_watcher(struct xenbus_watch *, const char **,
				   unsigned int);

static int  balloon_xenbus_match(device_t, cfdata_t, void *);
static void balloon_xenbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(balloon, sizeof(struct balloon_xenbus_softc),
    balloon_xenbus_match, balloon_xenbus_attach, NULL, NULL);

static struct xenbus_watch balloon_xenbus_watch = {
	.node = __UNCONST("memory/target"),
	.xbw_callback = balloon_xenbus_watcher,
};

static struct balloon_xenbus_softc *balloon_sc;

static int
balloon_xenbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct xenbusdev_attach_args *xa = aux;

	if (strcmp(xa->xa_type, "balloon") != 0)
		return 0;

	if (match->cf_loc[XENBUSCF_ID] != XENBUSCF_ID_DEFAULT &&
	    match->cf_loc[XENBUSCF_ID] != xa->xa_id)
		return 0;

	return 1;
}

static void
balloon_xenbus_attach(device_t parent, device_t self, void *aux)
{
	xen_pfn_t *mfn_list;
	size_t currentpages;
	struct balloon_xenbus_softc *sc = balloon_sc = device_private(self);

	aprint_normal(": Xen Balloon driver\n");
	sc->sc_dev = self;

	/* Initialize target mutex and condvar */
	mutex_init(&sc->balloon_mtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->balloon_cv, "xen_balloon");

	SLIST_INIT(&sc->balloon_page_entries);
	sc->balloon_num_page_entries = 0;

	/* Get current number of pages */
	currentpages = xenmem_get_currentreservation();

	KASSERT(currentpages > 0);

	/* Update initial target value - no need to lock for initialization */
	sc->balloon_target = currentpages;

	/* Set the values used by sysctl */
	sc->balloon_res_min =
	    BALLOON_PAGES_TO_KB(XEN_RESERVATION_MIN);

	aprint_normal_dev(self, "current reservation: %"PRIu64" KiB\n",
	    BALLOON_PAGES_TO_KB(currentpages));
#if BALLOONDEBUG
	aprint_normal_dev(self, "min reservation: %"PRIu64" KiB\n",
	    sc->balloon_res_min);
	aprint_normal_dev(self, "max reservation: %"PRIu64" KiB\n",
	    BALLOON_PAGES_TO_KB(xenmem_get_maxreservation()));
#endif

	sc->bpge_pool = pool_cache_init(sizeof(struct balloon_page_entry),
	    0, 0, 0, "xen_bpge", NULL, IPL_NONE, bpge_ctor, bpge_dtor, NULL);

	sysctl_kern_xen_balloon_setup(sc);

	/* List of MFNs passed from/to balloon for inflating/deflating */
	mfn_list = kmem_alloc(BALLOON_DELTA * sizeof(*mfn_list), KM_SLEEP);
	sc->sc_mfn_list = mfn_list;

	/* Setup xenbus node watch callback */
	if (register_xenbus_watch(&balloon_xenbus_watch)) {
		aprint_error_dev(self, "unable to watch memory/target\n");
		goto error;
	}

	/* Setup kernel thread to asynchronously (in/de)-flate the balloon */
	if (kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL, balloon_thread,
	    sc, NULL, "xen_balloon")) {
		aprint_error_dev(self, "unable to create balloon thread\n");
		unregister_xenbus_watch(&balloon_xenbus_watch);
		goto error;
	}

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	return;

error:
	sysctl_teardown(&sc->sc_log);
	cv_destroy(&sc->balloon_cv);
	mutex_destroy(&sc->balloon_mtx);
	return;

}

/*
 * Returns maximum memory reservation available to current domain. In Xen
 * with DOMID_SELF, this hypercall never fails: return value should be
 * interpreted as unsigned.
 * 
 */
static size_t
xenmem_get_maxreservation(void)
{
	int s;
	unsigned int ret;

	s = splvm(); /* XXXSMP */
	ret = HYPERVISOR_memory_op(XENMEM_maximum_reservation, 
	    & (domid_t) { DOMID_SELF });

	splx(s);

	if (ret == 0) {
		/* well, a maximum reservation of 0 is really bogus */
		panic("%s failed, maximum reservation returned 0", __func__);
	}

	return ret;
}

/* Returns current reservation, in pages */
static size_t
xenmem_get_currentreservation(void)
{
	int s, ret;

	s = splvm(); /* XXXSMP */
	ret = HYPERVISOR_memory_op(XENMEM_current_reservation,
				   & (domid_t) { DOMID_SELF });
	splx(s);

	if (ret < 0) {
		panic("%s failed: %d", __func__, ret);
	}

	return ret;
}

/*
 * Get value (in KiB) of memory/target in XenStore for current domain
 * A return value of 0 can be considered as bogus or absent.
 */
static unsigned long long
balloon_xenbus_read_target(void)
{
	unsigned long long new_target;
	int err = xenbus_read_ull(NULL, "memory", "target", &new_target, 0);

	switch(err) {
	case 0:
		return new_target;
	case ENOENT:
		break;
	default:
		device_printf(balloon_sc->sc_dev,
		    "error %d, couldn't read xenbus target node\n", err);
		break;
	}

	return 0;
}

/* Set memory/target value (in KiB) in XenStore for current domain */
static void
balloon_xenbus_write_target(unsigned long long new_target)
{
	int err = xenbus_printf(NULL, "memory", "target", "%llu", new_target);

	if (err != 0) {
		device_printf(balloon_sc->sc_dev,
		    "error %d, couldn't write xenbus target node\n", err);
	}

	return;
}

static int
bpge_ctor(void *arg, void *obj, int flags)
{
	struct balloon_page_entry *bpge = obj;

	bpge->pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_ZERO);
	if (bpge->pg == NULL)
		return ENOMEM;

	return 0;

}

static void
bpge_dtor(void *arg, void *obj)
{
	struct balloon_page_entry *bpge = obj;

	uvm_pagefree(bpge->pg);
}

/*
 * Inflate balloon. Pages are moved out of domain's memory towards balloon.
 */
static size_t
balloon_inflate(struct balloon_xenbus_softc *sc, size_t tpages)
{
	int rpages, s, ret;
	paddr_t pa;
	struct balloon_page_entry *bpg_entry;
	xen_pfn_t *mfn_list = sc->sc_mfn_list;

	struct xen_memory_reservation reservation = {
		.address_bits = 0,
		.extent_order = 0,
		.domid        = DOMID_SELF
	};

	KASSERT(tpages > 0);
	KASSERT(tpages <= BALLOON_DELTA);
	
	memset(mfn_list, 0, BALLOON_DELTA * sizeof(*mfn_list));

	/* allocate pages that will be given to Hypervisor */
	for (rpages = 0; rpages < tpages; rpages++) {

		bpg_entry = pool_cache_get(sc->bpge_pool, PR_WAITOK);
		if (bpg_entry == NULL) {
			/* failed reserving a page for balloon */
			break;
		}

		pa = VM_PAGE_TO_PHYS(bpg_entry->pg);

		mfn_list[rpages] = xpmap_ptom(pa) >> PAGE_SHIFT;

		s = splvm(); /* XXXSMP */
		/* Invalidate pg */
		xpmap_ptom_unmap(pa);
		splx(s);

		SLIST_INSERT_HEAD(&balloon_sc->balloon_page_entries, 
				  bpg_entry, entry);
		balloon_sc->balloon_num_page_entries++;
	}

	/* Hand over pages to Hypervisor */
	set_xen_guest_handle(reservation.extent_start, mfn_list);
	reservation.nr_extents = rpages;

	s = splvm(); /* XXXSMP */
	ret = HYPERVISOR_memory_op(XENMEM_decrease_reservation,
				   &reservation);
	splx(s);

	if (ret != rpages) {
		/*
		 * we are in bad shape: the operation failed for certain
		 * MFNs. As the API does not allow us to know which frame
		 * numbers were erroneous, we cannot really recover safely.
		 */
		panic("%s: decrease reservation failed: was %d, "
		    "returned %d", device_xname(sc->sc_dev), rpages, ret);
	}

#if BALLOONDEBUG
	device_printf(sc->sc_dev, "inflate %zu => inflated by %d\n",
	    tpages, rpages);
#endif
	return rpages;
}

/*
 * Deflate balloon. Pages are given back to domain's memory.
 */
static size_t
balloon_deflate(struct balloon_xenbus_softc *sc, size_t tpages)
{
	int rpages, s, ret;
	paddr_t pa;
	struct balloon_page_entry *bpg_entry;
	xen_pfn_t *mfn_list = sc->sc_mfn_list;

	struct xen_memory_reservation reservation = {
		.address_bits = 0,
		.extent_order = 0,
		.domid        = DOMID_SELF
	};

	KASSERT(tpages > 0);
	KASSERT(tpages <= BALLOON_DELTA);
	
	memset(mfn_list, 0, BALLOON_DELTA * sizeof(*mfn_list));

#ifndef UVM_HOTPLUG
	/* 
	 * If the list is empty, we are deflating balloon beyond empty. This
	 * is currently unsupported as this would require to dynamically add
	 * new memory pages inside uvm(9) and instruct pmap(9) on how to
	 * handle them. For now, we clip reservation up to the point we
	 * can manage them, eg. the remaining bpg entries in the SLIST.
	 * XXX find a way to hotplug memory through uvm(9)/pmap(9).
	 */
	if (tpages > sc->balloon_num_page_entries) {
		device_printf(sc->sc_dev,
		    "memory 'hot-plug' unsupported - clipping "
		    "reservation %zu => %zu pages.\n",
		    tpages, sc->balloon_num_page_entries);
		tpages = sc->balloon_num_page_entries;
	}
#endif

	/* reclaim pages from balloon */
	set_xen_guest_handle(reservation.extent_start, mfn_list);
	reservation.nr_extents = tpages;

	s = splvm(); /* XXXSMP */
	ret = HYPERVISOR_memory_op(XENMEM_increase_reservation, &reservation);
	splx(s);

	if (ret < 0) {
		panic("%s: increase reservation failed, ret %d",
		    device_xname(sc->sc_dev), ret);
	}

	if (ret != tpages) {
		device_printf(sc->sc_dev,
		    "increase reservation incomplete: was %zu, "
		    "returned %d\n", tpages, ret);
	}

	/* plug pages back into memory through bpge entries */
	for (rpages = 0; rpages < ret; rpages++) {
#ifdef UVM_HOTPLUG
		extern paddr_t pmap_pa_end;
		if (sc->balloon_num_page_entries == 0) { /*XXX: consolidate */
			/* "hot-plug": Stick it at the end of memory */
			pa = pmap_pa_end;

			/* P2M update */
			s = splvm(); /* XXXSMP */
			pmap_pa_end += PAGE_SIZE; /* XXX: TLB flush ?*/
			xpmap_ptom_map(pa, ptoa(mfn_list[rpages]));
			xpq_queue_machphys_update(ptoa(mfn_list[rpages]), pa);
			splx(s);

			if (uvm_physseg_plug(atop(pa), 1, NULL) == false) {
				/* Undo P2M */
				s = splvm(); /* XXXSMP */
				xpmap_ptom_unmap(pa);
				xpq_queue_machphys_update(ptoa(mfn_list[rpages]), 0);
				pmap_pa_end -= PAGE_SIZE; /* XXX: TLB flush ?*/
				splx(s);
				break;
			}
			continue;
		}
#else
		if (sc->balloon_num_page_entries == 0) {
			/*
			 * XXX This is the case where extra "hot-plug"
			 * mem w.r.t boot comes in 
			 */
			device_printf(sc->sc_dev,
			    "List empty. Cannot be collapsed further!\n");
			break;
		}
#endif
		bpg_entry = SLIST_FIRST(&balloon_sc->balloon_page_entries);
		SLIST_REMOVE_HEAD(&balloon_sc->balloon_page_entries, entry);
		balloon_sc->balloon_num_page_entries--;

		/* Update P->M */
		pa = VM_PAGE_TO_PHYS(bpg_entry->pg);

		s = splvm(); /* XXXSMP */

		xpmap_ptom_map(pa, ptoa(mfn_list[rpages]));
		xpq_queue_machphys_update(ptoa(mfn_list[rpages]), pa);

		splx(s);

		pool_cache_put(sc->bpge_pool, bpg_entry);
	}

	xpq_flush_queue();

#if BALLOONDEBUG
	device_printf(sc->sc_dev, "deflate %zu => deflated by %d\n",
	    tpages, rpages);
#endif
	return rpages;
}

/*
 * The balloon thread is responsible for handling inflate/deflate balloon
 * requests for the current domain given the new "target" value.
 */
static void
balloon_thread(void *cookie)
{
	int ret;
	size_t current, diff, target;
	struct balloon_xenbus_softc *sc = cookie;

	for/*ever*/ (;;) {
		current = xenmem_get_currentreservation();

		/*
		 * We assume that balloon_xenbus_watcher() and
		 * sysctl(9) handlers checked the sanity of the
		 * new target value.
		 */
		mutex_enter(&sc->balloon_mtx);
		target = sc->balloon_target;
		if (current != target) {
			/*
			 * There is work to do. Inflate/deflate in
			 * increments of BALLOON_DELTA pages at maximum. The
			 * risk of integer wrapping is mitigated by
			 * BALLOON_DELTA, which is the upper bound.
			 */
			mutex_exit(&sc->balloon_mtx);
			diff = MIN(target - current, BALLOON_DELTA);
			if (current < target)
				ret = balloon_deflate(sc, diff);
			else
				ret = balloon_inflate(sc, diff);

			if (ret != diff) {
				/*
				 * Something went wrong during operation.
				 * Log error then feedback current value in
				 * target so that thread gets back to waiting
				 * for the next iteration
				 */
				device_printf(sc->sc_dev,
				    "WARNING: balloon could not reach target "
				    "%zu (current %zu)\n",
				    target, current);
				current = xenmem_get_currentreservation();
				mutex_enter(&sc->balloon_mtx);
				sc->balloon_target = current;
				mutex_exit(&sc->balloon_mtx);
			}
		} else {
			/* no need for change -- wait for a signal */
			cv_wait(&sc->balloon_cv, &sc->balloon_mtx);
			mutex_exit(&sc->balloon_mtx);
		}
	}
}

/*
 * Handler called when memory/target value changes inside Xenstore.
 * All sanity checks must also happen in this handler, as it is the common
 * entry point where controller domain schedules balloon operations.
 */
static void
balloon_xenbus_watcher(struct xenbus_watch *watch, const char **vec,
		       unsigned int len)
{
	size_t new_target;
	uint64_t target_kb, target_max, target_min;

	target_kb = balloon_xenbus_read_target();
	if (target_kb == 0) {
		/* bogus -- just return */
		return;
	}

	mutex_enter(&balloon_sc->balloon_mtx);
	target_min = balloon_sc->balloon_res_min;
	mutex_exit(&balloon_sc->balloon_mtx);
	if (target_kb < target_min) {
		device_printf(balloon_sc->sc_dev,
		    "new target %"PRIu64" is below min %"PRIu64"\n",
		    target_kb, target_min);
		return;
	}

	target_max = BALLOON_PAGES_TO_KB(xenmem_get_maxreservation());
	if (target_kb > target_max) {
		/*
		 * Should not happen. Hypervisor should block balloon
		 * requests above mem-max.
		 */
		device_printf(balloon_sc->sc_dev,
		    "new target %"PRIu64" is above max %"PRIu64"\n",
		    target_kb, target_max);
		return;
	}

	new_target = BALLOON_KB_TO_PAGES(target_kb);

	device_printf(balloon_sc->sc_dev,
	    "current reservation: %zu pages => target: %zu pages\n",
	    xenmem_get_currentreservation(), new_target);

	/* Only update target if its value changes */
	mutex_enter(&balloon_sc->balloon_mtx);
	if (balloon_sc->balloon_target != new_target) {
		balloon_sc->balloon_target = new_target;
		cv_signal(&balloon_sc->balloon_cv);
	}
	mutex_exit(&balloon_sc->balloon_mtx);
	
	return;
}

/* 
 * sysctl(9) stuff 
 */

/* routine to control the minimum memory reserved for the domain */
static int
sysctl_kern_xen_balloon_min(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	u_quad_t newval;
	int error;

	node = *rnode;
	node.sysctl_data = &newval;

	mutex_enter(&balloon_sc->balloon_mtx);
	newval = balloon_sc->balloon_res_min;
	mutex_exit(&balloon_sc->balloon_mtx);

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	/* Safeguard value: refuse to go below. */
	if (newval < XEN_RESERVATION_MIN) {
		device_printf(balloon_sc->sc_dev,
		    "cannot set min below minimum safe value (%d)\n",
		    XEN_RESERVATION_MIN);
		return EPERM;
	}

	mutex_enter(&balloon_sc->balloon_mtx);
	if (balloon_sc->balloon_res_min != newval)
		balloon_sc->balloon_res_min = newval;
	mutex_exit(&balloon_sc->balloon_mtx);

	return 0;
}

/* Returns the maximum memory reservation of the domain */
static int
sysctl_kern_xen_balloon_max(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	u_quad_t node_val;

	node = *rnode;

	node_val = BALLOON_PAGES_TO_KB(xenmem_get_maxreservation());
	node.sysctl_data = &node_val;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

/* Returns the current memory reservation of the domain */
static int
sysctl_kern_xen_balloon_current(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	u_quad_t node_val;

	node = *rnode;

	node_val = BALLOON_PAGES_TO_KB(xenmem_get_currentreservation());
	node.sysctl_data = &node_val;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

/*
 * Returns the target memory reservation of the domain
 * When reading, this sysctl will return the value of the balloon_target
 * variable, converted into KiB
 * When used for writing, it will update the new memory/target value
 * in XenStore, but will not update the balloon_target variable directly.
 * This will be done by the Xenbus watch handler, balloon_xenbus_watcher().
 */
static int
sysctl_kern_xen_balloon_target(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	u_quad_t newval, res_min, res_max;
	int error;

	node = *rnode;
	node.sysctl_data = &newval;

	mutex_enter(&balloon_sc->balloon_mtx);
	newval = BALLOON_PAGES_TO_KB(balloon_sc->balloon_target);
	res_min = balloon_sc->balloon_res_min;
	mutex_exit(&balloon_sc->balloon_mtx);

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (newp == NULL || error != 0) {
		return error;
	}

	/*
	 * Sanity check new size
	 * We should not balloon below the minimum reservation
	 * set by the domain, nor above the maximum reservation set
	 * by domain controller.
	 * Note: domain is not supposed to receive balloon requests when
	 * they are above maximum reservation, but better be safe than
	 * sorry.
	 */
	res_max = BALLOON_PAGES_TO_KB(xenmem_get_maxreservation());
	if (newval < res_min || newval > res_max) {
#if BALLOONDEBUG
		device_printf(balloon_sc->sc_dev,
		    "new value out of bounds: %"PRIu64"\n", newval);
		device_printf(balloon_sc->sc_dev,
		    "min %"PRIu64", max %"PRIu64"\n", res_min, res_max);
#endif
		return EPERM;
	}

	/*
	 * Write new value inside Xenstore. This will fire the memory/target
	 * watch handler, balloon_xenbus_watcher().
	 */
	balloon_xenbus_write_target(newval);

	return 0;
}

/* sysctl(9) nodes creation */
static void
sysctl_kern_xen_balloon_setup(struct balloon_xenbus_softc *sc)
{
	const struct sysctlnode *node = NULL;
	struct sysctllog **clog = &sc->sc_log;

	sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, &node, &node,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "xen",
	    SYSCTL_DESCR("Xen top level node"),
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &node, &node,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "balloon",
	    SYSCTL_DESCR("Balloon details"),
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &node, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READONLY,
	    CTLTYPE_QUAD, "current",
	    SYSCTL_DESCR("Domain's current memory reservation from "
		"hypervisor, in KiB."),
	    sysctl_kern_xen_balloon_current, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &node, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_QUAD, "target",
	    SYSCTL_DESCR("Target memory reservation for domain, in KiB."),
	    sysctl_kern_xen_balloon_target, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &node, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_QUAD, "min",
	    SYSCTL_DESCR("Minimum amount of memory the domain "
		"reserves, in KiB."),
	    sysctl_kern_xen_balloon_min, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &node, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READONLY,
	    CTLTYPE_QUAD, "max",
	    SYSCTL_DESCR("Maximum amount of memory the domain "
		"can use, in KiB."),
	    sysctl_kern_xen_balloon_max, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
}
