#	$NetBSD: Makefile,v 1.4 2010/02/10 02:31:00 pooka Exp $
#

PROG=	sdread

LDADD+=	-lrumpfs_ffs -lrumpfs_msdos -lrumpvfs
LDADD+=	-lrumpdev_disk -lrumpdev_usb -lrumpdev_ugenhc -lrumpdev_umass -lrumpdev
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>
