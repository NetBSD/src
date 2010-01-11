#	$NetBSD: Makefile,v 1.1 2010/01/11 02:16:51 pooka Exp $
#

PROG=	kbd

LDADD+=	-lrumpdev_ukbd -lrumpdev_wscons -lrumpdev_usb -lrumpdev_usbhc -lrumpdev
LDADD+=	-lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>
