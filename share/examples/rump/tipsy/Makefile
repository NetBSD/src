#	$NetBSD: Makefile,v 1.1 2009/12/20 19:50:29 pooka Exp $
#

PROG=	tipsy

LDADD+=	-lrumpvfs
LDADD+=	-lrumpdev_usb -lrumpdev_usbhc -lrumpdev_ucom -lrumpdev
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>
