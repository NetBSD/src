# bootptest/Makefile
# $NetBSD: Makefile,v 1.8 1997/10/25 06:57:58 lukem Exp $

WARNS?=	0
PROG=	bootptest
SRCDIR= ${.CURDIR}/../bootpd
CPPFLAGS+= -I${SRCDIR}
.PATH: ${SRCDIR}

SRCS=	bootptest.c print-bootp.c getif.c getether.c report.c

MAN=	bootptest.8

.include <bsd.prog.mk>

