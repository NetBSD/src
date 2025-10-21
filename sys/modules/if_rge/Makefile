# $NetBSD: Makefile,v 1.1 2025/10/21 04:21:27 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_rge
IOCONF=	if_rge.ioconf
SRCS=	if_rge.c

WARNS=	3

.include <bsd.kmodule.mk>
