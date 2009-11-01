/*      $NetBSD: xengnt.c,v 1.13.2.4 2009/11/01 21:43:28 jym Exp $      */

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
__KERNEL_RCSID(0, "$NetBSD: xengnt.c,v 1.13.2.4 2009/11/01 21:43:28 jym Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/extent.h>
#include <sys/kernel.h>
#include <uvm/uvm.h>

#include <xen/hypervisor.h>
#include <xen/xen.h>
#include <xen/granttables.h>

/* #define XENDEBUG */
#ifdef XENDEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

#define NR_GRANT_ENTRIES_PER_PAGE (PAGE_SIZE / sizeof(grant_entry_t))

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
grant_entry_t *grant_table;

static grant_ref_t xengnt_get_entry(void);
static void xengnt_free_entry(grant_ref_t);
static int xengnt_more_entries(void);

void
xengnt_init(void)
{
	struct gnttab_query_size query;
	int rc;
	int nr_grant_entries;
	int i;

	query.dom = DOMID_SELF;
	rc = HYPERVISOR_grant_table_op(GNTTABOP_query_size, &query, 1);
	if ((rc < 0) || (query.status != GNTST_okay))
		gnt_max_grant_frames = 4; /* Legacy max number of frames */
	else
		gnt_max_grant_frames = query.max_nr_frames;
	gnt_nr_grant_frames = 0;

	nr_grant_entries =
	    gnt_max_grant_frames * NR_GRANT_ENTRIES_PER_PAGE;

	grant_table = (void *)uvm_km_alloc(kernel_map,
	    gnt_max_grant_frames * PAGE_SIZE, 0, UVM_KMF_VAONLY);
	if (grant_table == NULL)
		panic("xengnt_init() no VM space");
	gnt_entries = malloc((nr_grant_entries + 1) * sizeof(grant_ref_t),
	    M_DEVBUF, M_NOWAIT);
	if (gnt_entries == NULL)
		panic("xengnt_init() no space for bitmask");
	for (i = 0; i <= nr_grant_entries; i++)
		gnt_entries[i] = XENGNT_NO_ENTRY;

	xengnt_resume();

}

/*
 * Resume grant table state
 */
bool
xengnt_resume(void)
{
	int previous_nr_grant_frames = gnt_nr_grant_frames;

	last_gnt_entry = 0;
	gnt_nr_grant_frames = 0;

	while (gnt_nr_grant_frames < previous_nr_grant_frames) {
		if (xengnt_more_entries() != 0)
			panic("xengnt_resume: can't restore grant frames");
	}
	return true;
}

/*
 * Suspend grant table state
 */
bool
xengnt_suspend() {

	int i;

	KASSERT(gnt_entries[last_gnt_entry] == XENGNT_NO_ENTRY);

	for (i = 0; i < last_gnt_entry; i++) {
		/* invalidate all grant entries (necessary for resume) */
		gnt_entries[i] = XENGNT_NO_ENTRY;
	}
	
	/* Remove virtual => machine mapping */
	pmap_kremove((vaddr_t)grant_table, gnt_nr_grant_frames * PAGE_SIZE);
	pmap_update(pmap_kernel());

	return true;
}


/*
 * Add another page to the grant table
 * Returns 0 on success, ENOMEM on failure
 */
static int
xengnt_more_entries(void)
{
	gnttab_setup_table_t setup;
	unsigned long *pages;
	int nframes_new = gnt_nr_grant_frames + 1;
	int i;

	if (gnt_nr_grant_frames == gnt_max_grant_frames)
		return ENOMEM;

	pages = malloc(nframes_new * sizeof(long), M_DEVBUF, M_NOWAIT);
	if (pages == NULL)
		return ENOMEM;

	setup.dom = DOMID_SELF;
	setup.nr_frames = nframes_new;
	xenguest_handle(setup.frame_list) = pages;

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
		free(pages, M_DEVBUF);
		return ENOMEM;
	}

	DPRINTF(("xengnt_more_entries: map 0x%lx -> %p\n",
	    pages[gnt_nr_grant_frames],
	    (char *)grant_table + gnt_nr_grant_frames * PAGE_SIZE));

	/*
	 * map between grant_table addresses and the machine addresses of
	 * the grant table frames
	 */
	pmap_kenter_ma(((vaddr_t)grant_table) + gnt_nr_grant_frames * PAGE_SIZE,
	    pages[gnt_nr_grant_frames] << PAGE_SHIFT, VM_PROT_WRITE);

	/*
	 * add the grant entries associated to the last grant table frame
	 * and mark them as free
	 */
	for (i = gnt_nr_grant_frames * NR_GRANT_ENTRIES_PER_PAGE;
	    i < nframes_new * NR_GRANT_ENTRIES_PER_PAGE;
	    i++) {
		KASSERT(gnt_entries[last_gnt_entry] == XENGNT_NO_ENTRY);
		gnt_entries[last_gnt_entry] = i;
		last_gnt_entry++;
	}
	gnt_nr_grant_frames = nframes_new;
	free(pages, M_DEVBUF);
	return 0;
}

/*
 * Returns a reference to the first free entry in grant table
 */
static grant_ref_t
xengnt_get_entry(void)
{
	grant_ref_t entry;
	int s = splvm();
	static struct timeval xengnt_nonmemtime;
	static const struct timeval xengnt_nonmemintvl = {5,0};

	if (last_gnt_entry == 0) {
		if (xengnt_more_entries()) {
			splx(s);
			if (ratecheck(&xengnt_nonmemtime, &xengnt_nonmemintvl))
				printf("xengnt_get_entry: out of grant "
				    "table entries\n");
			return XENGNT_NO_ENTRY;
		}
	}
	KASSERT(gnt_entries[last_gnt_entry] == XENGNT_NO_ENTRY);
	last_gnt_entry--;
	entry = gnt_entries[last_gnt_entry];
	gnt_entries[last_gnt_entry] = XENGNT_NO_ENTRY;
	splx(s);
	KASSERT(entry != XENGNT_NO_ENTRY);
	KASSERT(last_gnt_entry >= 0 && last_gnt_entry <= gnt_max_grant_frames * NR_GRANT_ENTRIES_PER_PAGE);
	return entry;
}

/*
 * Mark the grant table entry as free
 */
static void
xengnt_free_entry(grant_ref_t entry)
{
	int s = splvm();
	KASSERT(gnt_entries[last_gnt_entry] == XENGNT_NO_ENTRY);
	KASSERT(last_gnt_entry >= 0 && last_gnt_entry <= gnt_max_grant_frames * NR_GRANT_ENTRIES_PER_PAGE);
	gnt_entries[last_gnt_entry] = entry;
	last_gnt_entry++;
	splx(s);
}

int
xengnt_grant_access(domid_t dom, paddr_t ma, int ro, grant_ref_t *entryp)
{
	*entryp = xengnt_get_entry();
	if (__predict_false(*entryp == XENGNT_NO_ENTRY))
		return ENOMEM;

	grant_table[*entryp].frame = ma >> PAGE_SHIFT;
	grant_table[*entryp].domid = dom;
	/*
	 * ensure that the above values reach global visibility 
	 * before permitting frame's access (done when we set flags)
	 */
	xen_rmb();
	grant_table[*entryp].flags =
	    GTF_permit_access | (ro ? GTF_readonly : 0);
	return 0;
}

void
xengnt_revoke_access(grant_ref_t entry)
{
	uint16_t flags, nflags;

	nflags = grant_table[entry].flags;

	do {
		if ((flags = nflags) & (GTF_reading|GTF_writing))
			panic("xengnt_revoke_access: still in use");
		nflags = xen_atomic_cmpxchg16(&grant_table[entry].flags,
		    flags, 0);
	} while (nflags != flags);
	xengnt_free_entry(entry);
}

int
xengnt_grant_transfer(domid_t dom, grant_ref_t *entryp)
{
	*entryp = xengnt_get_entry();
	if (__predict_false(*entryp == XENGNT_NO_ENTRY))
		return ENOMEM;

	grant_table[*entryp].frame = 0;
	grant_table[*entryp].domid = dom;
	/*
	 * ensure that the above values reach global visibility 
	 * before permitting frame's transfer (done when we set flags)
	 */
	xen_rmb();
	grant_table[*entryp].flags = GTF_accept_transfer;
	return 0;
}

paddr_t
xengnt_revoke_transfer(grant_ref_t entry)
{
	paddr_t page;
	uint16_t flags;

	/* if the transfer has not started, free the entry and return 0 */
	while (!((flags = grant_table[entry].flags) & GTF_transfer_committed)) {
		if (xen_atomic_cmpxchg16(&grant_table[entry].flags,
		    flags, 0) == flags ) {
			xengnt_free_entry(entry);
			return 0;
		}
		HYPERVISOR_yield();
	}

	/* If transfer in progress, wait for completion */
	while (!((flags = grant_table[entry].flags) & GTF_transfer_completed))
		HYPERVISOR_yield();

	/* Read the frame number /after/ reading completion status. */
	__insn_barrier();
	page = grant_table[entry].frame;
	if (page == 0)
		printf("xengnt_revoke_transfer: guest sent pa 0\n");

	xengnt_free_entry(entry);
	return page;
}

int
xengnt_status(grant_ref_t entry)
{
	return (grant_table[entry].flags & (GTF_reading|GTF_writing));
}
