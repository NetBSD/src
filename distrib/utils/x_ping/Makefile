# $NetBSD: Makefile,v 1.8 2013/11/09 21:39:27 christos Exp $
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
