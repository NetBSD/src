# $NetBSD: Makefile,v 1.2.54.1 2019/06/10 22:09:11 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=   azalia
IOCONF=	azalia.ioconf

SRCS=	azalia.c
SRCS+=	azalia_codec.c

WARNS=	3

.include <bsd.kmodule.mk>
