#	$NetBSD: Makefile,v 1.2 2009/10/14 23:51:22 pooka Exp $
#

PROG=	wirelessconf

LDADD+=	-lrumpdev_net80211 -lrumpdev_usbrum -lrumpdev_usbhc
LDADD+=	-lrumpdev_usb -lrumpdev
LDADD+=	-lrumpnet_net -lrumpnet
LDADD+= -lrumpvfs -lrumpcrypto -lrump 
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>
