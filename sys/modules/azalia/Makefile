# $NetBSD: Makefile,v 1.1.2.2 2008/06/02 13:24:21 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=   azalia

SRCS=	azalia.c
SRCS+=	azalia_codec.c

.include <bsd.kmodule.mk>
