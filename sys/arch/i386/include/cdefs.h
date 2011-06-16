/*	$NetBSD: cdefs.h,v 1.8 2011/06/16 13:27:59 joerg Exp $	*/

#ifndef	_I386_CDEFS_H_
#define	_I386_CDEFS_H_

#if defined(_STANDALONE)
#define	__compactcall	__attribute__((__regparm__(3)))
#endif

#endif /* !_I386_CDEFS_H_ */
