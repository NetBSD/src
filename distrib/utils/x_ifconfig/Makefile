# $NetBSD: Makefile,v 1.37 2017/02/08 23:24:57 rin Exp $
# Build a smaller ifconfig (i.e. for boot media)

NOMAN=	# defined

.include <bsd.own.mk>

SRCDIR=		${.CURDIR}/../../../sbin/ifconfig


CPPFLAGS+=	-DCRUNCHOPS
.if defined(SMALLPROG_INET6) && (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
SRCS+= af_inet6.c
.endif

PROG=	ifconfig

.PATH:		${SRCDIR}

.include "${.CURDIR}/../../../sbin/ifconfig/Makefile.common"

.include <bsd.prog.mk>
