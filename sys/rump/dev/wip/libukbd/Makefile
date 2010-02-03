#	$NetBSD: Makefile,v 1.2 2010/02/03 21:18:38 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../kern	\
	${.CURDIR}/../../../../dev/wscons

LIB=	rumpdev_ukbd

SRCS=	ukbd.c ukbdmap.c

SRCS+=  hid.c uhidev.c uhid.c
SRCS+=	ukbd_at_usb.c

# XXX
SRCS+=  tty.c tty_conf.c tty_tty.c tty_subr.c

IOCONFIG= UKBD.ioconf

CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
