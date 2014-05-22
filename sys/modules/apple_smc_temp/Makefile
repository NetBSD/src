# $NetBSD: Makefile,v 1.1.6.2 2014/05/22 11:41:06 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	apple_smc_temp
IOCONF=	apple_smc_temp.ioconf
SRCS=	apple_smc_temp.c

WARNS=	4

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
