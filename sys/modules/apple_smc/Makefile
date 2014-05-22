# $NetBSD: Makefile,v 1.2.6.2 2014/05/22 11:41:06 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	apple_smc
SRCS=	apple_smc.c

WARNS=	4

# XXX What's the right thing here?
CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
