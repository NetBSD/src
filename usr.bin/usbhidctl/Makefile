#	$NetBSD: Makefile,v 1.3 1999/02/13 02:54:56 lukem Exp $

.include <bsd.own.mk>

PROG=	usbhidctl
SRCS=	usbhid.c hidsubr.c
.if ${MKSHARE} != "no"
FILES=  usb_hid_usages
FILESDIR=/usr/share/misc
.endif

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
