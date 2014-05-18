/*	$NetBSD: cdefs.h,v 1.9.4.1 2014/05/18 17:45:22 rmind Exp $	*/

#ifndef	_POWERPC_CDEFS_H_
#define	_POWERPC_CDEFS_H_

#define	__ALIGNBYTES	(sizeof(double) - 1)
#ifdef _KERNEL
#define	ALIGNBYTES32	__ALIGNBYTES
#endif

#endif /* !_POWERPC_CDEFS_H_ */
