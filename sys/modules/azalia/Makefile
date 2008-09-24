# $NetBSD: Makefile,v 1.1.10.2 2008/09/24 05:28:33 wrstuden Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=   azalia

SRCS=	azalia.c
SRCS+=	azalia_codec.c

.include <bsd.kmodule.mk>
