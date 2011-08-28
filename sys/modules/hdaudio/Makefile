#	$NetBSD: Makefile,v 1.2 2011/08/28 15:48:19 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/hdaudio

KMOD=	hdaudio
IOCONF=	hdaudio.ioconf
SRCS=	hdaudio.c
SRCS+=	hdaudio_ids.c
SRCS+=	hdaudio_pci.c

.include <bsd.kmodule.mk>
