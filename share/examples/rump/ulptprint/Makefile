#	$NetBSD: Makefile,v 1.1 2009/12/15 16:01:50 pooka Exp $
#

PROG=	ulptprint

LDADD+=	-lrumpvfs -lrumpdev_usb -lrumpdev_usbhc -lrumpdev_ulpt -lrumpdev
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>
