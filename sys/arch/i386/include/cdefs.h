/*	$NetBSD: cdefs.h,v 1.7.26.1 2011/06/23 14:19:15 cherry Exp $	*/

#ifndef	_I386_CDEFS_H_
#define	_I386_CDEFS_H_

#if defined(_STANDALONE)
#define	__compactcall	__attribute__((__regparm__(3)))
#endif

#endif /* !_I386_CDEFS_H_ */
