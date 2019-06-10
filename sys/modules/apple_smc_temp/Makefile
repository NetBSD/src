# $NetBSD: Makefile,v 1.1.36.1 2019/06/10 22:09:10 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	apple_smc_temp
IOCONF=	apple_smc_temp.ioconf
SRCS=	apple_smc_temp.c

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
