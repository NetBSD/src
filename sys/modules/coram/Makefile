#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:46 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	coram
IOCONF=	coram.ioconf
SRCS=	coram.c

WARNS=	3

.include <bsd.kmodule.mk>
