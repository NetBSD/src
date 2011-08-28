#	$NetBSD: Makefile,v 1.2 2011/08/28 15:48:20 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_cas
IOCONF=	if_cas.ioconf
SRCS=	if_cas.c
CPPFLAGS+= -DINET

.include <bsd.kmodule.mk>
