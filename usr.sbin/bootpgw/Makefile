# bootpgw/Makefile
# $NetBSD: Makefile,v 1.5 1997/10/18 04:37:11 lukem Exp $

WARNS?=	0
PROG=	bootpgw
SRCDIR= ${.CURDIR}/../bootpd
CFLAGS+= -DSYSLOG -DDEBUG -I${SRCDIR}
.PATH: ${SRCDIR}

SRCS=	bootpgw.c getif.c hwaddr.c report.c

NOMAN=

.include <bsd.prog.mk>

