# $NetBSD: Makefile,v 1.2 2000/05/18 00:43:07 matt Exp $
# Build ping without IPSEC

SRCDIR= ${.CURDIR}/../../../sbin/ping
PROG=	ping
MKMAN=	no
LDADD=	-lm
CPPFLAGS+= -I${SRCDIR}
COPTS+= -Os

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
