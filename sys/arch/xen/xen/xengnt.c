/*      $NetBSD: xengnt.c,v 1.2.4.2 2006/04/22 11:38:11 simonb Exp $      */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/extent.h>
#include <sys/kernel.h>
#include <uvm/uvm.h>

#include <machine/hypervisor.h>
#include <machine/xen.h>
#include <machine/granttables.h>

#undef XENDEBUG
#ifdef XENDEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

#define NR_GRANT_ENTRIES (NR_GRANT_FRAMES * PAGE_SIZE / sizeof(grant_entry_t))

/* bitmask of free grant entries. 1 = entrie is free */
#define NR_BITMASK_ENTRIES ((NR_GRANT_ENTRIES + 31) / 32)
u_int32_t gnt_entries_bitmask[NR_BITMASK_ENTRIES];

/* VM address of the grant table */
grant_entry_t *grant_table;

static grant_ref_t xengnt_get_entry(void);
#define XENGNT_NO_ENTRY 0xffffffff
static void xengnt_free_entry(grant_ref_t);
static void xengnt_resume(void);

void
xengnt_init()
{
	grant_table = (void *)uvm_km_alloc(kernel_map,
	    NR_GRANT_FRAMES * PAGE_SIZE, 0, UVM_KMF_VAONLY);
	if (grant_table == NULL)
		panic("xengnt_init() no VM space");

	xengnt_resume();

	memset(gnt_entries_bitmask, 0xff, sizeof(gnt_entries_bitmask));
}

static void
xengnt_resume()
{
	gnttab_setup_table_t setup;
	unsigned long pages[NR_GRANT_FRAMES];
	int i = 0;

	setup.dom = DOMID_SELF;
	setup.nr_frames = NR_GRANT_FRAMES;
	setup.frame_list = pages;

	if (HYPERVISOR_grant_table_op(GNTTABOP_setup_table, &setup, 1) != 0)
		panic("xengnt_resume: setup table failed");
	if (setup.status != 0)
		panic("xengnt_resume: setup table returned %d", setup.status);

	for (i = 0; i < NR_GRANT_FRAMES; i++) {
		DPRINTF(("xengnt_resume: map 0x%lx -> %p\n",
			pages[i], (char *)grant_table + i * PAGE_SIZE));
		    
		pmap_kenter_ma(((vaddr_t)grant_table) + i * PAGE_SIZE,
		    pages[i] << PAGE_SHIFT, VM_PROT_WRITE);
	}
}

static grant_ref_t
xengnt_get_entry()
{
	int i;
	grant_ref_t entry;
	int s = splvm();

	for (i = 0; i < NR_BITMASK_ENTRIES; i++) {
		entry = ffs(gnt_entries_bitmask[i]);
		if (entry != 0) {
			entry--;
			gnt_entries_bitmask[i] &= ~(1 << (entry & 0x1f));
			splx(s);
			return ((i << 5) + entry);
		}
	}
	splx(s);
	return XENGNT_NO_ENTRY;
}

static void
xengnt_free_entry(grant_ref_t entry)
{
	int s = splvm();
	KASSERT((entry >> 5) < NR_BITMASK_ENTRIES);
	gnt_entries_bitmask[entry >> 5] |= (1 << (entry & 0x1f));
	splx(s);
}

int
xengnt_grant_access(domid_t dom, paddr_t ma, int ro, grant_ref_t *entryp)
{
	*entryp = xengnt_get_entry();
	if (__predict_false(*entryp == XENGNT_NO_ENTRY))
		return ENOMEM;

	grant_table[*entryp].frame = ma >> PAGE_SHIFT;
	grant_table[*entryp].domid  = dom;
	x86_lfence();
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
	grant_table[*entryp].domid  =dom;
	x86_lfence();
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
