#	$NetBSD: Makefile,v 1.1 2019/12/10 12:08:53 yamaguchi Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_ixl
IOCONF=	if_ixl.ioconf

SRCS+=	if_ixl.c

.include <bsd.kmodule.mk>
