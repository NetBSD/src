# $NetBSD: Makefile,v 1.1.4.2 2008/06/04 02:05:47 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=   azalia

SRCS=	azalia.c
SRCS+=	azalia_codec.c

.include <bsd.kmodule.mk>
