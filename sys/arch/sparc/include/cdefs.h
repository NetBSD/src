/*	$NetBSD: cdefs.h,v 1.12.6.1 2014/08/20 00:03:24 tls Exp $	*/

#ifndef	_MACHINE_CDEFS_H_
#define	_MACHINE_CDEFS_H_

/* No arch-specific cdefs. */
#ifdef __arch64__
#define	__ALIGNBYTES		((size_t)0xf)
#else
#define	__ALIGNBYTES		((size_t)0x7)
#endif

#endif /* !_MACHINE_CDEFS_H_ */
