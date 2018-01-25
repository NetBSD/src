# $NetBSD: Makefile,v 1.1 2018/01/25 23:40:06 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/pci

KMOD=	amdsmn
IOCONF=	amdsmn.ioconf
SRCS=	amdsmn.c

WARNS=	4

.include <bsd.kmodule.mk>
