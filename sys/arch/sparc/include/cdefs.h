/*	$NetBSD: cdefs.h,v 1.11.112.2 2014/05/22 11:40:08 yamt Exp $	*/

#ifndef	_MACHINE_CDEFS_H_
#define	_MACHINE_CDEFS_H_

/* No arch-specific cdefs. */
#ifdef __arch64__
#define	__ALIGNBYTES		((size_t)0xf)
#else
#define	__ALIGNBYTES		((size_t)0x7)
#endif

#endif /* !_MACHINE_CDEFS_H_ */
