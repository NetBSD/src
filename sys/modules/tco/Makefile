# $NetBSD: Makefile,v 1.1.2.2 2015/06/06 14:40:25 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/pci

KMOD=	tco
IOCONF=	tco.ioconf
SRCS=	tco.c

WARNS=	4

.include <bsd.kmodule.mk>
