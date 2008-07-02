# $NetBSD: Makefile,v 1.27 2008/07/02 07:44:15 dyoung Exp $
# Build a smaller ifconfig (i.e. for boot media)

.include <bsd.own.mk>

NOMAN=	# defined

SRCDIR=		${.CURDIR}/../../../sbin/ifconfig


.if defined(SMALLPROG_INET6) && (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
SRCS+= af_inet6.c
.endif

.include "../../../sbin/ifconfig/Makefile.inc"
CPPFLAGS+=	-I${SRCDIR}

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
