#	$NetBSD: Makefile,v 1.2 2011/07/09 15:08:54 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	auvitek
IOCONF=	auvitek.ioconf
SRCS=	auvitek.c \
	auvitek_audio.c \
	auvitek_board.c \
	auvitek_dtv.c \
	auvitek_i2c.c \
	auvitek_video.c

WARNS=	4

.include <bsd.kmodule.mk>
