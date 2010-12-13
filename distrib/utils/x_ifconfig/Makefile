# $NetBSD: Makefile,v 1.31 2010/12/13 21:47:32 pooka Exp $
# Build a smaller ifconfig (i.e. for boot media)

.include <bsd.own.mk>

NOMAN=	# defined

SRCDIR=		${.CURDIR}/../../../sbin/ifconfig


.if defined(SMALLPROG_INET6) && (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6 -DCRUNCHOPS
SRCS+= af_inet6.c
.endif

PROG=	ifconfig
.include "${.CURDIR}/../../../sbin/ifconfig/Makefile.inc"

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
