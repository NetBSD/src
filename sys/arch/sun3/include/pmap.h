/*
 * Copyright (c) 1993 Adam Glass
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
 *	This product includes software developed by Adam Glass.
 * 4. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Adam Glass ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: pmap.h,v 1.9 1994/06/29 05:32:53 gwr Exp $
 */

#ifndef	_PMAP_MACHINE_
#define	_PMAP_MACHINE_

#include <sys/queue.h>

/*
 * Pmap stuff
 * [some ideas borrowed from torek, but no code]
 */

struct context_state {
	TAILQ_ENTRY(context_state) context_link;
	int            context_num;
	struct pmap   *context_upmap;
};

typedef struct context_state *context_t;

struct pmap {
	int	                pm_refcount;	/* pmap reference count */
	simple_lock_data_t      pm_lock;	/* lock on pmap */
	struct pmap_statistics	pm_stats;	/* pmap statistics */
	context_t               pm_context;     /* context if any */
	int                     pm_version;
	unsigned char           *pm_segmap;
};

typedef struct pmap *pmap_t;

extern pmap_t kernel_pmap;

#define PMEGQ_FREE     0
#define PMEGQ_INACTIVE 1
#define PMEGQ_ACTIVE   2
#define PMEGQ_KERNEL   3
#define PMEGQ_NONE     4

struct pmeg_state {
	TAILQ_ENTRY(pmeg_state) pmeg_link;
	int            pmeg_index;
	pmap_t         pmeg_owner;
	int            pmeg_owner_version;
	vm_offset_t    pmeg_va;
	int            pmeg_wired_count;
	int            pmeg_reserved;
	int            pmeg_vpages;
	int            pmeg_qstate;
};

typedef struct pmeg_state *pmeg_t;

#define PMAP_ACTIVATE(pmap, pcbp, iscurproc) \
      pmap_activate(pmap, pcbp)
#define PMAP_DEACTIVATE(pmap, pcbp) \
      pmap_deactivate(pmap, pcbp)

#define	pmap_kernel()			(kernel_pmap)

/* like the sparc port, use the lower bits of a pa which must be page
 *  aligned anyway to pass memtype, caching information.
 */
#define PMAP_MMEM      0x0
#define PMAP_OBIO      0x1
#define PMAP_VME16D    0x2
#define PMAP_VME32D    0x3
#define PMAP_MEMMASK   0x3
#define PMAP_NC        0x4
#define PMAP_SPECMASK  0x7

extern vm_offset_t virtual_avail, virtual_end;
extern vm_offset_t avail_start, avail_end;

#endif	_PMAP_MACHINE_
