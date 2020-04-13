#	$NetBSD: Makefile,v 1.1.8.2 2020/04/13 08:05:12 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_ixl
IOCONF=	if_ixl.ioconf

SRCS+=	if_ixl.c

.include <bsd.kmodule.mk>
