/*	$NetBSD: cdefs.h,v 1.9 2003/11/16 11:07:57 pk Exp $	*/

#ifndef	_MACHINE_CDEFS_H_
#define	_MACHINE_CDEFS_H_

#ifdef __GNUC__
/*
 * A statement designed to tell the compiler not to re-order instructions
 * across this barrier. It does not generate any code by itself and the
 * harmless `clobber' of %g0 prevents other side-effects such as re-loading
 * registers from memory.
 */
#define	__insn_barrier()	__asm __volatile("": : : "g0")
#else
#define	__insn_barrier()	/*void*/
#endif

#endif /* !_MACHINE_CDEFS_H_ */
