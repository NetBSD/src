#	$NetBSD: Makefile,v 1.2 2010/02/10 02:31:00 pooka Exp $
#

PROG=	tipsy

LDADD+=	-lrumpvfs
LDADD+=	-lrumpdev_usb -lrumpdev_ugenhc -lrumpdev_ucom -lrumpdev
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>
