# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:42 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/pci

KMOD=	amdsmn
IOCONF=	amdsmn.ioconf
SRCS=	amdsmn.c

.include <bsd.kmodule.mk>
