# $NetBSD: Makefile,v 1.1.2.2 2011/03/05 15:10:45 bouyer Exp $

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
