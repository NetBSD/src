# $NetBSD: Makefile,v 1.3 2000/05/18 01:10:30 matt Exp $
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

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
