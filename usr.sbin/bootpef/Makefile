# bootpef/Makefile
# $Id: Makefile,v 1.5 1997/05/08 21:12:10 gwr Exp $

PROG=	bootpef
SRCDIR= ${.CURDIR}/../bootpd
CFLAGS+= -DETC_ETHERS -DDEBUG -I${SRCDIR}
.PATH: ${SRCDIR}

SRCS=	bootpef.c dovend.c readfile.c hash.c dumptab.c \
	 lookup.c hwaddr.c report.c tzone.c

MAN=	bootpef.8

.include <bsd.prog.mk>

