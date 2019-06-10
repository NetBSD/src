#	$NetBSD: Makefile,v 1.1.36.1 2019/06/10 22:09:41 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	ubsec
IOCONF=	ubsec.ioconf
SRCS=	ubsec.c

WARNS=	3

.include <bsd.kmodule.mk>
