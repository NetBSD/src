#	$NetBSD: Makefile,v 1.2.2.1 2010/04/30 14:44:26 uebayasi Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../kern

LIB=	rumpdev_ums
IOCONF= UMS.ioconf

SRCS=	ums.c

SRCS+=  hid.c uhidev.c uhid.c
SRCS+=	ums_at_usb.c

# XXX
SRCS+=  tty.c tty_conf.c tty_tty.c tty_subr.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
