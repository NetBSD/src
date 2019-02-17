# $NetBSD: Makefile,v 1.3 2019/02/17 04:05:44 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=   azalia
IOCONF=	azalia.ioconf

SRCS=	azalia.c
SRCS+=	azalia_codec.c

WARNS=	3

.include <bsd.kmodule.mk>
