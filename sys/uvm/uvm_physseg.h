/* $NetBSD: uvm_physseg.h,v 1.8.18.2 2017/12/03 11:39:22 jdolecek Exp $ */

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by  Cherry G. Mathew <cherry@NetBSD.org>
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
 * Consolidated API from uvm_page.c and others.
 * Consolidated and designed by Cherry G. Mathew <cherry@NetBSD.org>
 */

#ifndef _UVM_UVM_PHYSSEG_H_
#define _UVM_UVM_PHYSSEG_H_

#if defined(_KERNEL_OPT)
#include "opt_uvm_hotplug.h"
#endif

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/types.h>

/*
 * No APIs are explicitly #included in uvm_physseg.c
 */

#if defined(UVM_HOTPLUG) /* rbtree impementation */
#define PRIxPHYSSEG "p"

/*
 * These are specific values of invalid constants for uvm_physseg_t.
 * uvm_physseg_valid_p() == false on any of the below constants.
 *
 * Specific invalid constants encapsulate specific explicit failure
 * scenarios (see the comments next to them)
 */

#define UVM_PHYSSEG_TYPE_INVALID NULL /* Generic invalid value */
#define UVM_PHYSSEG_TYPE_INVALID_EMPTY NULL /* empty segment access */
#define UVM_PHYSSEG_TYPE_INVALID_OVERFLOW NULL /* ran off the end of the last segment */

typedef struct uvm_physseg * uvm_physseg_t;

#else  /* UVM_HOTPLUG */

#define PRIxPHYSSEG "d"

/*
 * These are specific values of invalid constants for uvm_physseg_t.
 * uvm_physseg_valid_p() == false on any of the below constants.
 *
 * Specific invalid constants encapsulate specific explicit failure
 * scenarios (see the comments next to them)
 */

#define UVM_PHYSSEG_TYPE_INVALID -1 /* Generic invalid value */
#define UVM_PHYSSEG_TYPE_INVALID_EMPTY -1 /* empty segment access */
#define UVM_PHYSSEG_TYPE_INVALID_OVERFLOW (uvm_physseg_get_last() + 1) /* ran off the end of the last segment */

typedef int uvm_physseg_t;
#endif /* UVM_HOTPLUG */

void uvm_physseg_init(void);

bool uvm_physseg_valid_p(uvm_physseg_t);

/*
 * Return start/end pfn of given segment
 * Returns: -1 if the segment number is invalid
 */
paddr_t uvm_physseg_get_start(uvm_physseg_t);
paddr_t uvm_physseg_get_end(uvm_physseg_t);

paddr_t uvm_physseg_get_avail_start(uvm_physseg_t);
paddr_t uvm_physseg_get_avail_end(uvm_physseg_t);

struct vm_page * uvm_physseg_get_pg(uvm_physseg_t, paddr_t);

#ifdef __HAVE_PMAP_PHYSSEG
struct	pmap_physseg * uvm_physseg_get_pmseg(uvm_physseg_t);
#endif

int uvm_physseg_get_free_list(uvm_physseg_t);
u_int uvm_physseg_get_start_hint(uvm_physseg_t);
bool uvm_physseg_set_start_hint(uvm_physseg_t, u_int);

/*
 * Functions to help walk the list of segments.
 * Returns: NULL if the segment number is invalid
 */
uvm_physseg_t uvm_physseg_get_next(uvm_physseg_t);
uvm_physseg_t uvm_physseg_get_prev(uvm_physseg_t);
uvm_physseg_t uvm_physseg_get_first(void);
uvm_physseg_t uvm_physseg_get_last(void);


/* Return the frame number of the highest registered physical page frame */
paddr_t uvm_physseg_get_highest_frame(void);

/* Actually, uvm_page_physload takes PF#s which need their own type */
uvm_physseg_t uvm_page_physload(paddr_t, paddr_t, paddr_t,
    paddr_t, int);

bool uvm_page_physunload(uvm_physseg_t, int, paddr_t *);
bool uvm_page_physunload_force(uvm_physseg_t, int, paddr_t *);

uvm_physseg_t uvm_physseg_find(paddr_t, psize_t *);

bool uvm_physseg_plug(paddr_t, size_t, uvm_physseg_t *);
bool uvm_physseg_unplug(paddr_t, size_t);

#if defined(UVM_PHYSSEG_LEGACY)
/*
 * XXX: Legacy: This needs to be upgraded to a full pa management
 * layer.
 */
void uvm_physseg_set_avail_start(uvm_physseg_t, paddr_t);
void uvm_physseg_set_avail_end(uvm_physseg_t, paddr_t);
#endif /* UVM_PHYSSEG_LEGACY */

#endif /* _UVM_UVM_PHYSSEG_H_ */
