/*	$NetBSD: frame.h,v 1.7 2026/03/28 01:44:37 thorpej Exp $	*/

#ifndef _SUN3_FRAME_H_
#define	_SUN3_FRAME_H_

#include <m68k/frame.h>

#ifdef _KERNEL
int _nodb_trap(int, struct trapframe *);
#define	MACHINE_KDEBUG_TRAP_FALLBACK(t, f) _nodb_trap((t), (f))
#endif /* _KERNEL */

#endif /* _SUN3_FRAME_H_ */
