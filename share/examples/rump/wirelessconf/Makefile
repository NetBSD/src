#	$NetBSD: Makefile,v 1.4 2010/02/10 02:31:01 pooka Exp $
#

PROG=	wirelessconf

LDADD+=	-lrumpdev_usbrum -lrumpdev_ugenhc
LDADD+=	-lrumpdev_usb -lrumpdev
LDADD+=	-lrumpnet_net80211 -lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD+= -lrumpvfs -lrumpcrypto -lrump 
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>
