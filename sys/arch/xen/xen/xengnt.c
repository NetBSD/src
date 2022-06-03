/*      $NetBSD: xengnt.c,v 1.39 2022/06/03 10:42:17 bouyer Exp $      */

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xengnt.c,v 1.39 2022/06/03 10:42:17 bouyer Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/queue.h>
#include <sys/extent.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <uvm/uvm.h>

#include <xen/hypervisor.h>
#include <xen/xen.h>
#include <xen/granttables.h>

#include "opt_xen.h"

/* #define XENDEBUG */
#ifdef XENDEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

/* External tools reserve first few grant table entries. */
#define NR_RESERVED_ENTRIES 8

/* current supported version */
int gnt_v = 0;
#define GNT_ISV1 (gnt_v == 1)
#define GNT_ISV2 (gnt_v == 2)
/* Current number of frames making up the grant table */
int gnt_nr_grant_frames;
/* Maximum number of frames that can make up the grant table */
int gnt_max_grant_frames;

/* table of free grant entries */
grant_ref_t *gnt_entries;
/* last free entry */
int last_gnt_entry;
/* empty entry in the list */
#define XENGNT_NO_ENTRY 0xffffffff

/* VM address of the grant table */
#define NR_GRANT_ENTRIES_PER_PAGE_V1 (PAGE_SIZE / sizeof(grant_entry_v1_t))
#define NR_GRANT_ENTRIES_PER_PAGE_V2 (PAGE_SIZE / sizeof(grant_entry_v2_t))
#define NR_GRANT_ENTRIES_PER_PAGE \
    ((gnt_v == 1) ? NR_GRANT_ENTRIES_PER_PAGE_V1 : NR_GRANT_ENTRIES_PER_PAGE_V2)
#define NR_GRANT_STATUS_PER_PAGE (PAGE_SIZE / sizeof(grant_status_t))

union {
	grant_entry_v1_t *gntt_v1;
	grant_entry_v2_t *gntt_v2;
	void *gntt;
} grant_table;

/* Number of grant status frames (v2 only)*/
int gnt_status_frames;

grant_status_t *grant_status;
kmutex_t grant_lock;

static grant_ref_t xengnt_get_entry(void);
static void xengnt_free_entry(grant_ref_t);
static int xengnt_more_entries(void);
static int xengnt_map_status(void);
static bool xengnt_finish_init(void);

void
xengnt_init(void)
{
	struct gnttab_query_size query;
	int rc;
	int nr_grant_entries;
	int i;

	/* first try to see which version we support */
	struct gnttab_set_version gntversion;
	gnt_v = gntversion.version = 2;
	rc = HYPERVISOR_grant_table_op(GNTTABOP_set_version, &gntversion, 1);
	if (rc < 0 || gntversion.version != 2) {
		aprint_debug("GNTTABOP_set_version 2 failed (%d), "
		    "fall back to version 1\n", rc);
		gnt_v = 1;
	}

	query.dom = DOMID_SELF;
	rc = HYPERVISOR_grant_table_op(GNTTABOP_query_size, &query, 1);
	if ((rc < 0) || (query.status != GNTST_okay))
		gnt_max_grant_frames = 4; /* Legacy max number of frames */
	else
		gnt_max_grant_frames = query.max_nr_frames;

	/*
	 * Always allocate max number of grant frames, never expand in runtime
	 */
	gnt_nr_grant_frames = gnt_max_grant_frames;

	nr_grant_entries =
	    gnt_max_grant_frames * NR_GRANT_ENTRIES_PER_PAGE;

	grant_table.gntt = (void *)uvm_km_alloc(kernel_map,
	    gnt_max_grant_frames * PAGE_SIZE, 0, UVM_KMF_VAONLY);
	if (grant_table.gntt == NULL)
		panic("xengnt_init() table no VM space");

	gnt_entries = kmem_alloc((nr_grant_entries + 1) * sizeof(grant_ref_t),
	    KM_SLEEP);
	for (i = 0; i <= nr_grant_entries; i++)
		gnt_entries[i] = XENGNT_NO_ENTRY;

	if (GNT_ISV2) {
		gnt_status_frames =
		    round_page(nr_grant_entries * sizeof(grant_status_t)) / PAGE_SIZE;
		grant_status = (void *)uvm_km_alloc(kernel_map,
		    gnt_status_frames * PAGE_SIZE, 0, UVM_KMF_VAONLY);
		if (grant_status == NULL)
			panic("xengnt_init() status no VM space");
	}

	mutex_init(&grant_lock, MUTEX_DEFAULT, IPL_VM);

	xengnt_finish_init();
}

/*
 * Resume grant table state
 */
bool
xengnt_resume(void)
{
	int rc;

	struct gnttab_set_version gntversion;
	KASSERT(gnt_v == 1 || gnt_v == 2);
	gntversion.version = gnt_v;
	rc = HYPERVISOR_grant_table_op(GNTTABOP_set_version, &gntversion, 1);

	if (GNT_ISV2) {
		if (rc < 0 || gntversion.version != 2) {
			panic("GNTTABOP_set_version 2 failed %d", rc);
		}
	} else {
		if (rc == 0 && gntversion.version != 1) {
			panic("GNTTABOP_set_version 1 failed");
		}
	}

	return xengnt_finish_init();
}

static bool
xengnt_finish_init(void)
{
	int previous_nr_grant_frames = gnt_nr_grant_frames;

	last_gnt_entry = 0;
	gnt_nr_grant_frames = 0;

	mutex_enter(&grant_lock);
	while (gnt_nr_grant_frames < previous_nr_grant_frames) {
		if (xengnt_more_entries() != 0)
			panic("xengnt_resume: can't restore grant frames");
	}
	if (GNT_ISV2)
		xengnt_map_status();
	mutex_exit(&grant_lock);
	return true;
}

/*
 * Suspend grant table state
 */
bool
xengnt_suspend(void) {

	int i;

	mutex_enter(&grant_lock);
	KASSERT(gnt_entries[last_gnt_entry] == XENGNT_NO_ENTRY);

	for (i = 0; i < last_gnt_entry; i++) {
		/* invalidate all grant entries (necessary for resume) */
		gnt_entries[i] = XENGNT_NO_ENTRY;
	}
	
	/* Remove virtual => machine mapping for grant table */
	pmap_kremove((vaddr_t)grant_table.gntt, gnt_nr_grant_frames * PAGE_SIZE);

	if (GNT_ISV2) {
		/* Remove virtual => machine mapping for status table */
		pmap_kremove((vaddr_t)grant_status, gnt_status_frames * PAGE_SIZE);
	}

	pmap_update(pmap_kernel());
	mutex_exit(&grant_lock);
	return true;
}

/*
 * Get status frames and enter them into the VA space.
 */
static int
xengnt_map_status(void)
{
	uint64_t *pages;
	size_t sz;
	KASSERT(mutex_owned(&grant_lock));
	KASSERT(GNT_ISV2);

	sz = gnt_status_frames * sizeof(*pages);
	pages = kmem_alloc(sz, KM_NOSLEEP);
	if (pages == NULL)
		return ENOMEM;

#ifdef XENPV
	gnttab_get_status_frames_t getstatus;
	int err;

	getstatus.dom = DOMID_SELF;
	getstatus.nr_frames = gnt_status_frames;
	set_xen_guest_handle(getstatus.frame_list, pages);

	/*
	 * get the status frames, and return the list of their virtual
	 * addresses in 'pages'
	 */
	if ((err = HYPERVISOR_grant_table_op(GNTTABOP_get_status_frames,
	    &getstatus, 1)) != 0)
		panic("%s: get_status_frames failed: %d", __func__, err);
	if (getstatus.status != GNTST_okay) {
		aprint_error("%s: get_status_frames returned %d\n",
		    __func__, getstatus.status);
		kmem_free(pages, sz);
		return ENOMEM;
	}
#else /* XENPV */
	for (int i = 0; i < gnt_status_frames; i++) {
		struct vm_page *pg;
		struct xen_add_to_physmap xmap;

		pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_USERESERVE|UVM_PGA_ZERO);
		pages[i] = atop(uvm_vm_page_to_phys(pg));

		xmap.domid = DOMID_SELF;
		xmap.space = XENMAPSPACE_grant_table;
		xmap.idx = i | XENMAPIDX_grant_table_status;
		xmap.gpfn = pages[i];

		if (HYPERVISOR_memory_op(XENMEM_add_to_physmap, &xmap) < 0)
			panic("%s: Unable to add grant tables\n", __func__);
	}
#endif /* XENPV */
	/*
	 * map between status_table addresses and the machine addresses of
	 * the status table frames
	 */
	for (int i = 0; i < gnt_status_frames; i++) {
		pmap_kenter_ma(((vaddr_t)grant_status) + i * PAGE_SIZE,
		    ((paddr_t)pages[i]) << PAGE_SHIFT,
		    VM_PROT_WRITE, 0);
	}
	pmap_update(pmap_kernel());

	kmem_free(pages, sz);
	return 0;
}

/*
 * Add another page to the grant table
 * Returns 0 on success, ENOMEM on failure
 */
static int
xengnt_more_entries(void)
{
	gnttab_setup_table_t setup;
	u_long *pages;
	int nframes_new = gnt_nr_grant_frames + 1;
	int i, start_gnt;
	size_t sz;
	KASSERT(mutex_owned(&grant_lock));

	if (gnt_nr_grant_frames == gnt_max_grant_frames)
		return ENOMEM;

	sz = nframes_new * sizeof(*pages);
	pages = kmem_alloc(sz, KM_NOSLEEP);
	if (pages == NULL)
		return ENOMEM;

	if (xen_feature(XENFEAT_auto_translated_physmap)) {
		/*
		 * Note: Although we allocate space for the entire
		 * table, in this mode we only update one entry at a
		 * time.
		 */
		struct vm_page *pg;
		struct xen_add_to_physmap xmap;

		pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_USERESERVE|UVM_PGA_ZERO);
		pages[gnt_nr_grant_frames] = atop(uvm_vm_page_to_phys(pg));

		xmap.domid = DOMID_SELF;
		xmap.space = XENMAPSPACE_grant_table;
		xmap.idx = gnt_nr_grant_frames;
		xmap.gpfn = pages[gnt_nr_grant_frames];

		if (HYPERVISOR_memory_op(XENMEM_add_to_physmap, &xmap) < 0)
			panic("%s: Unable to add grant frames\n", __func__);

	} else {
		setup.dom = DOMID_SELF;
		setup.nr_frames = nframes_new;
		set_xen_guest_handle(setup.frame_list, pages);

		/*
		 * setup the grant table, made of nframes_new frames
		 * and return the list of their virtual addresses
		 * in 'pages'
		 */
		if (HYPERVISOR_grant_table_op(GNTTABOP_setup_table, &setup, 1) != 0)
			panic("%s: setup table failed", __func__);
		if (setup.status != GNTST_okay) {
			aprint_error("%s: setup table returned %d\n",
			    __func__, setup.status);
			kmem_free(pages, sz);
			return ENOMEM;
		}
	}

	DPRINTF(("xengnt_more_entries: map 0x%lx -> %p\n",
	    pages[gnt_nr_grant_frames],
	    (char *)grant_table + gnt_nr_grant_frames * PAGE_SIZE));

	/*
	 * map between grant_table addresses and the machine addresses of
	 * the grant table frames
	 */
	pmap_kenter_ma(((vaddr_t)grant_table.gntt) + gnt_nr_grant_frames * PAGE_SIZE,
	    ((paddr_t)pages[gnt_nr_grant_frames]) << PAGE_SHIFT,
	    VM_PROT_WRITE, 0);
	pmap_update(pmap_kernel());

	/*
	 * add the grant entries associated to the last grant table frame
	 * and mark them as free. Prevent using the first grants (from 0 to 8)
	 * since they are used by the tools.
	 */
	start_gnt = (gnt_nr_grant_frames * NR_GRANT_ENTRIES_PER_PAGE) <
	            (NR_RESERVED_ENTRIES + 1) ?
	            (NR_RESERVED_ENTRIES + 1) :
	            (gnt_nr_grant_frames * NR_GRANT_ENTRIES_PER_PAGE);
	for (i = start_gnt;
	    i < nframes_new * NR_GRANT_ENTRIES_PER_PAGE;
	    i++) {
		KASSERT(gnt_entries[last_gnt_entry] == XENGNT_NO_ENTRY);
		gnt_entries[last_gnt_entry] = i;
		last_gnt_entry++;
	}
	gnt_nr_grant_frames = nframes_new;
	kmem_free(pages, sz);
	return 0;
}

/*
 * Returns a reference to the first free entry in grant table
 */
static grant_ref_t
xengnt_get_entry(void)
{
	grant_ref_t entry;
	static struct timeval xengnt_nonmemtime;
	static const struct timeval xengnt_nonmemintvl = {5,0};

	KASSERT(mutex_owned(&grant_lock));

	if (__predict_false(last_gnt_entry == 0)) {
		if (ratecheck(&xengnt_nonmemtime, &xengnt_nonmemintvl))
			printf("xengnt_get_entry: out of grant "
			    "table entries\n");
		return XENGNT_NO_ENTRY;
	}
	KASSERT(gnt_entries[last_gnt_entry] == XENGNT_NO_ENTRY);
	last_gnt_entry--;
	entry = gnt_entries[last_gnt_entry];
	gnt_entries[last_gnt_entry] = XENGNT_NO_ENTRY;
	KASSERT(entry != XENGNT_NO_ENTRY && entry > NR_RESERVED_ENTRIES);
	KASSERT(last_gnt_entry >= 0);
	KASSERT(last_gnt_entry <= gnt_max_grant_frames * NR_GRANT_ENTRIES_PER_PAGE);
	return entry;
}

/*
 * Mark the grant table entry as free
 */
static void
xengnt_free_entry(grant_ref_t entry)
{
	mutex_enter(&grant_lock);
	KASSERT(entry > NR_RESERVED_ENTRIES);
	KASSERT(gnt_entries[last_gnt_entry] == XENGNT_NO_ENTRY);
	KASSERT(last_gnt_entry >= 0);
	KASSERT(last_gnt_entry <= gnt_max_grant_frames * NR_GRANT_ENTRIES_PER_PAGE);
	gnt_entries[last_gnt_entry] = entry;
	last_gnt_entry++;
	mutex_exit(&grant_lock);
}

int
xengnt_grant_access(domid_t dom, paddr_t ma, int ro, grant_ref_t *entryp)
{
	mutex_enter(&grant_lock);

	*entryp = xengnt_get_entry();
	if (__predict_false(*entryp == XENGNT_NO_ENTRY)) {
		mutex_exit(&grant_lock);
		return ENOMEM;
	}

	if (GNT_ISV2) {
		grant_table.gntt_v2[*entryp].full_page.frame = ma >> PAGE_SHIFT;
		grant_table.gntt_v2[*entryp].hdr.domid = dom;
		/*
		 * ensure that the above values reach global visibility 
		 * before permitting frame's access (done when we set flags)
		 */
		xen_rmb();
		grant_table.gntt_v2[*entryp].hdr.flags =
		    GTF_permit_access | (ro ? GTF_readonly : 0);
	} else {
		grant_table.gntt_v1[*entryp].frame = ma >> PAGE_SHIFT;
		grant_table.gntt_v1[*entryp].domid = dom;
		/*      
		* ensure that the above values reach global visibility
		* before permitting frame's access (done when we set flags)    
		*/
		xen_rmb();
		grant_table.gntt_v1[*entryp].flags =  
		   GTF_permit_access | (ro ? GTF_readonly : 0);
	}
	mutex_exit(&grant_lock);
	return 0;
}

static inline uint16_t
xen_atomic_cmpxchg16(volatile uint16_t *ptr, uint16_t  val, uint16_t newval) 
{
	unsigned long result;

	__asm volatile(__LOCK_PREFIX
	   "cmpxchgw %w1,%2"
	   :"=a" (result)
	   :"q"(newval), "m" (*ptr), "0" (val)
	   :"memory");

	return result;
}

void
xengnt_revoke_access(grant_ref_t entry)
{
	if (GNT_ISV2) {
		grant_table.gntt_v2[entry].hdr.flags = 0;
		xen_mb();	/* Concurrent access by hypervisor */

		if (__predict_false(
		    (grant_status[entry] & (GTF_reading|GTF_writing)) != 0)) {
			printf("xengnt_revoke_access(%u): still in use\n",
			    entry);
		} else {

			/*
			 * The read of grant_status needs to have acquire
			 * semantics.
			 * Reads already have that on x86, so need only protect
			 * against compiler reordering. May need full barrier
			 * on other architectures.
			 */
			__insn_barrier();
		}
	} else {
		uint16_t flags, nflags;

		nflags = grant_table.gntt_v1[entry].flags;

		do {
		       if ((flags = nflags) & (GTF_reading|GTF_writing))
			       panic("xengnt_revoke_access: still in use");
		       nflags = xen_atomic_cmpxchg16(
			    &grant_table.gntt_v1[entry].flags, flags, 0);
		} while (nflags != flags);

	}
	xengnt_free_entry(entry);
}

int
xengnt_status(grant_ref_t entry)
{
	if (GNT_ISV2)
		return grant_status[entry] & (GTF_reading|GTF_writing);
	else
		return (grant_table.gntt_v1[entry].flags & (GTF_reading|GTF_writing));
}
