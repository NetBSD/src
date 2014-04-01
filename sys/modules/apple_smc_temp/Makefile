# $NetBSD: Makefile,v 1.1 2014/04/01 17:48:39 riastradh Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	apple_smc_temp
IOCONF=	apple_smc_temp.ioconf
SRCS=	apple_smc_temp.c

WARNS=	4

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
