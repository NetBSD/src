#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:59 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	ubsec
IOCONF=	ubsec.ioconf
SRCS=	ubsec.c

WARNS=	3

.include <bsd.kmodule.mk>
