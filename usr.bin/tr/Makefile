#	$NetBSD: Makefile,v 1.7.2.1 2012/10/30 19:00:28 yamt Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	tr
SRCS=	str.c tr.c

.for f in str tr
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor

.include <bsd.prog.mk>
