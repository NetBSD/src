/*	$NetBSD: frame.h,v 1.1.252.1 2026/11/04 23:21:24 perseant Exp $	*/

#ifndef _SUN2_FRAME_H_
#define	_SUN2_FRAME_H_

#include <m68k/frame.h>

#ifdef _KERNEL
int _nodb_trap(int, struct trapframe *);
#define	MACHINE_KDEBUG_TRAP_FALLBACK(t, f) _nodb_trap((t), (f))
#endif /* _KERNEL */

#endif /* _SUN2_FRAME_H_ */
