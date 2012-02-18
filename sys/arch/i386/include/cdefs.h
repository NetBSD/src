/*	$NetBSD: cdefs.h,v 1.8.6.1 2012/02/18 07:32:22 mrg Exp $	*/

#ifndef	_I386_CDEFS_H_
#define	_I386_CDEFS_H_

#if defined(_STANDALONE)
#define	__compactcall	__attribute__((__regparm__(3)))
#endif

#define __ALIGNBYTES	(sizeof(int) - 1)

#endif /* !_I386_CDEFS_H_ */
