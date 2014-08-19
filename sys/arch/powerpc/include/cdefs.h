/*	$NetBSD: cdefs.h,v 1.9.2.1 2014/08/20 00:03:19 tls Exp $	*/

#ifndef	_POWERPC_CDEFS_H_
#define	_POWERPC_CDEFS_H_

#define	__ALIGNBYTES	(sizeof(double) - 1)
#ifdef _KERNEL
#define	ALIGNBYTES32	__ALIGNBYTES
#endif

#endif /* !_POWERPC_CDEFS_H_ */
