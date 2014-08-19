# $NetBSD: Makefile,v 1.1.10.2 2014/08/20 00:04:31 tls Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	apple_smc_temp
IOCONF=	apple_smc_temp.ioconf
SRCS=	apple_smc_temp.c

WARNS=	4

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
