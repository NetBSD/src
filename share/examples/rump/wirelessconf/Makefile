#	$NetBSD: Makefile,v 1.3 2010/02/05 22:07:51 pooka Exp $
#

PROG=	wirelessconf

LDADD+=	-lrumpdev_usbrum -lrumpdev_usbhc
LDADD+=	-lrumpdev_usb -lrumpdev
LDADD+=	-lrumpnet_net80211 -lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD+= -lrumpvfs -lrumpcrypto -lrump 
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>
