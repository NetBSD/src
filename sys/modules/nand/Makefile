# $NetBSD: Makefile,v 1.2.4.2 2011/06/06 09:09:49 jruoho Exp $

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
