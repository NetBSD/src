#	$NetBSD: Makefile,v 1.6 2008/08/29 00:02:24 gmcgarry Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	tr
SRCS=	str.c tr.c

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
.for f in str tr
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor
.endif
