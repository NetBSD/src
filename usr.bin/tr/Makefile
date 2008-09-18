#	$NetBSD: Makefile,v 1.5.22.1 2008/09/18 04:29:24 wrstuden Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	tr
SRCS=	str.c tr.c

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
.for f in str tr
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor
.endif
