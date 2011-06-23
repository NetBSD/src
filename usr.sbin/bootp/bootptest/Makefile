#	$NetBSD: Makefile,v 1.3.18.1 2011/06/23 14:20:46 cherry Exp $

PROG=	bootptest
SRCS=	bootptest.c print-bootp.c getether.c
MAN=	bootptest.8

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.print-bootp.c+=	-Wno-pointer-sign
.endif
