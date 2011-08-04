#	$NetBSD: Makefile,v 1.1 2011/08/04 22:26:07 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	coram
IOCONF=	coram.ioconf
SRCS=	coram.c

WARNS=	3

.include <bsd.kmodule.mk>
