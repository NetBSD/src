# $NetBSD: Makefile,v 1.1 1999/10/18 20:19:39 pk Exp $
# Build ping without IPSEC

SRCDIR= ${.CURDIR}/../../../sbin/ping
PROG=	ping
MKMAN=	no
LDADD=	-lm
CPPFLAGS+= -I${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
