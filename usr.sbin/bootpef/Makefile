# bootpef/Makefile
# $NetBSD: Makefile,v 1.8 1998/03/14 04:43:06 lukem Exp $

PROG=	bootpef
SRCDIR= ${.CURDIR}/../bootpd
CPPFLAGS+= -DETC_ETHERS -DDEBUG -I${SRCDIR}
.PATH: ${SRCDIR}

SRCS=	bootpef.c dovend.c readfile.c hash.c dumptab.c \
	 lookup.c hwaddr.c report.c tzone.c

MAN=	bootpef.8

.include <bsd.prog.mk>

