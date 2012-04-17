/*	$NetBSD: cdefs.h,v 1.8.2.1 2012/04/17 00:06:29 yamt Exp $	*/

#ifndef	_I386_CDEFS_H_
#define	_I386_CDEFS_H_

#if defined(_STANDALONE)
#define	__compactcall	__attribute__((__regparm__(3)))
#endif

#define __ALIGNBYTES	(sizeof(int) - 1)

#endif /* !_I386_CDEFS_H_ */
