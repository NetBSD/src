# $NetBSD: Makefile,v 1.1 2000/01/04 08:14:12 itojun Exp $
# Build ping6 without IPSEC

SRCDIR= ${.CURDIR}/../../../sbin/ping6
PROG=	ping6
NOMAN=	yes

CPPFLAGS+=-I${SRCDIR}
CPPFLAGS+=-DINET6

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
