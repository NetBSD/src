#	$NetBSD: Makefile,v 1.7 2011/06/20 07:44:01 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	tr
SRCS=	str.c tr.c

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
.for f in str tr
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor
.endif
