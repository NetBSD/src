#	$NetBSD: Makefile,v 1.2 2010/02/10 02:31:01 pooka Exp $
#

PROG=	kbd

LDADD+=	-lrumpdev_ukbd -lrumpdev_wscons -lrumpdev_usb -lrumpdev_ugenhc -lrumpdev
LDADD+=	-lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>
