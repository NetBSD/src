# bootptest/Makefile
# $Id: Makefile,v 1.3.2.2 1994/08/22 22:19:05 gwr Exp $

PROG=	bootptest
SRCDIR= ${.CURDIR}/../bootpd
CFLAGS+= -I${SRCDIR}
.PATH: ${SRCDIR}

SRCS=	bootptest.c print-bootp.c getif.c getether.c report.c

MAN8=	bootptest.0

.include <bsd.prog.mk>

