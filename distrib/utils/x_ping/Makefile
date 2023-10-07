# $NetBSD: Makefile,v 1.9 2023/10/07 12:15:49 rin Exp $
# Build ping without IPSEC

.include <bsd.own.mk>

SRCDIR=		${.CURDIR}/../../../sbin/ping

PROG=		ping
NOMAN=		# defined

CPPFLAGS+=	-I${SRCDIR} -DCRUNCHOPS
DPADD=		${LIBM}
LDADD=		-lm

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
