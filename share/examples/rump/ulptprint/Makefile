#	$NetBSD: Makefile,v 1.2 2010/02/10 02:31:01 pooka Exp $
#

PROG=	ulptprint

LDADD+=	-lrumpvfs -lrumpdev_usb -lrumpdev_ugenhc -lrumpdev_ulpt -lrumpdev
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>
