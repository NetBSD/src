# bootptest/Makefile
# $Id: Makefile,v 1.4 1994/12/22 11:33:18 cgd Exp $

PROG=	bootptest
SRCDIR= ${.CURDIR}/../bootpd
CFLAGS+= -I${SRCDIR}
.PATH: ${SRCDIR}

SRCS=	bootptest.c print-bootp.c getif.c getether.c report.c

MAN=	bootptest.8

.include <bsd.prog.mk>

