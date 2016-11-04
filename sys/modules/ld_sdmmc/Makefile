#	$NetBSD: Makefile,v 1.1.4.2 2016/11/04 14:49:19 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/sdmmc

KMOD=	ld_sdmmc
IOCONF=	ld_sdmmc.ioconf

SRCS+=	ld_sdmmc.c

.include <bsd.kmodule.mk>
