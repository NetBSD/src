# $NetBSD: Makefile,v 1.1.22.2 2009/05/04 08:14:06 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=   azalia

SRCS=	azalia.c
SRCS+=	azalia_codec.c

.include <bsd.kmodule.mk>
