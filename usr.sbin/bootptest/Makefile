# bootptest/Makefile
# $Id: Makefile,v 1.2 1994/06/27 21:31:46 gwr Exp $

PROG=	bootptest
SRCDIR= ${.CURDIR}/../bootpd
CFLAGS+= -DBOOTPTEST -I${SRCDIR}
.PATH: ${SRCDIR}

SRCS=	bootptest.c print-bootp.c getif.c getether.c report.c

MAN8=	bootptest.0

.include <bsd.prog.mk>

