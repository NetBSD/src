#	$NetBSD: Makefile,v 1.1 2009/10/05 13:05:31 pooka Exp $
#

PROG=	sdread

LDADD+=	-lrumpfs_msdos -lrumpvfs
LDADD+=	-lrumpdev_disk -lrumpdev_usb -lrumpdev_usbhc -lrumpdev_umass -lrumpdev
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	

.include <bsd.prog.mk>
