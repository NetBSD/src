# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:43 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	apple_smc_fan
IOCONF=	apple_smc_fan.ioconf
SRCS=	apple_smc_fan.c

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
