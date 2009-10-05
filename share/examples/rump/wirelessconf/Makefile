#	$NetBSD: Makefile,v 1.1 2009/10/05 13:07:28 pooka Exp $
#

PROG=	wirelessconf

LDADD+=	-lrumpdev_net80211 -lrumpdev_usbrum -lrumpdev_usbhc
LDADD+=	-lrumpdev_usb -lrumpdev
LDADD+=	-lrumpnet_net -lrumpnet
LDADD+= -lrumpvfs -lrumpcrypto -lrump 
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	

.include <bsd.prog.mk>
