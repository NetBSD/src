/*	$NetBSD: uvm_pglist.h,v 1.7.16.4 2012/02/09 03:05:01 matt Exp $	*/

/*-
 * Copyright (c) 2000, 2001, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _UVM_UVM_PGLIST_H_
#define _UVM_UVM_PGLIST_H_

#ifndef VM_NFREELIST
#include <machine/vmparam.h>
#endif

/*
 * This defines the type of a page queue, e.g. active list, inactive
 * list, etc.
 */
TAILQ_HEAD(pglist, vm_page);
LIST_HEAD(pgflist, vm_page);

/*
 * A page free list consists of free pages of unknown contents and free
 * pages of all zeros.  For each color, there is a global page free list
 * as well as one for each cpu.
 */
#define	PGFL_UNKNOWN	0
#define	PGFL_ZEROS	1
#define	PGFL_NQUEUES	2

struct pgfreelist {
	u_long pgfl_pages[PGFL_NQUEUES];
	struct pgflist pgfl_queues[VM_NFREELIST][PGFL_NQUEUES];

	/* pagealloc counters */
	uint64_t pgfl_colorfail;/* pagealloc where we got no page */
	uint64_t pgfl_colorhit;	/* pagealloc where we got optimal color */
	uint64_t pgfl_colormiss;/* pagealloc where we didn't */
	uint64_t pgfl_colorany;	/* pagealloc where we wanted any color */
};

#endif /* _UVM_UVM_PGLIST_H_ */
