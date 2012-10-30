#	$NetBSD: Makefile,v 1.4.2.1 2012/10/30 19:00:30 yamt Exp $

PROG=	bootptest
SRCS=	bootptest.c print-bootp.c getether.c
MAN=	bootptest.8

COPTS.print-bootp.c+=	-Wno-pointer-sign

.include <bsd.prog.mk>
