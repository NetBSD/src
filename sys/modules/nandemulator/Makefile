# $NetBSD: Makefile,v 1.1.20.1 2017/12/03 11:38:58 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/nand

KMOD=   nandemulator
IOCONF=	nandemulator.ioconf
SRCS=	nandemulator.c

.include <bsd.kmodule.mk>
