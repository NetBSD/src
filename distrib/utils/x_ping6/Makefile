# $NetBSD: Makefile,v 1.5 2001/03/17 07:10:54 tsutsui Exp $
# Build ping6 without IPSEC

SRCDIR= ${.CURDIR}/../../../sbin/ping6
PROG=	ping6
MKMAN=	no

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
