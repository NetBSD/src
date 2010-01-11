#	$NetBSD: Makefile,v 1.1 2010/01/11 02:18:45 pooka Exp $
#

PROG=	ms

LDADD+=	-lrumpdev_ums -lrumpdev_wscons -lrumpdev_usb -lrumpdev_usbhc -lrumpdev
LDADD+=	-lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

LDADD+=	-lcurses

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>
