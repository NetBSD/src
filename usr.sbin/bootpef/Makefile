# bootpef/Makefile
# $Id: Makefile,v 1.1.1.1 1994/05/30 20:22:32 gwr Exp $

PROG=	bootpef
SRCDIR= ${.CURDIR}/../../libexec/bootpd
CFLAGS+= -DETC_ETHERS -DDEBUG -I${SRCDIR}
.PATH: ${SRCDIR}

SRCS=	bootpef.c dovend.c readfile.c hash.c dumptab.c \
	 lookup.c hwaddr.c report.c tzone.c

MAN8=	bootpef.0

.include <bsd.prog.mk>

