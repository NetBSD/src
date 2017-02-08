# $NetBSD: Makefile,v 1.35 2017/02/08 23:16:41 rin Exp $
# Build a smaller ifconfig (i.e. for boot media)

NOMAN=	# defined

.include <bsd.own.mk>

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
.include "${.CURDIR}/../../../sbin/ifconfig/Makefile.common"

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
