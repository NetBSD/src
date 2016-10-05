#	$NetBSD: Makefile,v 1.1.2.2 2016/10/05 20:56:06 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/sdmmc

KMOD=	ld_sdmmc
IOCONF=	ld_sdmmc.ioconf

SRCS+=	ld_sdmmc.c

.include <bsd.kmodule.mk>
