#	$NetBSD: Makefile,v 1.5 2010/02/17 20:43:35 pooka Exp $
#

PROG=	sdread

LDADD+=	-lrumpfs_cd9660 -lrumpfs_ffs -lrumpfs_msdos -lrumpvfs
LDADD+=	-lrumpdev_disk -lrumpdev_usb -lrumpdev_ugenhc -lrumpdev_umass -lrumpdev
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

DBG=	-g
NOMAN=	
WARNS=	4

.include <bsd.prog.mk>
