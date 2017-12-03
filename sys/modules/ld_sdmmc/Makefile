#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:57 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/sdmmc

KMOD=	ld_sdmmc
IOCONF=	ld_sdmmc.ioconf

SRCS+=	ld_sdmmc.c

.include <bsd.kmodule.mk>
