/*	$NetBSD: cdefs.h,v 1.7.8.1 2011/08/27 15:37:25 jym Exp $	*/

#ifndef	_I386_CDEFS_H_
#define	_I386_CDEFS_H_

#if defined(_STANDALONE)
#define	__compactcall	__attribute__((__regparm__(3)))
#endif

#endif /* !_I386_CDEFS_H_ */
