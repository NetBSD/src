#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:46 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	cxdtv
IOCONF=	cxdtv.ioconf
SRCS=	cxdtv.c cxdtv_boards.c

.include <bsd.kmodule.mk>
