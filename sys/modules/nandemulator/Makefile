# $NetBSD: Makefile,v 1.3 2019/02/17 04:05:55 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/nand

KMOD=   nandemulator
IOCONF=	nandemulator.ioconf
SRCS=	nandemulator.c

WARNS=	3

.include <bsd.kmodule.mk>
