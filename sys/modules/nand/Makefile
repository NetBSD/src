# $NetBSD: Makefile,v 1.3.2.1 2013/01/16 05:33:47 yamt Exp $

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
