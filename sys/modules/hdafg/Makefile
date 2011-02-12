#	$NetBSD: Makefile,v 1.2 2011/02/12 15:01:00 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/hdaudio

KMOD=	hdafg
IOCONF=	hdafg.ioconf
SRCS=	hdafg.c

WARNS=	3

.include <bsd.kmodule.mk>
