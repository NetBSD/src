# $NetBSD: Makefile,v 1.2 2000/05/18 00:43:08 matt Exp $
# Build ping6 without IPSEC

SRCDIR= ${.CURDIR}/../../../sbin/ping6
PROG=	ping6
NOMAN=	yes

CPPFLAGS+=-I${SRCDIR}
CPPFLAGS+=-DINET6
COPTS+= -Os

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
