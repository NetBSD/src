#	$NetBSD: Makefile,v 1.1 2020/10/14 10:26:59 ryo Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_vmx
IOCONF=	if_vmx.ioconf

SRCS=	if_vmx.c

.include <bsd.kmodule.mk>
