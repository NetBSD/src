#	$NetBSD: Makefile,v 1.2 2025/10/21 04:35:29 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/sdmmc

KMOD=	ld_sdmmc
IOCONF=	ld_sdmmc.ioconf

SRCS+=	ld_sdmmc.c

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
