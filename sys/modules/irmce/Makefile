#	$NetBSD: Makefile,v 1.1 2011/07/19 12:23:51 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	irmce	
IOCONF=	irmce.ioconf
SRCS=	irmce.c

WARNS=	4

.include <bsd.kmodule.mk>
