#	$NetBSD: Makefile,v 1.2.20.1 2008/09/18 04:30:00 wrstuden Exp $

PROG=	bootptest
SRCS=	bootptest.c print-bootp.c getether.c
MAN=	bootptest.8

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.print-bootp.c+=	-Wno-pointer-sign
.endif
