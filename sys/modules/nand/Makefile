# $NetBSD: Makefile,v 1.4.30.1 2017/12/21 19:19:30 snj Exp $

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

.include <bsd.kmodule.mk>
.PATH:	${S}/dev/flash

SRCS+=	flash_mtdparts.c

