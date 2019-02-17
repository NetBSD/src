# $NetBSD: Makefile,v 1.7 2019/02/17 04:05:55 rin Exp $

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

WARNS=	3

.include <bsd.kmodule.mk>
