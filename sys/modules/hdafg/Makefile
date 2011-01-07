#	$NetBSD: Makefile,v 1.1 2011/01/07 15:32:11 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/hdaudio

KMOD=	hdafg
IOCONF=	hdafg.ioconf
SRCS=	hdaudio_afg.c

WARNS=	3

.include <bsd.kmodule.mk>
