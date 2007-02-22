/*	$NetBSD: lock_stubs.S,v 1.1 2007/02/22 22:53:26 uwe Exp $	*/

#include <sh3/asm.h>

/*
 * LINTSTUB: include <sys/types.h>
 */


/*
 * LINTSTUB: Func: int _lock_cas(volatile uintptr_t *ptr, uintptr_t old, uintptr_t new);
 */
NENTRY(_lock_cas)
	rts
	 mov	#0, r0
