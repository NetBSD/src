#	$NetBSD: Makefile,v 1.3 2010/03/01 11:34:27 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../kern

LIB=	rumpdev_ucom

SRCS=	ucom.c uplcom.c u3g.c

SRCS+=	ucom_at_usb.c
SRCS+=	tty.c tty_conf.c tty_tty.c tty_subr.c

IOCONFIG= UCOM.ioconf

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
