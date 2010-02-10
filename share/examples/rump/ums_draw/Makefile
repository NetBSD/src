#	$NetBSD: Makefile,v 1.2 2010/02/10 02:31:01 pooka Exp $
#

PROG=	ms

LDADD+=	-lrumpdev_ums -lrumpdev_wscons -lrumpdev_usb -lrumpdev_ugenhc -lrumpdev
LDADD+=	-lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

LDADD+=	-lcurses

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>
