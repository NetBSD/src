# $NetBSD: Makefile,v 1.1.4.1 1999/12/27 18:28:45 wrstuden Exp $
# Build ping without IPSEC

SRCDIR= ${.CURDIR}/../../../sbin/ping
PROG=	ping
MKMAN=	no
LDADD=	-lm
CPPFLAGS+= -I${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
