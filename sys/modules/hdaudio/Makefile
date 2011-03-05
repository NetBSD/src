#	$NetBSD: Makefile,v 1.1.6.2 2011/03/05 20:55:42 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/hdaudio

KMOD=	hdaudio
IOCONF=	hdaudio.ioconf
SRCS=	hdaudio.c
SRCS+=	hdaudio_ids.c
SRCS+=	hdaudio_pci.c

WARNS=	3

.include <bsd.kmodule.mk>
