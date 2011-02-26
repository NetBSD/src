# $NetBSD: Makefile,v 1.1 2011/02/26 18:07:31 ahoka Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/nand

KMOD=   nand
IOCONF=	nand.ioconf
SRCS=	nand.c
SRCS+=	nand_io.c
SRCS+=	nand_bbt.c
SRCS+=	nand_crc.c
SRCS+=	hamming.c

.include <bsd.kmodule.mk>
