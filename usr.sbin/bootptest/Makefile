# bootptest/Makefile
# $Id: Makefile,v 1.5 1997/05/06 20:46:05 gwr Exp $

PROG=	bootptest
SRCDIR= ${.CURDIR}/../bootpd
CFLAGS+= -I${SRCDIR}
.PATH.c: ${SRCDIR}

SRCS=	bootptest.c print-bootp.c getif.c getether.c report.c

MAN=	bootptest.8

.include <bsd.prog.mk>

