#	$NetBSD: Makefile,v 1.2.54.1 2019/06/10 22:09:11 christos Exp $

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

.include <bsd.kmodule.mk>
