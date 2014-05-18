# $NetBSD: Makefile,v 1.1.4.2 2014/05/18 17:46:10 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	apple_smc_temp
IOCONF=	apple_smc_temp.ioconf
SRCS=	apple_smc_temp.c

WARNS=	4

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
