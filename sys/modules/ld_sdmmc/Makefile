#	$NetBSD: Makefile,v 1.1.58.1 2026/11/04 23:27:55 perseant Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/sdmmc

KMOD=	ld_sdmmc
IOCONF=	ld_sdmmc.ioconf

SRCS+=	ld_sdmmc.c

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
