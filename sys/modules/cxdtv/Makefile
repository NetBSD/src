#	$NetBSD: Makefile,v 1.1 2011/07/14 23:50:13 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	cxdtv
IOCONF=	cxdtv.ioconf
SRCS=	cxdtv.c cxdtv_boards.c

WARNS=	4

.include <bsd.kmodule.mk>
