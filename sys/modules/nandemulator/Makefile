# $NetBSD: Makefile,v 1.2.18.1 2019/06/10 22:09:33 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/nand

KMOD=   nandemulator
IOCONF=	nandemulator.ioconf
SRCS=	nandemulator.c

WARNS=	3

.include <bsd.kmodule.mk>
