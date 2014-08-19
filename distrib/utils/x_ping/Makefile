# $NetBSD: Makefile,v 1.7.12.1 2014/08/19 23:45:49 tls Exp $
# Build ping without IPSEC

.include <bsd.own.mk>

SRCDIR=		${.CURDIR}/../../../sbin/ping

PROG=		ping
NOMAN=		# defined

CPPFLAGS+=	-I${SRCDIR} -DCRUNCHOPS
DPADD=		${LIBM}
LDADD=		-lm

.if ${MACHINE_ARCH} == "vax"
COPTS.ping.c=-O0
.endif

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
