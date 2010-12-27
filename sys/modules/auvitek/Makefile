#	$NetBSD: Makefile,v 1.1 2010/12/27 15:43:41 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	auvitek
IOCONF=	auvitek.ioconf
SRCS=	auvitek.c auvitek_audio.c auvitek_board.c auvitek_i2c.c auvitek_video.c

WARNS=	4

.include <bsd.kmodule.mk>
