#	$NetBSD: Makefile,v 1.1 2010/03/22 12:21:37 pooka Exp $
#

PROG=	btplay

LDADD+=	-lrumpdev_ubt -lrumpdev_usb -lrumpdev_ugenhc -lrumpdev
LDADD+=	-lrumpnet_netbt -lrumpnet
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread
LDADD+=	-lbluetooth

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>
