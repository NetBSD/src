# bootpgw/Makefile
# $NetBSD: Makefile,v 1.7 1998/03/14 04:51:46 lukem Exp $

PROG=	bootpgw
SRCDIR= ${.CURDIR}/../bootpd
CPPFLAGS+= -DSYSLOG -DDEBUG -I${SRCDIR}
.PATH: ${SRCDIR}

SRCS=	bootpgw.c getif.c hwaddr.c report.c

NOMAN=

.include <bsd.prog.mk>

