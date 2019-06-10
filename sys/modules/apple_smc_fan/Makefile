# $NetBSD: Makefile,v 1.1.36.1 2019/06/10 22:09:09 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	apple_smc_fan
IOCONF=	apple_smc_fan.ioconf
SRCS=	apple_smc_fan.c

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
