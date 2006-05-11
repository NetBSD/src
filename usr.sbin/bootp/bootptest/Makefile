#	$NetBSD: Makefile,v 1.2 2006/05/11 23:16:30 mrg Exp $

PROG=	bootptest
SRCS=	bootptest.c print-bootp.c getether.c
MAN=	bootptest.8

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS.print-bootp.c+=	-Wno-pointer-sign
.endif
