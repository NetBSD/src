#	$NetBSD: Makefile,v 1.4 2010/03/01 15:41:15 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../kern	\
	${.CURDIR}/../../../../dev/wscons

LIB=	rumpdev_ukbd
IOCONF= UKBD.ioconf

SRCS=	ukbd.c ukbdmap.c

SRCS+=  hid.c uhidev.c uhid.c
SRCS+=	ukbd_at_usb.c

# XXX
SRCS+=  tty.c tty_conf.c tty_tty.c tty_subr.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
