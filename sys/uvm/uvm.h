/*	$NetBSD: uvm.h,v 1.5 1998/02/10 14:12:01 mrg Exp $	*/

/*
 * XXXCDC: "ROUGH DRAFT" QUALITY UVM PRE-RELEASE FILE!   
 *	   >>>USE AT YOUR OWN RISK, WORK IS NOT FINISHED<<<
 */

/*
 *
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
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
 * from: Id: uvm.h,v 1.1.2.14 1998/02/02 20:07:19 chuck Exp
 */

#ifndef _UVM_UVM_H_
#define _UVM_UVM_H_

#include "opt_uvmhist.h"

#include <uvm/uvm_extern.h>

#include <uvm/uvm_stat.h>

/*
 * pull in prototypes
 */

#include <uvm/uvm_amap.h>
#include <uvm/uvm_aobj.h>
#include <uvm/uvm_fault.h>
#include <uvm/uvm_glue.h>
#include <uvm/uvm_km.h>
#include <uvm/uvm_loan.h>
#include <uvm/uvm_map.h>
#include <uvm/uvm_object.h>
#include <uvm/uvm_page.h>
#include <uvm/uvm_pager.h>
#include <uvm/uvm_pdaemon.h>
#include <uvm/uvm_swap.h>

/*
 * uvm structure (vm global state: collected in one structure for ease
 * of reference...)
 */

struct uvm {
	/* vm_page related parameters */
		/* vm_page queues */
	struct pglist page_free;	/* unallocated pages */
	struct pglist page_active;	/* allocated pages, in use */
	struct pglist page_inactive_swp;/* pages inactive (reclaim or free) */
	struct pglist page_inactive_obj;/* pages inactive (reclaim or free) */
#if NCPU > 1
	simple_lock_data_t pageqlock;	/* lock for active/inactive page q */
	simple_lock_data_t fpageqlock;	/* lock for free page q */
#endif /* NCPU > 1 */
		/* page daemon trigger */
	int pagedaemon;			/* daemon sleeps on this */
	struct proc *pagedaemon_proc;	/* daemon's pid */
	simple_lock_data_t pagedaemon_lock;
		/* page hash */
	struct pglist *page_hash;	/* page hash table (vp/off->page) */
	int page_nhash;			/* number of buckets */
	int page_hashmask;		/* hash mask */
#if NCPU > 1
	simple_lock_data_t hashlock;	/* lock on page_hash array */
#endif
	/* anon stuff */
	struct vm_anon *afree;		/* anon free list */
#if NCPU > 1
	simple_lock_data_t afreelock; 	/* lock on anon free list */
#endif /* NCPU > 1 */

	/* static kernel map entry pool */
	vm_map_entry_t kentry_free;	/* free page pool */
#if NCPU > 1
	simple_lock_data_t kentry_lock;
#endif

	/* aio_done is locked by uvm.pagedaemon_lock and splbio! */
	struct uvm_aiohead aio_done;	/* done async i/o reqs */

	/* pager VM area bounds */
	vm_offset_t pager_sva;		/* start of pager VA area */
	vm_offset_t pager_eva;		/* end of pager VA area */

	/* kernel object: to support anonymous pageable kernel memory */
	struct uvm_object *kernel_object;
};

extern struct uvm uvm;

/*
 * historys
 */

UVMHIST_DECL(maphist);
UVMHIST_DECL(pdhist);

/*
 * vm_map_entry etype bits:
 */

#define UVM_ET_OBJ		0x01	/* it is a uvm_object */
#define UVM_ET_MAP		0x02	/* it is a vm_map */
#define UVM_ET_SUBMAP		0x04	/* it is a submap (MAP must be 1 too) */
#define UVM_ET_COPYONWRITE 	0x08	/* copy_on_write */
#define UVM_ET_NEEDSCOPY	0x10	/* needs_copy */

#define UVM_ET_ISOBJ(E)		(((E)->etype & UVM_ET_OBJ) != 0)
#define UVM_ET_ISMAP(E)		(((E)->etype & UVM_ET_MAP) != 0)
#define UVM_ET_ISSUBMAP(E)	(((E)->etype & UVM_ET_SUBMAP) != 0)
#define UVM_ET_ISCOPYONWRITE(E)	(((E)->etype & UVM_ET_COPYONWRITE) != 0)
#define UVM_ET_ISNEEDSCOPY(E)	(((E)->etype & UVM_ET_NEEDSCOPY) != 0)

/*
 * macros
 */

/*
 * UVM_UNLOCK_AND_WAIT: atomic unlock+wait... front end for the 
 * (poorly named) thread_sleep_msg function.
 */

#if NCPU > 1

#define UVM_UNLOCK_AND_WAIT(event,lock,intr,msg, timo) \
	thread_sleep_msg(event,lock,intr,msg, timo)

#else

#define UVM_UNLOCK_AND_WAIT(event,lock,intr,msg, timo) \
	thread_sleep_msg(event,NULL,intr,msg, timo)

#endif

/*
 * UVM_PAGE_OWN: track page ownership (only if UVM_PAGE_TRKOWN)
 */

#if defined(UVM_PAGE_TRKOWN)

#define UVM_PAGE_OWN(PG, TAG) uvm_page_own(PG, TAG)

#else /* UVM_PAGE_TRKOWN */

#define UVM_PAGE_OWN(PG, TAG) /* nothing */

#endif /* UVM_PAGE_TRKOWN */

/*
 * pull in inlines
 */

#include <uvm/uvm_amap_i.h>
#include <uvm/uvm_fault_i.h>
#include <uvm/uvm_map_i.h>
#include <uvm/uvm_page_i.h>
#include <uvm/uvm_pager_i.h>

#endif /* _UVM_UVM_H_ */
