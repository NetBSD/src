# $NetBSD: Makefile,v 1.25.2.2 2008/09/18 04:41:22 wrstuden Exp $
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
