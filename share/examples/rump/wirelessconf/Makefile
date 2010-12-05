#	$NetBSD: Makefile,v 1.5 2010/12/05 20:13:26 pooka Exp $
#

PROG=	wirelessconf

LDADD+=	-lrumpdev_usbrum -lrumpdev_ugenhc
LDADD+=	-lrumpdev_usb -lrumpdev
LDADD+=	-lrumpnet_net80211 -lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD+= -lrumpkern_crypto
LDADD+= -lrumpvfs -lrump 
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>
