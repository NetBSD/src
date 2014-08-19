# $NetBSD: Makefile,v 1.1.10.2 2014/08/20 00:04:31 tls Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	apple_smc_fan
IOCONF=	apple_smc_fan.ioconf
SRCS=	apple_smc_fan.c

WARNS=	4

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
