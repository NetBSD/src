# $NetBSD: Makefile,v 1.6 2017/12/09 22:42:17 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/nand

KMOD=   nand
IOCONF=	nand.ioconf
SRCS=	nand.c
SRCS+=	nand_bbt.c
SRCS+=	nand_crc.c
SRCS+=	hamming.c
SRCS+=	nand_micron.c
SRCS+=	nand_samsung.c
SRCS+=	nand_toshiba.c

.PATH:	${S}/dev/flash

SRCS+=	flash_mtdparts.c

.include <bsd.kmodule.mk>
