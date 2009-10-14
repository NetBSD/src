#	$NetBSD: Makefile,v 1.3 2009/10/14 23:51:52 pooka Exp $
#

PROG=	sdread

LDADD+=	-lrumpfs_ffs -lrumpfs_msdos -lrumpvfs
LDADD+=	-lrumpdev_disk -lrumpdev_usb -lrumpdev_usbhc -lrumpdev_umass -lrumpdev
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>
