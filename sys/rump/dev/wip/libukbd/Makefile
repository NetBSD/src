#	$NetBSD: Makefile,v 1.1 2010/01/11 02:15:20 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../kern	\
	${.CURDIR}/../../../../dev/wscons

LIB=	rumpdev_ukbd

SRCS=	ukbd.c ukbdmap.c

SRCS+=  hid.c uhidev.c uhid.c
SRCS+=	ukbd_at_usb.c

# XXX
SRCS+=  tty.c tty_conf.c tty_tty.c tty_subr.c

CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
