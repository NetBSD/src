# $NetBSD: Makefile,v 1.5 2017/11/15 00:48:42 jmcneill Exp $

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
