#	$NetBSD: Makefile,v 1.1 2020/09/08 10:05:48 yamaguchi Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_iavf
IOCONF=	if_iavf.ioconf

SRCS+=	if_iavf.c

.include <bsd.kmodule.mk>
