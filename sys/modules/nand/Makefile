# $NetBSD: Makefile,v 1.1.4.3 2011/04/21 01:42:13 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/nand

KMOD=   nand
IOCONF=	nand.ioconf
SRCS=	nand.c
SRCS+=	nand_io.c
SRCS+=	nand_bbt.c
SRCS+=	nand_crc.c
SRCS+=	hamming.c
SRCS+=	nand_micron.c

.include <bsd.kmodule.mk>
