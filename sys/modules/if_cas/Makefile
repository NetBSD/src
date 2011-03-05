#	$NetBSD: Makefile,v 1.1.6.2 2011/03/05 20:55:45 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_cas
IOCONF=	if_cas.ioconf
SRCS=	if_cas.c
CPPFLAGS+= -DINET

WARNS=	3

.include <bsd.kmodule.mk>
