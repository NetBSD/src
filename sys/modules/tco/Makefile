# $NetBSD: Makefile,v 1.1 2015/05/03 02:54:07 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/arch/x86/pci

KMOD=	tco
IOCONF=	tco.ioconf
SRCS=	tco.c

WARNS=	4

.include <bsd.kmodule.mk>
