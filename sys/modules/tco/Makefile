# $NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:39:00 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/pci

KMOD=	tco
IOCONF=	tco.ioconf
SRCS=	tco.c

WARNS=	4

.include <bsd.kmodule.mk>
