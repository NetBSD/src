# $NetBSD: Makefile,v 1.3 2000/05/18 01:10:29 matt Exp $
# Build ping without IPSEC

SRCDIR= ${.CURDIR}/../../../sbin/ping
PROG=	ping
MKMAN=	no
LDADD=	-lm
LDSTATIC?=-static
CPPFLAGS+= -I${SRCDIR}
COPTS+= -Os

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
