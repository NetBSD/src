# $NetBSD: Makefile,v 1.10 2006/05/07 16:01:21 rpaulo Exp $
# Build ping6 without IPSEC

SRCDIR=		${.CURDIR}/../../../sbin/ping6

PROG=		ping6
NOMAN=		# defined

CPPFLAGS+=	-I${SRCDIR} -DINET6

# KAME scope id hack
CPPFLAGS+=	-DKAME_SCOPEID

LDADD+=		-lm
DPADD+=		${LIBM}

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
