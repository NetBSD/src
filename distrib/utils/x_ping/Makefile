# $NetBSD: Makefile,v 1.8.36.1 2023/10/08 14:35:19 martin Exp $
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
