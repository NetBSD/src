# bootpgw/Makefile
# $NetBSD: Makefile,v 1.6 1997/10/25 06:57:57 lukem Exp $

WARNS?=	0
PROG=	bootpgw
SRCDIR= ${.CURDIR}/../bootpd
CPPFLAGS+= -DSYSLOG -DDEBUG -I${SRCDIR}
.PATH: ${SRCDIR}

SRCS=	bootpgw.c getif.c hwaddr.c report.c

NOMAN=

.include <bsd.prog.mk>

