# bootptest/Makefile
# $NetBSD: Makefile,v 1.7.2.1 1997/11/08 15:11:23 lukem Exp $

WARNS?=	0
PROG=	bootptest
SRCDIR= ${.CURDIR}/../bootpd
CPPFLAGS+= -I${SRCDIR}
.PATH: ${SRCDIR}

SRCS=	bootptest.c print-bootp.c getif.c getether.c report.c

MAN=	bootptest.8

.include <bsd.prog.mk>

