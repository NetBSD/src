# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:43 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/pci

KMOD=	amdtemp
IOCONF=	amdtemp.ioconf
SRCS=	amdtemp.c

.include <bsd.kmodule.mk>
