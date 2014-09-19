#	$NetBSD: Makefile,v 1.3 2014/09/19 18:05:24 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/hdaudio

KMOD=	hdaudio
IOCONF=	hdaudio.ioconf
SRCS=	hdaudio.c
SRCS+=	hdaudio_pci.c

.include <bsd.kmodule.mk>
