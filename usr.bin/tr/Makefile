#	$NetBSD: Makefile,v 1.5 2006/05/12 01:26:05 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	tr
SRCS=	str.c tr.c

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
.for f in str tr
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor
.endif
