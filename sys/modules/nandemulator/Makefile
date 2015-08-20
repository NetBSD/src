# $NetBSD: Makefile,v 1.2 2015/08/20 11:05:01 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/nand

KMOD=   nandemulator
IOCONF=	nandemulator.ioconf
SRCS=	nandemulator.c

.include <bsd.kmodule.mk>
