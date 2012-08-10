#	$NetBSD: Makefile,v 1.5 2012/08/10 12:10:28 joerg Exp $

PROG=	bootptest
SRCS=	bootptest.c print-bootp.c getether.c
MAN=	bootptest.8

COPTS.print-bootp.c+=	-Wno-pointer-sign

.include <bsd.prog.mk>
