# bootptest/Makefile
# $Id: Makefile,v 1.1.1.1 1994/05/30 20:24:19 gwr Exp $

PROG=	bootptest
SRCDIR= ${.CURDIR}/../../libexec/bootpd
CFLAGS+= -DBOOTPTEST -I${SRCDIR}
.PATH: ${SRCDIR}

SRCS=	bootptest.c print-bootp.c getif.c getether.c report.c

MAN8=	bootptest.0

.include <bsd.prog.mk>

