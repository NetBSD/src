#	$NetBSD: Makefile,v 1.2 1998/09/27 17:22:13 lukem Exp $

PROG=	usbhidctl
SRCS=	usbhid.c hidsubr.c
.if !defined(NOSHARE)
FILES=  usb_hid_usages
FILESDIR=/usr/share/misc
.endif

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
