# bootpgw/Makefile
# $NetBSD: Makefile,v 1.5.2.1 1997/11/08 15:11:21 lukem Exp $

WARNS?=	0
PROG=	bootpgw
SRCDIR= ${.CURDIR}/../bootpd
CPPFLAGS+= -DSYSLOG -DDEBUG -I${SRCDIR}
.PATH: ${SRCDIR}

SRCS=	bootpgw.c getif.c hwaddr.c report.c

NOMAN=

.include <bsd.prog.mk>

