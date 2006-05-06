# $NetBSD: Makefile,v 1.9 2006/05/06 00:02:48 rpaulo Exp $
# Build ping6 without IPSEC

SRCDIR=		${.CURDIR}/../../../sbin/ping6

PROG=		ping6
NOMAN=		# defined

CPPFLAGS+=	-I${SRCDIR} -DINET6

# KAME scope id hack
CPPFLAGS+=	-DKAME_SCOPEID
CPPFLAGS+=	-DUSE_RFC3542

LDADD+=		-lm
DPADD+=		${LIBM}

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
