# $NetBSD: Makefile,v 1.1.6.2 2014/05/22 11:41:06 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	apple_smc_fan
IOCONF=	apple_smc_fan.ioconf
SRCS=	apple_smc_fan.c

WARNS=	4

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
