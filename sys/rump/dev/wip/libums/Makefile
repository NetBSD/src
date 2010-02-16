#	$NetBSD: Makefile,v 1.3 2010/02/16 20:42:45 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../kern

LIB=	rumpdev_ums

SRCS=	ums.c

SRCS+=  hid.c uhidev.c uhid.c
SRCS+=	ums_at_usb.c

# XXX
SRCS+=  tty.c tty_conf.c tty_tty.c tty_subr.c

IOCONFIG= UMS.ioconf

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
