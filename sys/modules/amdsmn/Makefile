# $NetBSD: Makefile,v 1.1.2.2 2018/02/06 20:31:13 snj Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/pci

KMOD=	amdsmn
IOCONF=	amdsmn.ioconf
SRCS=	amdsmn.c

WARNS=	4

.include <bsd.kmodule.mk>
