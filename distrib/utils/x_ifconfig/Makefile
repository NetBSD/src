# $NetBSD: Makefile,v 1.33 2015/04/28 15:14:07 christos Exp $
# Build a smaller ifconfig (i.e. for boot media)

.include <bsd.own.mk>

NOMAN=	# defined

SRCDIR=		${.CURDIR}/../../../sbin/ifconfig


CPPFLAGS+=	-DCRUNCHOPS
.if defined(SMALLPROG_INET6) && (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
SRCS+= af_inet6.c
.endif

.ifdef SMALLPROG
CPPFLAGS+=-DSMALL
.endif

PROG=	ifconfig
.include "${.CURDIR}/../../../sbin/ifconfig/Makefile.inc"

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
