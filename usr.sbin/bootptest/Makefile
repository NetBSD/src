# bootptest/Makefile
# $NetBSD: Makefile,v 1.9 1998/03/14 04:52:15 lukem Exp $

PROG=	bootptest
SRCDIR= ${.CURDIR}/../bootpd
CPPFLAGS+= -I${SRCDIR}
.PATH: ${SRCDIR}

SRCS=	bootptest.c print-bootp.c getif.c getether.c report.c

MAN=	bootptest.8

.include <bsd.prog.mk>

