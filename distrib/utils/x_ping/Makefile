# $NetBSD: Makefile,v 1.7 2010/12/13 19:19:10 pooka Exp $
# Build ping without IPSEC

SRCDIR=		${.CURDIR}/../../../sbin/ping

PROG=		ping
NOMAN=		# defined

CPPFLAGS+=	-I${SRCDIR} -DCRUNCHOPS
DPADD=		${LIBM}
LDADD=		-lm

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
