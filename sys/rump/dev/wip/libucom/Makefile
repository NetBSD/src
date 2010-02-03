#	$NetBSD: Makefile,v 1.3 2010/02/03 21:18:38 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../kern

LIB=	rumpdev_ucom

SRCS=	ucom.c uplcom.c

SRCS+=	ucom_at_usb.c
SRCS+=	tty.c tty_conf.c tty_tty.c tty_subr.c

IOCONFIG= UCOM.ioconf

CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
