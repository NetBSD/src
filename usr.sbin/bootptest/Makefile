# bootptest/Makefile
# $NetBSD: Makefile,v 1.7 1997/10/18 04:37:15 lukem Exp $

WARNS?=	0
PROG=	bootptest
SRCDIR= ${.CURDIR}/../bootpd
CFLAGS+= -I${SRCDIR}
.PATH: ${SRCDIR}

SRCS=	bootptest.c print-bootp.c getif.c getether.c report.c

MAN=	bootptest.8

.include <bsd.prog.mk>

