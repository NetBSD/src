# $NetBSD: Makefile,v 1.3.12.2 2017/12/03 11:38:58 jdolecek Exp $

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

.include <bsd.kmodule.mk>
