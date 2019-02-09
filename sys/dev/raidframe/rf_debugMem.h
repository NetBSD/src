/*	$NetBSD: rf_debugMem.h,v 1.14 2019/02/09 03:34:00 christos Exp $	*/
/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Daniel Stodolsky, Mark Holland
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * rf_debugMem.h -- memory leak debugging module
 *
 * IMPORTANT:  if you put the lock/unlock mutex stuff back in here, you
 *             need to take it out of the routines in debugMem.c
 *
 */

#ifndef _RF__RF_DEBUGMEM_H_
#define _RF__RF_DEBUGMEM_H_

#include "rf_alloclist.h"

#include <sys/types.h>
#include <sys/malloc.h>

int     rf_ConfigureDebugMem(RF_ShutdownList_t **);

#ifndef RF_DEBUG_MEM
# define RF_DEBUG_MEM	0
# define RF_Malloc(s)	malloc(s, M_RAIDFRAME, M_WAITOK | M_ZERO)
# define RF_Free(p, s)	free(p, M_RAIDFRAME)
# define RF_MallocAndAdd(s, l) _RF_MallocAndAdd(s, l)
#else
extern long rf_memDebug;

void    rf_record_malloc(void *, size_t, const char *, uint32_t);
void    rf_unrecord_malloc(void *, size_t);
void    rf_print_unfreed(void);

# define RF_Malloc(s) _RF_Malloc(s, __FILE__, __LINE__)
# define RF_MallocAndAdd(s, l) \
    _RF_MallocAndAdd(s, l, __FILE__, __LINE__)

static __inline void *
_RF_Malloc(size_t size, const char *file, uint32_t line)
{
	void *p = malloc(size, M_RAIDFRAME, M_WAITOK | M_ZERO);
	if (rf_memDebug) rf_record_malloc(p, size, file, line);
	return p;
}

static __inline void
RF_Free(void *p, size_t size)
{
	free(p, M_RAIDFRAME);
	if (rf_memDebug) rf_unrecord_malloc(p, size);
  }
#endif

static __inline void *
_RF_MallocAndAdd(size_t size, RF_AllocListElem_t *l
#if RF_DEBUG_MEM
    , const char *file, uint32_t line
#endif
)
{
	void *p = malloc(size, M_RAIDFRAME, M_WAITOK | M_ZERO);
#if RF_DEBUG_MEM
	if (rf_memDebug) rf_record_malloc(p, size, file, line);
#endif
	if (l) rf_AddToAllocList(l, p, size);
	return p;
}


#endif				/* !_RF__RF_DEBUGMEM_H_ */
