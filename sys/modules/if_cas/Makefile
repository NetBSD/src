#	$NetBSD: Makefile,v 1.1 2011/01/07 11:17:22 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_cas
IOCONF=	if_cas.ioconf
SRCS=	if_cas.c
CPPFLAGS+= -DINET

WARNS=	3

.include <bsd.kmodule.mk>
