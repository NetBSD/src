# bootpgw/Makefile
# $Id: Makefile,v 1.3 1997/05/06 20:46:04 gwr Exp $

PROG=	bootpgw
SRCDIR= ${.CURDIR}/../bootpd
CFLAGS+= -DSYSLOG -DDEBUG -I${SRCDIR}
.PATH.c: ${SRCDIR}

SRCS=	bootpgw.c getif.c hwaddr.c report.c

NOMAN=

.include <bsd.prog.mk>

