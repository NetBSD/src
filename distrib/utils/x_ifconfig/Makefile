# $NetBSD: Makefile,v 1.24.18.3 2008/09/28 11:09:22 mjf Exp $
# Build a smaller ifconfig (i.e. for boot media)

.include <bsd.own.mk>

NOMAN=	# defined

SRCDIR=		${.CURDIR}/../../../sbin/ifconfig


.if defined(SMALLPROG_INET6) && (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
SRCS+= af_inet6.c
.endif

.include "../../../sbin/ifconfig/Makefile.inc"

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
