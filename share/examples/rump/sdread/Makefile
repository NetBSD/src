#	$NetBSD: Makefile,v 1.2 2009/10/13 18:41:06 pooka Exp $
#

PROG=	sdread

LDADD+=	-lrumpfs_ffs -lrumpfs_msdos -lrumpvfs
LDADD+=	-lrumpdev_disk -lrumpdev_usb -lrumpdev_usbhc -lrumpdev_umass -lrumpdev
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	

.include <bsd.prog.mk>
