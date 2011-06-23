#	$NetBSD: Makefile,v 1.6.18.1 2011/06/23 14:20:45 cherry Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	tr
SRCS=	str.c tr.c

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
.for f in str tr
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor
.endif
