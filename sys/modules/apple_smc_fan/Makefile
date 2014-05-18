# $NetBSD: Makefile,v 1.1.4.2 2014/05/18 17:46:10 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	apple_smc_fan
IOCONF=	apple_smc_fan.ioconf
SRCS=	apple_smc_fan.c

WARNS=	4

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
