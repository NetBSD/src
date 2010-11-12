/* $NetBSD: balloon.c,v 1.6 2010/11/12 13:18:59 uebayasi Exp $ */

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
 * The Xen balloon driver enables growing and shrinking PV
 * domains on the fly, by allocating and freeing memory directly.
 */

#define BALLOONDEBUG 1

/*
 * sysctl TODOs:
 * xen.balloon
 * xen.balloon.current: DONE
 * xen.balloon.target: DONE
 * xen.balloon.low-balloon: In Progress
 * xen.balloon.high-balloon: In Progress
 * xen.balloon.limit: XXX
 *
 * sysctl labels = { 'current'      : 'Current allocation',
 *           'target'       : 'Requested target',
 *           'low-balloon'  : 'Low-mem balloon',
 *           'high-balloon' : 'High-mem balloon',
 *           'limit'        : 'Xen hard limit' }
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: balloon.c,v 1.6 2010/11/12 13:18:59 uebayasi Exp $");

#include <sys/inttypes.h>
#include <sys/param.h>

#include <sys/condvar.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/sysctl.h>

#include <xen/xen.h>
#include <xen/xenbus.h>
#include <xen/balloon.h>

#include <uvm/uvm.h>
#include <uvm/uvm.h>
#include <xen/xenpmap.h>

#define BALLOONINTERVALMS 100 /* milliseconds */

#define BALLOON_DELTA 1024 /* The maximum increments allowed in a
			    * single call of balloon_inflate() or
			    * balloon_deflate
			    */
#define BALLOON_RETRIES 4  /* Number of time every (in|de)flate of
			    * BALLOON_DELTA or less, occurs
			    */

/* XXX: fix limits */
#define BALLOON_BALLAST 256 /* In pages */
#define XEN_RESERVATION_MIN (uvmexp.freemin + BALLOON_BALLAST) /* In pages */
#define XEN_RESERVATION_MAX nkmempages /* In pages */

/* KB <-> PAGEs */
#define BALLOON_PAGES_TO_KB(_pg) (_pg * PAGE_SIZE / 1024)
#define BALLOON_KB_TO_PAGES(_kb) (_kb * 1024 / PAGE_SIZE)
#define BALLOON_PAGE_FLOOR(_kb) (_kb & PAGE_MASK)

/* Forward declaration */
static void xenbus_balloon_watcher(struct xenbus_watch *, const char **,
				   unsigned int);

struct balloon_page_entry {
	struct vm_page *pg;
	SLIST_ENTRY(balloon_page_entry) entry;
};

static struct balloon_conf {
	kmutex_t flaglock; /* Protects condvar (below) */
	kcondvar_t cv_memchanged; /* Notifier flag for target (below) */

	kmutex_t tgtlock; /* Spin lock, protects .target, below */
	size_t target; /* Target VM reservation size, in pages. */

	/* The following are not protected by above locks */
	SLIST_HEAD(, balloon_page_entry) balloon_page_entries;
	size_t balloon_num_page_entries;

	/* Balloon limits */
	size_t xen_res_min;
	size_t xen_res_max;
} balloon_conf;

static struct xenbus_watch xenbus_balloon_watch = {
	.node = __UNCONST("memory/target"),
	.xbw_callback = xenbus_balloon_watcher,
};

static uint64_t sysctl_current;
static uint64_t sysctl_target;

/* List of MFNs for inflating/deflating balloon */
static xen_pfn_t *mfn_lista;

/* Returns zero, on error */
static size_t
xenmem_get_maxreservation(void)
{
#if 0   /* XXX: Fix this call */
	int s, ret;

	s = splvm();
	ret = HYPERVISOR_memory_op(XENMEM_maximum_reservation, 
	    & (domid_t) { DOMID_SELF });

	splx(s);

	if (ret < 0) {
		panic("Could not obtain hypervisor max reservation for VM\n");
		return 0;
	}

	return ret;
#else
	return nkmempages;
#endif
}

/* Returns zero, on error */
static size_t
xenmem_get_currentreservation(void)
{
	int s, ret;

	s = splvm();
	ret = HYPERVISOR_memory_op(XENMEM_current_reservation,
				   & (domid_t) { DOMID_SELF });
	splx(s);

	if (ret < 0) {
		panic("Could not obtain hypervisor current "
		    "reservation for VM\n");
		return 0;
	}

	return ret;
}

/* 
 * The target value is managed in 3 variables:
 * a) Incoming xenbus copy, maintained by the hypervisor.
 * b) sysctl_target: This is an incoming target value via the
 *    sysctl(9) interface.
 * c) balloon_conf.target
 *    This is the canonical current target that the driver tries to
 *    attain.
 *
 */


static size_t
xenbus_balloon_read_target(void)
{
	unsigned long long new_target;

	if (0 != xenbus_read_ull(NULL, "memory", "target", &new_target, 0)) {
		printf("error, couldn't read xenbus target node\n");
		return 0;
	}

	/* Returned in KB */

	return new_target;
}

static void
xenbus_balloon_write_target(unsigned long long new_target)
{

	/* new_target is in KB */
	if (0 != xenbus_printf(NULL, "memory", "target", "%llu", new_target)) {
		printf("error, couldn't write xenbus target node\n");
	}

	return;
}

static size_t
balloon_get_target(void)
{
	size_t target;

	mutex_spin_enter(&balloon_conf.tgtlock);
	target = balloon_conf.target;
	mutex_spin_exit(&balloon_conf.tgtlock);

	return target;

}

static void
balloon_set_target(size_t target)
{

	mutex_spin_enter(&balloon_conf.tgtlock);
	balloon_conf.target = target;
	mutex_spin_exit(&balloon_conf.tgtlock);

	return;

}

/*
 * This is the special case where, due to the driver not reaching
 * current balloon_conf.target, a new value is internally calculated
 * and fed back to both the sysctl and the xenbus interfaces,
 * described above.
 */
static void
balloon_feedback_target(size_t newtarget)
{
	/* Notify XenStore. */
	xenbus_balloon_write_target(BALLOON_PAGES_TO_KB(newtarget));
	/* Update sysctl value XXX: Locking ? */
	sysctl_target = BALLOON_PAGES_TO_KB(newtarget);

	/* Finally update our private copy */
	balloon_set_target(newtarget);
}


/* Number of pages currently used up by balloon */
static size_t
balloon_reserve(void)
{
	return balloon_conf.balloon_num_page_entries;
}

static size_t
reserve_pages(size_t npages, xen_pfn_t *mfn_list)
{

	int s;

	struct vm_page *pg;
	struct balloon_page_entry *bpg_entry;
	size_t rpages;
	paddr_t pa;

	for (rpages = 0; rpages < npages; rpages++) {
		
		pg = uvm_pagealloc(NULL, 0, NULL,
				   UVM_PGA_ZERO);

		if (pg == NULL) {
			break;
		}

		pa = VM_PAGE_TO_PHYS(pg);
		
		mfn_list[rpages] = xpmap_ptom(pa) >> PAGE_SHIFT;

		s = splvm();

		/* Invalidate pg */
		xpmap_phys_to_machine_mapping[
			(pa - XPMAP_OFFSET) >>	PAGE_SHIFT
			] = INVALID_P2M_ENTRY;

		splx(s);

		/* Save mfn */
		/* 
		 * XXX: We don't keep a copy, but just save a pointer
		 * to the uvm pg handle. Is this ok ?
		 */

		bpg_entry = kmem_alloc(sizeof *bpg_entry, KM_SLEEP);

		if (bpg_entry == NULL) {
			uvm_pagefree(pg);
			break;
		}

		bpg_entry->pg = pg;

		SLIST_INSERT_HEAD(&balloon_conf.balloon_page_entries, 
				  bpg_entry, entry);
		balloon_conf.balloon_num_page_entries++;
	}

	return rpages;
}

static size_t
unreserve_pages(size_t ret, xen_pfn_t *mfn_list)
{

	int s;
	size_t npages;
	paddr_t pa;
	struct vm_page *pg;
	struct balloon_page_entry *bpg_entry;
		
	for (npages = 0; npages < ret; npages++) {

		if (SLIST_EMPTY(&balloon_conf.balloon_page_entries)) {
			/*
			 * XXX: This is the case where extra "hot-plug"
			 * mem w.r.t boot comes in 
			 */
			printf("Balloon is empty. can't be collapsed further!");
			break;
		}

		bpg_entry = SLIST_FIRST(&balloon_conf.balloon_page_entries);
		SLIST_REMOVE_HEAD(&balloon_conf.balloon_page_entries, entry);
		balloon_conf.balloon_num_page_entries--;

		pg = bpg_entry->pg;

		kmem_free(bpg_entry, sizeof *bpg_entry);

		s = splvm();

		/* Update P->M */
		pa = VM_PAGE_TO_PHYS(pg);
		xpmap_phys_to_machine_mapping[
		    (pa - XPMAP_OFFSET) >> PAGE_SHIFT] = mfn_list[npages];

		xpq_queue_machphys_update(
		    ((paddr_t) (mfn_list[npages])) << PAGE_SHIFT, pa);

		xpq_flush_queue();

		/* Free it to UVM */
		uvm_pagefree(pg);

		splx(s);
	}

	return npages;
}

static void
balloon_inflate(size_t tpages)
{

	int s, ret;
	size_t npages, respgcnt;

	struct xen_memory_reservation reservation = {
		.address_bits = 0,
		.extent_order = 0,
		.domid        = DOMID_SELF
	};


	npages = xenmem_get_currentreservation();
	KASSERT (npages > tpages);
	npages -= tpages;


	KASSERT(npages > 0);
	KASSERT(npages <= BALLOON_DELTA);

	memset(mfn_lista, 0, BALLOON_DELTA * sizeof *mfn_lista);

	/* 
	 * There's a risk that npages might overflow ret. 
	 * Do this is smaller steps then.
	 * See: HYPERVISOR_memory_op(...) below....
	 */

	if (npages > XEN_RESERVATION_MAX) {
		return;
	}

	respgcnt = reserve_pages(npages, mfn_lista);

	if (respgcnt == 0) {
		return;
	}
	/* Hand over pages to Hypervisor */
	xenguest_handle(reservation.extent_start) = mfn_lista;
	reservation.nr_extents = respgcnt;

	s = splvm();
	ret = HYPERVISOR_memory_op(XENMEM_decrease_reservation, &reservation);
	splx(s);

	if (ret > 0 && ret != respgcnt) {
#if BALLOONDEBUG
		printf("decrease reservation incomplete\n");
#endif
		/* Unroll loop and release page frames back to the OS. */
		KASSERT(respgcnt > ret);
		if ((respgcnt - ret) !=
		    unreserve_pages(respgcnt - ret, mfn_lista + ret)) {
			panic("Could not unreserve balloon pages in "
			    "inflate incomplete path!");
		}

		return;
	}

#if BALLOONDEBUG
	printf("inflated by %d\n", ret);
#endif
	return;
}

static void
balloon_deflate(size_t tpages)
{

	int s, ret; 
	size_t npages, pgmax, pgcur;

	struct xen_memory_reservation reservation = {
		.address_bits = 0,
		.extent_order = 0,
		.domid        = DOMID_SELF
	};


	/* 
	 * Trim npages, if it has exceeded the hard limit 
	 */
 	pgmax = xenmem_get_maxreservation();

	KASSERT(pgmax > 0);

	pgcur = xenmem_get_currentreservation();

	KASSERT(pgcur > 0);

	pgmax -= pgcur;

	KASSERT(tpages > pgcur);
	npages = tpages - pgcur;

	/* 
	 * There's a risk that npages might overflow ret. 
	 * Do this in smaller steps then.
	 * See: HYPERVISOR_memory_op(...) below....
	 */

	KASSERT(npages > 0);
	KASSERT(npages <= BALLOON_DELTA);
	
	memset(mfn_lista, 0, BALLOON_DELTA * sizeof *mfn_lista);

	if (npages > XEN_RESERVATION_MAX) {
		return;
	}

 	if (npages > pgmax) {
		return;
 	}

	/* 
	 * Check to see if we're deflating beyond empty. 
	 * This is currently unsupported. XXX: See if we can
	 * "hot-plug" these extra pages into uvm(9)
	 */
	   
	if (npages > balloon_reserve()) {
		npages = balloon_reserve();

#if BALLOONDEBUG
		printf("\"hot-plug\" memory unsupported - clipping "
		    "reservation to %zd pages.\n", pgcur + npages);
#endif
		if (!npages) { /* Nothing to do */
			return;
		}
	}

	xenguest_handle(reservation.extent_start) = mfn_lista;
	reservation.nr_extents = npages;

	s = splvm();
	ret = HYPERVISOR_memory_op(XENMEM_increase_reservation, &reservation);
	splx(s);

	if (ret <= 0) {
		printf("%s: Increase reservation failed.\n",
			__FILE__);

		return;
	}

	npages = unreserve_pages(ret, mfn_lista);

#if BALLOONDEBUG
	printf("deflated by %zu\n", npages);
#endif

	return;

}

/*
 * Synchronous call that resizes reservation
 */
static void
balloon_resize(size_t targetpages)
{

	size_t currentpages;

	/* Get current number of pages */
	currentpages = xenmem_get_currentreservation();

	KASSERT(currentpages > 0);

	if (targetpages == currentpages) {
		return;
	}

#if BALLOONDEBUG
	printf("Current pages == %zu\n", currentpages);
#endif

	/* Increase or decrease, accordingly */
	if (targetpages > currentpages) {
		balloon_deflate(targetpages);
	} else {
		balloon_inflate(targetpages);
	}

	return;
}

static void
balloon_thread(void *ignore)
{

	int i = 0, deltachunk = 0, pollticks;
	size_t current, tgtcache;
	ssize_t delta = 0; /* The balloon increment size */

	pollticks = mstohz(BALLOONINTERVALMS);

	/* 
	 * Get target. This will ensure that the wait loop (below)
	 * won't break out until the target is set properly for the
	 * first time. The value of targetinprogress is probably
	 * rubbish.
	 */

	for/*ever*/ (;;) {

		mutex_enter(&balloon_conf.flaglock);

		while (!(delta = balloon_get_target() - 
			 (current = xenmem_get_currentreservation()))) {

			if (EWOULDBLOCK == 
			    cv_timedwait(&balloon_conf.cv_memchanged,
					 &balloon_conf.flaglock, 
					 pollticks)) {
				/*
				 * Get a bit more lethargic. Rollover
				 * is ok.
				 */
				pollticks += mstohz(BALLOONINTERVALMS);

			} else { /* activity! Poll fast! */
				pollticks = mstohz(BALLOONINTERVALMS);
			}
		}

		KASSERT(delta <= INT_MAX && delta >= INT_MIN); /* int abs(int); */
		KASSERT(abs(delta) < XEN_RESERVATION_MAX);

		if (delta >= 0) {
                        deltachunk = MIN(BALLOON_DELTA, delta);
                } else {
                        deltachunk = MAX(-BALLOON_DELTA, delta);
                }

		tgtcache = current + deltachunk;

		if (deltachunk && i >= BALLOON_RETRIES) {
			tgtcache = xenmem_get_currentreservation();
			balloon_feedback_target(tgtcache);
			if (i > BALLOON_RETRIES) {
				/* Perhaps the "feedback" failed ? */
				panic("Multiple Balloon retry resets.\n");
			}

#if BALLOONDEBUG
			printf("Aborted new target at %d tries\n", i);
			printf("Fed back new target value %zu\n", tgtcache);
			printf("delta == %zd\n", delta);
			printf("deltachunk == %d\n", deltachunk);
#endif			

		} else {

#if BALLOONDEBUG
			printf("new target ==> %zu\n", tgtcache);
#endif
			balloon_resize(tgtcache);
		}

		current = xenmem_get_currentreservation();

		/* 
		 * Every deltachunk gets a fresh set of
		 * BALLOON_RETRIES
		 */
		i = (current != tgtcache) ? i + 1 : 0; 

		mutex_exit(&balloon_conf.flaglock);

	}

}

static void
xenbus_balloon_watcher(struct xenbus_watch *watch, const char **vec,
		       unsigned int len)
{
	size_t new_target; /* In KB */

	if (0 == (new_target = (size_t) xenbus_balloon_read_target())) {
		/* Don't update target value */
		return;
	}

	new_target = BALLOON_PAGE_FLOOR(new_target);

#if BALLOONDEBUG 
	if (new_target < BALLOON_KB_TO_PAGES(balloon_conf.xen_res_min) ||
	    new_target > BALLOON_KB_TO_PAGES(balloon_conf.xen_res_max)) {
		printf("Requested target is unacceptable.\n");
		return;
	}
#endif

	/* 
	 * balloon_set_target() calls
	 * xenbus_balloon_write_target(). Not sure if this is racy 
	 */
	balloon_set_target(BALLOON_KB_TO_PAGES(new_target));

#if BALLOONDEBUG
	printf("Setting target to %zu\n", new_target);
	printf("Current reservation is %zu\n", xenmem_get_currentreservation());
#endif

	/* Notify balloon thread, if we can. */
	if (mutex_tryenter(&balloon_conf.flaglock)) {
		cv_signal(&balloon_conf.cv_memchanged);
		mutex_exit(&balloon_conf.flaglock);
	}
	
	return;
}

void
balloon_xenbus_setup(void)
{

	size_t currentpages;

	/* Allocate list of MFNs for inflating/deflating balloon */
	mfn_lista = kmem_alloc(BALLOON_DELTA * sizeof *mfn_lista, KM_NOSLEEP);
	if (mfn_lista == NULL) {
		aprint_error("%s: could not allocate mfn_lista\n", __func__);
		return;
	}

	/* Setup flaglocks, condvars et. al */
	mutex_init(&balloon_conf.flaglock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&balloon_conf.tgtlock, MUTEX_DEFAULT, IPL_HIGH);
	cv_init(&balloon_conf.cv_memchanged, "balloon");

	SLIST_INIT(&balloon_conf.balloon_page_entries);
	balloon_conf.balloon_num_page_entries = 0;

	/* Deliberately not-constified for future extensibility */
	balloon_conf.xen_res_min = XEN_RESERVATION_MIN;
	balloon_conf.xen_res_max = XEN_RESERVATION_MAX;	

#if BALLOONDEBUG
	printf("uvmexp.freemin == %d\n", uvmexp.freemin);
	printf("xen_res_min == %zu\n", balloon_conf.xen_res_min);
	printf("xen_res_max == %zu\n", balloon_conf.xen_res_max);
#endif
	/* Get current number of pages */
	currentpages = xenmem_get_currentreservation();

	KASSERT(currentpages > 0);

	/* Update initial target value */
	balloon_set_target(currentpages);

	/* 
	 * Initialise the sysctl_xxx copies of target and current
	 * as above, because sysctl inits before balloon_xenbus_setup()
	 */
	sysctl_current = currentpages;
	sysctl_target = BALLOON_PAGES_TO_KB(currentpages);

	/* Setup xenbus node watch callback */
	if (register_xenbus_watch(&xenbus_balloon_watch)) {
		aprint_error("%s: unable to watch memory/target\n", __func__);
		cv_destroy(&balloon_conf.cv_memchanged);
		mutex_destroy(&balloon_conf.tgtlock);
		mutex_destroy(&balloon_conf.flaglock);
		kmem_free(mfn_lista, BALLOON_DELTA * sizeof *mfn_lista);
		mfn_lista = NULL;
		return;

	}

	/* Setup kernel thread to asynchronously (in/de)-flate the balloon */
	if (kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL, balloon_thread,
		NULL /* arg */, NULL, "balloon")) {
		aprint_error("%s: unable to create balloon thread\n", __func__);
		unregister_xenbus_watch(&xenbus_balloon_watch);
		cv_destroy(&balloon_conf.cv_memchanged);
		mutex_destroy(&balloon_conf.tgtlock);
		mutex_destroy(&balloon_conf.flaglock);
	}

	return;

}

#if DOM0OPS

/* 
 * sysctl(9) stuff 
 */

/* sysctl helper routine */
static int
sysctl_kern_xen_balloon(SYSCTLFN_ARGS)
{

	struct sysctlnode node;

	/* 
	 * Assumes SIZE_T_MAX <= ((uint64_t) -1) see createv() in
	 * SYSCTL_SETUP(...) below
	 */

	int error;
	int64_t node_val;

	KASSERT(rnode != NULL);
	node = *rnode;

	if (strcmp(node.sysctl_name, "current") == 0) {
		node_val = BALLOON_PAGES_TO_KB(xenmem_get_currentreservation());
		node.sysctl_data = &node_val;
		return sysctl_lookup(SYSCTLFN_CALL(&node));
#ifndef XEN_BALLOON /* Read only, if balloon is disabled */
	} else if (strcmp(node.sysctl_name, "target") == 0) {
		if (newp != NULL || newlen != 0) {
			return (EPERM);
		}
		node_val = BALLOON_PAGES_TO_KB(xenmem_get_currentreservation());
		node.sysctl_data = &node_val;
		error = sysctl_lookup(SYSCTLFN_CALL(&node));
		return error;
	}
#else
	} else if (strcmp(node.sysctl_name, "target") == 0) {
		node_val = * (int64_t *) rnode->sysctl_data;
		node_val = BALLOON_PAGE_FLOOR(node_val);
		node.sysctl_data = &node_val;
		error = sysctl_lookup(SYSCTLFN_CALL(&node));
		if (error != 0) {
			return error;
		}

		/* Sanity check new size */
		if (node_val < BALLOON_PAGES_TO_KB(XEN_RESERVATION_MIN) || 
   		    node_val > BALLOON_PAGES_TO_KB(XEN_RESERVATION_MAX) ) {
#if BALLOONDEBUG
			printf("node_val out of range.\n");
			printf("node_val = %"PRIu64"\n", node_val);
#endif
			return EINVAL;
		}

#if BALLOONDEBUG
		printf("node_val = %"PRIu64"\n", node_val);
#endif

		if (node_val != BALLOON_PAGES_TO_KB(balloon_get_target())) {
			* (int64_t *) rnode->sysctl_data = node_val;

#if BALLOONDEBUG
			printf("setting to %" PRIu64"\n", node_val);
#endif

			balloon_set_target(BALLOON_KB_TO_PAGES(node_val));

			/* Notify balloon thread, if we can. */
			if (mutex_tryenter(&balloon_conf.flaglock)) {
				cv_signal(&balloon_conf.cv_memchanged);
				mutex_exit(&balloon_conf.flaglock);
			}

			/* Notify XenStore. */
			xenbus_balloon_write_target(node_val);
		}

		return 0;
	}
#endif /* XEN_BALLOON */

	return EINVAL;
}

/* Setup nodes. */
SYSCTL_SETUP(sysctl_kern_xen_balloon_setup, "sysctl kern.xen.balloon setup")
{
	const struct sysctlnode *node = NULL;

	sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "kern", NULL,
	    NULL, 0, NULL, 0,
	    CTL_KERN, CTL_EOL);

	if (node == NULL) {
		printf("sysctl create failed\n");
	}

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
	    CTLFLAG_PERMANENT,
	    CTLTYPE_QUAD, "current",
	    SYSCTL_DESCR("current memory reservation from "
		"hypervisor, in pages."),
	    sysctl_kern_xen_balloon, 0, &sysctl_current, 0,
	    CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &node, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_QUAD, "target",
	    SYSCTL_DESCR("Target memory reservation to adjust "
		"balloon size to, in pages"),
	    sysctl_kern_xen_balloon, 0, &sysctl_target, 0,
	    CTL_CREATE, CTL_EOL);
}

#endif /* DOM0OPS */
