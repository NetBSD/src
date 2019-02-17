# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:43 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	apple_smc_temp
IOCONF=	apple_smc_temp.ioconf
SRCS=	apple_smc_temp.c

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
