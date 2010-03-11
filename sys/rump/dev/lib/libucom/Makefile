#	$NetBSD: Makefile,v 1.4.2.2 2010/03/11 15:04:33 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../kern

LIB=	rumpdev_ucom
IOCONF= UCOM.ioconf

SRCS=	ucom.c uplcom.c u3g.c

SRCS+=	ucom_at_usb.c
SRCS+=	tty.c tty_conf.c tty_tty.c tty_subr.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
