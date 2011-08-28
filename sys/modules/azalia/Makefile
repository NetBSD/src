# $NetBSD: Makefile,v 1.2 2011/08/28 15:40:50 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=   azalia
IOCONF=	azalia.ioconf

SRCS=	azalia.c
SRCS+=	azalia_codec.c

.include <bsd.kmodule.mk>
