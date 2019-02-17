#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:44 rin Exp $

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
