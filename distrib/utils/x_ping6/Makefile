# $NetBSD: Makefile,v 1.4 2000/10/08 12:37:06 itojun Exp $
# Build ping6 without IPSEC

SRCDIR= ${.CURDIR}/../../../sbin/ping6
PROG=	ping6
NOMAN=	yes

CPPFLAGS+=-I${SRCDIR}
CPPFLAGS+=-DINET6
COPTS+= -Os
LDSTATIC?=-static

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

LDADD+=	-lm
DPADD+=	${LIBM}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
