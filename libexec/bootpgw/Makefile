# bootpgw/Makefile
# $Id: Makefile,v 1.1.1.1 1994/05/30 20:18:34 gwr Exp $

PROG=	bootpgw
SRCDIR= ${.CURDIR}/../bootpd
CFLAGS+= -DSYSLOG -DDEBUG -I${SRCDIR}
.PATH: ${SRCDIR}

SRCS=	bootpgw.c getif.c hwaddr.c report.c

MAN8=

.include <bsd.prog.mk>

