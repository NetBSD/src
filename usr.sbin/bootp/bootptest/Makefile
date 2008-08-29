#	$NetBSD: Makefile,v 1.3 2008/08/29 00:02:25 gmcgarry Exp $

PROG=	bootptest
SRCS=	bootptest.c print-bootp.c getether.c
MAN=	bootptest.8

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.print-bootp.c+=	-Wno-pointer-sign
.endif
