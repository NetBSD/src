# $NetBSD: Makefile,v 1.6 2001/12/12 00:05:13 tv Exp $
# Build ping without IPSEC

SRCDIR=		${.CURDIR}/../../../sbin/ping

PROG=		ping
NOMAN=		# defined

CPPFLAGS+=	-I${SRCDIR}
DPADD=		${LIBM}
LDADD=		-lm

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
