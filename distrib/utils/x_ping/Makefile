# $NetBSD: Makefile,v 1.7.6.1 2014/05/22 12:01:36 yamt Exp $
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
