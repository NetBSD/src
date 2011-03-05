#	$NetBSD: Makefile,v 1.1.6.2 2011/03/05 20:55:38 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	auvitek
IOCONF=	auvitek.ioconf
SRCS=	auvitek.c auvitek_audio.c auvitek_board.c auvitek_i2c.c auvitek_video.c

WARNS=	4

.include <bsd.kmodule.mk>
