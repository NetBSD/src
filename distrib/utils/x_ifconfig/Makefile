# $NetBSD: Makefile,v 1.32 2010/12/14 17:07:08 hannken Exp $
# Build a smaller ifconfig (i.e. for boot media)

.include <bsd.own.mk>

NOMAN=	# defined

SRCDIR=		${.CURDIR}/../../../sbin/ifconfig


CPPFLAGS+=	-DCRUNCHOPS
.if defined(SMALLPROG_INET6) && (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
SRCS+= af_inet6.c
.endif

PROG=	ifconfig
.include "${.CURDIR}/../../../sbin/ifconfig/Makefile.inc"

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
