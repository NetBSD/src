/* $NetBSD: uvm_physseg.h,v 1.1 2016/12/19 12:21:29 cherry Exp $ */

/*
 * Consolidated API from uvm_page.c and others.
 * Consolidated and designed by Cherry G. Mathew <cherry@zyx.in>
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
 * uvm_physseg_valid() == false on any of the below constants.
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
 * uvm_physseg_valid() == false on any of the below constants.
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

bool uvm_physseg_valid(uvm_physseg_t);

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

#if defined(PMAP_STEAL_MEMORY)
/*
 * XXX: Legacy: This needs to be upgraded to a full pa management
 * layer.
 */
void uvm_physseg_set_avail_start(uvm_physseg_t, paddr_t);
void uvm_physseg_set_avail_end(uvm_physseg_t, paddr_t);
#endif /* PMAP_STEAL_MEMORY */

#endif /* _UVM_UVM_PHYSSEG_H_ */

